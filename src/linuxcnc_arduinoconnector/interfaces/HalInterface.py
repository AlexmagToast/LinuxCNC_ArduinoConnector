# HalInterface.py
import sys
import logging
from datetime import datetime
import threading
import time
from linuxcnc_arduinoconnector.models.ConfigModels import HalPinDirection, HalPinTypes
# Add console logging handler
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
console_handler.setFormatter(formatter)
logging.getLogger().addHandler(console_handler)
logging.getLogger().setLevel(logging.DEBUG)
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
        #if self.hal_emulation == False:
        self.load_linuxcnc()

    def load_linuxcnc(self):
        from linuxcnc_arduinoconnector.utils.Utils import try_load_linuxcnc
        try:
            try_load_linuxcnc()
            self.linuxcnc = sys.modules['linuxcnc']
            self.hal = sys.modules['hal']
            print(f'Arduino Connector: Successfully loaded linuxcnc modules.')
            #logging.debug('Successfully loaded linuxcnc module.')

        except ImportError:
            logging.error('Error. linuxcnc module not found. Switching to HAL emulation mode.')
            print(f'Arduino Connector: Error. linuxcnc module not found!')
            #self.hal_emulation = True
            self.linuxcnc_error = True
        #except linuxcnc.error as ex:
        #    logging.error(f'Error loading linuxcnc: {str(ex)}')
        #    print(f'Arduino Connector: Error loading linuxcnc: {str(ex)}')
        #    self.linuxcnc_error = True
        except Exception as ex:
            #logging.error(f'Error loading linuxcnc: {str(ex)}')
            print(f'Arduino Connector: Error loading linuxcnc: {str(ex)}')
            self.linuxcnc_error = True
    def register_component(self, component_name):
        if not self.hal_emulation or self.linuxcnc_error:
            return None
        try:
            component = self.hal.component(component_name)
            logging.debug(f'Registered component {component_name} with HAL.')
            return component
        except Exception as ex:
            logging.error(f'Error registering component {component_name}: {str(ex)}')
            return None

    def register_pin(self, component, pin_name, pin_type, pin_direction):
        if not self.hal_emulation or self.linuxcnc_error:
            return None
        try:
            #import linuxcnc
            converted_type = None
            if pin_type == HalPinTypes.HAL_BIT:
                converted_type = self.hal.HAL_BIT
            elif pin_type == HalPinTypes.HAL_FLOAT:
                converted_type = self.hal.HAL_FLOAT
            else:
                raise Exception(f'Invalid pin type: {pin_type}')
            
            converted_dir = None
            if pin_direction == HalPinDirection.HAL_IN:
                converted_dir = self.hal.HAL_IN
            elif pin_direction == HalPinDirection.HAL_OUT:
                converted_dir = self.hal.HAL_OUT
            else:
                raise Exception(f'Invalid pin direction: {pin_direction}')
                
            pin = component.newpin(pin_name, converted_type, converted_dir)
            logging.debug(f'Registered pin {pin_name} with HAL.')
            return pin
        except Exception as ex:
            logging.error(f'Error registering pin {pin_name}: {str(ex)}')
            return None
        except self.linuxcnc.error as ex:
            logging.error(f'Error registering pin {pin_name}: {str(ex)}')
            return None