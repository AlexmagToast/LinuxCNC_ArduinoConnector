# HalInterface.py
import sys
import logging

class HalInterface:
    def __init__(self, hal_emulation=True):
        self.hal_emulation = hal_emulation
        self.linuxcnc = None
        self.linuxcnc_error = False
        if self.hal_emulation:
            self.load_linuxcnc()

    def load_linuxcnc(self):
        from linuxcnc_arduinoconnector.Utils import try_load_linuxcnc
        try:
            try_load_linuxcnc()
            #self.linuxcnc = sys.modules['linuxcnc']
            logging.debug('Successfully loaded linuxcnc module.')
        except ImportError:
            logging.error('Error. linuxcnc module not found. Switching to HAL emulation mode.')
            #self.hal_emulation = True
            self.linuxcnc_error = True
            
    def register_component(self, component_name):
        if not self.hal_emulation or self.linuxcnc_error:
            return None
        try:
            component = self.linuxcnc.hal.component(component_name)
            logging.debug(f'Registered component {component_name} with HAL.')
            return component
        except self.linuxcnc.error as ex:
            logging.error(f'Error registering component {component_name}: {str(ex)}')
            return None

    def register_pin(self, component, pin_name, pin_type, pin_direction):
        if not self.hal_emulation or self.linuxcnc_error:
            return None
        try:
            pin = component.newpin(pin_name, pin_type, pin_direction)
            logging.debug(f'Registered pin {pin_name} with HAL.')
            return pin
        except self.linuxcnc.error as ex:
            logging.error(f'Error registering pin {pin_name}: {str(ex)}')
            return None