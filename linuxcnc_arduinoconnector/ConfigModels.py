from enum import Enum
from strenum import StrEnum

DEFAULT_PROFILE = "config.yaml"

'''
    YAML Parsing Objects
'''

# ConfigElement keys are the values which can be included in a YAML profile.


class ConfigElement(StrEnum):
    ARDUINO_KEY = 'mcu' 
    ALIAS = 'alias' 
    COMPONENT_NAME = 'component_name'
    HAL_EMULATION = 'hal_emulation' 
    DEV = 'dev'
    CONNECTION = 'connection'
    ENABLED = 'enabled'
    IO_MAP = 'io_map'
    def __str__(self) -> str:
        return self.value

class PinConfigElement(Enum):
    PIN_ID = ['pin_id', None]
    PIN_NAME = ['pin_name', None]
    PIN_TYPE = ['pin_type', None]
    PIN_INITIAL_STATE = ['pin_initial_state', -1]
    PIN_CONNECTED_STATE = ['pin_connected_state', -1]
    PIN_DISCONNECTED_STATE = ['pin_disconnected_state', -1]
    PIN_ENABLED = ['pin_enabled', True]
    def __str__(self) -> str:
        return self.value[0]
    
    def defaultValue(self):
        return self.value[1]
    
class AnalogConfigElement(Enum):
    PIN_SMOOTHING = ['pin_smoothing', 200]
    PIN_MIN_VALUE = ['pin_min_val', 0]
    PIN_MAX_VALUE = ['pin_max_val', 1023]
    def __str__(self) -> str:
        return self.value[0]
    
    def defaultValue(self):
        return self.value[1]
    
class DigitalConfigElement(Enum):
    PIN_DEBOUNCE = ['pin_debounce', 100]
    INPUT_PULLUP = ['input_pullup', False]
    def __str__(self) -> str:
        return self.value[0]
    def defaultValue(self):
        return self.value[1]

class ConnectionConfigElement(Enum):
    TIMEOUT = ['timeout', 5000]
    TYPE = ['type', None]
    def defaultValue(self):
        return self.value[1]

class UDPConfigElement(Enum):
    ARDUINO_IP = ['arduino_ip', None]
    ARDUINO_PORT = ['arduino_port', 54321]
    LISTEN_PORT = ['listen_port', 54321] # optional, default is set to arduino_port  
    def __str__(self) -> str:
        return self.value[0]
    def defaultValue(self):
        return self.value[1]

class SerialConfigElement(Enum):
    BAUDRATE = ['baudrate', 115200]
    def __str__(self) -> str:
        return self.value[0]
    def defaultValue(self):
        return self.value[1]

class ConfigConnectionTypes(Enum):
    SERIAL = ['Serial', lambda yaml : None]
    UDP = ['UDP', lambda yaml: None]
    def __str__(self) -> str:
        return self.value[0]
    
class PinTypes(StrEnum):
    ANALOG_INPUT = 'ain'
    ANALOG_OUTPUT = 'aout'
    DIGITAL_INPUT = 'din'
    DIGITAL_OUTPUT = 'dout'
    BINARY_SELECTOR_SWITCH = 'binSel'
    UNDEFINED = 'undefined'
    def __str__(self) -> str:
        return self.value

class HalPinTypes(StrEnum):
    HAL_BIT = 'HAL_BIT'
    HAL_FLOAT = 'HAL_FLOAT'
    UNDEFINED = 'undefined'
    def __str__(self) -> str:
        return self.value
    
class HalPinDirection(StrEnum):
    HAL_IN = 'HAL_IN'
    HAL_OUT = 'HAL_OUT'
    HAL_IO = 'HAL_IO'
    UNDEFINED = 'undefined'
    def __str__(self) -> str:
        return self.value



class ArduinoPin:
    def __init__(self, pinName:str='', 
                 featureID:int=0, 
                 pinID:str='', 
                 pinType:PinTypes=PinTypes.UNDEFINED, 
                 halPinType:HalPinTypes=HalPinTypes.UNDEFINED, 
                 halPinDirection:HalPinDirection=HalPinDirection.UNDEFINED, 
                 yaml:dict=None):
        self.pinName = pinName
        self.pinType = pinType
        self.halPinType = halPinType
        self.halPinDirection = halPinDirection
        self.pinID = pinID
        self.featureID = featureID
        self.pinInitialState = PinConfigElement.PIN_INITIAL_STATE.defaultValue()
        self.pinConnectedState = PinConfigElement.PIN_CONNECTED_STATE.defaultValue()
        self.pinDisconnectedState = PinConfigElement.PIN_DISCONNECTED_STATE.defaultValue()
        self.pinEnabled = PinConfigElement.PIN_ENABLED.defaultValue()
        self.halPinConnection = None
        self.halPinCurrentValue = 0
        self.pinLogicalID = 0
        self.pinConfigSynced = False
        
        if yaml is not None:
            self.parseYAML(yaml)

    def parseYAML(self, doc):
        required_keys = [
            PinConfigElement.PIN_ID.value[0]
        ]

        for key in required_keys:
            if key not in doc:
                raise ValueError(f'Error: {key} undefined in config yaml')
        
        self.pinID = doc[PinConfigElement.PIN_ID.value[0]]
        optional_mappings = {
            PinConfigElement.PIN_ENABLED.value[0]: 'pinEnabled',
            PinConfigElement.PIN_TYPE.value[0]: 'halPinType',
            PinConfigElement.PIN_NAME.value[0]: 'pinName',
            PinConfigElement.PIN_INITIAL_STATE.value[0]: 'pinInitialState',
            PinConfigElement.PIN_DISCONNECTED_STATE.value[0]: 'pinDisconnectedState',
            PinConfigElement.PIN_CONNECTED_STATE.value[0]: 'pinConnectedState'
        }

        for yaml_key, attr_name in optional_mappings.items():
            if yaml_key in doc:
                value = doc[yaml_key]
                if attr_name == 'halPinType':
                    value = HalPinTypes(str(value).upper())
                setattr(self, attr_name, value)

        if not self.pinName:
            self.pinName = f"{self.pinType.value}"
        #try:
        #    import blahblah
        #except ImportError:
        #    print(f'You have not imported the blahblah module')
        #modulename = 'linuxcnc'
        #if modulename not in sys.modules:
        #    print(f'You have not imported the {modulename} module')

    def __str__(self) -> str:
        return (f'pinName = {self.pinName}, pinType = {self.pinType.name}, halPinType = {self.halPinType}, '
                f'pinEnabled = {self.pinEnabled}, pinInitialState = {self.pinInitialState}, '
                f'pinConnectedState = {self.pinConnectedState}, pinDisconnectedState = {self.pinDisconnectedState}')
    
    def toJson(self):
        return {
            'fi': self.featureID,
            'id': self.pinID,
            'li': self.pinLogicalID,
            'is': self.pinInitialState,
            'cs': self.pinConnectedState,
            'ds': self.pinDisconnectedState
        }
    
class AnalogPin(ArduinoPin):
    def __init__(self, yaml:dict=None, featureID:int=0, halPinDirection=HalPinDirection.UNDEFINED):
        pinType = PinTypes.ANALOG_INPUT if halPinDirection in [HalPinDirection.HAL_IN, HalPinDirection.HAL_IO] else PinTypes.ANALOG_OUTPUT
        super().__init__(featureID=featureID, pinType=pinType, halPinType=HalPinTypes.HAL_FLOAT, halPinDirection=halPinDirection, yaml=yaml)

        # Set the defaults, which can be overridden through the yaml profile
        self.pinSmoothing = AnalogConfigElement.PIN_SMOOTHING.defaultValue()
        self.pinMinVal = AnalogConfigElement.PIN_MIN_VALUE.defaultValue()
        self.pinMaxVal = AnalogConfigElement.PIN_MAX_VALUE.defaultValue()

        if yaml is not None: 
            self.parseYAML(yaml)

    def parseYAML(self, doc):
        if AnalogConfigElement.PIN_SMOOTHING.value[0] in doc:
            self.pinSmoothing = int(doc[AnalogConfigElement.PIN_SMOOTHING.value[0]])
        if AnalogConfigElement.PIN_MIN_VALUE.value[0] in doc:
            self.pinMinVal = int(doc[AnalogConfigElement.PIN_MIN_VALUE.value[0]])
        if AnalogConfigElement.PIN_MAX_VALUE.value[0] in doc:
            self.pinMaxVal = int(doc[AnalogConfigElement.PIN_MAX_VALUE.value[0]])

        # Also parse the parent class YAML settings
        super().parseYAML(doc)

    def __str__(self) -> str:
        return (f'\npinID={self.pinID}, pinName={self.pinName}, pinType={self.pinType.name}, '
                f'halPinDirection={self.halPinDirection}, halPinType={self.halPinType}, '
                f'pinSmoothing={self.pinSmoothing}, pinMinVal={self.pinMinVal}, pinMaxVal={self.pinMaxVal}')

    def toJson(self):
        s = super().toJson()
        s.update({
            'ps': self.pinSmoothing,
            'pm': self.pinMaxVal,
            'pn': self.pinMinVal
        })
        return s

class DigitalPin(ArduinoPin):
    def __init__(self, halPinDirection: HalPinDirection, featureID: int = 0, yaml: dict = None):
        pinType = PinTypes.DIGITAL_INPUT if halPinDirection in [HalPinDirection.HAL_IN, HalPinDirection.HAL_IO] else PinTypes.DIGITAL_OUTPUT
        super().__init__(pinType=pinType, featureID=featureID, halPinType=HalPinTypes.HAL_BIT, halPinDirection=halPinDirection, yaml=yaml)

        # Set the defaults, which can be overridden through the yaml profile
        self.pinDebounce = DigitalConfigElement.PIN_DEBOUNCE.defaultValue()
        self.inputPullup = DigitalConfigElement.INPUT_PULLUP.defaultValue()

        if yaml is not None:
            self.parseYAML(yaml)

    def parseYAML(self, doc):
        if DigitalConfigElement.PIN_DEBOUNCE.value[0] in doc:
            self.pinDebounce = int(doc[DigitalConfigElement.PIN_DEBOUNCE.value[0]])
        if DigitalConfigElement.INPUT_PULLUP.value[0] in doc:
            self.inputPullup = bool(doc[DigitalConfigElement.INPUT_PULLUP.value[0]])

        # Also parse the parent class YAML settings
        super().parseYAML(doc)

    def __str__(self) -> str:
        return (f'\npinID={self.pinID}, pinName={self.pinName}, pinType={self.pinType.name}, '
                f'halPinDirection={self.halPinDirection}, halPinType={self.halPinType}, '
                f'pinDebounce={self.pinDebounce}, inputPullup={self.inputPullup}')

    def toJson(self):
        s = super().toJson()
        s.update({
            'pd': self.pinDebounce,
            'ip': self.inputPullup
        })
        return s
    
    '''
    MCU state control objects and helper classes
'''
class ArduinoSettings:
    def __init__(self, alias='undefined', component_name='arduino', dev='undefined', hal_emulation=False):
        self.alias = alias
        self.component_name = component_name
        self.dev = dev
        self.baud_rate = 115200
        self.connection_timeout = 10
        self.io_map = {}
        self.yamlProfileSignature = 0# CRC32 for now
        self.enabled = True
        self.hal_emulation = hal_emulation
        #if self.hal_emulation == False:
        #    try:
        #        try_load_linuxcnc()
        #    except ImportError:
        #        raise ImportError('Error. linuxcnc module not found. Hal emulation requires linuxcnc module.')


    def printIOMap(self) -> str:
        s = ''
        for k,v in self.io_map.items():
            s += f'\nPin Type = {k}, values: ['
            for p in v:
                s += str(p)
            s += ']'
        return s
            
    def __str__(self) -> str:
        return f'alias = {self.alias}, component_name={self.component_name}, dev={self.dev}, hal_emulation={self.hal_emulation}, io_map = {self.printIOMap()}'
    
    def configJSON(self):
        pc = {}
        for k, v in self.io_map.items():
            pc[k.featureID] = {}  #k.value[FEATURE_INDEX_KEY]] = {}
            #pc[k.name + '_COUNT'] = len(v)
            i = 0
            for pin in v:
                if pin.pinEnabled == True:
                    pin.pinLogicalID = i
                    p = pin.toJson()
                    pc[k.featureID][i] = pin.toJson() # pc[k.value[FEATURE_INDEX_KEY]][i] = pin.toJson()
                    i += 1
        return pc