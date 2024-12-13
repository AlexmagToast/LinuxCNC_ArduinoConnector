import serial, struct, time, yaml

# Initialize serial communication
ser = serial.Serial(port='COM5', baudrate=115200, timeout=1)

def load_config(file_path):
    with open(file_path, 'r') as file:
        configs = list(yaml.safe_load_all(file))
    return configs
debug = True

def calculate_checksum(data):
    """
    Calculate checksum as the sum of all bytes modulo 256.
    """
    return sum(data) % 256

def send_message(binary_pins, integer_pins, float_pins):
    """
    Send a grouped binary message to the Arduino.
    Groups binary, integer, and float pins for efficient communication.

    Mesaage format:
    [Start marker] [Section 1: Binary pins] [Section 2: Integer pins] [Section 3: Float pins] [Checksum] [End marker]

    """
    message = bytearray()

    # Start marker
    message.append(0xAA)

    # Section 1: Binary pins
    
    message.append(0)  # Type: binary
    if (binary_pins is None):
        message.append(0)  # Number of binary pins
    else:
        message.append(len(binary_pins))  # Number of binary pins
        packed_binary = 0
        for i, state in enumerate(binary_pins):
            packed_binary |= (state << (i % 8))
            if (i + 1) % 8 == 0 or i == len(binary_pins) - 1:
                message.append(packed_binary)
                packed_binary = 0

    # Section 2: Integer pins
    message.append(1)  # Type: integer
    if (integer_pins is None):
        message.append(0)
    else:
        message.append(len(integer_pins))  # Number of integer pins
    for value in integer_pins:
        message += struct.pack('<h', value)  # 2-byte little-endian integer

    # Section 3: Float pins
    message.append(2)  # Type: float
    message.append(len(float_pins))  # Number of float pins
    for value in float_pins:
        message += struct.pack('<f', value)  # 4-byte little-endian float

    # Compute and append checksum
    checksum = calculate_checksum(message[1:])  # Exclude the start marker
    message.append(checksum)

    # End marker
    message.append(0xBB)

    # Send the message
    print("Sending:", message)
    ser.write(message)

def read_message():
    """
    Read and parse a binary message from the Arduino.
    """
    message = ser.read_until(b'\xBB')  # Read until the end marker (0xBB)
    if not message or message[0] != 0xAA:  # Validate start marker
        print("Invalid message start")
        print(message)
        return None

    payload = message[1:-2]  # Exclude start marker and checksum/end marker
    checksum = message[-2]  # Second-to-last byte is the checksum
    if calculate_checksum(payload) != checksum:
        print("Checksum error")
        return None

    pos = 0
    pins = {"binary": [], "integer": {}, "float": {}}  # Organize pins by type

    while pos < len(payload):
        pin_type = payload[pos]
        pos += 1
        count = payload[pos]
        pos += 1

        if pin_type == 0:  # Binary pins
            bit_data = payload[pos:pos + ((count + 7) // 8)]
            pos += (count + 7) // 8
            for i in range(count):
                pins["binary"].append((bit_data[i // 8] >> (i % 8)) & 1)

        elif pin_type == 1:  # Integer pins
            for _ in range(count):
                pin_id = payload[pos]
                pos += 1
                value = struct.unpack('<h', payload[pos:pos + 2])[0]
                pos += 2
                pins["integer"][pin_id] = value

        elif pin_type == 2:  # Float pins
            for _ in range(count):
                pin_id = payload[pos]
                pos += 1
                value = struct.unpack('<f', payload[pos:pos + 4])[0]
                pos += 4
                pins["float"][pin_id] = value

    return pins

def get_firmware_version():
    ser.write(b'V')  # Send 'V' command to request firmware version
    time.sleep(0.1)  # Wait for the Arduino to respond
    version = ser.readline().decode().strip()
    return version

def compare_firmware_version(expected_version):
    arduino_version = get_firmware_version()
    if arduino_version == expected_version:
        print("Firmware version matches:", arduino_version)
        return True
    else:
        print("Firmware version mismatch. Expected:", expected_version, "Got:", arduino_version)
        return False
# Main loop

configs = load_config('config.yaml')
if compare_firmware_version(configs[0]['firmware_version']):
while True:
    send_message([1, 0, 1, 1, 0, 0, 1, 1],[12,4,12,22,55],[2.34,1331.24])  # Send pin states to Arduino
    time.sleep(0.001)  # Adjust delay as needed
    response = read_message()  # Read response from Arduino
    if response:
        print("Received:", response)
