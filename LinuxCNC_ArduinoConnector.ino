/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com 2022

  This Software is used as IO Expansion for LinuxCNC.

  It is NOT intended for timing and security relevant IO's. Don't use it for Emergency Stops or Endstop switches!

  You can create as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
  You can also generate "virtual Pins" by using latching Potentiometers, which are connected to one analog Pin, but are read in Hal as individual Pins.

  Currently the Software Supports:
  - analog Inputs
  - latching Potentiometers
  - 1 binary encoded selector Switch
  - digital Inputs
  - digital Outputs
  - Matrix Keypad
  - Multiplexed LEDs
  - Quadrature encoders
  - Joysticks


  Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal. If the Signal is not received again, the Status LED will Flash.
  The Board will still work as usual and try to send it's data, so this feature is only to inform the User.


  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "firmware2.h"
#include <Arduino.h>

#define START_BYTE 0xAA
#define END_BYTE 0xFF
#define ESCAPE_BYTE 0x7D

byte messageBuffer[256];
int bufferIndex = 0;
bool inMessage = false;

void setup() {
  Serial.begin(9600);
}

void loop() {
  while (Serial.available()) {
    byte receivedByte = Serial.read();

    if (!inMessage) {
      if (receivedByte == START_BYTE) {
        bufferIndex = 0;
        inMessage = true;
      }
    } else {
      if (receivedByte == END_BYTE) {
        // End of message
        inMessage = false;
        processMessage(messageBuffer, bufferIndex);
      } else if (receivedByte == ESCAPE_BYTE) {
        // Read next byte and unescape it
        while (!Serial.available());
        byte escapedByte = Serial.read() ^ 0x20;
        messageBuffer[bufferIndex++] = escapedByte;
      } else {
        messageBuffer[bufferIndex++] = receivedByte;
      }
    }
  }
}

void processMessage(byte* message, int length) {
  // Validate checksum
  byte checksum = message[length - 1];
  byte calculatedChecksum = 0;
  for (int i = 0; i < length - 1; i++) {
    calculatedChecksum += message[i];
  }
  calculatedChecksum &= 0xFF;

  if (checksum == calculatedChecksum) {
    Serial.println("Valid Message Received");
    // Process message here
  } else {
    Serial.println("Invalid Checksum");
  }
}

