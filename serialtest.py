import traceback
import serial
from cobs import cobs
import msgpack
import crc8

#connection = '/dev/ttyACM0'
connection = '/dev/cu.usbmodem3485187ABE842'
arduino = serial.Serial(connection, 115200, timeout=1, xonxoff=False, rtscts=False, dsrdtr=True)

recv_buffer = bytes()

while True:
    try:
        data = arduino.read()
        if data == b'\x00': # MsgPacketizer framed received
            recv_buffer += bytearray(data)
            strb = ''
            for b in bytes(recv_buffer):
                strb += f'[{hex(b)}]'
            print(strb) # Show the received bytes
            
            
            messageType = recv_buffer[1]

            data_bytes = recv_buffer[2:]
            
            crc = recv_buffer[-2]
            
            data_bytes = recv_buffer[2:]
            data_bytes = data_bytes[:-2]
            combined = (len(data_bytes)+1).to_bytes(1, 'big') + data_bytes 
            decoded = cobs.decode(combined) 

            hash = crc8.crc8()
            hash.update(decoded)
            if hash.digest() == crc.to_bytes(1,'big'):
                payload = msgpack.unpackb(data_bytes, use_list=True, raw=False)
                print(f'Decode success! Unpacked data: {payload}')
            else:
                print(f"Error. CRC validation failed for received message! Computed CRC = {hash.digest()}")
            recv_buffer = bytes() # reset the receive buffer for the next frame
        elif data == b'\n': # Debug Serial.println received
            recv_buffer += bytearray(data)
            print(bytes(recv_buffer).decode('utf8', errors='ignore'))
            recv_buffer = bytes()
        else:
            recv_buffer += bytearray(data) #Continue to add bytes to array
    except Exception as error:
        just_the_string = traceback.format_exc()
        print(just_the_string)