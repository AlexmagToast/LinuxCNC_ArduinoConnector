from asyncio import Queue
from datetime import datetime
import logging
from threading import Thread
import time
import traceback
import serial
from strenum import StrEnum
from linuxcnc_arduinoconnector.ConfigModels import ArduinoSettings
from linuxcnc_arduinoconnector.ProtocolModels import ConnectionState, ConnectionType, InviteSyncMessage, MessageDecoder, MessageEncoder, MessageType, ProtocolMessage
import serial.tools.list_ports

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
                            if len(chunk) > 0:
                                try:
                                    md = self.parseBytes(chunk)
                                    self.onMessageRecv(m=md)
                                except Exception as ex:
                                    logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, Exception: {traceback.format_exc()}')
                            else:
                                logging.debug(f'PYDEBUG: SerialConnection::rxTask, dev={self.dev}, Warning. Received empty message from arduino.')
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

class ThreadStatus(StrEnum):
    RUNNING = "RUNNING"
    FINISHED_OK = "FINISHED_OK"
    STOPPED = "STOPPED"
    CRASHED = "CRASHED"
    def __str__(self) -> str:
        return self.value