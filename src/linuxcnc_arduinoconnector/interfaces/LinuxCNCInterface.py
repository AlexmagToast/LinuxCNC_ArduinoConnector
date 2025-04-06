# HalInterface.py
import sys

from linuxcnc_arduinoconnector.config.Config import DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_LOG_LEVEL_KEY, DEFAULT_LINUXCNC_PROFILE_INI_LOG_PATH_KEY, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_BIND_ADDRESS_KEY, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_KEY, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_PORT_KEY, DEFAULT_LINUXCNC_PROFILE_INI_WAIT_ON_REMOTE_DEBUG_CONNECT_KEY, DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY, DEFAULT_LOG_LEVEL, DEFAULT_REMOTE_DEBUG_BIND_ADDRESS, DEFAULT_REMOTE_DEBUG_PORT


class LinuxCNCInterface:
    def __init__(self, hal_emulation=True):
        self.linuxcnc = None
        self.linuxcnc_error = False
        self.linuxcnc_ini = None
        self.yaml_profile_path = None
        self.remote_debug_enabled = False
        self.remote_debug_port = 5678
        self.wait_on_remote_debug_connect = False
        self.log_level = "INFO"
        self.log_file_path = None
        #if self.hal_emulation == False:
        self.load_linuxcnc()

    def load_linuxcnc(self):
        from linuxcnc_arduinoconnector.utils.Utils import try_load_linuxcnc
        try:
            try_load_linuxcnc()
            self.linuxcnc = sys.modules['linuxcnc']
            self.hal = sys.modules['hal']
            
            
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
                        raise Exception('No ini file found')
                except Exception as e:
                    retries -= 1
                    time.sleep(2)
                    print(f'Error loading ini file: {e}')
            print(f'INI FILE NAME = {stat.ini_filename}')
            inifile = linuxcnc.ini(stat.ini_filename)
            #launch_remote_debugger_listen(DEFAULT_REMOTE_DEBUG_BIND_ADDRESS, DEFAULT_REMOTE_DEBUG_WAIT_ON_CONNECT, DEFAULT_REMOTE_DEBUG_PORT)
            maybe_remote_debug = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_KEY) or False
            if maybe_remote_debug == '1' or maybe_remote_debug.lower() == 'true':
                self.remote_debug_enabled = True
            else:
                self.remote_debug_enabled = False
            maybe_wait_on_remote_debug = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_WAIT_ON_REMOTE_DEBUG_CONNECT_KEY) or False
            if maybe_wait_on_remote_debug == '1' or maybe_wait_on_remote_debug.lower() == 'true':
                self.wait_on_remote_debug_connect = True
            else:
                self.wait_on_remote_debug_connect = False
            maybe_remote_debug_port = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_PORT_KEY) or DEFAULT_REMOTE_DEBUG_PORT
            if maybe_remote_debug_port is not None:
                self.remote_debug_port = maybe_remote_debug_port
            maybe_remote_debug_bind_address = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_BIND_ADDRESS_KEY) or DEFAULT_REMOTE_DEBUG_BIND_ADDRESS
            if maybe_remote_debug_bind_address is not None:
                self.remote_debug_bind_address = maybe_remote_debug_bind_address    
            maybe_log_level = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_LOG_LEVEL_KEY) or DEFAULT_LOG_LEVEL
            if maybe_log_level is not None:
                self.log_level = maybe_log_level
                
            maybe_log_file_path = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_LOG_PATH_KEY) or None
            if maybe_log_file_path is not None:
                self.log_file_path = maybe_log_file_path

            yaml_path = inifile.find(DEFAULT_LINUXCNC_PROFILE_INI_HEADER, DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY)
            if yaml_path is None:
                print(f'Error. YAML_PROFILE_PATH not found in linuxcnc.ini')
                raise Exception('YAML_PROFILE_PATH not found in linuxcnc.ini')
            else:
                self.yaml_profile_path = yaml_path

            
            print('Successfully loaded linuxcnc module.')

        except ImportError:
            print(f'Arduino Connector: Error. linuxcnc module not found!')
            #self.hal_emulation = True
            self.linuxcnc_error = True
        #except linuxcnc.error as ex:
        #    print(f'Arduino Connector: Error loading linuxcnc: {str(ex)}')
        #    self.linuxcnc_error = True
        except Exception as ex:
            print(f'Arduino Connector: Error loading linuxcnc: {str(ex)}')
            self.linuxcnc_error = True


