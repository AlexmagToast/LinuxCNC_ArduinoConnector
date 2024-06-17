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
import asyncio
import curses
from datetime import datetime, timedelta
import json
import os
from re import T
import getopt, sys
import zlib
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
from cobs import cobs
import yaml
from pathlib import Path
import copy
from abc import ABCMeta, abstractmethod
import serial.tools.list_ports
import concurrent.futures
logging.basicConfig(level=logging.CRITICAL, format='%(message)s\r\n')

# Filename of default yaml profile.
DEFAULT_PROFILE = "config.yaml"

MCU_PROTOCOL_VERSION = 1

#INFO = Info()

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
    LISTEN_PORT: ['listen_port', 54321] # optional, default is set to arduino_port  
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

class ConnectionType:
    def __init__(self, yaml:dict = None):
        pass

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
    End YAML Parser Objects
'''   
'''
    Message/Protcol Objects and Helpers
'''

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
    MT_INVITE_SYNC = 0,
    MT_HEARTBEAT = 1,
    MT_RESPONSE = 2, 
    MT_HANDSHAKE = 3, 
    MT_PINCHANGE = 4, 
    MT_PINSTATUS = 5, 
    MT_DEBUG = 6, 
    MT_CONFIG = 7,
    MT_CONFIG_ACK = 8,
    MT_CONFIG_NAK = 9,
    UNKNOWN = -1



ConnectionFeatureTypes = {
    'SERIAL_TO_LINUXCNC':1,
    'ETHERNET_UDP_TO_LINUXCNC':2,
    'ETHERNET_TCP_TO_LINUXCNC':3,
    'WIFI_TCP_TO_LINUXCNC':4,
    'WIFI_UDP_TO_LINUXCNC':5,
    'WIFI_UDP_ASYNC_TO_LINUXCNC':6
}




class MessageEncoder:
    @staticmethod
    def encodeBytes(payload) -> bytes:
        packed = msgpack.dumps(payload)
        encoded = cobs.encode(packed)
        return encoded

class ProtocolMessage:
    def __init__(self, messageType: MessageType):
        self.mt = messageType
        self.payload = None

    def packetize(self) -> bytes:
        if self.payload is None:
            raise ValueError("Payload cannot be None")
        encoded_payload = MessageEncoder.encodeBytes(self.payload)
        return encoded_payload + b'\x00'

    def depacketize(self, data: bytes):
        if not data.endswith(b'\x00'):
            raise ValueError("Invalid packet: does not end with null terminator")
        encoded_payload = data[:-1]
        try:
            packed = cobs.decode(encoded_payload)
            self.payload = msgpack.loads(packed)
        except (cobs.DecodeError, msgpack.UnpackException) as e:
            raise ValueError(f"Failed to depacketize: {e}")
        
class ConfigMessage(ProtocolMessage):
    def __init__(self, configJSON, seq: int, total: int, featureID: str):
        super().__init__(messageType=MessageType.MT_CONFIG)
        self.payload = {
            'mt': MessageType.MT_CONFIG,
            'fi': featureID,
            'se': seq,
            'to': total,
            'cs': configJSON
        }

class ConfigMessageNak(ProtocolMessage):
    def __init__(self, payload):
        super().__init__(messageType=MessageType.MT_CONFIG_NAK)
        
        required_fields = ['fi', 'se', 'ec', 'es']
        for field in required_fields:
            if field not in payload:
                raise ValueError(f"{field} is undefined in payload")
        
        self.payload = payload
        self.featureID = payload['fi']
        self.sequenceID = payload['se']
        self.errorCode = payload['ec']
        self.errorString = payload['es']

class ConfigMessageAck(ProtocolMessage):
    def __init__(self, payload):
        super().__init__(messageType=MessageType.MT_CONFIG_ACK)
        
        required_fields = ['fi', 'se', 'fa']
        for field in required_fields:
            if field not in payload:
                raise ValueError(f"{field} is undefined in payload")
        
        self.payload = payload
        self.featureID = payload['fi']
        self.sequenceID = payload['se']
        self.featureArrayIndex = payload['fa']
        
class DebugMessage(ProtocolMessage):
    def __init__(self, payload):
        super().__init__(messageType=MessageType.MT_DEBUG)
        
        required_fields = ['ds']
        for field in required_fields:
            if field not in payload:
                raise ValueError(f"{field} is undefined in payload")
        self.payload = payload
        self.message = payload['ds']
        


class InviteSyncMessage(ProtocolMessage):
    def __init__(self, payload=None):
        super().__init__(messageType=MessageType.MT_INVITE_SYNC)

        if payload is not None:
            if 'pv' not in payload:
                raise ValueError('Protocol version undefined')
            self.payload = payload
            self.pv = payload['pv']
        else:
            self.payload = { 'pv': f'{MCU_PROTOCOL_VERSION}' }
            self.pv = self.payload['pv']


class PinInfoElement:
    def __init__(self, message):
        required_fields = ['l', 'p', 'v']
        for field in required_fields:
            if field not in message:
                raise ValueError(f"{field} is undefined in message")
        self.logicalID = message['l']
        self.pinID = message['p']
        self.pinValue = message['v']
        
    def  __repr__(self):
        return f"PinInfoElement(logicalID={self.logicalID}, pinID={self.pinID}, pinValue={self.pinValue})"

def parse_pin_info_elements(json_string):
    try:
        # Parse the JSON string
        data = json.loads(json_string)
        
        # Create a list of PinInfoElement objects
        elements = [PinInfoElement(item) for item in data]
        
        return elements
    except json.JSONDecodeError:
        logging.debug("Invalid JSON string")
        return None
    
class PinChangeMessage(ProtocolMessage):
    def __init__(self, featureID: int = 0, seqID: int = 0, responseReq: int = 0, message: str = '', payload=None):
        super().__init__(messageType=MessageType.MT_PINCHANGE)
        if payload is None:
            self.payload = {
                'mt': MessageType.MT_PINCHANGE,
                'fi': featureID,
                'si': seqID,
                'rr': responseReq,
                'ms': message
            }
        else:
            #self.payload = payload
            required_fields = ['fi', 'si', 'rr', 'ms']
            for field in required_fields:
                if field not in payload:
                    raise ValueError(f"{field} is undefined in payload")
            self.payload = payload
            self.featureID = payload['fi']
            self.seqID = payload['si']
            self.responseReq = payload['rr']
            required_fields = ['l', 'p', 'v']
            #self.message = payload['ms']
            self.pinInfo = parse_pin_info_elements(payload['ms'])
            #pass
        #self.payload = payload
        #self.featureID = payload['fi']
        #self.sequenceID = payload['se']
        #self.featureArrayIndex = payload['fa']
        
class HeartbeatMessage(ProtocolMessage):
    def __init__(self, payload):
        super().__init__(messageType=MessageType.MT_HEARTBEAT)
        self.payload = payload
        self.uptime = payload['ut']
        
class HandshakeMessage(ProtocolMessage):
    def __init__(self, payload):
        super().__init__(messageType=MessageType.MT_HANDSHAKE)
        
        required_fields = ['pv', 'fm', 'to', 'ps']
        for field in required_fields:
            if field not in payload:
                raise ValueError(f"{field} is undefined in payload")
        
        self.protocolVersion = payload['pv']
        if self.protocolVersion != MCU_PROTOCOL_VERSION:
            raise ValueError(f"Expected protocol version {MCU_PROTOCOL_VERSION}, got {self.protocolVersion}")

        self.enabledFeatures = FeatureMapDecoder(payload['fm'])
        self.timeout = payload['to']
        self.profileSignature = payload['ps']
        self.UID = payload.get('ui', 'UNDEFINED')
        self.digitalPins = payload.get('dp', 0)
        self.analogInputs = payload.get('ai', 0)
        self.analogOutputs = payload.get('ao', 0)
        
        self.payload = payload

class MessageDecoder:
    @staticmethod
    def parseBytes(b: bytearray) -> ProtocolMessage:
        logging.debug(f"PYDEBUG: cobs encoded: {b}")
        
        try:
            decoded = cobs.decode(b)
            logging.debug(f"PYDEBUG: cobs decoded: {decoded}")
        except cobs.DecodeError as e:
            raise ValueError(f"Failed to decode COBS data: {e}")

        try:
            payload = msgpack.loads(decoded)
            logging.debug(f"PYDEBUG: msgpack json decoded: {payload}")
        except msgpack.ExtraData as e:
            raise ValueError(f"Failed to unpack msgpack data: {e}")
        except msgpack.FormatError as e:
            raise ValueError(f"Failed to unpack msgpack data: {e}")

        if 'mt' not in payload:
            raise ValueError("PYDEBUG: Message type undefined.")
        
        messageType = payload['mt']

        if messageType == MessageType.MT_HANDSHAKE:
            return HandshakeMessage(payload=payload)
        elif messageType == MessageType.MT_HEARTBEAT:
            return HeartbeatMessage(payload=payload)
        elif messageType == MessageType.MT_CONFIG_NAK:
            return ConfigMessageNak(payload=payload)
        elif messageType == MessageType.MT_CONFIG_ACK:
            return ConfigMessageAck(payload=payload)
        elif messageType == MessageType.MT_DEBUG:
            return DebugMessage(payload=payload)
        elif messageType == MessageType.MT_PINCHANGE:
            return PinChangeMessage(
                payload=payload
            )
        else:
            raise ValueError(f"Error. Message Type Unsupported, type = {messageType}")

'''
    End Message/Protcol Objects and Helpers
'''
# The Features enum is used by the Feature objects to set the Feature properties such as the corresponding constant name, config string name, and feature ID
class Features(Enum):
    DEBUG               = ['DEBUG', '', 0]
    DEBUG_VERBOSE       = ['DEBUG_VERBOSE', '', 1]
    FEATUREMAP          = ['FEATURE_MAP', '', 2]
    LOWMEM              = ['LOWMEM', '', 3]
    DIGITAL_INPUTS      = ['DIGITAL_INPUTS', 'digitalInputs', 4]
    DIGITAL_OUTPUTS     = ['DIGITAL_OUTPUTS', 'digitalOutputs', 5]
    ANALOG_INPUTS       = ['ANALOG_INPUTS', 'analogInputs', 6]
    ANALOG_OUTPUTS      = ['ANALOG_OUTPUTS', 'analogOutputs', 7]
    PWM_OUTPUTS         = ['PWM_OUTPUTS', 'pwmOutputs', 8]
    
    def __str__(self) -> str:
        return self.value[0]
    
    def __int__(self) -> int:
        return self.value[2]
    
    def configName(self) -> str:
        return self.value[1]
    
'''
IO FEATURE OBJECTS
'''
   
class IOFeatureConfigItem():
    def __init__(self, pinConfig:ArduinoPin, maxRetries:int=3, retryInterval:int=10) -> None:
        self.pinConfig = pinConfig
        self.retryCount = maxRetries
        self.lastTickCount = 0
        self.retryInterval = retryInterval
        self.seqID = pinConfig.pinID
        
'''
    Helpful information.
    Since a Yaml config can define multiple MCUs, each
    MCU gets its own copies of IOFeature objects based on what pins have been defined.  For instance, each MCU defined in the YAML
    that has digitalInput pins defined will have its own DigitalInputs objects created during runtime.

    With that said, each IOFeature-deriving object is meant to be Arduino sketch-esque. The 'Setup' method gets called automatically at runtime to allow for custom initialization of properties.
    The 'Loop' method gets called at a regular interval (Todo: Make the interval definable), and tasks such as looking for HAL pin changes can be performed.

    However, certain tasks such as processing of recevied messages directed to a IOFeature (e.g., a pin change reported by the Arduino) should be handled
    based on associated callbacks.
'''

class IOFeature(metaclass=ABCMeta):
    def __init__(self, featureName:str, featureConfigName:str, featureID:int) -> None:
        self.featureID = featureID
        self.arduinoFeatureIndex = -1
        self.featureName = featureName
        self.featureConfigName = featureConfigName
        self.featureReady = False # Indicates if Feature is ready for IO processing
        self.pinList = [] # Holds a list of pins which were parsed from the Yaml config
        self.configComplete = False # Indicates if the Arduino has the Feature config applied
        self.configSyncError = False # Indicates if there was an error during config sync
        self.pinConfigSyncMap = {}
        self._sendMessageCallbacks = []
        self._debugCallbacks = []

    def FeatureName(self):
        return self.featureName
    
    def FeatureID(self):
        return self.featureID
    
    def FeatureConfigName(self):
        return self.featureConfigName
    
    def FeatureReady(self) -> bool:
        return self.featureReady
    
    def ConfigComplete(self) -> bool:
        return self.configComplete

    def ConfigSyncError(self) -> bool:
        return self.configSyncError
    
    def Debug(self, s:str):
        for dc in self._debugCallbacks:
            dc(f'[{self.featureName}] {s}')
            
    @abstractmethod
    def OnConnected(self):
        self.configSyncError = False # Clear the error flag on reconnect
        for p in self.pinList: # Iterate through the pin config list and queue pin config messages for syncing with MCU
            self.pinConfigSyncMap[p.pinLogicalID] = IOFeatureConfigItem(p)
                          
    @abstractmethod
    def OnDisconnected(self):
        for p in self.pinList:
            p.pinConfigSynced = False

    @abstractmethod
    def OnMessageRecv(self, pm:ProtocolMessage):
        if pm.mt == MessageType.MT_CONFIG_ACK:
            # Use response to set the feature array index value
            # This value is used later on when communicating with the arduino.
            # The Arduino can use the index value to do a direct access of the feature
            # rather than needing to perform a for loop to find the target feature object
            if (self.arduinoFeatureIndex == -1):
                self.arduinoFeatureIndex = pm.featureArrayIndex
            self.pinConfigSyncMap[pm.sequenceID].pinConfigSynced = True
            del self.pinConfigSyncMap[pm.sequenceID]
            if len(self.pinConfigSyncMap.values()) == 0:
                self.configComplete = True
                self.Debug("Config successfully applied.")

        elif pm.mt == MessageType.MT_CONFIG_NAK:
            self.Debug(f'Error. MCU NAK on config. Reason = {pm.errorString}. Terminating sync.')
            self.configSyncError = True
            pass
    
    @abstractmethod
    def Loop(self):
        if self.ConfigSyncError() != True and len(self.pinConfigSyncMap.values()) > 0:
            p = next(iter(self.pinConfigSyncMap.values()))
            if (time.time() - p.lastTickCount) > p.retryInterval:
                #print( f'{time.time() - p.lastTickCount}' )
                if p.retryCount > 0:
                    p.retryCount -= 1
                    # do send
                    p.lastTickCount = time.time()
                    j = p.pinConfig.toJson()
                    cf = ConfigMessage(configJSON=j, seq=p.pinConfig.pinLogicalID, total=len(self.pinList), featureID=p.pinConfig.featureID)
                    self.Debug(f'Sending Config Message, Message = {cf.packetize()}')
                    for c in self._sendMessageCallbacks:
                        c(cf.packetize())
                    return
                else:
                    self.Debug('Error. Config timed out during sending! Sync failed.')
                    #del self.pinConfigSyncMap[p.pinConfig.pinLogicalID]
                    self.pinConfigSyncMap.clear()
                    self.configSyncError = True
        #if self.FeatureReady() == True and self.ConfigComplete() == True:
            
        #if self.ConfigComplete() == True:
        #    self.Debug('Feature is ready for processing.')

    @abstractmethod
    def Setup(self):
        self.Debug('Setup called')
        for i in range(0, len(self.pinList)):
            self.pinList[i].pinConfigSynced = False
            self.pinList[i].pinLogicalID = i # set the logical ID. 
            
                    
    def sendMessageSubscribe(self, callback: callable):
        self._sendMessageCallbacks.append(callback)

    def debugSubscribe(self, callback: callable):
        self._debugCallbacks.append(callback)
'''
    DigitalInputs
'''
class DigitalInputs(IOFeature):
    def __init__(self) -> None:
        '''
            All IOFeature objects must call the IOFeature.__init__ method to establish base properties including the featureName (also the constant defined on the Config.h, e.g., 'DIGITAL_INPUTS'),
            the featureConfigName (how the feature name appears in yaml, e.g., 'digitalInputs'), and the featureID (e.g., also the INT constant defined in the Config.h and the python script)
        ''' 
        IOFeature.__init__(self, featureName=str(Features.DIGITAL_INPUTS), featureConfigName=Features.DIGITAL_INPUTS.configName(), featureID=int(Features.DIGITAL_INPUTS))
        self.featureReady = True
    '''
        All IOFeature objects must also define a YamlParser method which returns a lambda.  The lamdda method is what gets called when parsing properties from the Yaml profile.
        DigitalPin and AnalogPin have been predefined in this python script to provide a uinform way of parsing out required properties, optional properties, and default values.
        Some future features may require more complex logic, and additional Yaml helpers will need to be defined.
    '''
    def YamlParser(self):
        return lambda yaml, featureID : DigitalPin(yaml=yaml, featureID=featureID, halPinDirection=HalPinDirection.HAL_OUT)
    
    def OnMessageRecv(self, pm:ProtocolMessage):
        super().OnMessageRecv(pm)
        if (pm.mt == MessageType.MT_PINCHANGE):
        
           # maybe_message = #PinChangeMessage#MessageDecoder.parseBytes(pm.payload)
            logging.debug(f'PINCHANGE: {pm.payload}')
            for pi in pm.pinInfo:
                # find the pin in the pinList
                for p in self.pinList:
                    if p.pinID == pi.pinID:
                        p.halPinCurrentValue = pi.pinValue
                        logging.debug(f'PININFO: {pi}')
                        break
            #for p in pm.pinInfo:
            #    logging.debug(f'PININFO: {p}')

            #pass    
    
    def OnConnected(self):
        super().OnConnected()
    
    def OnDisconnected(self):
        super().OnDisconnected()
    
    def Setup(self):
        super().Setup()
    
    def Loop(self):
        super().Loop()
        if (self.FeatureReady() == True and self.ConfigComplete() == True):
            #self.Debug('Feature is ready for processing.')
            pass
        
    

    

'''
    DigitalOutputs
'''
class DigitalOutputs(IOFeature):
    def __init__(self) -> None:
        IOFeature.__init__(self, featureName=str(Features.DIGITAL_OUTPUTS), featureConfigName=Features.DIGITAL_OUTPUTS.configName(), featureID=int(Features.DIGITAL_OUTPUTS))
    
    def YamlParser(self):
        return lambda yaml, featureID : DigitalPin(yaml=yaml, featureID=featureID, halPinDirection=HalPinDirection.HAL_IN)
    
    def OnMessageRecv(self, pm:ProtocolMessage):
        super().OnMessageRecv(pm)
        #if (pm.mt == MessageType.MT_PINCHANGE):
           # maybe_message = #PinChangeMessage#MessageDecoder.parseBytes(pm.payload)
        #    print(f'PINCHANGE: {pm.payload}')
        #    for p in pm.pinInfo:
        #        print(f'PININFO: {p}')

        #    pass    
    
    def OnConnected(self):
        super().OnConnected()
    
    def OnDisconnected(self):
        super().OnDisconnected()
    
    def Setup(self):
        super().Setup()
    
    def Loop(self):
        super().Loop()
        if (self.FeatureReady() == True and self.ConfigComplete() == True):
            #self.Debug('Feature is ready for processing.')
            pass
'''
    AnalogInputs
'''
class AnalogInputs(IOFeature):
    def __init__(self) -> None:
        IOFeature.__init__(self, featureName=str(Features.ANALOG_INPUTS), featureConfigName=Features.ANALOG_INPUTS.configName(), featureID=int(Features.ANALOG_INPUTS))
    
    def YamlParser(self):
        return lambda yaml, featureID : AnalogPin(yaml=yaml, featureID=featureID, halPinDirection=HalPinDirection.HAL_OUT)
'''
    AnalogOutputs
'''
class AnalogOutputs(IOFeature):
    def __init__(self) -> None:
        IOFeature.__init__(self, featureName=str(Features.ANALOG_OUTPUTS), featureConfigName=Features.ANALOG_OUTPUTS.configName(), featureID=int(Features.ANALOG_OUTPUTS))
    
    def YamlParser(self):
        return lambda yaml, featureID : AnalogPin(yaml=yaml, featureID=featureID, halPinDirection=HalPinDirection.HAL_IN)

# Create an instance of each feature for YAML processing purposes.
# When the yaml config is parsed, the objects get copied/duplicated and assigned to a particular MCU.  Each MCU has its own copy of a feature object so
# logic can be executed as needed for Config updates, pin updates, etc.
 
di = DigitalInputs()
do = DigitalOutputs()
#ai = AnalogInputs()
#ao = AnalogOutputs()

# the featureList holds the IOFeature object copies for reference during yaml parsing.
featureList = [ di, 
                do,
                #ai,
                #ao
              ]

'''
END IO FEATURE OBJECTS
'''

def try_load_linuxcnc():
    try:
        modulename = 'linuxcnc'
        if modulename not in sys.modules:
        #    print(f'You have not imported the {modulename} module')
            import linuxcnc
    except ImportError:
        raise ImportError('Error. linuxcnc module not found. Hal emulation requires linuxcnc module.')
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

            
# taken from https://stackoverflow.com/questions/1742866/compute-crc-of-file-in-python
def forLoopCrc(fpath):
    """With for loop and buffer."""
    crc = 0
    with open(fpath, 'rb', 65536) as ins:
        for x in range(int((os.stat(fpath).st_size / 65536)) + 1):
            crc = zlib.crc32(ins.read(65536), crc)
    return (crc & 0xFFFFFFFF)
    
class ArduinoYamlParser:
    def parseYaml(path:str) -> list[ArduinoSettings]: # parseYaml returns a list of ArduinoSettings objects. WILL throw exceptions on error
        if os.path.exists(path) == False:
            raise FileNotFoundError(f'Error. {path} not found.')
        crcval = int(forLoopCrc(path))
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
                if ConfigElement.HAL_EMULATION in doc[ConfigElement.ARDUINO_KEY].keys(): 
                    new_arduino.hal_emulation = doc[ConfigElement.ARDUINO_KEY][ConfigElement.HAL_EMULATION]
                if ConfigElement.ENABLED in doc[ConfigElement.ARDUINO_KEY].keys(): 
                    new_arduino.enabled = doc[ConfigElement.ARDUINO_KEY][ConfigElement.ENABLED]
                    if new_arduino.enabled == False:
                        new_arduino.component_name = f'{new_arduino.component_name}_DISABLED'
                new_arduino.baud_rate = SerialConfigElement.BAUDRATE.defaultValue()#.value[DEFAULT_VALUE_KEY]
                new_arduino.connection_timeout = ConnectionConfigElement.TIMEOUT.defaultValue() #.TIMEOUT.value[DEFAULT_VALUE_KEY]

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
                        #for ff in featureList:
                        #    pass
                        #f = list(filter(lambda a: str(a.featureConfigName) == k, featureList))
                        
                        if len([e for e in featureList if e.featureConfigName == k]) == 0:
                            # This logic skips unsupported features that happen to be included in the YAML.
                            # Future TODO: throw an exception. For now, just ignore as more developmet is needed to finish the feature parsers.
                            continue
                            
                        f = [e for e in featureList if e.featureConfigName == k][0] # object reference from feature
                        #b = [e.value[0] for e in ConfigPinTypes if e.value[0] == k][0] # String of enum
                        c = [e.YamlParser() for e in featureList if e.featureConfigName == k][0] # Parser lamda from feature object
                        d = [e.featureID for e in featureList if e.featureConfigName == k][0] #d = [e.value[FEATURE_INDEX_KEY] for e in ConfigPinTypes if e.value[0] == k][0] # Feature ID
                        copy_f = copy.deepcopy(f) # Creates a copy of the feature object so each arduino can utilize the logic independent of each other
                        new_arduino.io_map[copy_f] = []
                        copy_f.pinList = new_arduino.io_map[copy_f] 
                        if v != None:
                            for v1 in v:   
                                new_arduino.io_map[copy_f].append(c(v1, d)) # Here we just call the lamda function, which magically returns a correct object with all the settings
                                pass
                        
                new_arduino.yamlProfileSignature = crcval
                mcu_list.append(new_arduino)
                logging.debug(f'PYDEBUG: Loaded Arduino from config:\n{new_arduino}')
        return mcu_list      





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
    
    '''
        def getIndexOfFeature(self, str:str):
        if str.upper() not in FeatureTypes.keys():
            raise Exception(f'PYDEBUG Error, key {str} not found in FeatureTypes map.')
        t = FeatureTypes[str.upper()]
        return FeatureTypes[str.upper()]
    '''


    def isFeatureEnabledByString(self, str:str):
        return self.bits[self.getIndexOfFeature(str)] == 1 
    '''
        def getFeatureString(self, index:int):
        return list(FeatureTypes.keys())[list(FeatureTypes.values()).index(index)][0]
    '''

    '''
        def getEnabledFeatures(self):
        ret = {}
        for k,v in FeatureTypes.items():
            if self.isFeatureEnabledByInt(v) == True:
                ret[k] = v
        return ret
    '''


'''
    End MCU state control objects and helper classes
'''



'''
    Connection Objects and Helpers
'''
RX_MAX_QUEUE_SIZE = 10

class Connection(MessageDecoder):
    # Constructor
    def __init__(self, myType:ConnectionType, alias:str=''):
        self.connectionType = myType
        self.connectionState = ConnectionState.DISCONNECTED
        self.timeout = 10
        self.arduinoProfileSignature = 0
        self.lastMessageReceived = time.time()
        #self.maxMsgSize = 512
        self.enabledFeatures = None
        self.uid = 'UD'
        self.alias = ''
        self.rxQueue = Queue(RX_MAX_QUEUE_SIZE)
        self._messageReceivedCallbacks = {}
        self._connectionStateCallbacks = []
        self.connLastFormed = None #datetime of when the connection was last formed   
        self.arduinoReportedUptime = 0     

    def sendCommand(self, m:str):
        cm = MessageEncoder().encodeBytes(mt=MessageType.MT_COMMAND, payload=[m, 1])
        self.sendMessage(bytes(cm))

    def setState(self, newState:ConnectionState):
        if newState != self.connectionState:
            logging.debug(f'PYDEBUG: changing state from {self.connectionState} to {newState}')
            self.connectionState = newState
            for o in self._connectionStateCallbacks:
                o(newState) # Send callback to any listener who wants to react to connection state changes

    def onMessageRecv(self, m:ProtocolMessage):
        if m.mt == MessageType.MT_HANDSHAKE:
            logging.debug(f'PYDEBUG: onMessageRecv() - Received MT_HANDSHAKE, Values = {m.payload}')
            try:
                self.timeout = m.timeout / 1000
                self.arduinoProfileSignature = m.profileSignature
                self.enabledFeatures = m.enabledFeatures
                self.uid = m.UID
                self.setState(ConnectionState.CONNECTED)
                self.lastMessageReceived = time.time()
                resp = m.packetize()
                self.sendMessage(resp)
            except Exception as ex:
                just_the_string = traceback.format_exc()
                #logging.debug(just_the_string)
                logging.debug(f'PYDEBUG: error: {str(ex)}')
        
        if self.connectionState != ConnectionState.CONNECTED:
                name = 'UNKNOWN_TYPE'
                try:
                    name = MessageType(m.mt)
                except Exception as ex:
                    pass
                    #debugstr = f'PYDEBUG Error. Received unknown message of type {m.mt} from arduino. Ignoring.'
                    #logging.debug(debugstr)
                debugstr = f'PYDEBUG Error. Received message of type {name} from arduino prior to completing handshake. Ignoring.'
                logging.debug(debugstr)
                return
        if m.mt == MessageType.MT_DEBUG:
            logging.debug(f'PYDEBUG onMessageRecv() - Received MT_DEBUG, Values = {m.payload}')
        if m.mt == MessageType.MT_HEARTBEAT:
            logging.debug(f'PYDEBUG onMessageRecv() - Received MT_HEARTBEAT, Values = {m.payload}')
            #bi = m.payload[0]-1 # board index is always sent over incremeented by one
            self.arduinoReportedUptime = m.uptime
            self.lastMessageReceived = time.time()
            hb = MessageEncoder().encodeBytes(payload=m.payload) + b'\x00'
            self.sendMessage(bytes(hb))

        if m.mt in self._messageReceivedCallbacks.keys():
            if m.mt in self._messageReceivedCallbacks.keys():
                self._messageReceivedCallbacks[m.mt](m)
            else:
                debugstr = f'PYDEBUG Error. Received message of unknown type {m.mt} from arduino, ignoring.'
                logging.debug(debugstr)
        
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

    def sendInviteSyncMessage(self):
        si = InviteSyncMessage()
        self.sendMessage(si.packetize())

    def updateState(self):
        if self.connectionState == ConnectionState.DISCONNECTED or self.connectionState == ConnectionState.ERROR:
            #self.lastMessageReceived = time.process_time()
            self.connLastFormed = None
            self.arduinoReportedUptime = 0 
            self.setState(ConnectionState.CONNECTING)
            
        elif self.connectionState == ConnectionState.CONNECTING:
            pass
            #if time.process_time() - arduino.lastMessageReceived >= arduino.timeout:
            #    arduino.setState(ConnectionState.CONNECTING)
        elif self.connectionState == ConnectionState.CONNECTED:
            d = time.time() - self.lastMessageReceived
            if self.connLastFormed is None:
                self.connLastFormed = datetime.now()
            if (time.time() - self.lastMessageReceived) >= self.timeout:
                self.setState(ConnectionState.DISCONNECTED)
                self.sendInviteSyncMessage() # Send sync invite to arduino. This avoids the Arduino waiting for a timeout to occur before eventually re-trying the handshake routine (shaves off upwards of 10 seconds)

    def getConnectionState(self) -> ConnectionState:
        return self.connectionState
    
    def messageReceivedSubscribe(self, mt:MessageType, callback: callable):
        self._messageReceivedCallbacks[mt] = callback
  
    def connectionStateSubscribe(self, callback: callable):
        self._connectionStateCallbacks.append(callback)
        

 
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
'''
class Feature:
    def __init__(self, ft):
        self.ft = ft
        pass
    
class DigitalInputFeauture(Feature):
    def __init__(self):
        Feature.__init__(self, ft=FeatureTypes['DIGITAL_INPUTS'])

di = DigitalInputFeauture()
'''

        
class SerialConnection(Connection):
    def __init__(self, dev: str, myType=ConnectionType.SERIAL, profileSignature: int = 0, baudRate: int = 115200, timeout: int = 1):
        super().__init__(myType)
        logging.debug(f'PYDEBUG: SerialConnection __init__: dev={dev}, baudRate={baudRate}, timeout={timeout}')
        self.rxBuffer = bytearray()
        self.shutdown = False
        self.dev = dev
        self.daemon = None
        self.serial = None    
        self.baudRate = baudRate
        self.timeout = timeout  
        self.profileSignature = profileSignature
        self.status = ThreadStatus.STOPPED
        self.arduino = None

    def startRxTask(self):
        logging.debug(f'PYDEBUG: SerialConnection::startRxTask: dev={self.dev}')
        if self.daemon is not None:
            raise RuntimeError(f'PYDEBUG: SerialConnection::startRxTask: dev={self.dev}, error: RX thread already started!')
        
        self.daemon = Thread(target=self.rxTask, daemon=False, name='Arduino RX')
        self.daemon.start()

    def stopRxTask(self):
        logging.debug(f'PYDEBUG: SerialConnection::stopRxTask dev={self.dev}')
        if self.daemon is not None:
            self.shutdown = True
            self.daemon.join()
            self.daemon = None

    def sendMessage(self, b: bytes):
        logging.debug(f'PYDEBUG: SerialConnection::sendMessage, dev={self.dev}, Message={b}')
        if self.arduino and self.arduino.is_open:
            self.arduino.write(b)

    def sendCommand(self, m: str):
        cm = MessageEncoder().encodeBytes(payload=[MessageType.MT_COMMAND, m, 1])
        logging.debug(f'PYDEBUG: SerialConnection::sendCommand, dev={self.dev}, Command={bytes(cm)}')
        self.sendMessage(bytes(cm))

    def rxTask(self):
        self.status = ThreadStatus.RUNNING
        while not self.shutdown:
            try:
                if self.arduino is None or not self.arduino.is_open:
                    self.arduino = serial.Serial(self.dev, baudrate=self.baudRate, timeout=self.timeout, xonxoff=False, rtscts=False, dsrdtr=True)
                    for port in serial.tools.list_ports.comports():
                        if port.serial_number and (self.dev in port.device or port.serial_number in self.dev):
                            self.serial = port.serial_number
                    self.sendInviteSyncMessage()

                num_bytes = self.arduino.in_waiting
                self.updateState()
                if num_bytes > 0:
                    self.rxBuffer += self.arduino.read(num_bytes)
                    while True:
                        newlinepos = -1; # TODO: get rid of all of this raw serial strings debugging self.rxBuffer.find(b'\r\n')
                        termpos = self.rxBuffer.find(b'\x00')

                        if newlinepos == -1 and termpos == -1:
                            break

                        readDebug = newlinepos != -1 and (termpos == -1 or newlinepos < termpos)
                        readMessage = termpos != -1 and (newlinepos == -1 or termpos < newlinepos)

                        if readDebug:
                            chunk, self.rxBuffer = self.rxBuffer.split(b'\r\n', maxsplit=1)
                            logging.debug(f'[{self.alias}]: {bytes(chunk).decode("utf8", errors="ignore")}')
                        elif readMessage:
                            chunk, self.rxBuffer = self.rxBuffer.split(b'\x00', maxsplit=1)
                            logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, chunk bytes: {chunk}')
                            try:
                                md = self.parseBytes(chunk)
                                self.onMessageRecv(m=md)
                            except Exception as ex:
                                logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, Exception: {traceback.format_exc()}')
                else:
                    time.sleep(0.01)
            except OSError as oserror:
                logging.error(f'OS Error = : {str(oserror)}')
                logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, Exception: {traceback.format_exc()}, OS Error: {str(oserror)}')
                
                self.daemon = None
                self.arduino = None
                self.setState(ConnectionState.DISCONNECTED)
                self.status = ThreadStatus.CRASHED
                break

            except Exception as error:
                logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, Exception: {traceback.format_exc()}')
                
                self.daemon = None
                self.arduino = None
                self.setState(ConnectionState.DISCONNECTED)
                self.status = ThreadStatus.CRASHED
                break
'''
class HalPinConnection:
    def __init__(self, component, pinName:str, pinType:HalPinTypes, pinDirection:HalPinDirection):
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
'''





class ArduinoConnection:
    def __init__(self, settings:ArduinoSettings):
        self.settings = settings
        self.serialConn = SerialConnection(dev=settings.dev, baudRate=settings.baud_rate, profileSignature=self.settings.yamlProfileSignature, timeout=settings.connection_timeout)
        self.serialConn.alias = settings.alias
        self.serialDeviceAvailable = False
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
        '''
        self.component = hal.component(self.settings.component_name) # Future TODO: Implement unload function as shown above.  Appear to need to handle CTRL+C in main loop to respect the unload request
                
        for k, v in settings.io_map.items():
            for v1 in v:
                if v1.pinEnabled == True: 
                    v1.halPinConnection = HalPinConnection(component=self.component, pinName=v1.pinName, pinType=v1.halPinType, pinDirection=v1.halPinDirection)

        self.component.ready()  
        
        '''

        if self.settings.enabled == True:
            self.serialConn.messageReceivedSubscribe(mt=MessageType.MT_PINCHANGE, callback=lambda m: self.onMessage(m))
            self.serialConn.messageReceivedSubscribe(mt=MessageType.MT_CONFIG_ACK, callback=lambda m: self.onMessage(m))
            self.serialConn.messageReceivedSubscribe(mt=MessageType.MT_CONFIG_NAK, callback=lambda m: self.onMessage(m))

            self.serialConn.connectionStateSubscribe(callback=lambda s: self.onConnectionStateChange(s))
            
            try:
                for f in self.settings.io_map.keys():
                    f.debugSubscribe(callback=lambda d: self.onDebug(d))
                    f.Setup()
                    f.sendMessageSubscribe(callback=lambda m: self.sendMessage(m))
            except Exception as ex:
                just_the_string = traceback.format_exc()
                logging.debug(f'PYDEBUG: Error [{settings.alias}: {str(just_the_string)}') 
        
    def __str__(self) -> str:
        return f'Arduino Alias = {self.settings.alias}, Component Name = {self.settings.component_name}, Enabled = {self.settings.enabled}'
    
    def sendMessage(self, pm:ProtocolMessage):
        self.serialConn.sendMessage(pm)
        
    def onDebug(self, d:str):
        logging.debug(f'PYDEBUG: [{self.settings.alias}]: {d}') 
    
    def onConnectionStateChange(self, state:ConnectionState):
        if self.settings.enabled == True:
            try:
                for f in self.settings.io_map.keys():
                    if state == ConnectionState.CONNECTED:
                        f.OnConnected()
                    elif state == ConnectionState.DISCONNECTED:
                        f.OnDisconnected()
            except Exception as ex:
                just_the_string = traceback.format_exc()
                logging.debug(f'PYDEBUG: Error [{self.settings.alias}: {str(just_the_string)}') 

    def onMessage(self, m:ProtocolMessage):
        try:
            if 'fi' in m.payload:
                fid = m.payload['fi']
                d = [e for e in self.settings.io_map.keys() if e.featureID == int(fid)][0]
                if d != None:
                    d.OnMessageRecv(m)
                #pass
            pass
        except Exception as ex:
            just_the_string = traceback.format_exc()
            logging.debug(f'PYDEBUG: Error [{self.settings.alias}: {str(just_the_string)}') 
        pass
        '''
        if debug_comm:print(f'PYDEBUG onMessageRecv() - Message Type {m.messageType}, Values = {m.payload}')
        if m.messageType == MessageType.MT_CONFIG_ACK:
            pass
        if m.messageType == MessageType.MT_CONFIG_NAK:
            pass
        if m.messageType == MessageType.MT_PINCHANGE:
            if debug_comm:print(f'PYDEBUG onMessageRecv() - Received MT_PINCHANGE, Values = {m.payload}')

            try:
                #pc = PinChangeMessage()
                #pc.parse(m)
                fid = m.payload['fi']
                sid = m.payload['si']
                rr = m.payload['rr']
                ms = m.payload['ms']


                for k, v in self.settings.io_map.items():
                    if fid == int(k):#k.value[FEATURE_INDEX_KEY]:
                        j = json.loads(ms)
                        for val in j['pa']:
                            for pin in v:
                                if pin.pinEnabled == False:
                                    logging.debug(f'PYDEBUG: Error. Cannot update PIN that is disabled by the yaml profile.') 
                                    return
                                if pin.pinID == val['pid']:
                                    self.component[pin.pinName] = val['v']
                        break
                
            except Exception as ex:
                just_the_string = traceback.format_exc()
                print(just_the_string)
                logging.debug(f'PYDEBUG: error: {str(ex)}') 
         
        '''
    #def doFeaturePinUpdates(self):
        '''
        for k, v in self.settings.io_map.items():
            if k.name == ConfigPinTypes.DIGITAL_OUTPUTS.name or k.name == ConfigPinTypes.ANALOG_OUTPUTS.name:
                for v1 in v:
                    
                    #if v1.halPinDirection == HalPinDirection.HAL_OUT:
                    r = v1.halPinConnection.Get()
                    if r == v1.halPinCurrentValue:
                        
                        continue
                    else:
                        
                        #print(f'VALUE CHANGED!!! Old Value {v1.halPinCurrentValue}, New Value {r}')
                        ## Send with logical pin ID to avoid for loop of pins by arduino.
                        j = {
                        "pa": [
                            {"lid": v1.pinLogicalID, "pid": int(v1.pinID), "v":int(r)}
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
                        #time.sleep(.2)
                        v1.halPinCurrentValue = r
                    
                         #= HalPinConnection(component=self.component, pinName=v1.pinName, pinType=v1.halPinType, pinDirection=v1.halPinDirection) 
            
        '''
        

    def doWork(self):
        if self.settings.enabled == False:
            return # do nothing. 
        
        #if self.serialConn.connectionState == ConnectionState.CONNECTED:

        #self.doFeaturePinUpdates()

        if self.serialConn.status == ThreadStatus.STOPPED:
            self.serialConn.startRxTask()

        if self.serialConn.status == ThreadStatus.CRASHED:
            logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, ThreadStatus == CRASHED')
            # perhaps device was unplugged..
            found = False
            for port in serial.tools.list_ports.comports():
                #print(port)
                #if port.device == '/dev/ttyUSB0':
                #    pass
                if self.settings.dev in port or (port.serial_number != None and self.serialConn.serial != None and self.serialConn.serial in port.serial_number):
                    found = True
                    self.serialDeviceAvailable = True
            if found == False:
                self.serialDeviceAvailable = False
                retry = 2.5
                logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}. Error: Serial device not found! Retrying in {retry} seconds..')
                time.sleep(retry) # TODO: Make retry settable via the yaml config?
                

            else:
                logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, Serial device found, restarting RX thread..')
                time.sleep(1) # TODO: Consider making this delay settable? Trying to avoid hammering the serial port when its in a strange state
                self.serialConn.startRxTask()
        if self.serialConn.connectionState == ConnectionState.CONNECTED:
            self.serialDeviceAvailable = True
        for f in self.settings.io_map.keys():
            if self.serialConn.connectionState == ConnectionState.CONNECTED and f.ConfigSyncError() == True:
                logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, Feature {f.featureName} reported sync error. Terminating connection..')
                self.serialConn.setState(newState=ConnectionState.ERROR)
                return
            else:
                f.Loop()
        '''
                if self.serialConn.getConnectionState() == ConnectionState.CONNECTED and self.settings.profileSignature is not self.serialConn.arduinoProfileSignature:
            
            #time.sleep(5)
            j = self.settings.configJSON()
            #config_json = json.dumps(j)
            #h = int(hashlib.sha256(config_json.encode('utf-8')).hexdigest(), 16) % 10**8
            for k, v in j.items():
                total = len(v)
                seq = 0
                for k1, v1 in v.items():
                    v1['li'] = k1
                    #print(v1)
                    cf = ConfigMessage(configJSON=json.dumps(v1), seq=seq, total=total, featureID=v1['fi'])
                    print(json.dumps(v1))
                    seq += 1
                    try:
                        self.serialConn.sendMessage(cf.packetize())
                        self.serialConn.arduino.flush()
                        #if seq == 1:
                        #    time.sleep(5)
                    except Exception as error:
                        just_the_string = traceback.format_exc()
                        logging.debug(f'PYDEBUG: ArduinoConnection::doWork, dev={self.settings.dev}, alias={self.settings.alias}, Exception: {str(error)}, Traceback = {just_the_string}')
                        # Future TODO: Consider doing something intelligent and not just reporting an error. Maybe increment a hal pin that reflects error counts?
                        return
                    time.sleep(.05)
            self.serialConn.arduinoProfileSignature = self.settings.profileSignature
        
        '''

    
'''
    End Connection Objects and Helpers
'''
            
arduino_map = []

def listDevices():
    
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

    
class ThreadStatus(StrEnum):
    RUNNING = "RUNNING"
    FINISHED_OK = "FINISHED_OK"
    STOPPED = "STOPPED"
    CRASHED = "CRASHED"
    def __str__(self) -> str:
        return self.value
def display_arduino_statuses(stdscr, arduino_connections, scroll_offset, selected_index):
    # Initialize colors
    curses.start_color()
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_RED)    # Red background
    curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_GREEN)  # Green background
    curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_YELLOW) # Yellow background
    curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_WHITE)  # Highlight background

    curses.curs_set(0)

    stdscr.clear()

    height, width = stdscr.getmaxyx()
    max_widths = {
        "alias": 17,
        "component_name": 22,
        "device": 15,
        "status": 13,
        "hal_emulation": 8,  # Adjusted width for "HalEmu?"
        "features": width - (17 + 22 + 15 + 13 + 8 + 11)  # Remaining width for features, minus 11 for separators
    }

    # Check if the console is too small height-wise to show the column names and at least one row
    if height < 4:  # 1 row for the title, 1 row for the column names, 1 row for the separator, and at least 1 data row
        return

    def truncate(text, max_length):
        return text if len(text) <= max_length else text[:max_length-3] + '...'

    row = 0
    stdscr.addstr(row, 0, "Arduino Connection Statuses:")
    row += 1
    stdscr.addstr(row, 0, "{:<{alias}} | {:<{component_name}} | {:<{device}} | {:<{status}} | {:<{hal_emulation}} | {}".format(
        "Alias", "Component Name", "Device", "Status", "HalEmu?", "Features",
        **max_widths))
    row += 1
    stdscr.addstr(row, 0, "-" * width)
    row += 1

    # Separate enabled and disabled connections
    enabled_connections = [conn for conn in arduino_connections if conn.settings.enabled]
    disabled_connections = [conn for conn in arduino_connections if not conn.settings.enabled]

    all_connections = enabled_connections + disabled_connections

    for index, connection in enumerate(all_connections):
        alias = truncate(connection.settings.alias, max_widths["alias"])
        component_name = truncate(connection.settings.component_name, max_widths["component_name"])
        device = connection.settings.dev
        hal_emulation = truncate(str(connection.settings.hal_emulation), max_widths["hal_emulation"])
        features = truncate(", ".join([f"{k.featureName} [{len(v)}]" for k, v in connection.settings.io_map.items()]), max_widths["features"])

        if connection.settings.enabled:
            status = truncate(connection.serialConn.connectionState, max_widths["status"])
        else:
            status = "DISABLED"

        # Determine the color pair for the status
        if status == "DISCONNECTED":
            color_pair = curses.color_pair(1)  # Red background
        elif status == "CONNECTED":
            color_pair = curses.color_pair(2)  # Green background
        elif status == "DISABLED":
            color_pair = curses.color_pair(0)  # Default background
        else:
            color_pair = curses.color_pair(3)  # Yellow background

        # Handle scrolling for device
        if len(device) > max_widths["device"]:
            if scroll_offset < len(device):
                device_display = device[scroll_offset:scroll_offset + max_widths["device"]]
                if scroll_offset + max_widths["device"] < len(device):
                    device_display = device_display[:-2] + '..'
                else:
                    device_display = device_display + ' ' * (max_widths["device"] - len(device_display))
            else:
                scroll_position = scroll_offset - len(device)
                device_display = device[scroll_position:scroll_position + max_widths["device"]]
                if scroll_position + max_widths["device"] < len(device):
                    device_display = device_display[:-2] + '..'
                else:
                    device_display = device_display + ' ' * (max_widths["device"] - len(device_display))
        else:
            device_display = device.ljust(max_widths["device"])

        # Highlight the selected row
        if index == selected_index:
            stdscr.addstr(row, 0, "{:<{alias}} | {:<{component_name}} | {:<{device}} | ".format(
                alias, component_name, device_display,
                **max_widths), curses.color_pair(4))
            stdscr.addstr("{:<{status}}".format(status, **max_widths), curses.color_pair(4) | color_pair)
            stdscr.addstr(" | {:<{hal_emulation}} | {}".format(hal_emulation, features, **max_widths), curses.color_pair(4))
        else:
            stdscr.addstr(row, 0, "{:<{alias}} | {:<{component_name}} | {:<{device}} | ".format(
                alias, component_name, device_display,
                **max_widths))
            stdscr.addstr("{:<{status}}".format(status, **max_widths), color_pair)
            stdscr.addstr(" | {:<{hal_emulation}} | {}".format(hal_emulation, features, **max_widths))
        row += 1

    stdscr.refresh()

def format_elapsed_time(elapsed_time):
    days = elapsed_time.days
    hours, remainder = divmod(elapsed_time.seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{days}d {hours}h {minutes}m {seconds}s"

def display_connection_details(stdscr, connection):
    stdscr.clear()
    height, width = stdscr.getmaxyx()

    # Initialize colors
    curses.start_color()
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_RED)    # Red background
    curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_GREEN)  # Green background
    curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_YELLOW) # Yellow background
    curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_WHITE)  # Highlight background

    alias_display = connection.settings.alias
    if not connection.settings.enabled:
        alias_display += " [DISABLED]"

    component_name = connection.settings.component_name
    device = connection.settings.dev
    is_serial_port_available = connection.serialDeviceAvailable if connection.settings.enabled else "N/A"
    hal_emulation = str(connection.settings.hal_emulation)
    status = connection.serialConn.connectionState if connection.settings.enabled else "DISABLED"
    
    # Determine arduinoReportedUptime value
    if connection.settings.enabled and status == "CONNECTED":
        arduino_reported_uptime = connection.serialConn.arduinoReportedUptime
        if arduino_reported_uptime:
            elapsed_uptime = timedelta(minutes=arduino_reported_uptime)
            uptime_str = format_elapsed_time(elapsed_uptime)
        else:
            uptime_str = "N/A"
    else:
        uptime_str = "N/A"

    # Determine connLastFormed value
    if connection.settings.enabled and status == "CONNECTED":
        conn_last_formed = connection.serialConn.connLastFormed
        if conn_last_formed:
            elapsed_time = datetime.now() - conn_last_formed
            elapsed_str = format_elapsed_time(elapsed_time)
        else:
            elapsed_str = "N/A"
    else:
        elapsed_str = "N/A"

    row = 0
    stdscr.addstr(row, 0, f"Details for {alias_display}")
    row += 2  # Add a blank line

    stdscr.addstr(row, 0, f"Component Name: {component_name}")
    row += 1
    stdscr.addstr(row, 0, f"Device: {device}")
    row += 1
    stdscr.addstr(row, 0, f"Serial Port Available: {is_serial_port_available}")
    row += 1
    stdscr.addstr(row, 0, f"HalEmu?: {hal_emulation}")
    row += 1

    # Determine the color pair for the status
    if status == "DISCONNECTED":
        color_pair = curses.color_pair(1)  # Red background
    elif status == "CONNECTED":
        color_pair = curses.color_pair(2)  # Green background
    elif status == "DISABLED":
        color_pair = curses.color_pair(0)  # Default background
    else:
        color_pair = curses.color_pair(3)  # Yellow background

    stdscr.addstr(row, 0, "Status: ")
    stdscr.addstr(f"{status}", color_pair)
    row += 1
    stdscr.addstr(row, 0, f"Arduino Reported Uptime: {uptime_str}")
    row += 1
    stdscr.addstr(row, 0, f"Connection to Arduino Uptime: {elapsed_str}")
    row += 2  # Add a blank line

    stdscr.addstr(row, 0, "Pins:")
    row += 1

    pin_headers = ["Pin Name", "Pin Type", "HAL Pin Type", "HAL Pin Direction", "Pin ID", "Current Value"]
    pin_column_widths = [15, 10, 15, 15, 10, 15]

    total_width = sum(pin_column_widths) + len(pin_headers) - 1
    available_width = width - 1
    truncate_length = lambda text, max_length: text if len(text) <= max_length else text[:max_length-3] + '...'

    if total_width > available_width:
        scaling_factor = available_width / total_width
        pin_column_widths = [int(w * scaling_factor) for w in pin_column_widths]

    stdscr.addstr(row, 0, "{:<{width0}} | {:<{width1}} | {:<{width2}} | {:<{width3}} | {:<{width4}} | {:<{width5}}".format(
        *[truncate_length(header, pin_column_widths[i]) for i, header in enumerate(pin_headers)],
        width0=pin_column_widths[0],
        width1=pin_column_widths[1],
        width2=pin_column_widths[2],
        width3=pin_column_widths[3],
        width4=pin_column_widths[4],
        width5=pin_column_widths[5]
    ))
    row += 1
    stdscr.addstr(row, 0, "-" * width)
    row += 1

    for feature, pins in connection.settings.io_map.items():
        for pin in pins:
            if not pin.pinEnabled:
                current_value = "DISABLED"
            else:
                current_value = str(getattr(pin, 'halPinCurrentValue', 'N/A'))  # Example way to get current value, adjust as needed
            stdscr.addstr(row, 0, "{:<{width0}} | {:<{width1}} | {:<{width2}} | {:<{width3}} | {:<{width4}} | {:<{width5}}".format(
                truncate_length(pin.pinName, pin_column_widths[0]),
                truncate_length(str(pin.pinType), pin_column_widths[1]),
                truncate_length(str(pin.halPinType), pin_column_widths[2]),
                truncate_length(str(pin.halPinDirection), pin_column_widths[3]),
                truncate_length(str(pin.pinID), pin_column_widths[4]),
                truncate_length(current_value, pin_column_widths[5]),
                width0=pin_column_widths[0],
                width1=pin_column_widths[1],
                width2=pin_column_widths[2],
                width3=pin_column_widths[3],
                width4=pin_column_widths[4],
                width5=pin_column_widths[5]
            ))
            row += 1
            if row >= height - 2:
                stdscr.addstr(row, 0, "... (truncated)")
                break

    stdscr.addstr(height - 1, 0, "Press any key to return")
    stdscr.refresh()
async def do_work_async(ac):
    loop = asyncio.get_event_loop()
    with concurrent.futures.ThreadPoolExecutor() as pool:
        await loop.run_in_executor(pool, ac.doWork)

async def main_async(stdscr, arduino_connections):
    scroll_offset = 0
    selected_index = 0
    details_mode = False
    stdscr.nodelay(1)  # Set nodelay mode
    last_update = time.time()
    task_status = {ac: None for ac in arduino_connections}

    while True:
        try:
            current_time = time.time()
            for ac in arduino_connections:
                if task_status[ac] is None or task_status[ac].done():
                    task_status[ac] = asyncio.create_task(do_work_async(ac))
            
            if current_time - last_update >= .01:
                if not details_mode:
                    # Sort connections by enabled/disabled status
                    enabled_connections = [conn for conn in arduino_connections if conn.settings.enabled]
                    disabled_connections = [conn for conn in arduino_connections if not conn.settings.enabled]
                    sorted_connections = enabled_connections + disabled_connections
                    
                    # Display statuses using sorted_connections
                    display_arduino_statuses(stdscr, sorted_connections, scroll_offset, selected_index)
            if current_time - last_update >= .5:
                if not details_mode:     
                    scroll_offset = (scroll_offset + 1) % (max(len(conn.settings.dev) for conn in arduino_connections) + 15)
                    last_update = current_time

            key = stdscr.getch()
            if key == ord('q'):
                break
            elif not details_mode and key == curses.KEY_UP and selected_index > 0:
                selected_index -= 1
            elif not details_mode and key == curses.KEY_DOWN and selected_index < len(arduino_connections) - 1:
                selected_index += 1
            elif not details_mode and (key == curses.KEY_ENTER or key in [10, 13]):
                details_mode = True
                stdscr.nodelay(1)  # Disable nodelay mode for details display
                # Use the sorted_connections to find the selected connection
                display_connection_details(stdscr, sorted_connections[selected_index])
            elif details_mode and key != -1:
                details_mode = False
                stdscr.nodelay(1)  # Re-enable nodelay mode
            elif details_mode:
                display_connection_details(stdscr, sorted_connections[selected_index])
            await asyncio.sleep(0.05)

        except KeyboardInterrupt:
            for ac in arduino_connections:
                ac.serialConn.stopRxTask()
            raise SystemExit
        except Exception as err:
            arduino_connections.clear()
            just_the_string = traceback.format_exc()
            logging.critical(f'PYDEBUG: error: {str(just_the_string)}')
            pass
        
def debug_print(message):
    # Save the current program state (curses mode)
    curses.def_prog_mode()
    # End curses mode temporarily
    curses.endwin()
    # Print the debug message to the console
    print(message)
    # Flush the output to ensure it appears immediately
    sys.stdout.flush()
    # Restore the program state (curses mode)
    curses.reset_prog_mode()
    
def main(stdscr):
    argumentList = sys.argv[1:]
    options = "hdp:"
    long_options = ["Help", "Devices", "Profile="]
    target_profile = None
    devs = []
    
    # these lines enable debug messages to the console
    #curses.curs_set(0)
    #stdscr.clear()
    #stdscr.refresh()

    try:
        arguments, values = getopt.getopt(argumentList, options, long_options)
        for currentArgument, currentValue in arguments:
            if currentArgument in ("-h", "--Help"):
                print("Displaying Help")
            elif currentArgument in ("-d", "--devices"):
                print('Listing available Serial devices:')
                listDevices()
                sys.exit()
            elif currentArgument in ("-p", "--profile"):
                logging.debug(f'PYDEBUG: Profile: {currentValue}')
                target_profile = currentValue
    except getopt.error as err:
        just_the_string = traceback.format_exc()
        logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit()

    if target_profile is not None:
        try:
            devs = ArduinoYamlParser.parseYaml(path=target_profile)
        except Exception as err:
            just_the_string = traceback.format_exc()
            logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
            sys.exit()
    else:
        devs = locateProfile()

    if len(devs) == 0:
        print('No Arduino profiles found in profile yaml!')
        sys.exit()

    arduino_connections = []
    try:
        for a in devs:
            c = ArduinoConnection(a)
            arduino_connections.append(c)
            logging.info(f'PYDEBUG: Loaded Arduino profile: {str(c)}')
    except Exception as err:
        just_the_string = traceback.format_exc()
        logging.debug(f'PYDEBUG: error: {str(just_the_string)}')
        sys.exit()

    asyncio.run(main_async(stdscr, arduino_connections))

if __name__ == "__main__":
   curses.wrapper(main)