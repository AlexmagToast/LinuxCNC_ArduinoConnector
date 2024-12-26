/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com 2022

  This Software is used as IO Expansion for LinuxCNC.
*/
#include "firmware1.h"
#include <Arduino.h>
#include <stdint.h>

void communicate(int firmwareID, bool inputStates[]) {
  const byte startByte = 0xAA;
  const byte endByte = 0xFF;
  byte buffer[1024];
  int bufferIndex = 0;
  byte id_bytes[4];
  byte binary_payload[InBinaryLength];

  // [start_byte]
  //       [id_bytes]
  //       [binary_length][binary_payload]
  //       [int16_length][int16_payload]
  //       [uint16_length][uint16_payload]
  //       [int32_length][int32_payload]
  //       [uint32_length][uint32_payload]
  //       [float_length][float_payload]
  //       [char_length][char_payload]
  //       [string_length][string_payload]
  //       [checksum]
  //   [end_byte]
}

const int bufferSize = 256; // Adjust the buffer size as needed
byte buffer[bufferSize];
int bufferIndex = 0;
bool inMessage = false;
const byte startByte = 0xAA;
const byte endByte = 0xFF;
byte testBitstream[] = {
  0xAA, 0x37, 0x1E, 0xC5, 0x9C, 0x59, 0x00, 0xDD, 0xED, 0x6E, 0x77, 0xBB, 0xDB, 0xDD, 0xEE, 0x76, 0xB7, 0xBB, 0x01, 0x02, 0x00, 0x00, 0x80, 0xFF, 0x7F, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0xFF, 0x7F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x00, 0xC3, 0xF5, 0x48, 0x40, 0x28, 0x0A, 0x34, 0x40, 0x02, 0x00, 0x41, 0x5A, 0x02, 0x00, 0x05, 0x00, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x05, 0x00, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x91, 0xFF
  };

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // For debugging: process the test bitstream
  Serial.println(" New Stuff: ");
  processMessage(testBitstream, sizeof(testBitstream));
}

void loop() {
  if (Serial.available() > 0) {
    byte inByte = Serial.read();
    if (inByte == startByte) {
      if (Serial.available() >= 2) {
        digitalWrite(LED_BUILTIN, LOW);  // turn the LED on (HIGH is the voltage level)
        uint16_t receivedFirmwareID = Serial.read() | (Serial.read() << 8);
        //Serial.println("receivedFirmwareID");
        if (receivedFirmwareID == (FIRMWAREID & 0xFFFF)) { // Compare with lower 16 bits of FIRMWAREID
          inMessage = true;
          bufferIndex = 0;
          buffer[bufferIndex++] = startByte;
          buffer[bufferIndex++] = lowByte(receivedFirmwareID);
          buffer[bufferIndex++] = highByte(receivedFirmwareID);
        }
      }
    }

    if (inMessage) {
      buffer[bufferIndex++] = inByte;
      if (bufferIndex >= bufferSize) {
        // Handle buffer overflow if necessary
        bufferIndex = bufferSize - 1;
      }
      if (inByte == endByte) {
        inMessage = false;
        // Process the complete message stored in buffer
        processMessage(buffer, bufferIndex);
        bufferIndex = 0;
      }
    }
  }
}

void processMessage(byte buffer[], int length) {
  int index = 0;
  int bitIndex = 0;
  if (buffer[index++] != startByte) return; // Invalid start byte, abort

  uint32_t firmwareID = buffer[index++] | (buffer[index++] << 8) | (buffer[index++] << 16) | (buffer[index++] << 24);

  if (firmwareID != (FIRMWAREID & 0xFFFFFFFF)){
    Serial.print("wrong FirmwareID, ABORT");
    //return; // Invalid firmware ID, abort
  }

  // Read binary payload length as uint16_t
  uint16_t SerialOutBinaryLength = buffer[index++] | (buffer[index++] << 8);

  // Read binary payload as bool
  if (SerialOutBinaryLength != OutBinaryLength){
    Serial.println("binary length no match!");
    Serial.print(SerialOutBinaryLength);
    Serial.print(" ");
    Serial.println(OutBinaryLength);
  }
  bitIndex = 0;

  for (int i = 0; i < SerialOutBinaryLength; i++) {
    bool done = 0;
    byte currentByte = buffer[index++];
    for (int j = 0; j < 8; j++) {
      if (bitIndex < OutBinaryLength) {
        OutBinaryValues[bitIndex++] = (currentByte >> j) & 0x01;
      } else {
        done = 1;
        // Prevent out-of-bounds access
        break;
      }
    }
    if(done){
      break;
    }
  }


  // Read int16 payload
  uint16_t int16Length = buffer[index++] | (buffer[index++] << 8);
  for (int i = 0; i < OutInt16Length; i++) {
    OutInt16Values[i] = buffer[index++] | (buffer[index++] << 8);
  }


  // Read uint16 payload
  uint16_t uint16Length = buffer[index++] | (buffer[index++] << 8);
  Serial.print("uint16 length: ");
  Serial.println(int16Length);
  Serial.print("uint16 Values: ");
  for (int i = 0; i < uint16Length; i++) {
    OutUint16Values[i] = buffer[index++] | (buffer[index++] << 8);
  }

  // Read int32 payload
  uint16_t int32Length = buffer[index++] | (buffer[index++] << 8);
  Serial.print("int32 length: ");
  Serial.println(int32Length);
  Serial.print("uint32 Values: ");
  for (int i = 0; i < int32Length; i++) {
    OutInt32Values[i] = buffer[index++] | (buffer[index++] << 8) | (buffer[index++] << 16) | (buffer[index++] << 24);
  }

  // Read uint32 payload
  uint16_t uint32Length = buffer[index++] | (buffer[index++] << 8);
  uint32_t OutUint32Values[uint32Length];
  for (int i = 0; i < uint32Length; i++) {
    OutUint32Values[i] = buffer[index++] | (buffer[index++] << 8) | (buffer[index++] << 16) | (buffer[index++] << 24);
  }

  // Read float payload
  uint16_t floatLength = buffer[index++] | (buffer[index++] << 8);
  float OutFloatValues[floatLength];
  for (int i = 0; i < floatLength; i++) {
    uint32_t temp = buffer[index++] | (buffer[index++] << 8) | (buffer[index++] << 16) | (buffer[index++] << 24);
    OutFloatValues[i] = *((float*)&temp);
  }

  // Read char payload
  uint16_t charLength = buffer[index++] | (buffer[index++] << 8);
  char OutCharValues[charLength];
  for (int i = 0; i < charLength; i++) {
    OutCharValues[i] = buffer[index++];
  }

  // Read string payload

  // Read the number of strings
  uint16_t numberOfStrings = buffer[index++] | (buffer[index++] << 8);

  // Loop through each string
  for (int strIndex = 0; strIndex < numberOfStrings; strIndex++) {
    // Read the length of the current string
    uint16_t stringLength = buffer[index++] | (buffer[index++] << 8);


    // Allocate memory for the string (including null terminator)
    OutStringValues[strIndex] = new char[stringLength + 1];

    // Read the string characters
    for (int i = 0; i < stringLength; i++) {
      OutStringValues[strIndex][i] = buffer[index++];
    }

    // Null terminate the string
    OutStringValues[strIndex][stringLength] = '\0';

  }

  // Read checksum
  byte checksum = buffer[index++];

  // Validate checksum
  byte calculatedChecksum = 0;
  for (int i = 1; i < index - 1; i++) { // Exclude start byte and end byte
    calculatedChecksum += buffer[i];
  }
  calculatedChecksum %= 256;

  if (calculatedChecksum != checksum) {
    Invalid checksum
  
    return;
  }

  // Print the received message
  
  Serial.println("Received message:");
  Serial.print("Firmware ID: ");
  Serial.println(firmwareID, DEC);
  Serial.print("Binary Payload: ");
  for (int i = 0; i < OutBinaryLength; i++) {
    Serial.print(OutBinaryValues[i], HEX);
    Serial.print("");
  }
  Serial.println();
  Serial.print("Int16 Payload: ");
  for (int i = 0; i < int16Length; i++) {
    Serial.print(OutInt16Values[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Uint16 Payload: ");
  for (int i = 0; i < uint16Length; i++) {
    Serial.print(OutUint16Values[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Int32 Payload: ");
  for (int i = 0; i < int32Length; i++) {
    Serial.print(OutInt32Values[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Uint32 Payload: ");
  for (int i = 0; i < uint32Length; i++) {
    Serial.print(OutUint32Values[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Float Payload: ");
  for (int i = 0; i < floatLength; i++) {
    Serial.print(OutFloatValues[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Char length: ");
  Serial.println(charLength);
  Serial.print("Char Payload: ");
  for (int i = 0; i < charLength; i++) {
    Serial.print(OutCharValues[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Extracted Strings:");
  Serial.println(numberOfStrings);
  for (int strIndex = 0; strIndex < numberOfStrings; strIndex++) {
    Serial.print("Extracted String ");
    Serial.print(strIndex);
    Serial.print(": ");
    Serial.println(OutStringValues[strIndex]);
  }
  Serial.print("Checksum: ");
  Serial.println(checksum);  
}