# HalInterface.py
import sys
import logging
from datetime import datetime
import threading
import time
import traceback
from linuxcnc_arduinoconnector.models.ConfigModels import HalPinDirection, HalPinTypes
from linuxcnc_arduinoconnector.utils.LoggingUtils import get_logger

# Remove these console handlers as we'll use our global logger
# console_handler = logging.StreamHandler()
# console_handler.setLevel(logging.DEBUG)
# formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
# console_handler.setFormatter(formatter)
# logging.getLogger().addHandler(console_handler)
# logging.getLogger().setLevel(logging.DEBUG)

class HalInterface:
    def __init__(self, hal_emulation=True):
        self.hal_emulation = hal_emulation
        self.linuxcnc = None
        self.linuxcnc_error = False
        self.linuxcnc_ini = None
        self.yaml_profile_path = None
        self.remote_debug_enabled = False
        self.remote_debug_port = 5678
        self.wait_on_remote_debug_connect = False
        self.logger = get_logger()  # Get logger once during initialization
        #self.logger.debug(f'HalInterface::__init__, hal_emulation: {self.hal_emulation}')
        if self.hal_emulation == False:
            self.logger.debug(f'HalInterface::__init__, hal_emulation: {self.hal_emulation}, loading linuxcnc')
            self.load_linuxcnc()
        else:
            self.logger.debug(f'HalInterface::__init__, hal_emulation: {self.hal_emulation}, skipping linuxcnc loading')

    def load_linuxcnc(self):
        from linuxcnc_arduinoconnector.utils.Utils import try_load_linuxcnc
        try:
            try_load_linuxcnc()
            self.linuxcnc = sys.modules['linuxcnc']
            self.hal = sys.modules['hal']
            self.logger.debug('Successfully loaded linuxcnc module.')
            #logging.debug('Successfully loaded linuxcnc module.')

        except ImportError:
            self.logger.error('Error. linuxcnc module not found. Switching to HAL emulation mode.')
            self.logger.error('Arduino Connector: Error. linuxcnc module not found!')
            #self.hal_emulation = True
            self.linuxcnc_error = True
        #except linuxcnc.error as ex:
        #    logging.error(f'Error loading linuxcnc: {str(ex)}')
        #    print(f'Arduino Connector: Error loading linuxcnc: {str(ex)}')
        #    self.linuxcnc_error = True
        except Exception as ex:
            #logging.error(f'Error loading linuxcnc: {str(ex)}')
            self.logger.error(f'Error loading linuxcnc: {str(ex)}')
            self.linuxcnc_error = True
    def register_component(self, component_name):
        if self.hal_emulation or self.linuxcnc_error:
            return None
        try:
            component = self.hal.component(component_name)
            self.logger.debug(f'Registered component {component_name} with HAL.')
            return component
        except Exception as ex:
            just_the_string = traceback.format_exc()
            error_string = f'Error registering component {component_name}: {just_the_string}, hal_emulation: {self.hal_emulation}, linuxcnc_error: {self.linuxcnc_error}'
            self.logger.error(error_string)
            raise Exception(error_string)

    def register_pin(self, component, pin_name, pin_type, pin_direction):
        if self.hal_emulation or self.linuxcnc_error:
            return None
        try:
            #import linuxcnc
            converted_type = None
            if pin_type == HalPinTypes.HAL_BIT:
                converted_type = self.hal.HAL_BIT
            elif pin_type == HalPinTypes.HAL_FLOAT:
                converted_type = self.hal.HAL_FLOAT
            else:
                self.logger.error(f'Invalid pin type: {pin_type}, pin_name: {pin_name}, pin_direction: {pin_direction}')
                raise Exception(f'Invalid pin type: {pin_type}')
            
            converted_dir = None
            if pin_direction == HalPinDirection.HAL_IN:
                converted_dir = self.hal.HAL_IN
            elif pin_direction == HalPinDirection.HAL_OUT:
                converted_dir = self.hal.HAL_OUT
            else:
                self.logger.error(f'Invalid pin direction: {pin_direction}, pin_name: {pin_name}, pin_type: {pin_type}')
                raise Exception(f'Invalid pin direction: {pin_direction}')
                
            pin = component.newpin(pin_name, converted_type, converted_dir)
            self.logger.info(f'Registered pin {pin_name} with HAL. pin_type: {pin_type}, pin_direction: {pin_direction}')
            return pin
        except Exception as ex:
            just_the_string = traceback.format_exc()
            error_string = f'Error registering pin {pin_name}: {just_the_string}, hal_emulation: {self.hal_emulation}, linuxcnc_error: {self.linuxcnc_error}'
            self.logger.error(error_string)
            raise Exception(error_string)
        except self.linuxcnc.error as ex:
            just_the_string = traceback.format_exc()
            error_string = f'Error registering pin {pin_name}: {just_the_string}, hal_emulation: {self.hal_emulation}, linuxcnc_error: {self.linuxcnc_error}'
            self.logger.error(error_string)
            raise Exception(error_string)
        except Exception as ex:
            just_the_string = traceback.format_exc()
            error_string = f'Error registering pin {pin_name}: {just_the_string}, hal_emulation: {self.hal_emulation}, linuxcnc_error: {self.linuxcnc_error}'
            self.logger.error(error_string)
            raise Exception(error_string)