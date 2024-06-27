
import logging
import os
from pathlib import Path
import sys
import zlib

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
        err = f'PYDEBUG: No porfile yaml found!'
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