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
#file_path = '/home/alex/Documents/GitHub/LinuxCNC_ArduinoConnector/new_config.yaml'

# Reading YAML file
yaml_data = read_yaml(file_path)
#print(yaml_data)
#print(yaml_data['mcu'])
#print(yaml_data['mcu']['io_map']['analogInputs'])
#print(yaml_data['mcu']['io_map']['digitalInputs'])

class yamlData():

    def readMCU(file):
        yaml_data = read_yaml(file)
        mcu = yaml_data
        print(mcu)
        
  
        mcu_configurations = {  'alias':                {'standard_val': 'new Arduino', 'ignore': 0},
                                'component_name':       {'standard_val': 'arduino.', 'ignore': 0},
                                'dev':                  {'standard_val': '', 'ignore': 0},
                                'debug':                {
                                    'debug_level:':     {'standard_val': '0.', 'ignore': 0}},
                                'connection':           {
                                    'baudrate':         {'standard_val': '115200.', 'ignore': 0},
                                    'connection_serial2':{'standard_val': '0.', 'ignore': 0},
                                    'timeout':          {'standard_val': '5000', 'ignore': 0},
                                    'arduino_ip':       {'standard_val': '', 'ignore': 0},
                                    'arduino_port':     {'standard_val': '54321', 'ignore': 0},
                                    'listen_port':      {'standard_val': '54321', 'ignore': 0}},
                                'io_map':               {'standard_val': '', 'ignore': 1}
        }

    def readMCUS(file, MCU_no):
        def get_value(setting_key, supplied_dict):
            if setting_key in supplied_dict:
                return supplied_dict[setting_key]
            else:
                return mcu_configurations[setting_key]['value']

        yaml_data = read_yaml(file)
        mcu = yaml_data[MCU_no]['mcu']

        mcu_configurations = {
            'alias': {'value': 'new Arduino', 'ignore': 0, 'level': 0},
            'component_name': {'value': 'arduino.', 'ignore': 0, 'level': 0},
            'dev': {'value': '', 'ignore': 0, 'level': 0},
            'debug': {
                'debug_level': {'value': '0.', 'ignore': 0, 'level': 1}
            },
            'connection': {
                'baudrate': {'value': '115200.', 'ignore': 0, 'level': 1},
                'connection_serial2': {'value': '0.', 'ignore': 0, 'level': 1},
                'timeout': {'value': '5000', 'ignore': 0, 'level': 1},
                'arduino_ip': {'value': '', 'ignore': 0, 'level': 1},
                'arduino_port': {'value': '54321', 'ignore': 0, 'level': 1},
                'listen_port': {'value': '54321', 'ignore': 0, 'level': 1}
            },
            'io_map': {'value': '', 'ignore': 1, 'level': 0},
            'enabled': {'value': 'TRUE', 'ignore': 0, 'level': 0}

        }
        new_mcu_configuration =[] 
        # Loop through keys in mcu_configurations
        for key in mcu_configurations:
            try:
                if mcu_configurations[key]['ignore'] == 0 and key in mcu:
                    #new_mcu_configuration[key]['value'] = get_value(key, mcu)
                    print(key,get_value(key, mcu))
                elif mcu_configurations[key]['ignore'] == 0:
                    print(key, mcu_configurations[key]['value'])
                

            except: 
                for secondkey in mcu_configurations[key]:
                    if mcu_configurations[key][secondkey]['ignore'] == 0 and key in mcu:
                        #new_mcu_configuration[key][secondkey]['value'] = get_value(key, mcu)
                    
                        print(secondkey,get_value(key,secondkey, mcu))
                    elif mcu_configurations[key][secondkey]['ignore'] == 0:
                        print(secondkey,mcu_configurations[key][secondkey]['value'])
                    
                    
                    

            
        return new_mcu_configuration

out = yamlData.readMCUS(file_path,0)
#print(yamlData.readMCUS(file_path,1))
