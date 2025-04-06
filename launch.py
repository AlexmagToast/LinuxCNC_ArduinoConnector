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
import signal

from linuxcnc_arduinoconnector.interfaces.LinuxCNCInterface import LinuxCNCInterface
from linuxcnc_arduinoconnector.utils.LoggingUtils import setup_logger, init_logger, get_logger, reconfigure_logger
from linuxcnc_arduinoconnector.utils.YamlParser import ArduinoYamlParser
from linuxcnc_arduinoconnector.interfaces.ArduinoComms import ArduinoConnection
from linuxcnc_arduinoconnector.api import set_arduino_connections, run_api_server_in_thread

from linuxcnc_arduinoconnector.utils.Utils import get_parent_process_name, try_load_linuxcnc, launch_connector, listDevices
from linuxcnc_arduinoconnector.config.Config import (
    DEFAULT_API_ENABLED, DEFAULT_LOG_LEVEL,
    DEFAULT_LOG_FILE_PATH, DEFAULT_API_PORT, DEFAULT_LOGGING_FORMAT,
)
# Since Linuxcnc launches this script using systemd, we can store the path to the script for later use
#base_directory = os.path.dirname(os.path.abspath(os.path.abspath(__file__)))

# Initialize default logger
logger = init_logger(log_file_path=DEFAULT_LOG_FILE_PATH, log_level=DEFAULT_LOG_LEVEL)

logger.info(f'Arduino Connector: Starting up...')


def launch_remote_debugger_listen(bind_addres:str, wait_on_connect=True, port=5678):
    import debugpy
    try:
        logger.info(f'Remote Debug Enabled based on override from Config.py, listening on port {port}')
        debugpy.listen((bind_addres,port))
        if wait_on_connect == True:
            logger.info(f'Waiting for remote debugger to connect...')
            debugpy.wait_for_client()
            pass
    except Exception as e:
        logger.error(f'Error launching remote debugger: {e}')
        print(f'Error launching remote debugger: {e}')


launchedByLinuxCNC = False
# get_parent_process_name() returns 'systemd' if Linuxcnc launches it, otherwise its something else like 'bash' 
#test = get_parent_process_cmdline()
maybe_linuxcnc = get_parent_process_name()
if maybe_linuxcnc== 'systemd':# try_load_linux will throw an exception if it fails
    logger.info(f'Arduino Connector: Detected LinuxCNC launch, loading linuxcnc interface...')
    launchedByLinuxCNC = True
    if try_load_linuxcnc():
        logger.info(f'Arduino Connector: Successfully loaded linuxcnc interface!')
    else:
        logger.error(f'Error loading linuxcnc, unable to load modules!')
        raise Exception(f'Error loading linuxcnc: unable to load modules!')
else:
    logger.info(f'Arduino Connector: Not launched by LinuxCNC, skipping module loading...')
    
linuxcnc_instance = None
if launchedByLinuxCNC:
    logger.info(f'Arduino Connector: Creating linuxcnc interface instance...')
    linuxcnc_instance = LinuxCNCInterface()
    if linuxcnc_instance.linuxcnc_error:
        logger.error(f'Error loading linuxcnc interface: {linuxcnc_instance.linuxcnc_error}')
        raise Exception(f'Error loading linuxcnc interface: {linuxcnc_instance.linuxcnc_error}')
    else:
        logger.info(f'Arduino Connector: Successfully created linuxcnc interface instance!')
    
    if linuxcnc_instance.remote_debug_enabled:
        logger.info(f'Arduino Connector: Remote Debug Enabled based on linuxcnc.ini, listening on port {linuxcnc_instance.remote_debug_port}')
        launch_remote_debugger_listen(linuxcnc_instance.remote_debug_bind_address, linuxcnc_instance.wait_on_remote_debug_connect, int(linuxcnc_instance.remote_debug_port))




arduino_map = []

def do_work(ac, logger):
    """Run Arduino connection worker function in a thread"""
    while True:
        try:
            ac.doWork()

        except KeyboardInterrupt:
            # Handle graceful shutdown
            ac.serialConn.stopRxTask()
            logger.info(f'Received keyboard interrupt during do_work, shutting down')
            print("DEBUG - Received keyboard interrupt during do_work, shutting down")
            sys.exit(0)
        except Exception as e:
                logger.error(f"Error in Arduino worker thread: {str(e)}")

def main_loop(arduino_connections, logger):
    """Main non-async loop for Arduino connections"""
    #last_update = time.time()
    worker_threads = {}
    #api_thread = None
    running = True

    # Set up signal handler for graceful shutdown
    def signal_handler(sig, frame):
        nonlocal running
        running = False
        logger.info(f'Received keyboard interrupt via signal handler, shutting down!!')
        print("DEBUG - Received keyboard interrupt via signal handler, shutting down!!")
        # Handle graceful shutdown
        try:
            logger.info(f'Stopping RxTask for all Arduino connections, Arduino connections: {len(arduino_connections)}')
            print(f"DEBUG - Stopping RxTask for all Arduino connections, Arduino connections: {len(arduino_connections)}")
            for ac in arduino_connections:
                ac.serialConn.stopRxTask()
        except Exception as e:
            logger.error(f'Error stopping RxTask: {str(e)}')
            print("DEBUG - Error stopping RxTask: {str(e)}")

    # Register signal handler
    logger.debug(f'Registering signal handler for SIGINT')
    original_sigint_handler = signal.getsignal(signal.SIGINT)
    signal.signal(signal.SIGINT, signal_handler)

    # Set up Arduino connections for the API
    logger.info(f'Setting up Arduino connections for the API')
    set_arduino_connections(arduino_connections)
    if DEFAULT_API_ENABLED:
        logger.info(f'Starting API server')
        api_thread = run_api_server_in_thread()
        logger.info(f"API server started on port {DEFAULT_API_PORT}")

    try:
        
        # Start worker threads for each Arduino connection
        logger.info(f'Starting worker threads for each Arduino connection')
        for ac in arduino_connections:
            if launchedByLinuxCNC and ac.hal_emulation == True:
                raise Exception(f'Error. ArduinoConnection::doWork, dev={ac.settings.dev}, alias={ac.settings.alias}. HAL emulation is enabled, but this is not supported when launched by LinuxCNC.')
            worker_thread = threading.Thread(target=do_work, args=(ac, logger), daemon=True)
            worker_thread.start()
            worker_threads[ac] = worker_thread
        
        # Main loop
        while running:
            # Restart any threads that have stopped
            #logger.debug(f'Checking for stopped worker threads..')
            for ac, thread in list(worker_threads.items()):
                if not thread.is_alive():
                    logger.debug(f'Restarting worker thread for Arduino connection: {ac}')
                    # Restart the thread
                    worker_thread = threading.Thread(target=do_work, args=(ac, logger), daemon=True)
                    worker_thread.start()
                    worker_threads[ac] = worker_thread
            
            time.sleep(0.01)
    except KeyboardInterrupt:
        # Handle graceful shutdown
        logger.info(f'Received keyboard interrupt in main loop, shutting down!')
        print("DEBUG - Received keyboard interrupt in main loop, shutting down!")
        for ac in arduino_connections:
            ac.serialConn.stopRxTask()

        #sys.exit(0)
    except Exception as err:
        # Handle unexpected errors
        arduino_connections.clear()
        just_the_string = traceback.format_exc()
        logger.critical(f'Error in main loop: {str(just_the_string)}')
        sys.exit(1)
    finally:
        # Restore original signal handler
        signal.signal(signal.SIGINT, original_sigint_handler)
        logger.info(f'Restored original signal handler for SIGINT')
        
def main(stdscr=None):
    argumentList = sys.argv[1:]
    options = "hdo:p:"
    long_options = ["Help", "Devices", "Output=", "Profile="]
    target_profile = None
    devs = []
    launch_separate_console = False
    additional_args = []
    
    logger.info(f'Starting main function')
    
    
    try:
        logger.info(f'Parsing command line arguments')
        arguments, values = getopt.getopt(argumentList, options, long_options)
        for currentArgument, currentValue in arguments:
            if currentArgument in ("-h", "--Help"):
                print("Displaying Help")
            elif currentArgument in ("-d", "--devices"):
                print('Listing available Serial devices:')
                listDevices()
                sys.exit()
            elif currentArgument in ("-p", "--profile"):
                logger.debug(f'PYDEBUG: Profile: {currentValue}')
                target_profile = currentValue
            
    except getopt.error as err:
        just_the_string = traceback.format_exc()
        logger.debug(f'PYDEBUG: error: {str(just_the_string)}')
        print(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit()
        
    if target_profile is not None:
        try:
            logger.info(f'Parsing YAML profile: {target_profile}')
            devs = ArduinoYamlParser.parseYaml(path=target_profile)
        except Exception as err:
            just_the_string = traceback.format_exc()
            logger.debug(f'PYDEBUG: error: {str(just_the_string)}')
            print(f'PYDEBUG: error: {str(just_the_string)}\r\n')
            
            sys.exit(1)
    elif launchedByLinuxCNC and linuxcnc_instance is not None:
        logger.info(f'Successfully created linuxcnc interface instance!')
        # Reconfigure the global logger with the LinuxCNC settings
        if linuxcnc_instance.log_file_path:
            reconfigure_logger(
                log_file_path=linuxcnc_instance.log_file_path, 
                log_level=linuxcnc_instance.log_level, 
                log_format=DEFAULT_LOGGING_FORMAT
            )
        logger.info(f'Successfully reconfigured logger with LinuxCNC settings')
        
        if os.path.exists(linuxcnc_instance.yaml_profile_path):
            logger.info(f'Parsing YAML profile: {linuxcnc_instance.yaml_profile_path}')
            devs = ArduinoYamlParser.parseYaml(path=linuxcnc_instance.yaml_profile_path)
            #for a in devs:
            #    arduino_map.append(ArduinoConnection(a))
        else:
            logger.error(f'Error. YAML_PROFILE_PATH not found in linuxcnc.ini: {linuxcnc_instance.yaml_profile_path}')
            sys.exit(1)
    if len(devs) == 0:
        logger.error('No Arduino profiles found in profile yaml!')
        raise Exception('No Arduino profiles found in profile yaml!')

    arduino_connections = []
    try:
        logger.info(f'Loading Arduino connections')
        for a in devs:
            logger.info(f'Loading Arduino connection: {a}')
            c = ArduinoConnection(a)
            arduino_connections.append(c)
            #file_logger.info(f'PYDEBUG: Loaded Arduino profile: {str(c)}')
    except Exception as err:
        just_the_string = traceback.format_exc()
        logger.error(f'Error loading Arduino connections: {str(just_the_string)}')
        raise Exception(f'Error loading Arduino connections: {str(just_the_string)}')

    logger.info(f'Starting main loop')
    main_loop(arduino_connections, logger)
    
if __name__ == "__main__":
   main()