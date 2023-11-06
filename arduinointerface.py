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
    'INPUTS':1,
    'SINPUTS':2,
    'OUTPUTS':3,
    'PWMOUTPUTS:':4,
    'AINPUTS':5,
    'DALLAS_TEMPERATURE_SENSOR':6,
    'LPOTIS':7,
    'BINSEL':8,
    'QUADENC':9,
    'JOYSTICK':10,
    'STATUSLED':11,
    'DLED':12,
    'KEYPAD':13,
    'SERIAL_TO_LINUXCNC':14,
    'ETHERNET_UDP_TO_LINUXCNC':15,
    'ETHERNET_TCP_TO_LINUXCNC':16,
    'WIFI_TCP_TO_LINUXCNC':17,
    'WIFI_UDP_TO_LINUXCNC':18,
    'MEMORY_MONITOR':19
}



serial_dev = '/dev/ttyACM0' 

arduino = serial.Serial(serial_dev, 115200, timeout=1, xonxoff=False, rtscts=False, dsrdtr=True)

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
            raise Exception(f'DEBUG: Error, key {str} not found in FeatureTypes map.')
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
        if hash.digest() == crc:
            return True
        else:
            return False
        
    def parseBytes(self, b:bytes):
        #try:
        self.messageType = b[0]
        data_bytes = b[:-1][1:]
        self.crc = b[-1:]
        if self.validateCRC( data=data_bytes, crc=self.crc) == False:
            raise Exception(f"Error. CRC validation failed for received message. Bytes = {b}")
        buf = BytesIO(data_bytes)
        self.payload = msgpack.unpackb(data_bytes, raw=False) 

            #me = MessageEncoder().encodeBytes(mt = MessageType.MT_HANDSHAKE.value[0], payload= self.payload)   

        #except Exception as error:
        #    just_the_string = traceback.format_exc()
        #    print(just_the_string)
        #    raise error

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
        #except Exception as ex:
        #    print(f'ERROR: {str(ex)}')
        #    raise ex


RX_MAX_QUEUE_SIZE = 10

class ArduinoConn:
    def __init__(self, bi:int, cs:ConnectionState, timeout:int):
        self.boardIndex = bi
        self.connectionState = cs
        self.timeout = timeout
        self.lastMessageReceived = time.process_time()
    def setState(self, newState:ConnectionState):
        print(f'DEBUG: Board Index: {self.boardIndex}, changing state from {self.connectionState} to {newState}')
        self.connectionState = newState


class Connection:
    # Constructor
    def __init__(self, myType:ConnectionType):
        self.connectionType = myType
        self.arduinos = [ArduinoConn(bi=0, cs=ConnectionState.DISCONNECTED, timeout=3000)] #hard coded for testing, should be based on config
        self.rxQueue = Queue(RX_MAX_QUEUE_SIZE)
    
    def onMessageRecv(self, m:MessageDecoder):
        if m.messageType == MessageType.MT_HANDSHAKE:
            print(f'DEGUG: onMessageRecv() - Recvied MT_HANDSHAKE, Values = {m.payload}')
            if m.payload[0] != protocol_ver:
                debugstr = f'DEGUG: Error. Protocol version mismatched. Expected {protocol_ver}, got {m.payload[0]}'
                print(debugstr)
                raise Exception(debugstr)
            bi = m.payload[2]-1 # board index is always sent over incremeented by one
            fmd = FeatureMapDecoder(m.payload[1])
            ef = fmd.getEnabledFeatures()
            self.arduinos[bi].setState(ConnectionState.CONNECTED)
            self.arduinos[bi].lastMessageReceived = time.process_time()
            hsr = MessageEncoder().encodeBytes(mt=MessageType.MT_HANDSHAKE, payload=m.payload)
            self.sendMessage(bytes(hsr))
        if m.messageType == MessageType.MT_HEARTBEAT:
            print(f'DEGUG: onMessageRecv() - Recvied MT_HEARTBEAT, Values = {m.payload}')
            bi = m.payload[0]-1 # board index is always sent over incremeented by one
            if self.arduinos[bi].connectionState != ConnectionState.CONNECTED:
                debugstr = f'DEGUG: Error. Received message from arduino ({m.payload[2]-1}) prior to completing handhsake. Ignoring.'
                print(debugstr)
                return
            #self.arduinos[bi].setState(ConnectionState.CONNECTED)
            self.arduinos[bi].lastMessageReceived = time.process_time()
            hb = MessageEncoder().encodeBytes(mt=MessageType.MT_HEARTBEAT, payload=m.payload)
            self.sendMessage(bytes(hb))
        if m.messageType == MessageType.MT_PINSTATUS:
            print(f'DEGUG: onMessageRecv() - Recvied MT_PINSTATUS, Values = {m.payload}')
            bi = m.payload[0]-1 # board index is always sent over incremeented by one
            if self.arduinos[bi].connectionState != ConnectionState.CONNECTED:
                debugstr = f'DEGUG: Error. Received message from arduino ({m.payload[2]-1}) prior to completing handhsake. Ignoring.'
                print(debugstr)
                return
            #self.arduinos[bi].setState(ConnectionState.CONNECTED)
            self.arduinos[bi].lastMessageReceived = time.process_time()
            try:
                self.rxQueue.put(m, timeout=5)
            except Queue.Empty:
                print("DEBUG: Error. Timed out waiting to gain access to RxQueue!")
            except Queue.Full:
                print("Error. RxQueue is full!")
            #return None 
            #hb = MessageEncoder().encodeBytes(mt=MessageType.MT_HEARTBEAT, payload=m.payload)
            #self.sendMessage(bytes(hb))
            
    def sendMessage(self, b:bytes):
        pass

    def update_state(self):
        for arduino in self.arduinos:
            if arduino.connectionState == ConnectionState.DISCONNECTED:
                #self.lastMessageReceived = time.process_time()
                arduino.setState(ConnectionState.CONNECTING)
            elif arduino.connectionState == ConnectionState.CONNECTING:
                pass
                #if time.process_time() - arduino.lastMessageReceived >= arduino.timeout:
                #    arduino.setState(ConnectionState.CONNECTING)
            elif arduino.connectionState == ConnectionState.CONNECTED:
                pass
                #if time.process_time() - arduino.lastMessageReceived >= arduino.timeout:
                #    arduino.setState(ConnectionState.DISCONNECTED)


        # at startup, the arduino map will be empty until a connection is made
  
 


class SerialConnetion(Connection):
    def __init__(self, myType:ConnectionType):
        super().__init__(myType)
        self.buffer = bytes()
        self.shutdown = False
        arduino.timeout = 3000
        
    def isSerialConnection(self):
        return True
    
    def startRxTask(self):
        # create and start the daemon thread
        #print('Starting background proceed watch task...')
        daemon = Thread(target=self.rxTask, daemon=True, name='Arduino RX')
        daemon.start()

    def sendMessage(self, b: bytes):
        #return super().sendMessage()
        arduino.write(b)
    def rxTask(self):
        while(self.shutdown == False):
            try:
                data = arduino.read()
                if data == b'\x00':
                    #print(bytes(self.buffer))
                    md = MessageDecoder(bytes(self.buffer)[1:])
                    self.onMessageRecv(m=md)
                    #arduino.write(bytes(self.buffer))
                    self.buffer = bytes()
                elif data == b'\n':
                    self.buffer += bytearray(data)
                   # print(bytes(self.buffer).decode('utf8', errors='ignore'))
                    self.buffer = bytes()
                else:
                    self.buffer += bytearray(data)
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