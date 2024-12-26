/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com 2022

  This Software is used as IO Expansion for LinuxCNC.
*/
#include "firmware0.h"
#include <Arduino.h>


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
  0xAA, 0xF9, 0x56, 0xDA, 0x12, 0xED, 0x6E, 0x77, 0xBB, 0xDB, 0xDD, 0xEE, 0x76, 0xB7, 0xBB, 0x01, 0x02, 0x00, 0x00, 0x80, 0xFF, 0x7F, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0xFF, 0x7F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x00, 0xC3, 0xF5, 0x48, 0x40, 0x28, 0x0A, 0x34, 0x40, 0x02, 0x00, 0x41, 0x5A, 0x0E, 0x00, 0x05, 0x00, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x05, 0x00, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x22, 0xFF
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // For debugging: process the test bitstream
  Serial.println(" ");
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
  if (buffer[index++] != startByte) return; // Invalid start byte, abort

  uint32_t firmwareID = buffer[index++] | (buffer[index++] << 8) | (buffer[index++] << 16) | (buffer[index++] << 24);

  if (firmwareID != (FIRMWAREID & 0xFFFFFFFF)){
    return; // Invalid firmware ID, abort
  }

  // Read binary payload
  byte SerialOutBinaryLength = buffer[index++];
  if (SerialOutBinaryLength != OutBinaryLength){
    Serial.println("binary length no match!");
    Serial.print(SerialOutBinaryLength);
    Serial.print(" ");
    Serial.println(OutBinaryLength);
  }
  int bitIndex = 0;

   for (int i = 0; i < SerialOutBinaryLength; i++) {
    byte currentByte = buffer[index++];
    Serial.print(currentByte);
    for (int j = 0; j < 8; j++) {
      if (bitIndex < OutBinaryLength) {
        OutBinaryValues[bitIndex++] = (currentByte >> j) & 0x01;
      } else {
        // Prevent out-of-bounds access
        break;
      }
    }
  }

  Serial.print("Binary Payload: ");
  for (int i = 0; i < OutBinaryLength; i++) {
    Serial.print(OutBinaryValues[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Read int16 payload
  byte int16Length = buffer[index++];
  int16_t int16Payload[int16Length];
  for (int i = 0; i < int16Length; i++) {
    int16Payload[i] = buffer[index++] | (buffer[index++] << 8);
  }

  // Read uint16 payload
  byte uint16Length = buffer[index++];
  uint16_t uint16Payload[uint16Length];
  for (int i = 0; i < uint16Length; i++) {
    uint16Payload[i] = buffer[index++] | (buffer[index++] << 8);
  }

  // Read int32 payload
  byte int32Length = buffer[index++];
  int32_t int32Payload[int32Length];
  for (int i = 0; i < int32Length; i++) {
    int32Payload[i] = buffer[index++] | (buffer[index++] << 8) | (buffer[index++] << 16) | (buffer[index++] << 24);
  }

  // Read uint32 payload
  byte uint32Length = buffer[index++];
  uint32_t uint32Payload[uint32Length];
  for (int i = 0; i < uint32Length; i++) {
    uint32Payload[i] = buffer[index++] | (buffer[index++] << 8) | (buffer[index++] << 16) | (buffer[index++] << 24);
  }

  // Read float payload
  byte floatLength = buffer[index++];
  float floatPayload[floatLength];
  for (int i = 0; i < floatLength; i++) {
    uint32_t temp = buffer[index++] | (buffer[index++] << 8) | (buffer[index++] << 16) | (buffer[index++] << 24);
    floatPayload[i] = *((float*)&temp);
  }

  // Read char payload
  byte charLength = buffer[index++];
  char charPayload[charLength];
  for (int i = 0; i < charLength; i++) {
    charPayload[i] = buffer[index++];
  }

  // Read string payload
  byte stringLength = buffer[index++];
  char stringPayload[stringLength];
  for (int i = 0; i < stringLength; i++) {
    stringPayload[i] = buffer[index++];
  }

  // Read checksum
  byte checksum = buffer[index++];

  // Validate checksum
  byte calculatedChecksum = 0;
  for (int i = 0; i < length - 1; i++) {
    calculatedChecksum += buffer[i];
  }
  calculatedChecksum %= 256;

  /*if (calculatedChecksum != checksum) {
    // Invalid checksum
    return;
  }*/

  // Send the complete message back over serial
  //Serial.write(buffer, length);

  // Print the received message
  Serial.println("Received message:");
  Serial.print("Firmware ID: ");
  Serial.println(firmwareID, DEC);
  Serial.print("Binary Payload: ");
  for (int i = 0; i < OutBinaryLength; i++) {
    Serial.print(OutBinaryValues[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Int16 Payload: ");
  for (int i = 0; i < int16Length; i++) {
    Serial.print(int16Payload[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Uint16 Payload: ");
  for (int i = 0; i < uint16Length; i++) {
    Serial.print(uint16Payload[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Int32 Payload: ");
  for (int i = 0; i < int32Length; i++) {
    Serial.print(int32Payload[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Uint32 Payload: ");
  for (int i = 0; i < uint32Length; i++) {
    Serial.print(uint32Payload[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Float Payload: ");
  for (int i = 0; i < floatLength; i++) {
    Serial.print(floatPayload[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Char Payload: ");
  for (int i = 0; i < charLength; i++) {
    Serial.print(charPayload[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("String Payload: ");
  Serial.println(stringPayload);
  Serial.print("Checksum: ");
  Serial.println(checksum, HEX);

  // Send the complete message back over serial
  Serial.write(buffer, length);
}