
import logging
import os
from pathlib import Path
import sys
import traceback
import zlib

import os
import psutil

import serial

from linuxcnc_arduinoconnector.ArduinoComms import ArduinoConnection
from linuxcnc_arduinoconnector.ConfigModels import DEFAULT_PROFILE



def try_load_linuxcnc():
    try:
        modulename = 'hal'
        if modulename not in sys.modules:
        #    print(f'You have not imported the {modulename} module')
            import hal
        modulename = 'linuxcnc'
        if modulename not in sys.modules:
        #    print(f'You have not imported the {modulename} module')
            import linuxcnc
    except ImportError:
        raise ImportError('Error. linuxcnc module not found. Hal emulation requires linuxcnc module.')
    return True
    
# taken from https://stackoverflow.com/questions/1742866/compute-crc-of-file-in-python
def forLoopCrc(fpath):
    """With for loop and buffer."""
    crc = 0
    with open(fpath, 'rb', 65536) as ins:
        for x in range(int((os.stat(fpath).st_size / 65536)) + 1):
            crc = zlib.crc32(ins.read(65536), crc)
    return (crc & 0xFFFFFFFF)
    
def listDevices():
    import serial.tools.list_ports
    for port in serial.tools.list_ports.comports():
        print(f'Device: {port}')

def locateProfile() -> list[ArduinoConnection]:
    from linuxcnc_arduinoconnector.YamlParser import ArduinoYamlParser
    #logging.debug(f'Starting up!')
    home_profile_loc = Path.home() / ".arduino" / DEFAULT_PROFILE #"profile.yaml"
    arduino_profiles = []
    if os.path.exists(home_profile_loc):
        logging.debug(f'PYDEBUG: Found config: {str(home_profile_loc)}')
        arduino_profiles = ArduinoYamlParser.parseYaml(path=home_profile_loc)
    elif os.path.exists(DEFAULT_PROFILE):
        logging.debug(f'PYDEBUG: Found config: {DEFAULT_PROFILE} in local directory')
        arduino_profiles = ArduinoYamlParser.parseYaml(path=DEFAULT_PROFILE)
    else:
        err = f'PYDEBUG: No profile yaml found!'
        logging.error(err)
        raise Exception(err)
    if len(arduino_profiles) == 0:
        err = f'PYDEBUG: No arduino properties found in porfile yaml!'
        logging.error(err)
        raise Exception(err)
    #for a in arduino_profiles:
    #    arduino_map.append(ArduinoConnection(a))
    return arduino_profiles



def format_elapsed_time(elapsed_time):
    days = elapsed_time.days
    hours, remainder = divmod(elapsed_time.seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{days}d {hours}h {minutes}m {seconds}s"




def get_parent_process_name():
    # Method 1: Using os module
    parentpid = os.getppid()
    try:
        parent = psutil.Process(parentpid)
        return parent.name()
    except psutil.NoSuchProcess:
        return None

def get_parent_process_cmdline():
    # Method 2: Using psutil to get full command line
    parent_pid = os.getppid()
    try:
        parent = psutil.Process(parent_pid)
        return parent.cmdline()
    except psutil.NoSuchProcess:
        return None

def check_parent_executable(target_executable):
    parent_name = get_parent_process_name()
    if parent_name and parent_name == target_executable:
        print(f"Script was launched by {target_executable}")
        return True
    else:
        print(f"Script was not launched by {target_executable}")
        return False

def launch_ui_in_new_console(additional_args=[]):
    import subprocess
    import shlex
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

async def do_work_async(ac):
    import asyncio
    import concurrent.futures
    loop = asyncio.get_event_loop()
    with concurrent.futures.ThreadPoolExecutor() as pool:
        await loop.run_in_executor(pool, ac.doWork)

async def main_async(arduino_connections):
    import asyncio
    task_status = {ac: None for ac in arduino_connections}
    while True:
        try:           
            for ac in arduino_connections:
                if task_status[ac] is None or task_status[ac].done():
                    task_status[ac] = asyncio.create_task(do_work_async(ac))
            await asyncio.sleep(0.05)
            continue

        except KeyboardInterrupt:
            for ac in arduino_connections:
                ac.serialConn.stopRxTask()
            raise SystemExit
        except Exception as err:
            arduino_connections.clear()
            just_the_string = traceback.format_exc()
            logging.critical(f'PYDEBUG: error: {str(just_the_string)}')
            pass
        
        
def launch_connector(target_profile:str):
    
    try:
        from linuxcnc_arduinoconnector.YamlParser import ArduinoYamlParser
        devs = ArduinoYamlParser.parseYaml(path=target_profile)
    except Exception as err:
        just_the_string = traceback.format_exc()
        raise Exception(just_the_string)
        sys.exit()

    if len(devs) == 0:
        raise Exception('No Arduino profiles found in profile yaml!')
        sys.exit()

    arduino_connections = []
    try:
        for a in devs:
            c = ArduinoConnection(a)
            arduino_connections.append(c)
            #logging.info(f'PYDEBUG: Loaded Arduino profile: {str(c)}')
    except Exception as err:
        just_the_string = traceback.format_exc()
        #logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit()
        
    import asyncio
    asyncio.run(main_async(arduino_connections))

