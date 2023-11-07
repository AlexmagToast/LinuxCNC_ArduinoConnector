from multiprocessing import Value
import sys
import serial
#import socket
from xdrlib import Unpacker
import msgpack
from io import BytesIO
from strenum import StrEnum
from enum import IntEnum
from threading import Thread
from queue import Queue
import time
import crc8
import traceback
import numpy

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
    MT_HANDSHAKE = 2, 
    MT_COMMAND = 3, 
    MT_PINSTATUS = 4, 
    MT_DEBUG = 5, 
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
    'SERIAL_TO_LINUXCNC':15,
    'ETHERNET_UDP_TO_LINUXCNC':16,
    'ETHERNET_TCP_TO_LINUXCNC':17,
    'WIFI_TCP_TO_LINUXCNC':18,
    'WIFI_UDP_TO_LINUXCNC':19,
    'MEMORY_MONITOR':20
}

debug_comm = True

serial_dev = '/dev/ttyACM0' 
#serial_dev = '/dev/tty.usbmodemF412FA68D6802'

arduino = serial.Serial(serial_dev, 115200, timeout=1.0, xonxoff=False, rtscts=False, dsrdtr=True)

protocol_ver = 1

class FeatureMapDecoder:
    def __init__(self, b:bytes):
        self.features = b
        self.bits = self.unpackbits(b)
        #self.bits = numpy.unpackbits(numpy.arange(b.astype(numpy.uint64), dtype=numpy.uint64))
  
    def unpackbits(self, x):
        z_as_int64 = numpy.int64(x)
        xshape = list(z_as_int64.shape)
        z_as_int64 = z_as_int64.reshape([-1, 1])
        mask = 2**numpy.arange(64, dtype=z_as_int64.dtype).reshape([1, 64])
        return (z_as_int64 & mask).astype(bool).astype(int).reshape(xshape + [64])

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
    def __init__(self, b:bytes):
        self.parseBytes(b)

    def validateCRC(self, data:bytes, crc:bytes):
        hash = crc8.crc8()
        hash.update(data)
        d = hash.digest()
        if hash.digest() == crc:
            return True
        else:
            return False
        
    def parseBytes(self, b:bytes):
        #try:
        self.messageType = b[1]
        data_bytes = b[2:]
        data_bytes = data_bytes[:-1]
        self.crc = b[-1:]
        #strc = ''
        #print('Data_Bytes=')
        #for b1 in bytes(data_bytes):
        #    strc += f'[{hex(b1)}]'
        #print(strc)
        #test = msgpack.unpackb(b'\x92\xA4\x49\x33\x3A\x30\x01', use_list=False, raw=False)
        if self.validateCRC( data=data_bytes, crc=self.crc) == False:
            raise Exception(f"Error. CRC validation failed for received message. Bytes = {b}")
        
        self.payload = msgpack.unpackb(data_bytes, use_list=True, raw=False)

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
        self.lastMessageReceived = time.process_time()
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
                #d = time.time() - arduino.lastMessageReceived
                if (time.time() - arduino.lastMessageReceived) >= arduino.timeout:
                    arduino.setState(ConnectionState.DISCONNECTED)

    def getState(self, index:int):
        return self.arduinos[index].connectionState
  
 


class SerialConnetion(Connection):
    def __init__(self, myType:ConnectionType):
        super().__init__(myType)
        self.buffer = bytes()
        self.shutdown = False
        arduino.timeout = 1
        self.daemon = None
        
    def isSerialConnection(self):
        return True
    
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
        arduino.write(b)
        arduino.flush()
    
    def sendCommand(self, m:str):
        cm = MessageEncoder().encodeBytes(mt=MessageType.MT_COMMAND, payload=[m, 1])
        self.sendMessage(bytes(cm))
        
    def rxTask(self):
        arduino.timeout = 1
        while(self.shutdown == False):
            try:
                data = arduino.read()
                if data == b'\x00':
                    #print(bytes(self.buffer))
                    #strb = ''
                    #print('Bytes from wire: ')
                    #for b in bytes(self.buffer):
                    #    strb += f'[{hex(b)}]'
                    #print(strb)
                    try:
                        md = MessageDecoder(bytes(self.buffer))
                        self.onMessageRecv(m=md)
                    except Exception as ex:
                        print(f'PYDEBUG {str(ex)}')
                
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
		
#def main():
#    sc = SerialConnetion(ConnectionType.SERIAL)
#    sc.startRxTask()
#    while(True):
#        time.sleep(1)


#if __name__ == '__main__':
#    sys.exit(main()) 