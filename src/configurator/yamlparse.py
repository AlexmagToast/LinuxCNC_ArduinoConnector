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

print(yamlData.readMCU(file_path,0))
print(yamlData.readMCU(file_path,1))

print(yamlData.readAnalogInputs(file_path2,0))
#print(yamlData.readAnalogInputs(file_path,1))
