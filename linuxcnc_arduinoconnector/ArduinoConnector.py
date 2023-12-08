from multiprocessing import Value
import os
from re import T

import serial
import msgpack
from strenum import StrEnum
from enum import Enum, IntEnum
from threading import Thread
from queue import Queue
import time
import crc8
import traceback
import logging
import numpy
import socket
from cobs import cobs
import yaml

logging.basicConfig(level=logging.DEBUG)

DEFAULT_VALUE_KEY = 1 # List position for default values
YAML_PARSER_KEY = 1

class ConfigElement(StrEnum):
    ARDUINO_KEY = 'mcu'
    ALIAS = 'alias'
    COMPONENT_NAME = 'component_name'
    DEV = 'dev'
    CONNECTION = 'connection'
    IO_MAP = 'io_map'
    def __str__(self) -> str:
        return self.value
    
class PinConfigElement(Enum):
    PIN_ID = ['pin_id', None]
    PIN_NAME = ['pin_name', None]
    PIN_TYPE = ['pin_type', None]
    PIN_INITIAL_STATE = ['pin_initial_state', -1]
    PIN_CONNECT_STATE = ['pin_connect_state', -1]
    PIN_DISCONNECT_STATE = ['pin_disconnect_state', -1]
    def __str__(self) -> str:
        return self.value[0]
    
class AnalogConfigElement(Enum):
    PIN_SMOOTHING = ['pin_smoothing', 200]
    PIN_MIN_VALUE = ['pin_min_val', 0]
    PIN_MAX_VALUE = ['pin_max_val', 1023]
    def __str__(self) -> str:
        return self.value[0]
    
class DigitalConfigElement(Enum):
    PIN_DEBOUNCE = ['pin_debounce', 1]
    def __str__(self) -> str:
        return self.value[0]

class ConnectionConfigElement(Enum):
    TIMEOUT = ['timeout', 5000]
    TYPE = ['type', None]

class UDPConfigElement(Enum):
    ARDUINO_IP = ['arduino_ip', None]
    ARDUINO_PORT = ['arduino_port', 54321]
    LISTEN_PORT: ['listen_port', 54321] # optional, default is set to arduino_port  
    def __str__(self) -> str:
        return self.value[0]

class SerialConfigElement(Enum):
    BAUDRATE = ['baudrate', 115200]
    def __str__(self) -> str:
        return self.value[0]
# ConfigPinTypes enum now includes both a string constant and a parsing lamda for each IO/feature type
# As shown below, the YAML can be read in and the lamda for the feature can be called so all of
# the associated settings for a given feature get auto-magically parsed from the YAML.
# See the YAML parsing method in the YamlArduinoParser class below for an example of how this
# enum + lamda enables some dark magic elegance.
    
class ConfigPinTypes(Enum):
    ANALOG_INPUTS = ['analogInputs', lambda yaml : AnalogPin(yaml=yaml, halPinDirection=HalPinDirection.HAL_IN)]
    ANALOG_OUTPUTS = ['analogOutputs',  lambda yaml : AnalogPin(yaml=yaml, halPinDirection=HalPinDirection.HAL_OUT)]
    PWM_OUTPUTS = ['pwmOutputs',  lambda yaml : ArduinoPin(yaml=yaml, pinType=ConfigPinTypes.PWM_OUTPUTS, halPinDirection=HalPinDirection.HAL_OUT)]
    DIGITAL_INPUTS = ['digitalInputs', lambda yaml : DigitalPin(yaml=yaml, halPinDirection=HalPinDirection.HAL_IN)]
    DIGITAL_OUTPUTS = ['digitalOutputs', lambda yaml : DigitalPin(yaml=yaml, halPinDirection=HalPinDirection.HAL_OUT)]
    def __str__(self) -> str:
        return self.value[0]
    
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
    UNDEFINED = 'undefined'
    def __str__(self) -> str:
        return self.value


class ConnectionType:
    def __init__(self, yaml:dict = None):
        pass


class ArduinoPin:
    def __init__(self, pinName:str='', pinID:str='', pinType:PinTypes=PinTypes.UNDEFINED, halPinType:HalPinTypes=HalPinTypes.UNDEFINED, 
                 halPinDirection:HalPinDirection=HalPinDirection.UNDEFINED, yaml:dict = None): 
        self.pinName = pinName
        self.pinType = pinType
        self.halPinType = halPinType
        self.halPinDirection = halPinDirection
        self.pinID = pinID
        self.pinInitialState = PinConfigElement.PIN_INITIAL_STATE.value[DEFAULT_VALUE_KEY]
        self.pinConnectState = PinConfigElement.PIN_CONNECT_STATE.value[DEFAULT_VALUE_KEY]
        self.pinDisconnectState = PinConfigElement.PIN_DISCONNECT_STATE.value[DEFAULT_VALUE_KEY]
        #if yaml != None: self.parseYAML(doc=yaml)

    def parseYAML(self, doc):
        if PinConfigElement.PIN_ID.value[0] not in doc.keys():
            raise Exception(f'Error. {PinConfigElement.PIN_ID.value[0]} undefined in config yaml')
        self.pinID = doc[PinConfigElement.PIN_ID.value[0]]
        if PinConfigElement.PIN_TYPE.value[0] in doc.keys():
            self.halPinType = HalPinTypes(str(doc[PinConfigElement.PIN_TYPE.value[0]]).upper())
        if PinConfigElement.PIN_NAME.value[0] in doc.keys():    
            self.pinName = doc[PinConfigElement.PIN_NAME.value[0]]
        else:
            self.pinName = f"{self.pinType.value}"
        if PinConfigElement.PIN_INITIAL_STATE.value[0] in doc.keys():    
            self.pinInitialState = doc[PinConfigElement.PIN_INITIAL_STATE.value[0]]
        if PinConfigElement.PIN_DISCONNECT_STATE.value[0] in doc.keys():    
            self.pinDisconnectState = doc[PinConfigElement.PIN_DISCONNECT_STATE.value[0]]
        if PinConfigElement.PIN_CONNECT_STATE.value[0] in doc.keys():    
            self.pinConnectState = doc[PinConfigElement.PIN_CONNECT_STATE.value[0]]

    def __str__(self) -> str:
        return f'pinName = {self.pinName}, pinType={self.pinType.name}, halPinType={self.halPinType}'
    
class AnalogPin(ArduinoPin):
    def __init__(self,yaml:dict = None, halPinDirection=HalPinDirection):
        if halPinDirection == HalPinDirection.HAL_IN:
            ArduinoPin.__init__(self, pinType=PinTypes.ANALOG_INPUT, 
                        halPinType=HalPinTypes.HAL_FLOAT, halPinDirection=halPinDirection,
                        yaml=yaml)
        else:
            ArduinoPin.__init__(self, pinType=PinTypes.ANALOG_OUTPUT, 
                        halPinType=HalPinTypes.HAL_FLOAT, halPinDirection=halPinDirection,
                        yaml=yaml)
        
        # set the defaults, which can be overriden through the yaml profile
        self.pinSmoothing = AnalogConfigElement.PIN_SMOOTHING.value[DEFAULT_VALUE_KEY] #smoothing const   #optional
        self.pinMinVal = AnalogConfigElement.PIN_MIN_VALUE.value[DEFAULT_VALUE_KEY] #minimum value         #optional     these could be used to convert the value to 0-10 for example
        self.pinMaxVal = AnalogConfigElement.PIN_MAX_VALUE.value[DEFAULT_VALUE_KEY] #maximum value      #optional
        
        if yaml != None: 
            self.parseYAML(doc=yaml)
            ArduinoPin.parseYAML(self, doc=yaml)

    def __str__(self) -> str:
        return f'\npinID={self.pinID}, pinName = {self.pinName}, pinType={self.pinType.name}, '\
            f'halPinDirection = {self.halPinDirection}, halPinType={self.halPinType}, pinSmoothing={self.pinSmoothing}, '\
            f'pinMinVal={self.pinMinVal}, pinMaxVal={self.pinMaxVal}'  
    
    def parseYAML(self, doc):
        if AnalogConfigElement.PIN_SMOOTHING.value[0] in doc.keys():
            self.pinSmoothing = int(doc[AnalogConfigElement.PIN_SMOOTHING.value[0]])
        if AnalogConfigElement.PIN_MIN_VALUE.value[0] in doc.keys():
            self.pinMinVal = int(doc[AnalogConfigElement.PIN_MIN_VALUE.value[0]])
        if AnalogConfigElement.PIN_MAX_VALUE.value[0] in doc.keys():
            self.pinMaxVal = int(doc[AnalogConfigElement.PIN_MAX_VALUE.value[0]])

class DigitalPin(ArduinoPin):
    def __init__(self,  halPinDirection:HalPinDirection, yaml:dict = None):
        if halPinDirection == HalPinDirection.HAL_IN:
            ArduinoPin.__init__(self, pinType=PinTypes.DIGITAL_INPUT, 
                        halPinType=HalPinTypes.HAL_BIT, halPinDirection=halPinDirection,
                        yaml=yaml)
        else:
            ArduinoPin.__init__(self, pinType=PinTypes.DIGITAL_OUTPUT, 
                        halPinType=HalPinTypes.HAL_BIT, halPinDirection=halPinDirection,
                        yaml=yaml)
        
        # set the defaults, which can be overriden through the yaml profile
        self.pinDebounce = DigitalConfigElement.PIN_DEBOUNCE.value[DEFAULT_VALUE_KEY] #smoothing const   #optional

        if yaml != None: 
            self.parseYAML(doc=yaml)
            ArduinoPin.parseYAML(self, doc=yaml)

    def __str__(self) -> str:
        return f'\npinID={self.pinID}, pinName = {self.pinName}, pinType={self.pinType.name}, '\
            f'halPinDirection = {self.halPinDirection}, halPinType={self.halPinType}, pinDebounce={self.pinDebounce}'
    
    def parseYAML(self, doc):
        if DigitalConfigElement.PIN_DEBOUNCE.value[0] in doc.keys():
            self.pinDebounce = int(doc[DigitalConfigElement.PIN_DEBOUNCE.value[0]])





class ArduinoSettings:
    def __init__(self, alias='undefined', component_name='arduino', dev='undefined'):
        self.alias = alias
        self.component_name = component_name
        self.dev = dev
        self.io_map = {}
    def printIOMap(self) -> str:
        s = ''
        for k,v in self.io_map.items():
            s += f'\nPin Type = {k}, values: ['
            for p in v:
                s += str(p)
            s += ']'
        return s
            
    def __str__(self) -> str:
        return f'alias = {self.alias}, component_name={self.component_name}, dev={self.dev}, io_map = {self.printIOMap()}'

class ArduinoYamlParser:
    #def __init__(self, path:str):
    #    self.parseYaml(path=path)
    #    pass
    #def parsePin(self, doc: dict, pinType: PinTypes) -> ArduinoPin:
    #    return ArduinoPin(pinName=pinName, pinType=pinType, halPinType=halPinType) 
        
    def parseYaml(path:str) -> list[ArduinoSettings]: # parseYaml returns a list of ArduinoSettings objects. WILL throw exceptions on error
        if os.path.exists(path) == False:
            raise FileNotFoundError(f'Error. {path} not found.')
        with open(path, 'r') as file:
            logging.debug(f'Loading config, path = {path}')
            docs = yaml.safe_load_all(file)
            mcu_list = []
            for doc in docs:
                new_arduino = ArduinoSettings() # create a new arduino config object
                if ConfigElement.ARDUINO_KEY not in doc.keys():
                    raise Exception(f'Error. {ConfigElement.ALIAS} undefined in config file ({str})')
                if ConfigElement.ALIAS not in doc[ConfigElement.ARDUINO_KEY].keys(): # TODO: Make this optional
                    raise Exception(f'Error. {ConfigElement.ALIAS} undefined in config file ({str})')
                else:
                    new_arduino.alias = doc[ConfigElement.ARDUINO_KEY][ConfigElement.ALIAS]
                if ConfigElement.DEV not in doc[ConfigElement.ARDUINO_KEY].keys(): # TODO: Make this optional
                    raise Exception(f'Error. {ConfigElement.DEV} undefined in config file ({str})')
                else:
                    new_arduino.dev = doc[ConfigElement.ARDUINO_KEY][ConfigElement.DEV]
                if ConfigElement.COMPONENT_NAME not in doc[ConfigElement.ARDUINO_KEY].keys(): # TODO: Make this optional
                    raise Exception(f'Error. {ConfigElement.COMPONENT_NAME} undefined in config file ({str})')
                else:
                    new_arduino.component_name = doc[ConfigElement.ARDUINO_KEY][ConfigElement.COMPONENT_NAME]
                if ConfigElement.IO_MAP in doc[ConfigElement.ARDUINO_KEY].keys():
                    for k, v in doc[ConfigElement.ARDUINO_KEY][ConfigElement.IO_MAP].items():
                        # here is the promised dark magic elegance referenced above.
                        # The key 'k', e.g., 'analogInputs' is cast to the corresponding enum object as 'a', the string value 'b',
                        # and the YAML parsing lamda function 'c'.
                        if len([e for e in ConfigPinTypes if e.value[0] == k]) == 0:
                            # This logic skips unsupported features that happen to be included in the YAML.
                            # Future TODO: throw an exception. For now, just ignore as more developmet is needed to finish the feature parsers.
                            continue
                            
                        a = [e for e in ConfigPinTypes if e.value[0] == k][0] # object reference from enum
                        b = [e.value[0] for e in ConfigPinTypes if e.value[0] == k][0] # String of enum
                        c = [e.value[YAML_PARSER_KEY] for e in ConfigPinTypes if e.value[0] == k][0] # Parser lamda from enum
                        new_arduino.io_map[a] = []
                        for v1 in v:   
                            new_arduino.io_map[a].append(c(v1)) # Here we just call the lamda function, which magically returns a correct object with all the settings
                mcu_list.append(new_arduino)
                logging.debug(f'Loaded Arduino from config:\n{new_arduino}')
        return mcu_list      


class ConnectionType(StrEnum):
    SERIAL = 'SERIAL'
    UDP = 'UDP'
    TCP = 'TCP'
    NONE = 'UNKNOWN'
    def __str__(self) -> str:
        return self.value

class ConnectionState(StrEnum):
    DISCONNECTED = 'DISCONNECTED'
    CONNECTING = 'CONNECTING'
    CONNECTED = 'CONNECTED'
    DISCONNECTING = 'DISCONNECTING'
    CONNECTION_TIMOUT = 'CONNECTION_TIMEOUT'
    ERROR = 'ERROR'
    NONE = 'UNKNOWN'
    def __str__(self) -> str:
        return self.value
       
class MessageType(IntEnum):
    MT_HEARTBEAT = 1,
    MT_RESPONSE = 2, 
    MT_HANDSHAKE = 3, 
    MT_COMMAND = 4, 
    MT_PINSTATUS = 5, 
    MT_DEBUG = 6, 
    UNKNOWN = -1

FeatureTypes = {
    'DEBUG': 0,
    'DEBUG_PROTOCOL_VERBOSE': 1,
    'INPUTS':2,
    'SINPUTS':3,
    'OUTPUTS':4,
    'PWMOUTPUTS:':5,
    'AINPUTS':6,
    'DALLAS_TEMPERATURE_SENSOR':7,
    'LPOTIS':8,
    'BINSEL':9,
    'QUADENC':10,
    'JOYSTICK':11,
    'STATUSLED':12,
    'DLED':13,
    'KEYPAD':14,
    'MEMORY_MONITOR':15
}
ConnectionFeatureTypes = {
    'SERIAL_TO_LINUXCNC':1,
    'ETHERNET_UDP_TO_LINUXCNC':2,
    'ETHERNET_TCP_TO_LINUXCNC':3,
    'WIFI_TCP_TO_LINUXCNC':4,
    'WIFI_UDP_TO_LINUXCNC':5,
    'WIFI_UDP_ASYNC_TO_LINUXCNC':6
}

debug_comm = True

#serial_dev = '/dev/ttyACM0' 
#serial_dev = '/dev/tty.usbmodemF412FA68D6802'

#arduino = None #serial.Serial(serial_dev, 115200, timeout=1, xonxoff=False, rtscts=False, dsrdtr=True)

protocol_ver = 1


class FeatureMapDecoder:
    def __init__(self, b:bytes):
        self.features = b
        self.bits = self.unpackbits(b)
        #self.bits = numpy.unpackbits(numpy.arange(b.astype(numpy.uint64), dtype=numpy.uint64))
  
    def unpackbits(self, x):
        z_as_uint64 = numpy.uint64(x)#int64(x)
        xshape = list(z_as_uint64.shape)
        z_as_uint64 = z_as_uint64.reshape([-1, 1])
        mask = 2**numpy.arange(64, dtype=z_as_uint64.dtype).reshape([1, 64])
        return (z_as_uint64 & mask).astype(bool).astype(int).reshape(xshape + [64])

    def isFeatureEnabledByInt(self, index:int):
        return self.bits[index] == 1
    
    def getIndexOfFeature(self, str:str):
        if str.upper() not in FeatureTypes.keys():
            raise Exception(f'PYDEBUG Error, key {str} not found in FeatureTypes map.')
        t = FeatureTypes[str.upper()]
        return FeatureTypes[str.upper()]

    def isFeatureEnabledByString(self, str:str):
        return self.bits[self.getIndexOfFeature(str)] == 1 
    
    def getFeatureString(self, index:int):
        return list(FeatureTypes.keys())[list(FeatureTypes.values()).index(index)][0]
    
    def getEnabledFeatures(self):
        ret = {}
        for k,v in FeatureTypes.items():
            if self.isFeatureEnabledByInt(v) == True:
                ret[k] = v
        return ret


class MessageDecoder:
    def __init__(self, b:bytearray):
        self.parseBytes(b)

    def validateCRC(self, data:bytearray, crc:bytes):
        hash = crc8.crc8()
        hash.update(data)
        #d = hash.digest()#.to_bytes(1, 'big')
        if hash.digest() == crc:#.to_bytes(1,'big'):
            return True
        else:
            return False
        
    def parseBytes(self, b:bytearray):
        decoded = cobs.decode(b)
        logging.debug(f"cobs encoded: {b}, rest: {b}")
        # divide into index, data, crc
        self.messageType = decoded[0]
        data = decoded[1:-1]
        self.crc = decoded[-1].to_bytes(1, byteorder="big")
        logging.debug(f"message type: {self.messageType}, data: {data}, crc: {self.crc}")
        # check crc8
        if self.validateCRC( data=data, crc=self.crc) == False:
            raise Exception(f"Error. CRC validation failed for received message. Bytes = {b}")
        self.payload = msgpack.unpackb(data, use_list=True, raw=False)

class MessageEncoder:
    #def __init__(self):
        #self.encodeBytes()

    def getCRC(self, data:bytes) -> bytes:
        hash = crc8.crc8()
        hash.update(data)
        return hash.digest()
        
    def encodeBytes(self, mt:MessageType, payload:list) -> bytes:
        #try:
        mt_enc = msgpack.packb(mt)
        data_enc = msgpack.packb(payload)  
        payload_size = len(mt_enc) + len(data_enc) + 2
            
        
        index = 0
        cobbered = cobs.encode(data_enc)
        cob_head = cobbered[0]
        #cob_len = cobbered[0]
        cobbered_payload = cobbered[1:]
            
            
                
        len_enc = msgpack.packb( len(mt_enc) + len(data_enc) + 2 )        
        crc_enc = self.getCRC(data=data_enc)
        eot_enc = b'\x00'
        return len_enc + mt_enc + data_enc + crc_enc + eot_enc    



RX_MAX_QUEUE_SIZE = 10

class ArduinoConn:
    def __init__(self, bi:int, cs:ConnectionState, timeout:int):
        self.boardIndex = bi
        self.connectionState = cs
        self.timeout = timeout
        self.lastMessageReceived = time.time()
    def setState(self, newState:ConnectionState):
        if newState != self.connectionState:
            if debug_comm:print(f'PYDEBUG Board Index: {self.boardIndex}, changing state from {self.connectionState} to {newState}')
            self.connectionState = newState


class Connection:
    # Constructor
    def __init__(self, myType:ConnectionType):
        self.connectionType = myType
        self.arduinos = [ArduinoConn(bi=0, cs=ConnectionState.DISCONNECTED, timeout=10)] #TODO: Fix this. hard coded for testing, should be based on config
        self.rxQueue = Queue(RX_MAX_QUEUE_SIZE)

    def sendCommand(self, m:str):
        cm = MessageEncoder().encodeBytes(mt=MessageType.MT_COMMAND, payload=[m, 1])
        self.sendMessage(bytes(cm))

    def onMessageRecv(self, m:MessageDecoder):
        if m.messageType == MessageType.MT_HANDSHAKE:
            if debug_comm:print(f'PYDEBUG onMessageRecv() - Received MT_HANDSHAKE, Values = {m.payload}')
            '''
                struct HandshakeMessage {
                uint8_t protocolVersion = PROTOCOL_VERSION;
                uint64_t featureMap = 0;
                uiint32_t timeOut = 0;
                uint8_t boardIndex = BOARD_INDEX+1;
                MSGPACK_DEFINE(protocolVersion, featureMap, boardIndex); 
            }hm;
            '''
            # FUTURE TODO: Make a MT_HANDSHAKE decoder class rather than the following hard codes..
            if m.payload[0] != protocol_ver:
                debugstr = f'PYDEBUG Error. Protocol version mismatched. Expected {protocol_ver}, got {m.payload[0]}'
                if debug_comm:print(debugstr)
                raise Exception(debugstr)
            
            
            fmd = FeatureMapDecoder(m.payload[1])
            if debug_comm:
                ef = fmd.getEnabledFeatures()
                print(f'PYDEBUG: Enabled Features : {ef}')
            to = m.payload[2] #timeout value
            bi = m.payload[3]-1 # board index is always sent over incremeented by one
            
            self.arduinos[bi].setState(ConnectionState.CONNECTED)
            self.arduinos[bi].lastMessageReceived = time.time()
            self.arduinos[bi].timeout = to / 1000 # always delivered in ms, convert to seconds
            hsr = MessageEncoder().encodeBytes(mt=MessageType.MT_HANDSHAKE, payload=m.payload)
            self.sendMessage(bytes(hsr))
            
        if m.messageType == MessageType.MT_HEARTBEAT:
            if debug_comm:print(f'PYDEBUG onMessageRecv() - Received MT_HEARTBEAT, Values = {m.payload}')
            bi = m.payload[0]-1 # board index is always sent over incremeented by one
            if self.arduinos[bi].connectionState != ConnectionState.CONNECTED:
                debugstr = f'PYDEBUG Error. Received message from arduino ({m.payload[0]-1}) prior to completing handshake. Ignoring.'
                if debug_comm:print(debugstr)
                return
            self.arduinos[bi].lastMessageReceived = time.time()
            hb = MessageEncoder().encodeBytes(mt=MessageType.MT_HEARTBEAT, payload=m.payload)
            self.sendMessage(bytes(hb))
        if m.messageType == MessageType.MT_PINSTATUS:
            if debug_comm:print(f'PYDEBUG onMessageRecv() - Received MT_PINSTATUS, Values = {m.payload}')
            bi = m.payload[1]-1 # board index is always sent over incremeented by one
            if self.arduinos[bi].connectionState != ConnectionState.CONNECTED:
                debugstr = f'PYDEBUG Error. Received message from arduino ({m.payload[1]-1}) prior to completing handshake. Ignoring.'
                if debug_comm:print(debugstr)
                return
            self.arduinos[bi].lastMessageReceived = time.time()
            try:
                self.rxQueue.put(m, timeout=5)
            except Queue.Empty:
                if debug_comm:print("PYDEBUG Error. Timed out waiting to gain access to RxQueue!")
            except Queue.Full:
                if debug_comm:print("Error. RxQueue is full!")
            #return None 
            #hb = MessageEncoder().encodeBytes(mt=MessageType.MT_HEARTBEAT, payload=m.payload)
            #self.sendMessage(bytes(hb))
            
    def sendMessage(self, b:bytes):
        pass

    def updateState(self):
        for arduino in self.arduinos:
            if arduino.connectionState == ConnectionState.DISCONNECTED:
                #self.lastMessageReceived = time.process_time()
                arduino.setState(ConnectionState.CONNECTING)
            elif arduino.connectionState == ConnectionState.CONNECTING:
                pass
                #if time.process_time() - arduino.lastMessageReceived >= arduino.timeout:
                #    arduino.setState(ConnectionState.CONNECTING)
            elif arduino.connectionState == ConnectionState.CONNECTED:
                d = time.time() - arduino.lastMessageReceived
                if (time.time() - arduino.lastMessageReceived) >= arduino.timeout:
                    arduino.setState(ConnectionState.DISCONNECTED)

    def getState(self, index:int):
        return self.arduinos[index].connectionState
  
 

class UDPConnection(Connection):
    def __init__(self,  listenip:str, listenport:int, maxpacketsize=512, myType = ConnectionType.UDP):
        super().__init__(myType)
        self.buffer = bytes()
        self.shutdown = False
        self.maxPacketSize = maxpacketsize
        self.daemon = None
        self.listenip = listenip
        self.listenport = listenport
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
        self.sock.bind((listenip, listenport))
        self.sock.settimeout(1)
    
    def startRxTask(self):
        # create and start the daemon thread
        #print('Starting background proceed watch task...')
        self.daemon = Thread(target=self.rxTask, daemon=False, name='Arduino RX')
        self.daemon.start()
        
    def stopRxTask(self):
        self.shutdown = True
        self.daemon.join()
        
    def sendMessage(self, b: bytes):
        self.sock.sendto(b, (self.fromip, self.fromport))
        #return super().sendMessage()
        #self.arduino.write(b)
        #self.arduino.flush()
        pass

        
    def rxTask(self):
        while(self.shutdown == False):
            try:
                self.buffer, add = self.sock.recvfrom(self.maxPacketSize)
                output = ''
                for b in self.buffer:
                    output += f'[{hex(b)}] '
                print(output)
                
                try:
                    md = MessageDecoder(bytes(self.buffer))
                    self.fromip = add[0] # TODO: Allow for multiple arduino's to communicate via UDP. Hardcoding is for lazy weasels!
                    self.fromport = add[1]
                    self.onMessageRecv(m=md)
                except Exception as ex:
                    just_the_string = traceback.format_exc()
                    print(just_the_string)
                    print(f'PYDEBUG: {str(ex)}')
                
                self.updateState()
            except TimeoutError:
                self.updateState()
                pass
            except Exception as error:
                just_the_string = traceback.format_exc()
                print(just_the_string)
		
class SerialConnetion(Connection):
    def __init__(self, dev:str, myType = ConnectionType.SERIAL):
        super().__init__(myType)
        self.buffer = bytearray()
        self.shutdown = False
        
        self.daemon = None
        self.arduino = serial.Serial(dev, 115200, timeout=1, xonxoff=False, rtscts=False, dsrdtr=True)
        self.arduino.timeout = 1
        
    def startRxTask(self):
        # create and start the daemon thread
        #print('Starting background proceed watch task...')
        self.daemon = Thread(target=self.rxTask, daemon=False, name='Arduino RX')
        self.daemon.start()
        
    def stopRxTask(self):
        self.shutdown = True
        self.daemon.join()
        
    def sendMessage(self, b: bytes):
        #return super().sendMessage()
        self.arduino.write(b)
        self.arduino.flush()
    
    def sendCommand(self, m:str):
        cm = MessageEncoder().encodeBytes(mt=MessageType.MT_COMMAND, payload=[m, 1])
        self.sendMessage(bytes(cm))
        
    def rxTask(self):
        
        while(self.shutdown == False):
            try:
                data = self.arduino.read()
                if data == b'\x00':
                    self.buffer += bytearray(data)
                    #print(bytes(self.buffer))
                    strb = ''
                    #print('Bytes from wire: ')
                    
                    for b in bytes(self.buffer):
                        strb += f'[{hex(b)}]'
                    print(strb)
                    try:
                        md = MessageDecoder(self.buffer[:-1])
                        self.onMessageRecv(m=md)
                    except Exception as ex:
                        just_the_string = traceback.format_exc()
                        print(f'PYDEBUG: {str(just_the_string)}')
                
                    #arduino.write(bytes(self.buffer))
                    self.buffer = bytes()
                elif data == b'\n':
                    self.buffer += bytearray(data)
                    print(bytes(self.buffer).decode('utf8', errors='ignore'))
                    self.buffer = bytes()
                else:
                    self.buffer += bytearray(data)
                self.updateState()
            except Exception as error:
                just_the_string = traceback.format_exc()
                print(just_the_string)
		
