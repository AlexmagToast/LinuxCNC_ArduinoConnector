from enum import IntEnum
import logging
import json
from strenum import StrEnum
from cobs import cobs
import msgpack



MCU_PROTOCOL_VERSION = 1

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
            #self.pinInfo = parse_pin_info_elements(payload['ms'])
                    # Parse the JSON string
            # Create a list of PinInfoElement objects
            self.pinInfo = [PinInfoElement(item) for item in json.loads(payload['ms'])]
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

        self.enabledFeatures = payload['fm']
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
