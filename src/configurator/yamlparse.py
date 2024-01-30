import yaml

def read_yaml(file_path):
    """
    Reads a YAML file and returns the parsed content as a Python object.
    """
    with open(file_path, 'r') as file:
        yaml_content = list(yaml.safe_load_all(file))
    return yaml_content

def write_yaml(data, file_path):
    """
    Writes a Python object to a YAML file.
    """
    with open(file_path, 'w') as file:
        yaml.dump(data, file, default_flow_style=False)

def count_occurrences(data, key_to_count):
    """
    Counts the occurrences of a specific key in a YAML object.
    """
    count = 0
    if isinstance(data, dict):
        for key, value in data.items():
            if key == key_to_count:
                count += 1
            if isinstance(value, (dict, list)):
                count += count_occurrences(value, key_to_count)
    elif isinstance(data, list):
        for item in data:
            count += count_occurrences(item, key_to_count)
    return count

def get_all_keys(d):
    keys = []
    for key, value in d.items():
        keys.append(key)
        if isinstance(value, dict):
            keys.extend(get_all_keys(value))
    return keys

class Features:
    def __init__(self, key):
        self.key = key

    def featureList2(self,key):
        if key == 'mcu':
            return self.mcu()
        elif key == 'analogInputs':
            return self.analogInputs()
        elif key == 'pwmOutputs':
            return self.pwmOutputs()

    def featureList(self, key):
        # Dynamically call the method based on the provided key
        method_name = key
        if hasattr(self, method_name) and callable(getattr(self, method_name)):
            return getattr(self, method_name)()
        else:
            # Handle other cases or raise an exception
            return None  # Placeholder, adjust as needed


    def mcu(self):
        mcu_pins = {
            'alias': {'value': 'new Arduino', 'ignore': 0, 'optional': 0},
            'component_name': {'value': 'arduino.', 'ignore': 0, 'optional': 0},
            'dev': {'value': '', 'ignore': 0, 'optional': 1},
            'debug': {
                'debug_level': {'value': '0', 'ignore': 0, 'optional': 1}
            },
            'connection': {
                'baudrate': {'value': '115200', 'ignore': 0, 'optional': 1},
                'connection_serial2': {'value': '0', 'ignore': 0, 'optional': 1},
                'timeout': {'value': '5000', 'ignore': 0, 'optional': 1},
                'arduino_ip': {'value': '', 'ignore': 0, 'optional': 1},
                'arduino_port': {'value': '54321', 'ignore': 0, 'optional': 1},
                'listen_port': {'value': '54321', 'ignore': 0, 'optional': 1}
            },
            'io_map': {'value': '', 'ignore': 1, 'optional': 1},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'optional': 1}
        }
        return mcu_pins

    def analogInputs(self):
        analogPins = {
                'analogInputs':{
                'pin_id': {'value': '', 'ignore': 0, 'optional':0},
                'pin_name': {'value': 'ain.', 'ignore': 0, 'optional':1},
                'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},
                'pin_smoothing': {'value': 200, 'ignore': 0, 'optional':1},
                'pin_min_val': {'value': 0, 'ignore': 0, 'optional':1},
                'pin_max_val': {'value': 1023, 'ignore': 0, 'optional':1},
                'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
                'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
                'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
                'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
                }
        }
        return analogPins
        
    def digitalInputs(self):
        digitalInputs = {
                'digitalInputs':{  
                'pin_id': {'value': '', 'ignore': 0, 'optional':0},
                'pin_name': {'value': 'ain.', 'ignore': 0, 'optional':1},
                'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},
                'pin_debounce': {'value': 200, 'ignore': 0, 'optional':1},
                'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
                'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
                'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
                'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
                }
        }
        return digitalInputs
    
    def pwmOutputs(self):
        pwmOutputs = {
                'pwmOutputs':{
                'pin_id': {'value': '', 'ignore': 0, 'optional':0}, 
                'pin_name': {'value': 'pwmout.', 'ignore': 0, 'optional':1},
                'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},
                'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
                'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
                'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
                'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
                }
        }
        return pwmOutputs
    def digitalOutput(self):
        digitalOutput = {
                'digitalOutput':{
                'pin_id': {'value': '', 'ignore': 0, 'optional':0}, 
                'pin_name': {'value': 'dout.', 'ignore': 0, 'optional':1},
                'pin_type': {'value': 'HAL_BIT', 'ignore': 0, 'optional':1},   
                'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
                'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
                'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
                'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
                }
        }
        return digitalOutput
    
    def lPoti(self):
        lPoti = {
                'lPoti': {
                'pin_id': {'value': '', 'ignore': 0, 'optional':0}, 
                'pin_name': {'value': 'lpoti.', 'ignore': 0, 'optional':1},
                'pin_type': {'value': 'HAL_S32', 'ignore': 0, 'optional':1},   
                'lpoti_latches': {'value': 9, 'ignore': 0, 'optional':1},
                'value_replace': {'value': [40,50,60,70,80,90,100,110,120], 'ignore': 0, 'optional':1},
                'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
                'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
                'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
                'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
                }
        }
        return lPoti
    def binSel(self):
        binarySelectorSwitch = {
            'binarySelectorSwitch':{
            'pin_id': {'value': '', 'ignore': 0, 'optional':0}, 
            'pin_name': {'value': 'binSel.', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},   
            'pin_pins': {'value': [2,6,4,3,5], 'ignore': 0, 'optional':1},
            'value_replace': {'value': [180,190,200,0,0,0,0,0,0,0,0,0,0,0,0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170], 'ignore': 0, 'optional':1},
            'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
            'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
            'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
            }
        }
        return binarySelectorSwitch
    def quadEnc(self):
        quadratureEncoder={
            'quadratureEncoder':{
            'pin_id': {'value': '', 'ignore': 0, 'optional':0}, 
            'pin_name': {'value': 'encoder.', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_BIT', 'ignore': 0, 'optional':1},   
            'pin_pins': {'value': [2,3], 'ignore': 0, 'optional':0},
            'encoder_mode': {'value': 'counter', 'ignore': 0, 'optional':0}, #updown #vpins
            'encoder_steps': {'value': 4, 'ignore': 0, 'optional':0},
            
            #new feature: create list of Virtual Pins, only one is ever selected/ HIGH, by rotation select next / previous one. In this config a simple encoder is used to select scale of an MPG. (Standard 0.1mm)
            'encoder_virtualpins': {'value': 3, 'ignore': 0, 'optional':0}, #create 10 virtual Pins
            'encoder_virtualpin_start': {'value': 2, 'ignore': 0, 'optional':0}, #select pin 3 at program start
            'pin_type': {'value': 'FLOAT', 'ignore': 0, 'optional':0}, #hal pin type     #optional   HAL_BIT, HAL_S32 , HAL_FLOAT
            'value_replace': {'value': [0.001,0.01,0.1,1], 'ignore': 0, 'optional':0},
            'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
            'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
            'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
            }   
        }
        return quadratureEncoder()
    def joystick(self):
        joystick = {
            'joystick':{
            'pin_id': {'value': '', 'ignore': 0, 'optional':0}, 
            'pin_name': {'value': 'encoder', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_BIT', 'ignore': 0, 'optional':1},   
            'pin_pins': {'value': [2,3], 'ignore': 0, 'optional':0},
            'joystick_center': {'value': 512  , 'ignore': 0, 'optional':0},
            'joystick_deadband': {'value': 20, 'ignore': 0, 'optional':0}, #ignore values around joystick_center + - deadband 
            'joystick_scaling': {'value': 0.01, 'ignore': 0, 'optional':0},
            'pin_type': {'value': 'FLOAT', 'ignore': 0, 'optional':0}, #hal pin type     #optional   HAL_BIT, HAL_S32 , HAL_FLOAT
            'value_replace': {'value': [0.001,0.01,0.1,1], 'ignore': 0, 'optional':0},
            'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
            'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
            'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
            }
        }
        return joystick
    def statusled(self):
        statLed = {
            'statusled':{
            'pin_id': {'value': '', 'ignore': 0, 'optional':0}, 
            'pin_name': {'value': 'statusLED', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_BIT', 'ignore': 0, 'optional':1},   
            'pin_pins': {'value': 13 , 'ignore': 0, 'optional':0},
            'useDled': {'value': 0  , 'ignore': 0, 'optional':0},
            'joystick_deadband': {'value': 20, 'ignore': 0, 'optional':0}, #ignore values around joystick_center + - deadband 
            'joystick_scaling': {'value': 0.01, 'ignore': 0, 'optional':0},
            'pin_type': {'value': 'FLOAT', 'ignore': 0, 'optional':0}, #hal pin type     #optional   HAL_BIT, HAL_S32 , HAL_FLOAT
            'value_replace': {'value': [0.001,0.01,0.1,1], 'ignore': 0, 'optional':0},
            'pin_init_state': {'value': -1, 'ignore': 0, 'optional':1},
            'pin_connect_state': {'value': -1, 'ignore': 0, 'optional':1},   
            'pin_disconnect_state': {'value': 0, 'ignore': 0, 'optional':1},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
            }
        }
        return statLed


class yamlData:

    def readMCU(file, MCU_no):
        
        yaml_data = read_yaml(file)
        mcu = yaml_data[MCU_no]['mcu']
        mcu_configurations = Features.mcu()
        """
        mcu_configurations = {
            'alias': {'value': 'new Arduino', 'ignore': 0, 'optional': 1},
            'component_name': {'value': 'arduino.', 'ignore': 0, 'optional': 1},
            'dev': {'value': '', 'ignore': 0, 'optional': 1},
            'debug': {
                'debug_level': {'value': '0', 'ignore': 0, 'optional': 1}
            },
            'connection': {
                'baudrate': {'value': '115200', 'ignore': 0, 'optional': 1},
                'connection_serial2': {'value': '0', 'ignore': 0, 'optional': 1},
                'timeout': {'value': '5000', 'ignore': 0, 'optional': 1},
                'arduino_ip': {'value': '', 'ignore': 0, 'optional': 1},
                'arduino_port': {'value': '54321', 'ignore': 0, 'optional': 1},
                'listen_port': {'value': '54321', 'ignore': 0, 'optional': 1}
            },
            'io_map': {'value': '', 'ignore': 1, 'optional': 1},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'optional': 1}
        }"""
        new_mcu_configuration ={}
        # Loop through keys in mcu_configurations
        for key in mcu_configurations:
            try:
                if mcu_configurations[key]['ignore'] == 0 and key in mcu:
                    new_mcu_configuration[key] = mcu[key]
                    #print(key,new_mcu_configuration[key], "tetst")
                else:
                    if mcu_configurations[key]['ignore'] == 0:
                    #print(key, mcu_configurations[key]['value'])
                        new_mcu_configuration[key] = mcu_configurations[key]
            except: 
                new_mcu_configuration[key] = {}
                for secondkey in mcu_configurations[key]:
                    if mcu_configurations[key][secondkey]['ignore'] == 0 and key in mcu and secondkey in mcu[key]:
                        new_mcu_configuration[key][secondkey] = mcu[key][secondkey]
                    else:
                        if mcu_configurations[key][secondkey]['ignore'] == 0:
                            new_mcu_configuration[key][secondkey] = mcu_configurations[key][secondkey]['value']
            
        return new_mcu_configuration
    
    def read_yaml2(file,MCU_no, pin_map):
        yaml_data = read_yaml(file)
        Feature = yaml_data[MCU_no]['mcu']['io_map']
        #IOcapa = Features('')
        
        new_pin_map ={}
        top_level_key = next(iter(pin_map))
        if top_level_key in Feature:
            Feature = Feature[top_level_key]

            pin_map= pin_map[top_level_key]

            for pin_count, pin_no in enumerate(Feature):

                new_pin_map[pin_no['pin_id']]= {}
                
                for key  in pin_map:
                    #print(Feature[pin_count]['pin_id'],parameter+1, key)
                    if pin_map[key]['ignore'] == 0 and key in Feature[pin_count]:
                        #print(Feature[pin_count][key])
                        new_pin_map[pin_no['pin_id']][key] = Feature[pin_count][key]
                    else:
                        #print(pin_map[key]['value'])
                        if pin_map[key]['ignore'] == 0:
                            new_pin_map[pin_no['pin_id']][key] = pin_map[key]['value']

        return new_pin_map
    
    def parse_dict_to_yaml(yaml_file, newYaml, readWrite):
        #this Function parses a yaml file to dictionary or a dictionary to yaml file. 
        # parameters are: yaml file url, dictionary, read/write (0/1)

        #features = Features()
        mangledYaml = {}
        features_instance = Features(key='some_key')  # You need to provide a valid key here

        #print(features_instance.featureList(key='mcu'))

        def recursive_dict_traversal(dictionary, readWrite, parent):
            features = Features('')
            feature_list = {}

            for key, value in dictionary.items():
                #print(parent, "parent")
                if isinstance(value, dict):
                    # If the value is another dictionary, recursively call the function
                    parent.append(key)#set new parent
                    if(features.featureList(key)):#if new Parent is a independant Feature get Feature Pins
                        feature_list = features.featureList(key)

                    recursive_dict_traversal(value,readWrite,parent) #parse contents
                    parent.pop() #return back to previous parent
                if isinstance(value, list) and isinstance(value[0], dict):
                    #if value contains List with dictionary inside enter it and recurse                    
                    parent.append(key) #set new parent
                    if(key == features.featureList(key)): #if new Parent is a independant Feature get Feature Pins
                        feature_list = features.featureList(key)
                        
                    recursive_dict_traversal(value[0],readWrite,parent) #parse contents
                    parent.pop() #return back to previous parent
                    
                else:
                    # If the value is not a dictionary, do something with it
                    if isinstance(value, dict):
                        pass
                    else:

                        for i , key_no in enumerate(feature_list):
                            print(i, key_no)
                        print(f"parent:{parent[-1]},(key: {key}, value: {value},) ")

        for i in range(1):
            recursive_dict_traversal(newYaml[i],readWrite,[])

            #empty file
            with open(yaml_file, 'w'):
                pass
            #write each mcu
            for i in range(1):
                # Write the updated modified_data back to slave.yaml
                if i > 0:
                    with open(yaml_file, 'a') as slave_file:
                        slave_file.write("---\n")
                # Write the updated modified_data back to slave.yaml
                with open(yaml_file, 'a') as slave_file:
                    yaml.dump(newYaml[i], slave_file, default_flow_style=None, sort_keys=False)


    def parse_yaml_to_dict(yaml_file):
        #this Function parses a yaml file to dictionary or a dictionary to yaml file. 
        # parameters are: yaml file url, dictionary, read/write (0/1)

        mangledYaml = {}
        
        features_instance = Features(key='')
        feature_settings = {}
        

        def recursive_dict_traversal(dictionary, parent):
                    
            for key, value in dictionary.items():
                #print(parent, "parent")
                if isinstance(value, dict):
                    # If the value is another dictionary, recursively call the function
                    parent.append(key)#set new parent

                    recursive_dict_traversal(value,parent) #parse contents
                    parent.pop() #return back to previous parent
                if isinstance(value, list) and isinstance(value[0], dict):
                    #if value contains List with dictionary inside enter it and recurse                    
                    parent.append(key) #set new parent
                    #print(mangledYaml[parent[-1]])

                    recursive_dict_traversal(value[0],parent) #parse contents
                    parent.pop() #return back to previous parent
                    
                else:
                    
                    if not isinstance(value, dict): # If the value is not a dictionary, do something with it
                        if features_instance.featureList(parent[-1]) is not None:
                            feature_settings = features_instance.featureList(parent[-1])
                        """if features_instance.featureList(parent[-1]):# check if the current parent is a Feature and update feature_settins accordingly
                            feature_settings = features_instance.featureList(parent[-1])"""
                    print (feature_settings, "yooone")
                        
                                         

                        
                        #check if value is defined and if not replace by standard value
                        #if feature_settings[key]['ignore'] == 0 :
                        #    print(key, value, "replace by", feature_settings[key]['value'] , feature_settings[key]['ignore'])

                        #print(f"parent:{parent},(key: {key}, value: {value},) ")
                    
                    #mangledYaml[parent[-1]][key]= value
        
        recursive_dict_traversal(yaml_file[0],[])
        return mangledYaml

        

# Example Usage
file_path = 'example_config_nanoesp32_andesp8266.yaml'
file_path2 = 'new_config.yaml'

# Reading YAML file
yaml_data = read_yaml(file_path)

source_yaml_path2 = 'new_config2.yaml'
source_yaml_path = read_yaml('new_config.yaml')

                    #write to file      edited config   count of mcus in file (need to fix this to detect automatically)
rababer = yamlData.parse_yaml_to_dict(source_yaml_path)
print(rababer)

#print(yamlData.readMCU(file_path,0))
#print(yamlData.readMCU(file_path,1))

#print(yamlData.readAnalogInputs(file_path2,0))
#print(yamlData.readAnalogInputs(file_path,1))

#print(yamlData.readDigitalInputs(file_path2,0))
"""
IO_features = Features('')
print(Features.featureList('','analogInputs()'))
print(yamlData.read_yaml(file_path2,0,Features.analogInputs('')))
print()
print(yamlData.read_yaml(file_path2,0,Features.digitalInputs('')))  
print()
print(yamlData.read_yaml(file_path2,0,Features.digitalOutput('')))
print()
print(yamlData.read_yaml(file_path2,0,Features.pwmOutputs('')))
print()
print(yamlData.read_yaml(file_path2,0,Features.binSel('')))
print()
print(yamlData.read_yaml(file_path2,0,Features.lPoti('')))
print()

"""


"""

    def update_yaml_backup(yaml_file, newYaml,mcu):
        
        #features = Features()
        mangledYaml = {}
        features_instance = Features(key='some_key')  # You need to provide a valid key here

        #print(features_instance.featureList(key='mcu'))

        def recursive_dict_traversal(dictionary):
            features = Features('')
            feature_list = {}

            for key, value in dictionary.items():
                
                if isinstance(value, dict):
                    # If the value is another dictionary, recursively call the function
                    #print(f"Entering dictionary at key: {key}")
                    print(key, "dict")
                    if(features.featureList(key)):
                        feature_list = features.featureList(key)
                    print(feature_list, "AAAAADAJASJASJASDASDADASDAAAAAA")
                    recursive_dict_traversal(value)
                    #print(f"Exiting dictionary at key: {key}")
                    
                if isinstance(value, list) and isinstance(value[0], dict):
                    # If the value is another dictionary, recursively call the function
                    #print(f"Entering array at key: {key}")
                    print(key, "list")
                    if(features.featureList(key)):
                        feature_list = features.featureList(key)
                    print(feature_list, "AAAAADAJASJASJASDASDADASDAAAAAA")
                    recursive_dict_traversal(value[0])

                    #print(f"Exiting array at key: {value}")
                else:
                    # If the value is not a dictionary, do something with it
                    if isinstance(value, dict):
                        pass
                    else:
                        print(key, value)
                        print(feature_list)
                        for i , key_no in enumerate(feature_list):
                            print(i, key_no)
                        

                        #print(f"At key: {key}, value: {value}")

        for i in range(mcu):
            recursive_dict_traversal(newYaml[i])




        
        for mcu_count in range(mcu+1):
            #print(newYaml[mcu_count]['mcu'])

            for para_count , key in enumerate(newYaml[mcu_count]['mcu']):
                #print(para_count, key,feature_config )
                if key in feature_config:
                    if isinstance(newYaml[mcu_count]['mcu'][key],dict):
                        print(key, newYaml[mcu_count]['mcu'][key],feature_config[key]['value'])
                        if key in feature_config[key]['value']:
                            mangledYaml[mcu_count]['mcu'][key] = feature_config[key]['value']
                            print(mangledYaml, "esad")
                    else: 
                        for secondkey in feature_config[key]:
                            print(secondkey, newYaml[mcu_count]['mcu'][key],feature_config[key][secondkey])

                #if key in newYaml[mcu_count]['mcu'][key]:
                #    print(newYaml[mcu_count]['mcu'][key])
                if newYaml[mcu_count]['mcu'][key] == feature_config[key]:
                    print("yes")

    

    #empty file

    with open(yaml_file, 'w'):
        pass
    #write each mcu
    for i in range(mcu+1):
        # Write the updated modified_data back to slave.yaml
        if i > 0:
            with open(yaml_file, 'a') as slave_file:
                slave_file.write("---\n")
        # Write the updated modified_data back to slave.yaml
        with open(yaml_file, 'a') as slave_file:
            yaml.dump(newYaml[i], slave_file, default_flow_style=None, sort_keys=False)

                """
