#!/usr/bin/env python3
# MIT License
# ArduinoConnector
# By Alexander Richter, info@theartoftinkering.com &
# Ken Thompson (not THAT Ken Thompson), https://github.com/KennethThompson
# Copyright (c) 2023 Alexander Richter & Ken Thompson

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
import json
import os
from re import T
import getopt, sys
import subprocess
#import threading
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
#import socket
from cobs import cobs
import yaml
from pathlib import Path

#import linuxcnc
import hal
#from qtvcp.core import Info

logging.basicConfig(level=logging.DEBUG)

DEFAULT_VALUE_KEY = 1 # List position for default values
YAML_PARSER_KEY = 1
FEATURE_INDEX_KEY = 2
DEFAULT_PIN_NAME_KEY = 3

DEFAULT_PROFILE = "config.yaml"

#INFO = Info()

class ConfigElement(StrEnum):
    ARDUINO_KEY = 'mcu'
    ALIAS = 'alias'
    COMPONENT_NAME = 'component_name'
    DEV = 'dev'
    CONNECTION = 'connection'
    ENABLED = 'enabled'
    IO_MAP = 'io_map'
    def __str__(self) -> str:
        return self.value
    
class ThreadStatus(StrEnum):
    RUNNING = "RUNNING"
    FINISHED_OK = "FINISHED_OK"
    STOPPED = "STOPPED"
    CRASHED = "CRASHED"
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
    
class AnalogConfigElement(Enum):
    PIN_SMOOTHING = ['pin_smoothing', 200]
    PIN_MIN_VALUE = ['pin_min_val', 0]
    PIN_MAX_VALUE = ['pin_max_val', 1023]
    def __str__(self) -> str:
        return self.value[0]
    
class DigitalConfigElement(Enum):
    PIN_DEBOUNCE = ['pin_debounce', 250]
    INPUT_PULLUP = ['input_pullup', False]
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
    DIGITAL_INPUTS = ['digitalInputs', lambda yaml, featureID : DigitalPin(yaml=yaml, featureID=featureID, halPinDirection=HalPinDirection.HAL_OUT), 1]
    DIGITAL_OUTPUTS = ['digitalOutputs', lambda yaml, featureID : DigitalPin(yaml=yaml, featureID=featureID, halPinDirection=HalPinDirection.HAL_IN), 2]
    ANALOG_INPUTS = ['analogInputs', lambda yaml, featureID : AnalogPin(yaml=yaml,featureID=featureID, halPinDirection=HalPinDirection.HAL_OUT), 3]
    ANALOG_OUTPUTS = ['analogOutputs',  lambda yaml, featureID : AnalogPin(yaml=yaml, featureID=featureID, halPinDirection=HalPinDirection.HAL_IN), 4]
    PWM_OUTPUTS = ['pwmOutputs',  lambda yaml, featureID : ArduinoPin(yaml=yaml, featureID=featureID, pinType=ConfigPinTypes.PWM_OUTPUTS, halPinDirection=HalPinDirection.HAL_IN), 5]

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
    HAL_IO = 'HAL_IO'
    UNDEFINED = 'undefined'
    def __str__(self) -> str:
        return self.value


class ConnectionType:
    def __init__(self, yaml:dict = None):
        pass


class ArduinoPin:
    def __init__(self, pinName:str='', featureID:int=0, pinID:str='', pinType:PinTypes=PinTypes.UNDEFINED, halPinType:HalPinTypes=HalPinTypes.UNDEFINED, 
                 halPinDirection:HalPinDirection=HalPinDirection.UNDEFINED, yaml:dict = None): 
        self.pinName = pinName
        self.pinType = pinType
        self.halPinType = halPinType
        self.halPinDirection = halPinDirection
        self.pinID = pinID
        self.featureID = featureID
        self.pinInitialState = PinConfigElement.PIN_INITIAL_STATE.value[DEFAULT_VALUE_KEY]
        self.pinConnectState = PinConfigElement.PIN_CONNECTED_STATE.value[DEFAULT_VALUE_KEY]
        self.pinDisconnectState = PinConfigElement.PIN_DISCONNECTED_STATE.value[DEFAULT_VALUE_KEY]
        self.pinEnabled = PinConfigElement.PIN_ENABLED.value[DEFAULT_VALUE_KEY]
        self.halPinConnection = None
        self.halPinCurrentValue = 0
        #if yaml != None: self.parseYAML(doc=yaml)

    def parseYAML(self, doc):
        if PinConfigElement.PIN_ID.value[0] not in doc.keys():
            raise Exception(f'Error. {PinConfigElement.PIN_ID.value[0]} undefined in config yaml')
        self.pinID = doc[PinConfigElement.PIN_ID.value[0]]
        if PinConfigElement.PIN_ENABLED.value[0] in doc.keys():
            self.pinEnabled = doc[PinConfigElement.PIN_ENABLED.value[0]]
        if PinConfigElement.PIN_TYPE.value[0] in doc.keys():
            self.halPinType = HalPinTypes(str(doc[PinConfigElement.PIN_TYPE.value[0]]).upper())
        if PinConfigElement.PIN_NAME.value[0] in doc.keys():    
            self.pinName = doc[PinConfigElement.PIN_NAME.value[0]]
        else:
            self.pinName = f"{self.pinType.value}"
        if PinConfigElement.PIN_INITIAL_STATE.value[0] in doc.keys():    
            self.pinInitialState = doc[PinConfigElement.PIN_INITIAL_STATE.value[0]]
        if PinConfigElement.PIN_DISCONNECTED_STATE.value[0] in doc.keys():    
            self.pinDisconnectState = doc[PinConfigElement.PIN_DISCONNECTED_STATE.value[0]]
        if PinConfigElement.PIN_CONNECTED_STATE.value[0] in doc.keys():    
            self.pinConnectState = doc[PinConfigElement.PIN_CONNECTED_STATE.value[0]]

    def __str__(self) -> str:
        return f'pinName = {self.pinName}, pinType={self.pinType.name}, halPinType={self.halPinType}, pinEnabled={self.pinEnabled}, pinIniitalState={self.pinInitialState}, pinConnectState={self.pinConnectState}, pinDisconnectState={self.pinDisconnectState}'
    
    def toJson(self): # This is the JSON sent to the Arduino during configuration.  Should only include the values needed by the Arduino to limit memory footprint/processing within the Arduino.
        return {'featureID' : self.featureID,
                'pinID': self.pinID,
                'pinInitialState': self.pinInitialState,
                'pinConnectState': self.pinConnectState,
                'pinDisconnectState': self.pinDisconnectState}

    
class AnalogPin(ArduinoPin):
    def __init__(self,yaml:dict = None, featureID:int=0, halPinDirection=HalPinDirection):
        if halPinDirection == HalPinDirection.HAL_IN or halPinDirection == HalPinDirection.HAL_IO:
            ArduinoPin.__init__(self, featureID=featureID, pinType=PinTypes.ANALOG_INPUT, 
                        halPinType=HalPinTypes.HAL_FLOAT, halPinDirection=halPinDirection,
                        yaml=yaml)
        else:
            ArduinoPin.__init__(self, featureID=featureID, pinType=PinTypes.ANALOG_OUTPUT, 
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
    def __init__(self,  halPinDirection:HalPinDirection,featureID:int=0, yaml:dict = None):
        if halPinDirection == HalPinDirection.HAL_IN or halPinDirection == HalPinDirection.HAL_IO:
            ArduinoPin.__init__(self, pinType=PinTypes.DIGITAL_INPUT, featureID=featureID, 
                        halPinType=HalPinTypes.HAL_BIT, halPinDirection=halPinDirection,
                        yaml=yaml)
        else:
            ArduinoPin.__init__(self, pinType=PinTypes.DIGITAL_OUTPUT, featureID=featureID,
                        halPinType=HalPinTypes.HAL_BIT, halPinDirection=halPinDirection,
                        yaml=yaml)
        
        # set the defaults, which can be overriden through the yaml profile
        self.pinDebounce = DigitalConfigElement.PIN_DEBOUNCE.value[DEFAULT_VALUE_KEY]
        self.inputPullup = DigitalConfigElement.INPUT_PULLUP.value[DEFAULT_VALUE_KEY] 
        if yaml != None: 
            self.parseYAML(doc=yaml)
            ArduinoPin.parseYAML(self, doc=yaml)

    def __str__(self) -> str:

        return f'\npinID={self.pinID}, pinName = {self.pinName}, pinType={self.pinType.name}, '\
            f'halPinDirection = {self.halPinDirection}, halPinType={self.halPinType}, pinDebounce={self.pinDebounce}'
    
    def parseYAML(self, doc):
        if DigitalConfigElement.PIN_DEBOUNCE.value[0] in doc.keys():
            self.pinDebounce = int(doc[DigitalConfigElement.PIN_DEBOUNCE.value[0]])
        if DigitalConfigElement.INPUT_PULLUP.value[0] in doc.keys():
            self.inputPullup = bool(doc[DigitalConfigElement.INPUT_PULLUP.value[0]])
        
    def toJson(self):
        s = ArduinoPin.toJson(self)
        s['pinDebounce'] = self.pinDebounce
        s['inputPullup'] = self.inputPullup 
        return s



class ArduinoSettings:
    def __init__(self, alias='undefined', component_name='arduino', dev='undefined'):
        self.alias = alias
        self.component_name = component_name
        self.dev = dev
        self.baud_rate = 19200
        self.connection_timeout = 10
        self.io_map = {}
        
        self.enabled = True


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
    
    def configJSON(self):
        pc = {}
        for k, v in self.io_map.items():
            pc[k.value[FEATURE_INDEX_KEY]] = {}
            #pc[k.name + '_COUNT'] = len(v)
            i = 0
            for pin in v:
                if pin.pinEnabled == True:
                    p = pin.toJson()
                    pc[k.value[FEATURE_INDEX_KEY]][i] = pin.toJson()
                    i += 1
        return pc

            

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
            logging.debug(f'PYDEBUG: Loading config, path = {path}')
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
                if ConfigElement.ENABLED in doc[ConfigElement.ARDUINO_KEY].keys(): 
                    new_arduino.enabled = doc[ConfigElement.ARDUINO_KEY][ConfigElement.ENABLED]
                new_arduino.baud_rate = SerialConfigElement.BAUDRATE.value[DEFAULT_VALUE_KEY]
                new_arduino.connection_timeout = ConnectionConfigElement.TIMEOUT.value[DEFAULT_VALUE_KEY]

                if ConfigElement.CONNECTION in doc[ConfigElement.ARDUINO_KEY].keys():
                    
                    #lc = [x.lower() for x in doc[ConfigElement.ARDUINO_KEY][ConfigElement.CONNECTION].keys()]
                    if ConnectionConfigElement.TYPE.value[0] in doc[ConfigElement.ARDUINO_KEY][ConfigElement.CONNECTION].keys():
                        type = doc[ConfigElement.ARDUINO_KEY][ConfigElement.CONNECTION][ConnectionConfigElement.TYPE.value[0]].lower()
                        if type == ConfigConnectionTypes.SERIAL.value[0].lower():
                            if SerialConfigElement.BAUDRATE.value[0] in doc[ConfigElement.ARDUINO_KEY][ConfigElement.CONNECTION].keys():
                                new_arduino.baud_rate = doc[ConfigElement.ARDUINO_KEY][ConfigElement.CONNECTION][SerialConfigElement.BAUDRATE.value[0]]
                        #elif type == ConfigConnectionTypes.UDP.value[0].lower():
                        #    pass
                        else:
                            raise Exception(f'Error. Connection type of {type} is unsupported')
                    else:
                        raise Exception(f'Error. Connection type undefined in config file')

                        
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
                        #b = [e.value[0] for e in ConfigPinTypes if e.value[0] == k][0] # String of enum
                        c = [e.value[YAML_PARSER_KEY] for e in ConfigPinTypes if e.value[0] == k][0] # Parser lamda from enum
                        d = [e.value[FEATURE_INDEX_KEY] for e in ConfigPinTypes if e.value[0] == k][0] # Feature ID
                        new_arduino.io_map[a] = []
                        if v != None:
                            for v1 in v:   
                                new_arduino.io_map[a].append(c(v1, d)) # Here we just call the lamda function, which magically returns a correct object with all the settings
                mcu_list.append(new_arduino)
                logging.debug(f'PYDEBUG: Loaded Arduino from config:\n{new_arduino}')
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
    MT_PINCHANGE = 4, 
    MT_PINSTATUS = 5, 
    MT_DEBUG = 6, 
    MT_CONFIG = 7
    UNKNOWN = -1

FeatureTypes = {
    'DEBUG': 0,
    'DIGITAL_INPUTS':1,
    'DIGIAL_OUTPUTS':2,
    'ANALOG_INPUTS':3,
    'ANALOG_OUTPUTS':4,
    
    #'PWMOUTPUTS:':4,
    #'AINPUTS':5,
    #'DALLAS_TEMPERATURE_SENSOR':6,
    #'LPOTIS':7,
    #'BINSEL':8,
    #'QUADENC':9,
    #'JOYSTICK':10,
    #'STATUSLED':11,
    #'DLED':12,
    #'KEYPAD':13
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
        logging.debug(f"PYDEBUG: cobs encoded: {b}")
        decoded = cobs.decode(b)
        
        # divide into index, data, crc
        self.messageType = decoded[0]
        data = decoded[1:-1]
        self.crc = decoded[-1].to_bytes(1, byteorder="big")
        logging.debug(f"PYDEBUG: message type: {self.messageType}, data: {data}, crc: {self.crc}")
        # check crc8
        if self.validateCRC( data=data, crc=self.crc) == False:
            raise Exception(f"PYDEBUG: Error. CRC validation failed for received message. Bytes = {b}")
        self.payload = msgpack.unpackb(data, use_list=True, raw=False)

class MessageEncoder:
    #def __init__(self):
        #self.encodeBytes()

    def getCRC(self, data:bytes) -> bytes:
        hash = crc8.crc8()
        hash.update(data)
        return hash.digest()
        
    def encodeBytes(self, mt:MessageType, payload:list) -> bytes:
        #mt_enc = msgpack.packb(mt)
        data_enc = msgpack.packb(payload)  
        #print(f'DATA = {data_enc}')
        crc_enc = self.getCRC(data=data_enc)
        eot_enc = b'\x00'
        encoded =  cobs.encode( msgpack.packb(mt) + data_enc + crc_enc) + eot_enc
        #print(f'ENCODED = {encoded}')
        #print(f'DECODED AGAIN: {cobs.decode(encoded[:-1])}')
        #strb = ''
        #for b in bytes(encoded):
        #    strb += f'[{hex(b)}]'
        #print(strb)
        return encoded

class ProtocolMessage:
    def __init__(self, messageType:MessageType):
        self.mt = messageType
        self.payload = None
    
    def packetize(self):
        me = MessageEncoder()
        return me.encodeBytes(self.mt, self.payload)
        
    def depacketize(self):
        pass
            
class ConfigMessage(ProtocolMessage):
    def __init__(self, configJSON, seq:int, total:int, featureID:str):
        super().__init__(messageType=MessageType.MT_CONFIG)
        self.payload = [] 
        self.payload.append(featureID)
        self.payload.append(seq)
        self.payload.append(total)
        self.payload.append(configJSON)

class PinChangeMessage(ProtocolMessage):
    def __init__(self, featureID:int, seqID:int, responseReq:int, message:str):
        super().__init__(messageType=MessageType.MT_PINCHANGE)
        self.payload = [] 
        self.payload.append(featureID)
        self.payload.append(seqID)
        self.payload.append(responseReq)
        self.payload.append(message)
    def __init__(self, md:MessageDecoder):
        super().__init__(messageType=MessageType.MT_PINCHANGE)
        self.payload = md.payload

class HandshakeMessage(ProtocolMessage):
    def __init__(self, md:MessageDecoder):
        super().__init__(messageType=MessageType.MT_HANDSHAKE)
        self.protocolVersion = md.payload[0]
        if self.protocolVersion != protocol_ver:
            raise Exception(f'Expected protocol version {protocol_ver}, got {self.protocolVersion}')
        self.enabledFeatures = FeatureMapDecoder(md.payload[1])
        self.timeout = md.payload[2]
        self.maxMsgSize = md.payload[3]
        self.configVersion = md.payload[4]
        self.UID = md.payload[5]
        self.digitalPins = md.payload[6]
        self.analogInputs = md.payload[7]
        self.analogOutputs = md.payload[8]
        self.payload = md.payload
    


RX_MAX_QUEUE_SIZE = 10

class Connection:
    # Constructor
    def __init__(self, myType:ConnectionType):
        self.connectionType = myType
        self.connectionState = ConnectionState.DISCONNECTED
        self.timeout = 10
        self.configVersion = -1 # -1 initial state, this value changes once the arduino sends a handshake
        self.lastMessageReceived = time.time()
        self._callbacks = []
        self.maxMsgSize = 512
        self.enabledFeatures = None
        self.uid = ''
        self.rxQueue = Queue(RX_MAX_QUEUE_SIZE)

    def sendCommand(self, m:str):
        cm = MessageEncoder().encodeBytes(mt=MessageType.MT_COMMAND, payload=[m, 1])
        self.sendMessage(bytes(cm))

    def setState(self, newState:ConnectionState):
        if newState != self.connectionState:
            if debug_comm:print(f'PYDEBUG: changing state from {self.connectionState} to {newState}')
            self.connectionState = newState

    def onMessageRecv(self, m:MessageDecoder):
        if m.messageType == MessageType.MT_HANDSHAKE:
            if debug_comm:print(f'PYDEBUG: onMessageRecv() - Received MT_HANDSHAKE, Values = {m.payload}')
            try:
                hsm = HandshakeMessage(m)
                self.timeout = hsm.timeout / 1000
                self.configVersion = hsm.configVersion
                self.maxMsgSize = hsm.maxMsgSize
                self.enabledFeatures = hsm.enabledFeatures
                self.uid = hsm.UID
                self.setState(ConnectionState.CONNECTED)
                self.lastMessageReceived = time.time()

                resp = hsm.packetize()
                self.sendMessage(resp)
                
            except Exception as ex:
                just_the_string = traceback.format_exc()
                print(just_the_string)
                logging.debug(f'PYDEBUG: error: {str(ex)}')
                
            #pass
            '''
                struct HandshakeMessage {
                    uint8_t protocolVersion = PROTOCOL_VERSION;
                    uint64_t featureMap;
                    uint32_t timeout;
                    uint16_t configVersion; // 0 indicates no config, >0 indicates an existing config
                    String uid;
                    MSGPACK_DEFINE(protocolVersion, featureMap, timeout, configVersion, uid); 
                }hm;
            
            # FUTURE TODO: Make a MT_HANDSHAKE decoder class rather than the following hard codes..
            if m.payload[0] != protocol_ver:
                debugstr = f'PYDEBUG Error. Protocol version mismatched. Expected {protocol_ver}, got {m.payload[0]}'
                if debug_comm:print(debugstr)
                raise Exception(debugstr)
            
            fmd = FeatureMapDecoder(m.payload[1])
            if debug_comm:
                ef = fmd.getEnabledFeatures()
                print(f'PYDEBUG: Enabled Features : {ef}')
            to = m.payload[2] # timeout value
            self.maxMsgSize = m.payload[3]
            self.configVersion = m.payload[4] # config version
            self.setState(ConnectionState.CONNECTED)
            self.lastMessageReceived = time.time()
            self.timeout = to / 1000 # always delivered in ms, convert to seconds
            hsr = MessageEncoder().encodeBytes(mt=MessageType.MT_HANDSHAKE, payload=m.payload)
            self.sendMessage(bytes(hsr))
            '''
        if m.messageType == MessageType.MT_HEARTBEAT:
            if debug_comm:print(f'PYDEBUG onMessageRecv() - Received MT_HEARTBEAT, Values = {m.payload}')
            #bi = m.payload[0]-1 # board index is always sent over incremeented by one
            if self.connectionState != ConnectionState.CONNECTED:
                debugstr = f'PYDEBUG Error. Received message from arduino prior to completing handshake. Ignoring.'
                if debug_comm:print(debugstr)
                return
            self.lastMessageReceived = time.time()
            hb = MessageEncoder().encodeBytes(mt=MessageType.MT_HEARTBEAT, payload=m.payload)
            self.sendMessage(bytes(hb))
            
        if m.messageType == MessageType.MT_PINCHANGE:
            if debug_comm:print(f'PYDEBUG onMessageRecv() - Received MT_PINCHANGE, Values = {m.payload}')
            #bi = m.payload[0]-1 # board index is always sent over incremeented by one
            if self.connectionState != ConnectionState.CONNECTED:
                debugstr = f'PYDEBUG Error. Received message from arduino prior to completing handshake. Ignoring.'
                if debug_comm:print(debugstr)
                return
            self.lastMessageReceived = time.time()

            try:
                pc = PinChangeMessage(m)
            except Exception as ex:
                just_the_string = traceback.format_exc()
                print(just_the_string)
                logging.debug(f'PYDEBUG: error: {str(ex)}')

        '''
        if m.messageType == MessageType.MT_PINSTATUS:
            if debug_comm:print(f'PYDEBUG onMessageRecv() - Received MT_PINSTATUS, Values = {m.payload}')
            bi = m.payload[1]-1 # board index is always sent over incremeented by one
            if self.connectionState != ConnectionState.CONNECTED:
                debugstr = f'PYDEBUG Error. Received message from arduino ({m.payload[1]-1}) prior to completing handshake. Ignoring.'
                if debug_comm:print(debugstr)
                return
            self.lastMessageReceived = time.time()
            try:
                self.rxQueue.put(m, timeout=5)
            except Queue.Empty:
                if debug_comm:print("PYDEBUG Error. Timed out waiting to gain access to RxQueue!")
            except Queue.Full:
                if debug_comm:print("Error. RxQueue is full!")
        '''

            #return None 
            #hb = MessageEncoder().encodeBytes(mt=MessageType.MT_HEARTBEAT, payload=m.payload)
            #self.sendMessage(bytes(hb))
                # iterate over received messages and print them and remove them from buffer
        #for index, msg in self._recv_msgs.items():
            # do some stuff in user-defined callback function
        #    logging.debug(f"received msg = index: {index}, msg: {msg}")
        #    if (index in self._callbacks) and (self._callbacks[index] is not None):
        #        self._callbacks[index](msg)
            
    def sendMessage(self, b:bytes):
        pass

    def updateState(self):
        if self.connectionState == ConnectionState.DISCONNECTED:
            #self.lastMessageReceived = time.process_time()
            self.setState(ConnectionState.CONNECTING)
        elif self.connectionState == ConnectionState.CONNECTING:
            pass
            #if time.process_time() - arduino.lastMessageReceived >= arduino.timeout:
            #    arduino.setState(ConnectionState.CONNECTING)
        elif self.connectionState == ConnectionState.CONNECTED:
            d = time.time() - self.lastMessageReceived
            if (time.time() - self.lastMessageReceived) >= self.timeout:
                self.setState(ConnectionState.DISCONNECTED)

    def getConnectionState(self) -> ConnectionState:
        return self.connectionState
    
    def subscribe(self, index: int, callback: callable):
        self._callbacks[index] = callback
  
 
'''
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
'''
		
class SerialConnection(Connection):
    def __init__(self, dev:str, myType = ConnectionType.SERIAL, baudRate:int = 115200, timeout:int=1):
        super().__init__(myType)
        logging.debug(f'PYDEBUG: SerialConnection __init__: dev={dev}, baudRate={baudRate}, timeout={timeout}')
        self.rxBuffer = bytearray()
        self.shutdown = False
        self.dev = dev
        self.daemon = None    
        self.baudRate = baudRate
        self.timeout = timeout  

        self.status = ThreadStatus.STOPPED
  
        self.arduino = None #serial.Serial(dev, baudrate=baudRate, timeout=timeout, xonxoff=False, rtscts=False, dsrdtr=True)
        #self.arduino.timeout = 1
        
    def startRxTask(self):
        logging.debug(f'PYDEBUG: SerialConnection::startRxTask: dev={self.dev}')
        if self.daemon != None:
            raise Exception(f'PYDEBUG: SerialConnection::startRxTask: dev={self.dev}, error: RX thread already started!')
        
        self.daemon = Thread(target=self.rxTask, daemon=False, name='Arduino RX')
        self.daemon.start()
        
    def stopRxTask(self):
        logging.debug(f'PYDEBUG: SerialConnection::stopRxTask dev={self.dev}')
        if self.daemon != None:
            self.shutdown = True
            self.daemon.join()
            self.daemon = None
        
    def sendMessage(self, b: bytes):
        logging.debug(f'PYDEBUG: SerialConnection::sendMessage, dev={self.dev}, Message={b}')
        self.arduino.write(b)
        self.arduino.flush()
    
    def sendCommand(self, m:str):
        cm = MessageEncoder().encodeBytes(mt=MessageType.MT_COMMAND, payload=[m, 1])
        logging.debug(f'PYDEBUG: SerialConnection::sendCommand, dev={self.dev}, Command={bytes(cm)}')
        self.sendMessage(bytes(cm))
        
    def rxTask(self):
        self.status = ThreadStatus.RUNNING
        while(self.shutdown == False):
            try:
                if self.arduino == None:
                    self.arduino = serial.Serial(self.dev, baudrate=self.baudRate, timeout=self.timeout, xonxoff=False, rtscts=False, dsrdtr=True)
                num_bytes = self.arduino.in_waiting
                #logging.debug(f'SerialConnection::rxTask, dev={self.dev}, in_waiting={num_bytes}')
                if num_bytes > 0:
                    self.rxBuffer += self.arduino.read(num_bytes)
                    while(True):  
                        newlinepos = self.rxBuffer.find(b'\r\n')
                        termpos = self.rxBuffer.find(b'\x00')

                        readDebug = False
                        readMessage = False
                        if newlinepos == -1 and termpos == -1:
                            break
                        elif newlinepos != -1 and termpos != -1: 
                            if newlinepos < termpos:
                                readDebug = True
                            else:
                                readMessage = True
                        elif newlinepos != -1:
                            readDebug = True
                        elif termpos != -1:
                            readMessage = True
                        else:
                            break

                        if readDebug:
                            [chunk, self.rxBuffer] = self.rxBuffer.split(b'\r\n', maxsplit=1)
                            print( f'{bytes(chunk).decode("utf8", errors="ignore")}')
                        elif readMessage:
                            [chunk, self.rxBuffer] = self.rxBuffer.split(b'\x00', maxsplit=1)
                            logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, chunk bytes: {chunk}')
                            try:
                                md = MessageDecoder(chunk)
                                self.onMessageRecv(m=md)
                            except Exception as ex:
                                just_the_string = traceback.format_exc()
                                logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, Exception: {str(just_the_string)}')

            except OSError as oserror:
                print( f'OS Error = : {str(oserror)}')
                just_the_string = traceback.format_exc()
                logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, Exception: {str(just_the_string)}, OS Error: {str(oserror)}')
                
                self.daemon = None
                self.arduino = None
                self.configVersion = 0 # Force config send on reconnect; TODO: Consider making this less janky
                self.setState(newState=ConnectionState.DISCONNECTED)
                self.status = ThreadStatus.CRASHED
                break #break out of while
            except Exception as error:
                just_the_string = traceback.format_exc()
                logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, Exception: {str(just_the_string)}')
                
                self.daemon = None
                self.arduino = None
                self.configVersion = 0 # Force config send on reconnect; TODO: Consider making this less janky
                self.setState(newState=ConnectionState.DISCONNECTED)
                self.status = ThreadStatus.CRASHED
                break #break out of while

class HalPinConnection:
    def __init__(self, component:hal.component, pinName:str, pinType:HalPinTypes, pinDirection:HalPinDirection):
        self.component = component
        self.pinName = pinName
        
        if pinType == HalPinTypes.HAL_BIT:
            self.pinType = hal.HAL_BIT
        elif pinType == HalPinTypes.HAL_FLOAT:
            self.pinType = hal.HAL_FLOAT
        else:
            raise Exception(f'Error. Pin type {pinType} is unsupported.')

        if pinDirection == HalPinDirection.HAL_IN:
            self.pinDirection = hal.HAL_IN
        elif pinDirection == HalPinDirection.HAL_OUT:
            self.pinDirection = hal.HAL_OUT
        elif pinDirection == HalPinDirection.HAL_IO:
            self.pinDirection = hal.HAL_IO
        else:
            raise Exception(f'Error. Pin Direction {pinDirection} is unsupported.')

        self.halPin = self.component.newpin(self.pinName, self.pinType, self.pinDirection)

    def Get(self):
        return self.component[self.pinName]
    
    def Set(self, value):
        self.component[self.pinName] = value




class ArduinoConnection:
    def __init__(self, settings:ArduinoSettings):
        self.settings = settings
        self.serialConn = SerialConnection(dev=settings.dev, baudRate=settings.baud_rate, timeout=settings.connection_timeout)
        '''
                while( True ):
            try:
                self.component = hal.component(self.settings.component_name)
                logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, Successfully loaded component {self.settings.component_name} into HAL')
                break
            except hal.error as ex:
                logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, Detected existing instance of {self.settings.component_name} in HAL, calling halcmd unload..')
                subprocess.run(["halcmd", f"unload {self.settings.component_name}"])
        '''
        self.component = hal.component(self.settings.component_name) # Future TODO: Implement unload function as shown above.  Appear to need to handle CTRL+C in main loop to respect the unload request
                

        for k, v in settings.io_map.items():
            for v1 in v:
                v1.halPinConnection = HalPinConnection(component=self.component, pinName=v1.pinName, pinType=v1.halPinType, pinDirection=v1.halPinDirection)
        self.component.ready()
            
            
    def __str__(self) -> str:
        return f'Arduino Alias = {self.settings.alias}, Component Name = {self.settings.component_name}, Enabled = {self.settings.enabled}'
    
    def doFeaturePinUpdates(self):
        for k, v in self.settings.io_map.items():
            logicalID = 0
            for v1 in v:
                #if v1.halPinDirection == HalPinDirection.HAL_OUT:
                r = v1.halPinConnection.Get()
                if r != v1.halPinCurrentValue:
                    #print(f'VALUE CHANGED!!! Old Value {v1.halPinCurrentValue}, New Value {r}')
                    ## Send with logical pin ID to avoid for loop of pins by arduino.
                    '''
                        Proposed JSON for message within PinChange
                        {
                        "pa": [
                            {"lid": 0, "pid": 0, "v":1},
                            {"lid": 1, "pid": 1, "v":0}
                            ]
                        }
                    '''
                    j = {
                    "pa": [
                        {"lid": logicalID, "pid": int(v1.pinID), "v":r}
                    ]
                    }
                    #v1.halPinCurrentValue = r
                    pcm = PinChangeMessage(featureID=v1.featureID, seqID=0, responseReq=0, message=json.dumps(j))
                    try:
                        self.serialConn.sendMessage(pcm.packetize())
                    except Exception as error:
                        just_the_string = traceback.format_exc()
                        logging.debug(f'PYDEBUG: ArduinoConnection::doFeaturePinUpdates, dev={self.settings.dev}, alias={self.settings.alias}, Exception: {str(error)}, Traceback = {just_the_string}')
                        # Future TODO: Consider doing something intelligent and not just reporting an error. Maybe increment a hal pin that reflects error counts?
                        #return
                    time.sleep(.2)
                    v1.halPinCurrentValue = r
                    logicalID += 1
                         #= HalPinConnection(component=self.component, pinName=v1.pinName, pinType=v1.halPinType, pinDirection=v1.halPinDirection) 
            

    def doWork(self):
        if self.settings.enabled == False:
            return # do nothing. TODO: Still indicate the arduino is disabled via the HAL
        self.doFeaturePinUpdates()

        if self.serialConn.status == ThreadStatus.STOPPED:
            self.serialConn.startRxTask()

        if self.serialConn.status == ThreadStatus.CRASHED:
            logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, ThreadStatus == CRASHED')
            # perhaps device was unplugged..
            found = False
            for port in serial.tools.list_ports.comports():
                if self.settings.dev in port:
                    found = True
            if found == False:
                retry = 5
                logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}. Error: Serial device not found! Retrying in {retry} seconds..')
                time.sleep(retry) # TODO: Make retry settable via the yaml config?

            else:
                logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, Serial deice found, restarting RX thread..')
                time.sleep(1) # TODO: Consider making this delay settable? Trying to avoid hammering the serial port when its in a strange state
                self.serialConn.startRxTask()
            
        if self.serialConn.getConnectionState() == ConnectionState.CONNECTED and self.serialConn.configVersion == 0:
            j = self.settings.configJSON()
            #config_json = json.dumps(j)
            #h = int(hashlib.sha256(config_json.encode('utf-8')).hexdigest(), 16) % 10**8
            for k, v in j.items():
                total = len(v)
                seq = 0
                for k1, v1 in v.items():
                    v1['logicalID'] = k1
                    #print(v1)
                    cf = ConfigMessage(configJSON=json.dumps(v1), seq=seq, total=total, featureID=v1['featureID'])
                    seq += 1
                    try:
                        self.serialConn.sendMessage(cf.packetize())
                    except Exception as error:
                        just_the_string = traceback.format_exc()
                        logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, Exception: {str(error)}, Traceback = {just_the_string}')
                        # Future TODO: Consider doing something intelligent and not just reporting an error. Maybe increment a hal pin that reflects error counts?
                        return
                    time.sleep(.2)

            self.serialConn.configVersion = 1
arduino_map = []

def listDevices():
    import serial.tools.list_ports
    for port in serial.tools.list_ports.comports():
        print(f'Device: {port}')

def locateProfile() -> list[ArduinoConnection]:
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
    for a in arduino_profiles:
        arduino_map.append(ArduinoConnection(a))
    return arduino_profiles

def main():
    # Remove 1st argument from the
    # list of command line arguments
    argumentList = sys.argv[1:]
    
    # Options
    options = "hdp:"
    
    # Long options
    long_options = ["Help", "Devices", "Profile="]
    target_profile = None
    devs = []

    

    try:
        # Parsing argument
        arguments, values = getopt.getopt(argumentList, options, long_options)
     
        # checking each argument
        for currentArgument, currentValue in arguments:
    
            if currentArgument in ("-h", "--Help"):
                print ("Displaying Help")
                
            elif currentArgument in ("-d", "--devices"):
                print('Listing available Serial devices:')
                listDevices()
                sys.exit()
                ##print ("Displaying file_name:", sys.argv[0])
                
            elif currentArgument in ("-p", "--profile"):
                logging.debug(f'PYDEBUG: Profile: {currentValue}')
                target_profile = currentValue
             
    except getopt.error as err:
        # output error, and return with an error code
        just_the_string = traceback.format_exc()
        #print(just_the_string)
        logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
        #print (str(err))
        sys.exit()

    if target_profile is not None:
        # user provided a profile path to utilize
        try:
            devs = ArduinoYamlParser.parseYaml(path=target_profile)

        except Exception as err:
            just_the_string = traceback.format_exc()
            print(just_the_string)
            logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
            sys.exit()
    else:
        devs = locateProfile()

    if len(devs) == 0:
        print ('No Arduino profiles found in profile yaml!')
        sys.exit()
    
    listDevices()

    arduino_connections = []
    try:
        for a in devs:
            c = ArduinoConnection(a)
            arduino_connections.append(c)
            logging.info(f'PYDEBUG: Loaded Arduino profile: {str(c)}')
    except Exception as err:
        just_the_string = traceback.format_exc()
        #print(just_the_string)
        logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
        #print(str(err))
        sys.exit()

    while(True):
        try:
            for ac in arduino_connections:
                ac.doWork()
            time.sleep(0.01)
        except KeyboardInterrupt:
            for ac in arduino_connections:
                ac.serialConn.stopRxTask()
            raise SystemExit
        except Exception as err:
            arduino_connections.clear()
            #print(str(err))
            just_the_string = traceback.format_exc()
            print(just_the_string)
            #sys.exit()
            


    #pass
    # logging.debug(f'Looking for config.yaml in
    #with open(Path.home() / ".ssh" / "known_hosts") as f:
    #    lines = f.readlines()

if __name__ == "__main__":
    main()