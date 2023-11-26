from multiprocessing import Value
import os
from re import T
from pytest import Config
import serial
import msgpack
from strenum import StrEnum
from enum import IntEnum
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

class ConfigElement(StrEnum):
    ARDUINO_KEY = 'arduino'
    ALIAS = 'alias'
    COMPONENT_NAME = 'component_name'
    DEV = 'dev'
    PIN_MAP = 'pin_map'
    PIN_ID = 'pin_id'
    PIN_NAME = 'pin_name'
    PIN_TYPE = 'pin_type'
    def __str__(self) -> str:
        return self.value

class ConfigPinTypes(StrEnum):
    ANALOG_INPUTS = 'analogInputs'
    ANALOG_OUTPUTS = 'analogOutputs'
    DIGITAL_INPUTS = 'digitalInputs'
    DIGITAL_OUTPUTS = 'digitalOutputs'
    def __str__(self) -> str:
        return self.value
class PinTypes(StrEnum):
    ANALOG_INPUT = 'ain'
    ANALOG_OUTPUT = 'aout'
    DIGITAL_INPUT = 'din'
    DIGITAL_OUTPUT = 'dout'
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
    def __str__(self) -> str:
        return self.value
class ArduinoPin:
    def __init__(self, pinName:str, pinType:PinTypes, halPinType:HalPinTypes):
        self.pinName = pinName
        self.pinType = pinType
        self.halPinType = halPinType
        if pinType == PinTypes.ANALOG_INPUT or pinType == PinTypes.DIGITAL_INPUT:
            self.direction = HalPinDirection.HAL_IN
        else:
            self.direction = HalPinDirection.HAL_OUT
    def __str__(self) -> str:
        return f'pinName = {self.pinName}, pinType={self.pinType}, halPinType={self.halPinType}'

class ArduinoSettings:
    def __init__(self, alias='undefined', component_name='arduino', dev='undefined'):
        self.alias = alias
        self.component_name = component_name
        self.dev = dev
        self.pin_map = {}
    def printPinMap(self) -> str:
        s = ''
        for k,v in self.pin_map.items():
            s += f'\nPin Type = {k}, values: ['
            for p in v:
                s += str(p)
            s += ']'
        return s
            
    def __str__(self) -> str:
        return f'alias = {self.alias}, component_name={self.component_name}, dev={self.dev}, pin_map = {self.printPinMap()}'

class ArduinoYamlParser:
    def __init__(self, path:str):
        self.parseYaml(path=path)
        pass
    def parsePin(self, doc: dict, pinType: PinTypes) -> ArduinoPin:
        pinName = ''
        halPinType = HalPinTypes.UNDEFINED
        if ConfigElement.PIN_ID not in doc.keys():
            raise Exception(f'Error. {ConfigElement.PIN_ID} undefined in config yaml')
        if ConfigElement.PIN_TYPE in doc.keys():
            halPinType = HalPinTypes(str(doc[ConfigElement.PIN_TYPE]).upper())
        else:
            if pinType == PinTypes.ANALOG_INPUT or pinType == PinTypes.ANALOG_OUTPUT:
                halPinType = HalPinTypes.HAL_FLOAT
            if pinType == PinTypes.DIGITAL_INPUT or pinType == PinTypes.DIGITAL_OUTPUT:
                halPinType = HalPinTypes.HAL_BIT
            else:
                raise Exception(f'Error. {pinType} is an unsupported {ConfigElement.PIN_TYPE} value')
        if ConfigElement.PIN_NAME in doc.keys():    
            pinName = str(doc[ConfigElement.PIN_NAME]).upper()
        else:
            pinName = f"{pinType}.{str(doc[ConfigElement.PIN_TYPE])}"
        
        return ArduinoPin(pinName=pinName, pinType=pinType, halPinType=halPinType) 
        
    def parseYaml(self, path:str) -> list[ArduinoSettings]: # parseYaml returns a list of ArduinoSettings objects. WILL throw exceptions on error
        if os.path.exists(path) == False:
            raise Exception(f'Error. {path} not found.')
        with open(path, 'r') as file:
            logging.debug(f'Loading config, path = {path}')
            docs = yaml.safe_load_all(file)
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
                if ConfigElement.PIN_MAP in doc[ConfigElement.ARDUINO_KEY].keys():
                    for k, v in doc[ConfigElement.ARDUINO_KEY][ConfigElement.PIN_MAP].items():
                        pins = []
                        for v1 in v:    
                            pins.append(self.parsePin(doc=v1, pinType=ConfigPinTypes(k)))
                        new_arduino.pin_map[ConfigPinTypes(k)] = pins
                logging.debug(f'Loaded Arduino from config:\n{new_arduino}')
                


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
		
