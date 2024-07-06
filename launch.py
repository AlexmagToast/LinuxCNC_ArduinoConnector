#!/usr/bin/env python3
# Stage 1, check for debug environment variables
import os
from linuxcnc_arduinoconnector.Config import *

# Pre stage, look to see if the local config is set to enable logging by default. This can be helpful 
# First stage, figure out if LinuxCNC launched this script. If Linuxcnc is the host, then the user's profile will provide the settings to use.
file_logger = None
if DEFAULT_LOGGING_ENABLED:
    from linuxcnc_arduinoconnector.LoggingUtils import setup_logger
    file_logger = setup_logger('file_logger', log_file_path=os.path.join(log_file_path, log_file_name), log_format=logging_fmt)
    file_logger.debug('Starting up!')

'''
Since Linuxcnc launches the python script using a systemd daemon, environment variables are not accessible! Dang it.
The following code is basically useless as a result.


# Stage 1, set up logging
log_to_file = False
# Hint for later use of this env variable, echo 'export ARDUINO_CONNECTOR_ENABLE_LOG=1' >> ~/.bashrc && source ~/.bashrc
maybe_env_set = os.environ.get(EnvParams.ARDUINO_CONNECTOR_ENABLE_LOG)
if maybe_env_set is not None and maybe_env_set == '1':
    log_to_file = True

#logging_fmt = '%(message)s\r\n'
logging_fmt = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
maybe_env_set = os.environ.get(EnvParams.ARDUINO_CONNECTOR_LOG_FORMAT)
if maybe_env_set is not None:
    logging_fmt = maybe_env_set
    
log_file_path = '' # Path to where log files should be stored when enabled
# Hint for later use of this env variable, echo 'export ARDUINO_CONNECTOR_LOG_FILE_PATH=/home/ken/git-repos/LinuxCNC_ArduinoConnector-1' >> ~/.bashrc && source ~/.bashrc
maybe_env_set  = os.environ.get(EnvParams.ARDUINO_CONNECTOR_LOG_FILE_PATH)
if maybe_env_set is not None:
    log_file_path = maybe_env_set
    
log_file_name = 'arduino_connector.log' # Path to where log files should be stored when enabled
maybe_env_set  = os.environ.get(EnvParams.ARDUINO_CONNECTOR_LOGFILE_NAME)
if maybe_env_set is not None:
    log_file_name = maybe_env_set
    
file_logger = None
if log_to_file:
    from linuxcnc_arduinoconnector.LoggingUtils import setup_logger
    file_logger = setup_logger('file_logger', log_file_path=os.path.join(log_file_path, log_file_name), log_format=logging_fmt)
    file_logger.debug('Starting up!')
#import logging
#logging.basicConfig(level=logging.DEBUG, format='%(message)s\r\n')

enable_remote_debugger = False
maybe_env_set = os.environ.get(EnvParams.ARDUINO_CONNECTOR_ENABLE_REMOTE_DEBUG)
# Hint for later use of this env variable, echo 'export ARDUINO_CONNECTOR_ENABLE_REMOTE_DEBUG=1' >> ~/.bashrc && source ~/.bashrc
if maybe_env_set is not None and (maybe_env_set.lower() == 'true' or maybe_env_set == '1'):
    enable_remote_debugger = True
    if file_logger is not None:
        file_logger.debug(f'Remote Debug Enabled based on presence of Env Variable: {EnvParams.ARDUINO_CONNECTOR_ENABLE_REMOTE_DEBUG}')
    
enable_wait_on_remote_debug = False
maybe_env_set = os.environ.get(EnvParams.ARDUINO_CONNECTOR_WAIT_ON_REMOTE_DEBUG)
# Hint for later use of this env variable, echo 'export ARDUINO_CONNECTOR_WAIT_ON_REMOTE_DEBUG=1' >> ~/.bashrc && source ~/.bashrc
if maybe_env_set is not None and (maybe_env_set.lower() == 'true' or maybe_env_set == '1'):
    enable_wait_on_remote_debug = True
    if file_logger is not None:
        file_logger.debug(f'Remote Debug Wait Enabled based on presence of Env Variable: {EnvParams.ARDUINO_CONNECTOR_WAIT_ON_REMOTE_DEBUG}')

remote_debug_bind_address = '0.0.0.0'
maybe_env_set = os.environ.get(EnvParams.ARDUINO_CONNECTOR_REMOTE_DEBUG_BIND_ADDRESS)
if maybe_env_set is not None:
    remote_debug_bind_address = maybe_env_set
    if file_logger is not None:
        file_logger.debug(f'Remote Debug Bind Address set to {remote_debug_bind_address} based on presence of Env Variable: {EnvParams.ARDUINO_CONNECTOR_REMOTE_DEBUG_BIND_ADDRESS}')

remote_debug_listen_port = 5678
maybe_env_set = os.environ.get(EnvParams.ARDUINO_CONNECTOR_REMOTE_DEBUG_PORT)
if maybe_env_set is not None:
    remote_debug_listen_port = int(maybe_env_set)
    if file_logger is not None:
        file_logger.debug(f'Remote Debug Listen Port set to {remote_debug_listen_port} based on presence of Env Variable: {EnvParams.ARDUINO_CONNECTOR_REMOTE_DEBUG_PORT}')

def launch_debugger(wait_on_connect=False, port=5678):
    pass

import debugpy
port = 5678
debugpy.listen((remote_debug_bind_address,port))
print(f'Remote Debug Enabled, listening on port {port}')
if file_logger is not None:
    file_logger.debug(f'Remote Debug Enabled, bound to {remote_debug_bind_address} and listening on port {port}')
#if wait_on_connect:
    #print('Waiting for remote debugger to connect...')
#    if file_logger is not None:
#        file_logger.debug('Waiting for remote debugger to connect...')
#debugpy.wait_for_client()

if enable_remote_debugger:
    launch_debugger(wait_on_connect=enable_wait_on_remote_debug, port=remote_debug_listen_port)
# Stage 2, determine what launched this script. If its linuxcnc, then we need to handle errors in an appropriate way
from linuxcnc_arduinoconnector.Utils import get_parent_process_name, try_load_linuxcnc
launchedByLinuxCNC = False
# get_parent_process_name() returns 'systemd' if Linuxcnc launches it, otherwise its something else like 'bash' 
if get_parent_process_name() == 'systemd' and try_load_linuxcnc(): # try_load_linux will throw an exception if it fails
    launchedByLinuxCNC = True
    if file_logger is not None:
        file_logger.debug('Detected execution by LinuxCNC')
else:
    if file_logger is not None:
        file_logger.debug(f'Detected execution outside of LinuxCNC, parent process name {get_parent_process_name()}')
'''




# Stage 3, check for dependent modules.
import subprocess
import pkg_resources
import sys

class DebuggerExit(Exception):
    pass

def exit_program():
    print("Exiting program due to missing packages.")
    if sys.gettrace() is not None:
        raise DebuggerExit("Exiting program due to missing packages.")
    else:
        sys.exit(1)
def check_requirements(requirements_file='requirements.txt'):
    with open(requirements_file, 'r') as file:
        requirements = file.readlines()
    missing_packages = []
    for requirement in requirements:
        requirement = requirement.strip()
        if requirement:
            try:
                pkg_resources.require(requirement)
            except pkg_resources.DistributionNotFound:
                missing_packages.append(requirement)
    if missing_packages:
        print(f"The following packages are missing: {', '.join(missing_packages)}")
        user_input = input("Do you want to install the missing packages? (y/n) [n]: ").strip().lower()
        if user_input in ['yes', 'y']:
            print("Attempting to install missing packages...")
            subprocess.check_call([sys.executable, "-m", "pip", "install", *missing_packages])
            print("All required packages are now installed.")
        else:
            print("Exiting program due to missing packages.")
            sys.exit(1)

# Check requirements before running any logic
check_requirements()

# Your main program logic here
import asyncio
import concurrent.futures
import curses
import getopt

import sys
import time
import traceback
import os
import shlex

from linuxcnc_arduinoconnector.ArduinoComms import ArduinoConnection
from linuxcnc_arduinoconnector.Console import display_arduino_statuses, display_connection_details
from linuxcnc_arduinoconnector.Utils import listDevices, locateProfile
from linuxcnc_arduinoconnector.YamlParser import ArduinoYamlParser



arduino_map = []

async def do_work_async(ac):
    loop = asyncio.get_event_loop()
    with concurrent.futures.ThreadPoolExecutor() as pool:
        await loop.run_in_executor(pool, ac.doWork)

async def main_async(stdscr, arduino_connections):
    scroll_offset = 0
    selected_index = 0
    details_mode = False
    if stdscr is not None:
        stdscr.nodelay(1)  # Set nodelay mode
    last_update = time.time()
    task_status = {ac: None for ac in arduino_connections}

    while True:
        try:
            current_time = time.time()
            for ac in arduino_connections:
                if task_status[ac] is None or task_status[ac].done():
                    task_status[ac] = asyncio.create_task(do_work_async(ac))
            
            if stdscr is None:
                await asyncio.sleep(0.05)
                continue
            if current_time - last_update >= .01:
                if not details_mode:
                    # Sort connections by enabled/disabled status
                    enabled_connections = [conn for conn in arduino_connections if conn.settings.enabled]
                    disabled_connections = [conn for conn in arduino_connections if not conn.settings.enabled]
                    sorted_connections = enabled_connections + disabled_connections
                    
                    # Display statuses using sorted_connections
                    display_arduino_statuses(stdscr, sorted_connections, scroll_offset, selected_index)
            if current_time - last_update >= .5:
                if not details_mode:     
                    scroll_offset = (scroll_offset + 1) % (max(len(conn.settings.dev) for conn in arduino_connections) + 15)
                    last_update = current_time
            
            key = stdscr.getch()
            if key == ord('q'):
                break
            elif not details_mode and key == curses.KEY_UP and selected_index > 0:
                selected_index -= 1
            elif not details_mode and key == curses.KEY_DOWN and selected_index < len(arduino_connections) - 1:
                selected_index += 1
            elif not details_mode and (key == curses.KEY_ENTER or key in [10, 13]):
                details_mode = True
                stdscr.nodelay(1)  # Disable nodelay mode for details display
                # Use the sorted_connections to find the selected connection
                display_connection_details(stdscr, sorted_connections[selected_index])
            elif details_mode and key != -1:
                details_mode = False
                stdscr.nodelay(1)  # Re-enable nodelay mode
            elif details_mode:
                display_connection_details(stdscr, sorted_connections[selected_index])
            await asyncio.sleep(0.05)

        except KeyboardInterrupt:
            for ac in arduino_connections:
                ac.serialConn.stopRxTask()
            raise SystemExit
        except Exception as err:
            arduino_connections.clear()
            just_the_string = traceback.format_exc()
            logging.critical(f'PYDEBUG: error: {str(just_the_string)}')
            pass

def launch_ui_in_new_console(additional_args=[]):
    python_executable = sys.executable
    script_path = os.path.abspath(__file__)
    args_str = " ".join(additional_args)
    
    if sys.platform.startswith('win'):
        cmd = f'start cmd /c "{python_executable} {script_path} -o console {args_str}"'
        subprocess.Popen(cmd, shell=True)
    elif sys.platform.startswith('linux') or sys.platform == 'darwin':
        if sys.platform.startswith('linux'):
            terminals = [
                ('x-terminal-emulator', '-e'),
                ('gnome-terminal', '--'),
                ('konsole', '-e'),
                ('xfce4-terminal', '-e'),
                ('mate-terminal', '-e'),
                ('terminator', '-e'),
                ('urxvt', '-e'),
                ('xterm', '-e'),
            ]
            
            for term, arg in terminals:
                if subprocess.call(['which', term], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0:
                    cmd = f'{term} {arg} {python_executable} {script_path} -o console {args_str}'
                    break
            else:
                print("No suitable terminal found. Running in current console.")
                return False
        elif sys.platform == 'darwin':
            cmd = f"open -a Terminal.app {python_executable} {script_path} -o console {args_str}"
        
        try:
            subprocess.Popen(shlex.split(cmd))
        except Exception as e:
            print(f"Error launching UI: {e}")
            print("Running in current console.")
            return False
    else:
        print("Unsupported platform for launching separate console.")
        print("Running in current console.")
        return False
    
    return True

def main(stdscr=None):
    argumentList = sys.argv[1:]
    options = "hdo:p:"
    long_options = ["Help", "Devices", "Output=", "Profile="]
    target_profile = None
    devs = []
    launch_separate_console = False
    additional_args = []

    if stdscr == None:
        # Check for output=console first
        try:
            arguments, values = getopt.getopt(argumentList, options, long_options)
            for currentArgument, currentValue in arguments:
                if currentArgument in ("-o", "--Output"):
                    if currentValue == "console":
                        # prevent logging to console while showing the curses ui
                        logging.getLogger().setLevel(logging.CRITICAL)
                        curses.wrapper(main)
                        return
                    elif currentValue == "terminal":
                        launch_separate_console = True
                elif currentArgument not in ("-h", "-d"):
                    additional_args.extend([currentArgument, currentValue])
            
            # Add any remaining values to additional_args
            additional_args.extend(values)
        except getopt.error as err:
            just_the_string = traceback.format_exc()
            logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
            sys.exit()

        if launch_separate_console:
            launch_ui_in_new_console(additional_args)
            return

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
                logging.debug(f'PYDEBUG: Profile: {currentValue}')
                target_profile = currentValue
            
    except getopt.error as err:
        just_the_string = traceback.format_exc()
        logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit()

    if target_profile is not None:
        try:
            devs = ArduinoYamlParser.parseYaml(path=target_profile)
        except Exception as err:
            just_the_string = traceback.format_exc()
            logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
            sys.exit()
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
            logging.info(f'PYDEBUG: Loaded Arduino profile: {str(c)}')
    except Exception as err:
        just_the_string = traceback.format_exc()
        logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit()

    asyncio.run(main_async(stdscr, arduino_connections))

if __name__ == "__main__":
   main()