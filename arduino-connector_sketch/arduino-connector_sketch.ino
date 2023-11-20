/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com 2022

  MIT License
  Copyright (c) 2023 Alexander Richter

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#include <Arduino.h>
#include <EEPROM.h>
#include <UUID.h>
UUID uuid;


const int UUID_LENGTH = 8;  // Length of the UUID string
const int EEPROM_PROVISIONING_ADDRESS = 0;  // starting address in EEPROM
const uint16_t EEPROM_HEADER = 0xbeef;

struct eepromData
{
  uint16_t header = EEPROM_HEADER;
  uint8_t uid[UUID_LENGTH+1];
}epd;

void setup() {
  Serial.begin(115200);
  while (!Serial) {

    ; // wait for serial port to connect. Needed for native USB port only
  }
   
    Serial.print("EEPROM length: ");

    Serial.println(EEPROM.length());

    //Print the result of calling eeprom_crc()

    Serial.print("CRC32 of EEPROM data: 0x");

    Serial.print(eeprom_crc(), HEX);

    EEPROM.get(EEPROM_PROVISIONING_ADDRESS, epd);
    
    if(epd.header != EEPROM_HEADER)
    {
      Serial.println("EEPROM HEADER MISSING, GENERATING NEW HEADER AND UID..");

      uuid.generate();
      char u[9];

      // No reason to use all 37 characters of the generated UID. Instead,
      // we can just use the first 8 characters, which is unique enough to ensure
      // a user will likely never generate a duplicate UID unless they have thousands
      // of arduinos.
      memcpy( (void*)epd.uid, uuid.toCharArray(), 8);
      epd.uid[8] = 0;
      epd.header = EEPROM_HEADER;
      Serial.print("Writing header value = 0x");
      Serial.print(epd.header, HEX);
      Serial.print(" to EEPROM and uid value = ");
      Serial.print((char*)epd.uid);
      Serial.print(" to EEPROM...");

      EEPROM.put(EEPROM_PROVISIONING_ADDRESS, epd);
      Serial.print("SUCCESS!");
    }
    else
    {
      Serial.println("\nEEPROM HEADER DUMP");
      Serial.print("Head = 0x");
      Serial.println(epd.header, HEX);
      Serial.print("UID = ");
      Serial.println((char*)epd.uid);
    }


    //readUUID[UUID_LENGTH] = '\0';  // Null terminate the string

   // Serial.print("UUID read from EEPROM: ");
   // Serial.println(readUUID);
    
}

void loop() {


  
 // Serial.println((char*)u);
  delay(1000);
}

unsigned long eeprom_crc(void) {

  const unsigned long crc_table[16] = {

    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,

    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,

    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,

    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c

  };

  unsigned long crc = ~0L;

  for (int index = 0 ; index < EEPROM.length()  ; ++index) {

    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);

    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);

    crc = ~crc;

  }

  return crc;
}