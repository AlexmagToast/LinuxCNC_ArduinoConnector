
from abc import ABCMeta, abstractmethod
from enum import Enum
import logging
import time

import numpy

from linuxcnc_arduinoconnector.ConfigModels import AnalogPin, ArduinoPin, DigitalPin, HalPinDirection
from linuxcnc_arduinoconnector.ProtocolModels import ConfigMessage, MessageType, ProtocolMessage

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
            logging.debug(f'PINCHANGE: {pm.payload}')
            for pi in pm.pinInfo:
                # find the pin in the pinList
                for p in self.pinList:
                    if p.pinID == pi.pinID:
                        p.halPinCurrentValue = pi.pinValue
                        if p.halPinConnection != None:
                            p.halPinConnection.set(p.halPinCurrentValue)
                        logging.debug(f'PININFO: {pi}')
                        break
    
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
InstantiatedFeaturesList = [ di, 
                do,
                #ai,
                #ao
              ]

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
