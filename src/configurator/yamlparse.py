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

# Example Usage
file_path = '/home/alex/Documents/GitHub/LinuxCNC_ArduinoConnector/example_config_nanoesp32_andesp8266.yaml'
file_path2 = '/home/alex/Documents/GitHub/LinuxCNC_ArduinoConnector/new_config.yaml'

# Reading YAML file
yaml_data = read_yaml(file_path)
#print(yaml_data)
#print(yaml_data['mcu'])
#print(yaml_data['mcu']['io_map']['analogInputs'])
#print(yaml_data['mcu']['io_map']['digitalInputs'])

class yamlData():

    def readMCU(file, MCU_no):
        
        yaml_data = read_yaml(file)
        mcu = yaml_data[MCU_no]['mcu']

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

        }
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
    
    def readAnalogInputs(file,MCU_no):
        yaml_data = read_yaml(file)
        AIn = yaml_data[MCU_no]['mcu']['io_map']

        AIn_configuration = {
            'pin_id': {'value': '', 'ignore': 1, 'optional':0},
            'pin_name': {'value': 'ain.', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},    
            'pin_smoothing': {'value': '200', 'ignore': 0, 'optional':1},
            'pin_min_val': {'value': '0', 'ignore': 0, 'optional':1},
            'pin_max_val': {'value': '1023', 'ignore': 0, 'optional':1},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
        }

        new_AIn_configuration ={}

        if 'analogInputs' in AIn:
            new_AIn_configuration={}
            AIn = AIn['analogInputs']
            
            for pin_count, pin_no in enumerate(AIn):
    
                new_AIn_configuration[pin_no['pin_id']]= {}
                
                for key  in AIn_configuration:
                    
                    #print(AIn[pin_count]['pin_id'],parameter+1, key)
                    if AIn_configuration[key]['ignore'] == 0 and key in AIn[pin_count]:
                        #print(AIn[pin_count][key])
                        new_AIn_configuration[pin_no['pin_id']][key] = AIn[pin_count][key]
                    else:
                        #print(AIn_configuration[key]['value'])
                        if AIn_configuration[key]['ignore'] == 0:
                            new_AIn_configuration[pin_no['pin_id']][key] = AIn_configuration[key]['value']
                
        return new_AIn_configuration
    

    def readDigitalInputs(file,MCU_no):
        yaml_data = read_yaml(file)
        DIn = yaml_data[MCU_no]['mcu']['io_map']

        DIn_configuration = {
            'pin_id': {'value': '', 'ignore': 1, 'optional':0},
            'pin_name': {'value': 'Din.', 'ignore': 0, 'optional':1},
            'pin_mode': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},    
            'pin_debounce': {'value': '200', 'ignore': 0, 'optional':1}
        }

        new_DIn_configuration ={}

        if 'analogInputs' in DIn:
            new_DIn_configuration={}
            DIn = DIn['analogInputs']
            
            for pin_count, pin_no in enumerate(DIn):
    
                new_DIn_configuration[pin_no['pin_id']]= {}
                
                for key  in DIn_configuration:
                    
                    #print(DIn[pin_count]['pin_id'],parameter+1, key)
                    if DIn_configuration[key]['ignore'] == 0 and key in DIn[pin_count]:
                        #print(DIn[pin_count][key])
                        new_DIn_configuration[pin_no['pin_id']][key] = DIn[pin_count][key]
                    else:
                        #print(DIn_configuration[key]['value'])
                        if DIn_configuration[key]['ignore'] == 0:
                            new_DIn_configuration[pin_no['pin_id']][key] = DIn_configuration[key]['value']
                
        return new_DIn_configuration
    
    def readPins(file,MCU_no, pin_map):
        yaml_data = read_yaml(file)
        Feature = yaml_data[MCU_no]['mcu']['io_map']

        
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
    
    def update_slave_yaml(source_File, newYaml):
        
                
        def count_yaml_documents(file_path):
            with open(file_path, 'r') as file:
                content = file.read()

            # Split the content based on '---' to identify individual YAML documents
            yaml_documents = content.split('---')

            # Filter out empty documents (resulting from leading/trailing '---')
            yaml_documents = [doc.strip() for doc in yaml_documents if doc.strip()]

            return len(yaml_documents)
        

        source_data = read_yaml(source_File)
        modified_data = read_yaml(newYaml)
        
        with open("/home/alex/Documents/GitHub/LinuxCNC_ArduinoConnector/test.yaml", 'w'):
            pass
        for mcu in range(count_yaml_documents(newYaml)):
            print(source_data[mcu])    

            # Update modified_data based on source_data
            updated_modified_data = yamlData.update_yaml_recursive(source_data, modified_data)

            # Write the updated modified_data back to slave.yaml
            with open("/home/alex/Documents/GitHub/LinuxCNC_ArduinoConnector/test.yaml", 'a') as slave_file:
                yaml.dump(updated_modified_data, slave_file, default_flow_style=None, sort_keys=False)
        
    def update_yaml_recursive(source_data, modified_data):
        #print(source_data, modified_data)
        if isinstance(source_data, dict) and isinstance(modified_data, dict):
            updated_dict = {}
            for key, master_value in source_data.items():
                print(key, master_value)
                if key in modified_data:
                    # Recursively update nested dictionaries
                    updated_dict[key] = yamlData.update_yaml_recursive(master_value, modified_data[key])
                else:
                    # Key doesn't exist in modified_data, skip it
                    updated_dict[key] = master_value

            # Remove extra keys from modified_data
            for key in list(modified_data.keys()):
                print(key)
                if key not in source_data:
                    del modified_data[key]

            return updated_dict

        elif isinstance(source_data, list) and isinstance(modified_data, list):
            # Update each element in the list recursively
            return [yamlData.update_yaml_recursive(master_item, slave_item) for master_item, slave_item in zip(source_data, modified_data)]

        else:
            # Use the value from source_data
            return source_data
        

slave_yaml_path = '/home/alex/Documents/GitHub/LinuxCNC_ArduinoConnector/new_config copy.yaml'
source_yaml_path = file_path2
yamlData.update_slave_yaml(source_yaml_path, slave_yaml_path)

analogPins = {
            'analogInputs':{
            'pin_id': {'value': '', 'ignore': 1, 'optional':0},
            'pin_name': {'value': 'ain.', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},    
            'pin_smoothing': {'value': 200, 'ignore': 0, 'optional':1},
            'pin_min_val': {'value': 0, 'ignore': 0, 'optional':1},
            'pin_max_val': {'value': 1023, 'ignore': 0, 'optional':1},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'optional':1}
             }
}
DIn_configuration = {
            'digitalInputs':{
            'pin_id': {'value': '', 'ignore': 1, 'optional':0}, 
            'pin_name': {'value': 'Din.', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},    
            'pin_debounce': {'value': 200, 'ignore': 0, 'optional':1}
            }
}

pwmOutputs = {
            'pwmOutputs':{
            'pin_id': {'value': '', 'ignore': 1, 'optional':0}, 
            'pin_name': {'value': 'pwmout.', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},   
            'pin_init_state': {'value': 0, 'ignore': 0, 'optional':1},   
            }
}

digitalOutput = {
            'digitalOutput':{
            'pin_id': {'value': '', 'ignore': 1, 'optional':0}, 
            'pin_name': {'value': 'din.', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_BIT', 'ignore': 0, 'optional':1},   
            'pin_ondisconnected_state': {'value': '-1', 'ignore': 0, 'optional':1},
            'pin_onconnected_state': {'value': '-1', 'ignore': 0, 'optional':1},
            }
}

lPoti =     {'lPoti': {
            'pin_id': {'value': '', 'ignore': 1, 'optional':0}, 
            'pin_name': {'value': 'lpoti.', 'ignore': 0, 'optional':1},
            'pin_type': {'value': 'HAL_S32', 'ignore': 0, 'optional':1},   
            'lpoti_latches': {'value': 9, 'ignore': 0, 'optional':1},
            'value_replace': {'value': [40,50,60,70,80,90,100,110,120], 'ignore': 0, 'optional':1},
            }
}

        
binarySelectorSwitch = {
        'binarySelectorSwitch':{
        'pin_id': {'value': '', 'ignore': 1, 'optional':0}, 
        'pin_name': {'value': 'binSel.', 'ignore': 0, 'optional':1},
        'pin_type': {'value': 'HAL_FLOAT', 'ignore': 0, 'optional':1},   
        'pin_pins': {'value': [2,6,4,3,5], 'ignore': 0, 'optional':1},
        'value_replace': {'value': [180,190,200,0,0,0,0,0,0,0,0,0,0,0,0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170], 'ignore': 0, 'optional':1},   
        }
        
}
"""
    quadratureEncoder:
      - pin_id: 0
        pin_name: quadEnc  #hal pin name       #optional
        encoder_mode: counter #updown #vpins
        encoder_A_pin: 2  #?
        encoder_B_pin: 3  #?
        encoder_pins: [2,3] #?
        encoder_steps: 4  #electrical pulses per latch
        #new feature: create list of Virtual Pins, only one is ever selected/ HIGH, by rotation select next / previous one. In this config a simple encoder is used to select scale of an MPG. (Standard 0.1mm)
        encoder_virtualpins: 3 #create 10 virtual Pins
        encoder_virtualpin_start: 2 #select pin 3 at program start
        pin_type: HAL_FLOAT #hal pin type     #optional   HAL_BIT, HAL_S32 , HAL_FLOAT
        value_replace: [0.001,0.01,0.1,1]

    joystick:
      - pin_id: 2
        pin_name: joystick  #hal pin name       #optional
        joystick_center: 512  
        joystick_deadband: 20 #ignore values around joystick_center + - deadband 
        joystick_scaling: 0.01

    statusled:
      - pin_id: 13
        useDled: 1 #use digital LED instead of Output 13 #weird setting?
"""

#print(yamlData.readMCU(file_path,0))
#print(yamlData.readMCU(file_path,1))

#print(yamlData.readAnalogInputs(file_path2,0))
#print(yamlData.readAnalogInputs(file_path,1))

#print(yamlData.readDigitalInputs(file_path2,0))
"""print(yamlData.readPins(file_path2,0,analogPins))
print()
print(yamlData.readPins(file_path2,0,DIn_configuration))  
print()
print(yamlData.readPins(file_path2,0,pwmOutputs))
print()
print(yamlData.readPins(file_path2,0,digitalOutput))
print()
print(yamlData.readPins(file_path2,0,lPoti))
print()
print(yamlData.readPins(file_path2,0,binarySelectorSwitch))
print()
"""
