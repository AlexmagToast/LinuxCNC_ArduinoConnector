import logging
import os
import copy
import yaml
from linuxcnc_arduinoconnector.ConfigModels import ArduinoSettings, ConfigConnectionTypes, ConfigElement, ConnectionConfigElement, SerialConfigElement
from linuxcnc_arduinoconnector.Utils import forLoopCrc


class ArduinoYamlParser:
    def parseYaml(path:str) -> list[ArduinoSettings]: # parseYaml returns a list of ArduinoSettings objects. WILL throw exceptions on error
        if os.path.exists(path) == False:
            raise FileNotFoundError(f'Error. {path} not found.')
        #import Features.featureList from Features
        from linuxcnc_arduinoconnector.Features import InstantiatedFeaturesList
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
                        
                        if len([e for e in InstantiatedFeaturesList if e.featureConfigName == k]) == 0:
                            # This logic skips unsupported features that happen to be included in the YAML.
                            # Future TODO: throw an exception. For now, just ignore as more developmet is needed to finish the feature parsers.
                            continue
                            
                        f = [e for e in InstantiatedFeaturesList if e.featureConfigName == k][0] # object reference from feature
                        #b = [e.value[0] for e in ConfigPinTypes if e.value[0] == k][0] # String of enum
                        c = [e.YamlParser() for e in InstantiatedFeaturesList if e.featureConfigName == k][0] # Parser lamda from feature object
                        d = [e.featureID for e in InstantiatedFeaturesList if e.featureConfigName == k][0] #d = [e.value[FEATURE_INDEX_KEY] for e in ConfigPinTypes if e.value[0] == k][0] # Feature ID
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

