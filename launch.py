#!/usr/bin/env python3
# Stage 1, check for debug environment variables
import logging
import os
import sys
import time
import traceback
import shlex
import threading
import getopt
import concurrent.futures
import pkg_resources

from linuxcnc_arduinoconnector.utils.YamlParser import ArduinoYamlParser
from linuxcnc_arduinoconnector.interfaces.ArduinoComms import ArduinoConnection
from linuxcnc_arduinoconnector.api import set_arduino_connections, run_api_server_in_thread

from linuxcnc_arduinoconnector.utils.Utils import get_parent_process_name, try_load_linuxcnc, launch_connector, listDevices, locateProfile
from linuxcnc_arduinoconnector.config.Config import (
    DEFAULT_API_ENABLED, DEFAULT_LOGGING_ENABLED,
    DEFAULT_LOG_FILE_PATH, DEFAULT_LOG_FILE_NAME,
    DEFAULT_LOGGING_FORMAT, DEFAULT_REMOTE_DEBUG_ENABLED,
    DEFAULT_REMOTE_DEBUG_BIND_ADDRESS, DEFAULT_REMOTE_DEBUG_PORT,
    DEFAULT_REMOTE_DEBUG_WAIT_ON_CONNECT, DEFAULT_API_PORT,
    DEFAULT_PROFILE_NAME, DEFAULT_LINUXCNC_PROFILE_INI_HEADER,
    DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_KEY,
    DEFAULT_LINUXCNC_PROFILE_INI_WAIT_ON_REMOTE_DEBUG_CONNECT_KEY
)
# Since Linuxcnc launches this script using systemd, we can store the path to the script for later use
base_directory = os.path.dirname(os.path.abspath(os.path.abspath(__file__)))

# Set root logger to DEBUG level
# logging.getLogger().setLevel(logging.DEBUG)  # Comment out this line to avoid duplicate logging

# Pre stage, look to see if the local config is set to enable logging by default. This can be helpful 
file_logger = None
#DEFAULT_LOGGING_ENABLED = True
if DEFAULT_LOGGING_ENABLED:
    from linuxcnc_arduinoconnector.utils.LoggingUtils import setup_logger
    file_logger = setup_logger('file_logger', log_file_path=os.path.join(DEFAULT_LOG_FILE_PATH, DEFAULT_LOG_FILE_NAME), log_format=DEFAULT_LOGGING_FORMAT)
    file_logger.debug('Logging enabled based on override from Config.py, see DEFAULT_LOGGING_ENABLED')

def launch_remote_debugger_listen(bind_addres:str, wait_on_connect=False, port=5678):
    import debugpy
    try:
        debugpy.listen((bind_addres,port))
        #rint(f'Remote Debug Enabled based on override from Config.py, listening on port {port}')
        if file_logger is not None:
            file_logger.debug(f'Remote Debug Enabled based on override from Config.py or linuxcnc profile, bound to {bind_addres} and listening on port {port}')
        if wait_on_connect:
            if file_logger is not None:
                file_logger.debug('Waiting for remote debugger to connect...')
            debugpy.wait_for_client()
            pass
    except Exception as e:
        if file_logger is not None:
            file_logger.error(f'Error launching remote debugger: {e}')
        print(f'Error launching remote debugger: {e}')

if DEFAULT_REMOTE_DEBUG_ENABLED:
    launch_remote_debugger_listen(DEFAULT_REMOTE_DEBUG_BIND_ADDRESS, DEFAULT_REMOTE_DEBUG_WAIT_ON_CONNECT, DEFAULT_REMOTE_DEBUG_PORT)

# First stage, figure out if LinuxCNC launched this script. If Linuxcnc is the host, then the user's profile will provide the settings to use.
launchedByLinuxCNC = False
# get_parent_process_name() returns 'systemd' if Linuxcnc launches it, otherwise its something else like 'bash' 
#test = get_parent_process_cmdline()
maybe_linuxcnc = get_parent_process_name()
if maybe_linuxcnc== 'systemd' and try_load_linuxcnc(): # try_load_linux will throw an exception if it fails
    launchedByLinuxCNC = True
    if file_logger is not None:
        file_logger.debug('Detected execution by LinuxCNC')
else:
    if file_logger is not None:
        file_logger.debug(f'Detected execution outside of LinuxCNC, parent process name {maybe_linuxcnc}')        

if launchedByLinuxCNC:
 
    import linuxcnc
    import logging
    import time
    logging.basicConfig(level=logging.DEBUG, format='%(message)s\r\n')

    
    inifile = None
    retries = 3
    while retries > 0:
        try:
            stat = linuxcnc.stat()
            stat.poll()
            #print(f'INI FILE NAME = {stat.ini_filename}')
            if stat.ini_filename is not None and stat.ini_filename != '':
                break
            else:
                raise Exception('No ini file found')
        except Exception as e:
            retries -= 1
            time.sleep(2)
            print(f'Error loading ini file: {e}')
    print(f'INI FILE NAME = {stat.ini_filename}')
    inifile = linuxcnc.ini(stat.ini_filename)
    
    yaml_path = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY) or DEFAULT_PROFILE_NAME
    maybe_remote_debug = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_KEY) or False
    if maybe_remote_debug == '1':
        maybe_remote_debug = True
    else:
        maybe_remote_debug = False
    maybe_wait_on_remote_debug = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_WAIT_ON_REMOTE_DEBUG_CONNECT_KEY) or False
    if maybe_wait_on_remote_debug == '1':
        maybe_wait_on_remote_debug = True
    else:
        maybe_wait_on_remote_debug = False
    if maybe_remote_debug:
        launch_remote_debugger_listen(DEFAULT_REMOTE_DEBUG_BIND_ADDRESS, maybe_wait_on_remote_debug, DEFAULT_REMOTE_DEBUG_PORT)
    #from linuxcnc_arduinoconnector.Utils import launch_ui_in_new_console
    #launch_ui_in_new_console(additional_args=['-p', yaml_path])
    launch_connector(yaml_path)
    #machine_name = inifile.find("EMC", "MACHINE") or "unknown"
    pass




arduino_map = []

def do_work(ac):
    """Run Arduino connection worker function in a thread"""
    try:
        ac.doWork()
    except Exception as e:
        if file_logger is not None:
            file_logger.error(f"Error in Arduino worker thread: {str(e)}")
        else:
            print(f"Error in Arduino worker thread: {str(e)}")

def main_loop(stdscr, arduino_connections):
    """Main non-async loop for Arduino connections"""
    last_update = time.time()
    worker_threads = {}
    api_thread = None

    # Set up Arduino connections for the API
    set_arduino_connections(arduino_connections)
    
    # Start API server if running as daemon (no stdscr)
    if DEFAULT_API_ENABLED:
        api_thread = run_api_server_in_thread()
        if file_logger is not None:
            file_logger.info(f"API server started on port {DEFAULT_API_PORT}")
        else:
            logging.info(f"API server started on port {DEFAULT_API_PORT}")

    try:
        # Start worker threads for each Arduino connection
        for ac in arduino_connections:
            worker_thread = threading.Thread(target=do_work, args=(ac,), daemon=True)
            worker_thread.start()
            worker_threads[ac] = worker_thread
        
        # Main loop
        while True:
            # Restart any threads that have stopped
            for ac, thread in list(worker_threads.items()):
                if not thread.is_alive():
                    # Restart the thread
                    worker_thread = threading.Thread(target=do_work, args=(ac,), daemon=True)
                    worker_thread.start()
                    worker_threads[ac] = worker_thread
            
            if stdscr is None:
                time.sleep(0.05)
                continue
            
            # Handle UI if in console mode
            key = stdscr.getch()
            if key == ord('q'):
                break
            
            time.sleep(0.05)

    except KeyboardInterrupt:
        # Handle graceful shutdown
        for ac in arduino_connections:
            ac.serialConn.stopRxTask()
        if file_logger is not None:
            file_logger.info("Received keyboard interrupt, shutting down")
        print("Received keyboard interrupt, shutting down")
        sys.exit(0)
    except Exception as err:
        # Handle unexpected errors
        arduino_connections.clear()
        just_the_string = traceback.format_exc()
        if file_logger is not None:
            file_logger.critical(f'Error in main loop: {str(just_the_string)}')
        logging.critical(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit(1)
        
def main(stdscr=None):
    argumentList = sys.argv[1:]
    options = "hdo:p:"
    long_options = ["Help", "Devices", "Output=", "Profile="]
    target_profile = None
    devs = []
    launch_separate_console = False
    additional_args = []
    
    try:
        arguments, values = getopt.getopt(argumentList, options, long_options)
        for currentArgument, currentValue in arguments:
            if currentArgument in ("-h", "--Help"):
                print("Displaying Help")
            elif currentArgument in ("-d", "--devices"):
                print('Listing available Serial devices:')
                listDevices()
                sys.exit()
            elif currentArgument in ("-p", "--profile"):
                file_logger.debug(f'PYDEBUG: Profile: {currentValue}')
                target_profile = currentValue
            
    except getopt.error as err:
        just_the_string = traceback.format_exc()
        file_logger.debug(f'PYDEBUG: error: {str(just_the_string)}')
        print(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit()
        
    if target_profile is not None:
        try:
            devs = ArduinoYamlParser.parseYaml(path=target_profile)
        except Exception as err:
            just_the_string = traceback.format_exc()
            file_logger.debug(f'PYDEBUG: error: {str(just_the_string)}')
            print(f'PYDEBUG: error: {str(just_the_string)}\r\n')
            
            sys.exit(1)
    else:
        devs = locateProfile()
        for a in devs:
            arduino_map.append(ArduinoConnection(a))

    if len(devs) == 0:
        print('No Arduino profiles found in profile yaml!')
        sys.exit()

    arduino_connections = []
    try:
        for a in devs:
            c = ArduinoConnection(a)
            arduino_connections.append(c)
            file_logger.info(f'PYDEBUG: Loaded Arduino profile: {str(c)}')
    except Exception as err:
        just_the_string = traceback.format_exc()
        file_logger.debug(f'PYDEBUG: error: {str(just_the_string)}')
        print(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit()

    main_loop(stdscr, arduino_connections)

if __name__ == "__main__":
   main()