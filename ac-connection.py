import sys
import serial
#import socket
from xdrlib import Unpacker
import msgpack
from io import BytesIO
from strenum import StrEnum
from enum import Enum
from threading import Thread
from queue import Queue
import time

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
       
class MessageType(Enum):
    MT_HEARTBEAT = 1, 
    MT_HANDSHAKE = 2, 
    MT_COMMAND = 3, 
    MT_PINSTATUS = 4, 
    MT_DEBUG = 5, 
    UNKNOWN = -1
    

serial_dev = '/dev/ttyACM0' 

arduino = serial.Serial(serial_dev, 115200, timeout=1, xonxoff=False, rtscts=False, dsrdtr=True)

class HandshakeMessage:
    # Constructor
    def __init__(self, b:bytes):
        self.messageType = MessageType.MT_HANDSHAKE
        self.parseBytes(b)
        self.hasError = False

    def parseBytes(self, b:bytes):
        try:
            b = b[:-1]
            b = b[2:]
            buf = BytesIO(b)
            unpacker = msgpack.Unpacker(buf, raw=False)
            l = list(unpacker)
            self.protocolVersion = l[0]
            self.featureMap = l[1]
            self.boardIndex = (l[2]).to_bytes(1)
            print('')
            '''
                for unpacked in unpacker:
                if type(unpacked) == list:
                    i = 0
                    for v in unpacked:
                        #if type(v) == type(int):
                        u = v# (1 << 32)) 
                        print(f'Index [{i}], Unpacked Value: {u}, Type={type(v)}')
                        i += 1

                else:
                    print(f'Unpacked Value: {unpacked}, Type={type(unpacked)}') 
            '''
    
            
        except Exception as ex:
            raise ex

    def __init__(self, b:bytes):
        self.parseBytes(b)
        #self.messageType = myType
class Connection:
    # Constructor
    def __init__(self, myType:ConnectionType):
        self.connectionType = myType

    #def background_task_connection_processor(self):
    #    while True:
    #        print('fun')
    #        sleep(5)
            #sleep_val = do_ids_module_check_work(ds=ds, share_drive_location=share_drive_ids_module_reports)
            
    #def launch_connection_processor(self):
        # create and start the daemon thread
        #print('Starting background proceed watch task...')
    #    daemon = Thread(target=self.background_task_connection_processor, daemon=True, name='Arduino Connection')
    #    daemon.start()
 
RX_MAX_QUEUE_SIZE = 10
rxQueue = Queue(RX_MAX_QUEUE_SIZE)

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

    def rxTask(self):
        while(self.shutdown == False):
            try:
                data = arduino.read()
                self.buffer += bytearray(data)
                print(data)
                if data == b'\x00':
                    print(bytes(self.buffer))
                    #buf = BytesIO(bytes(buffer))
                    
                    if int(self.buffer[1]) == MessageType.MT_HANDSHAKE.value[0]:
                        m = HandshakeMessage(bytes(self.buffer))
                    #p
                    
                    '''
                    unpacker = msgpack.Unpacker(buf, raw=False)
                    for unpacked in unpacker:
                        if type(unpacked) == list:
                            print(f'Unpacked List:')
                            i = 0
                            for v in unpacked:
                                print(f'Index [{i}], Unpacked Value: {v}, Type={type(v)}')
                                i += 1
                        else:
                            print(f'Unpacked Value: {unpacked}, Type={type(unpacked)}')
                    '''
                    arduino.write(bytes(self.buffer))
                    self.buffer = bytes()
                elif data == b'\n':
                    print(bytes(self.buffer).decode('utf8', errors='ignore'))
                    self.buffer = bytes()
            except Exception as ex:
                print(str(ex))
		


def main():
    #mt = MessageType.MT_COMMAND
    #print(mt)
    sc = SerialConnetion(ConnectionType.SERIAL)
    sc.startRxTask()
    while(True):
        time.sleep(1)


if __name__ == '__main__':
    sys.exit(main())  # next section explains the use of sys.exit