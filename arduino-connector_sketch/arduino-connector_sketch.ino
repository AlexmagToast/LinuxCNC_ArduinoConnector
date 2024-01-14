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
#include "Config.h"
#include "FeatureMap.h"
//#include <EEPROM.h>
//#include <FastCRC.h>
//#include <UUID.h>
#include "SerialConnection.h"


#define DEBUG

featureMap fm;
SerialConnection serialClient(SERIAL_RX_TIMEOUT, fm.features);
/*
const int UUID_LENGTH = 8;  // Length of the UUID string
const int EEPROM_PROVISIONING_ADDRESS = 0;  // starting address in EEPROM
const uint16_t EEPROM_HEADER = 0xbeef;
const uint8_t EEPROM_CONFIG_FORMAT_VERSION = 0x1; 
const uint32_t EEPROM_DEFAULT_SIZE = 1024; // Default size of EEPROM to initialize if EEPROM.length() reports zero. This occurs on chips such as the ESP8266 ESP-12F, and EEPROM space can be simulated in flash by EEPROM.begin(). See https://forum.arduino.cc/t/esp-eeprom-is-data-persistent-when-flashing-new-rev-or-new-program/1167637
const int SERIAL_RX_TIMEOUT = 5000;

uint64_t fm;
SerialConnection serialClient(SERIAL_RX_TIMEOUT, fm);

FastCRC16 CRC16;
UUID uuid;
const char * uid;

// Builtin LED proposal for errors/diagnostics
// General failure during setup, e.g., EEPROM failure or some other condition preventing normal startup.  Blink pattern: 250 ms on, 250 ms off (rapid blinking). 
// Success at startup.  Blink Pattern: solid LED (no blinking)

struct eepromData
{
  uint16_t header;
  char uid[UUID_LENGTH+1];
  uint8_t configVersion;
  uint16_t configLen; // Length of config data
  uint16_t configCRC; // CRC of config block
}epd;

*/

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize builtin LED for error feedback/diagnostics 

  Serial.begin(115200);
  //while (!Serial) {

 //   ; // wait for serial port to connect. Needed for native USB port only
  //}
  delay(SERIAL_STARTUP_DELAY);
  #ifdef DEBUG
    Serial.println("STARTING UP.. ");
  #endif
  /*
  if( EEPROM.length() == 0 )
  {
    #ifdef DEBUG
      Serial.println("EEPROM.length() reported zero bytes, setting to default of 1024 using .begin()..");
    #endif
    EEPROM.begin(EEPROM_DEFAULT_SIZE);
  }


  #ifdef DEBUG
    Serial.print("EEPROM length: ");
    Serial.println(EEPROM.length());
  #endif


  if( EEPROM.length() == 0 )
  {
      #ifdef DEBUG
        Serial.println("Error. EEPROM.length() reported zero bytes.");
      #endif
      while (true)
      {
        do_blink_sequence(1, 250, 250);
      }

  }

  EEPROM.get(EEPROM_PROVISIONING_ADDRESS, epd);

  if(epd.header != EEPROM_HEADER)
  {
    #ifdef DEBUG
      Serial.println("EEPROM HEADER MISSING, GENERATING NEW UID..");
    #endif

    uuid.generate();
    char u[9];

    // No reason to use all 37 characters of the generated UID. Instead,
    // we can just use the first 8 characters, which is unique enough to ensure
    // a user will likely never generate a duplicate UID unless they have thousands
    // of arduinos.
    memcpy( (void*)epd.uid, uuid.toCharArray(), 8);
    epd.uid[8] = 0;
    epd.header = EEPROM_HEADER;
    epd.configLen = 0;
    epd.configVersion = EEPROM_CONFIG_FORMAT_VERSION;
    epd.configCRC = 0;
      
    EEPROM.put(EEPROM_PROVISIONING_ADDRESS, epd);
    EEPROM.commit();

    serialClient.setUID(epd.uid);
    
    
    #ifdef DEBUG
    Serial.print("Wrote header value = 0x");
    Serial.print(epd.header, HEX);
    Serial.print(" to EEPROM and uid value = ");
    Serial.print((char*)epd.uid);
    Serial.print(" to EEPROM.");
    #endif
    
  }
  else
  {
    #ifdef DEBUG
      Serial.println("\nEEPROM HEADER DUMP");
      Serial.print("Head = 0x");
      Serial.println(epd.header, HEX);
      Serial.print("Config Version = 0x");
      Serial.println(epd.configVersion, HEX);
    #endif

    if(epd.configVersion != EEPROM_CONFIG_FORMAT_VERSION)
    {
      #ifdef DEBUG
        Serial.print("Error. Expected EEPROM Config Version: 0x");
        Serial.print(EEPROM_CONFIG_FORMAT_VERSION);
        Serial.print(", got: 0x");
        Serial.println(epd.configVersion);
      #endif

      while (true)
      {
        do_blink_sequence(1, 250, 250);
      // Loop forver as the expected config version does not match!
      }
    }
    #ifdef DEBUG
      Serial.print("Config Length = 0x");
      Serial.println(epd.configLen, HEX);
      Serial.print("Config CRC = 0x");
      Serial.println(epd.configCRC, HEX);
    #endif

    serialClient.setUID(epd.uid);
   
  }
  */
  serialClient.setUID("UNDEFINED");
  digitalWrite(LED_BUILTIN, LOW);// Signal startup success to builtin LED
  serialClient.DoWork(); // Causes
}

void loop() {
  serialClient.DoWork(); // Causes
}

// Causes builtin LED to blink in a defined sequence.
// blinkCount: Total number of blinks to perform in the sequence
// blinkPulsePeriod: Total amount of time to pulse LED HIGH
// blinkPulseInterval: Total amount of time between next pulse of LED to HIGH
void do_blink_sequence(int blinkCount, int blinkPulsePeriod, int blinkPulseInterval)
{
  for( int x = 0; x < blinkCount; x++ )
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(blinkPulsePeriod);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(blinkPulseInterval);
  }
}
