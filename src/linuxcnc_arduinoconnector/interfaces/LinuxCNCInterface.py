# HalInterface.py
import sys

from linuxcnc_arduinoconnector.config.Config import DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_LOG_LEVEL_KEY, DEFAULT_LINUXCNC_PROFILE_INI_LOG_PATH_KEY, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_BIND_ADDRESS_KEY, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_KEY, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_PORT_KEY, DEFAULT_LINUXCNC_PROFILE_INI_WAIT_ON_REMOTE_DEBUG_CONNECT_KEY, DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY, DEFAULT_LOG_LEVEL, DEFAULT_REMOTE_DEBUG_BIND_ADDRESS, DEFAULT_REMOTE_DEBUG_PORT
from linuxcnc_arduinoconnector.utils.LoggingUtils import get_logger


class LinuxCNCInterface:
    def __init__(self):
        self.linuxcnc = None
        self.linuxcnc_error = False
        self.linuxcnc_ini = None
        self.yaml_profile_path = None
        self.remote_debug_enabled = False
        self.remote_debug_port = 5678
        self.wait_on_remote_debug_connect = False
        self.log_level = "INFO"
        self.log_file_path = None
        self.logger = get_logger()
        #if self.hal_emulation == False:
        self.load_linuxcnc()
        
    def load_linuxcnc(self):
        #from linuxcnc_arduinoconnector.utils.Utils import try_load_linuxcnc
        try:
            #try_load_linuxcnc()
            #self.linuxcnc = sys.modules['linuxcnc']
            #self.hal = sys.modules['hal']
            
            
            import linuxcnc
            import logging
            import time
            #logging.basicConfig(level=logging.DEBUG, format='%(message)s\r\n')

            
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
                        self.logger.error('No linuxcnc machine ini file found!')
                        raise Exception('No linuxcnc machine ini file found!')
                except Exception as e:
                    retries -= 1
                    time.sleep(2)
                    self.logger.error(f'Error loading ini file: {e}, retries left: {retries}')
            self.logger.info(f'INI FILE NAME = {stat.ini_filename}')
            inifile = linuxcnc.ini(stat.ini_filename)
            #launch_remote_debugger_listen(DEFAULT_REMOTE_DEBUG_BIND_ADDRESS, DEFAULT_REMOTE_DEBUG_WAIT_ON_CONNECT, DEFAULT_REMOTE_DEBUG_PORT)
            maybe_remote_debug = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_KEY) or False
            if maybe_remote_debug == '1' or maybe_remote_debug.lower() == 'true':
                self.logger.info('Remote debug enabled!')
                self.remote_debug_enabled = True
            else:
                self.logger.info('Remote debug disabled!')
                self.remote_debug_enabled = False
            maybe_wait_on_remote_debug = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_WAIT_ON_REMOTE_DEBUG_CONNECT_KEY) or False
            if maybe_wait_on_remote_debug == '1' or maybe_wait_on_remote_debug.lower() == 'true':
                self.logger.info('Waiting on remote debug connect!')
                self.wait_on_remote_debug_connect = True
            else:
                self.logger.info('Not waiting on remote debug connect!')
                self.wait_on_remote_debug_connect = False
            maybe_remote_debug_port = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_PORT_KEY) or DEFAULT_REMOTE_DEBUG_PORT
            if maybe_remote_debug_port is not None:
                self.logger.info(f'Remote debug port: {maybe_remote_debug_port}')
                self.remote_debug_port = maybe_remote_debug_port
            maybe_remote_debug_bind_address = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_BIND_ADDRESS_KEY) or DEFAULT_REMOTE_DEBUG_BIND_ADDRESS
            if maybe_remote_debug_bind_address is not None:
                self.logger.info(f'Remote debug bind address: {maybe_remote_debug_bind_address}')
                self.remote_debug_bind_address = maybe_remote_debug_bind_address    
            maybe_log_level = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_LOG_LEVEL_KEY) or DEFAULT_LOG_LEVEL
            if maybe_log_level is not None:
                self.log_level = maybe_log_level
                self.logger.info(f'Log level: {maybe_log_level}')
            maybe_log_file_path = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_LOG_PATH_KEY) or None
            if maybe_log_file_path is not None:
                self.log_file_path = maybe_log_file_path
                self.logger.info(f'Log file path: {maybe_log_file_path}')
            yaml_path = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY)
            if yaml_path is None:
                self.logger.error(f'Error. {DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY} not found in linuxcnc.ini')
                raise Exception(f'{DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY} not found in linuxcnc.ini')
            else:
                self.yaml_profile_path = yaml_path

            
            self.logger.info('Successfully loaded linuxcnc module.')

        except ImportError:
            self.logger.error('Arduino Connector: Error. linuxcnc module not found!')
            #self.hal_emulation = True
            self.linuxcnc_error = True
        except Exception as ex:
            self.logger.error(f'Arduino Connector: Error loading linuxcnc: {str(ex)}')
            self.linuxcnc_error = True


