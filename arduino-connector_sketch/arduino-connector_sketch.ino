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
#include <ArduinoJson.h>
#include "FeatureMap.h"
//#include <EEPROM.h>
//#include <FastCRC.h>
//#include <UUID.h>
#include "SerialConnection.h"
#include <Array.h>

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

/*

{
  "DIGITAL_INPUTS": {
    "din.3": {
      "pinName": "din.3",
      "pinType": "DIGITAL_INPUT",
      "pinID": 3,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_IN"
    },
    "din.4": {
      "pinName": "din.4",
      "pinType": "DIGITAL_INPUT",
      "pinID": 4,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_IN"
    },
    "din.5": {
      "pinName": "din.5",
      "pinType": "DIGITAL_INPUT",
      "pinID": 5,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_IN"
    },
    "din.6": {
      "pinName": "din.6",
      "pinType": "DIGITAL_INPUT",
      "pinID": 6,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_IN"
    },
    "din.7": {
      "pinName": "din.7",
      "pinType": "DIGITAL_INPUT",
      "pinID": 7,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_IN"
    },
    "din.8": {
      "pinName": "din.8",
      "pinType": "DIGITAL_INPUT",
      "pinID": 8,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_IN"
    }
  },
  "DIGITAL_OUTPUTS": {
    "dout.9": {
      "pinName": "dout.9",
      "pinType": "DIGITAL_OUTPUT",
      "pinID": 9,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_OUT"
    },
    "dout.10": {
      "pinName": "dout.10",
      "pinType": "DIGITAL_OUTPUT",
      "pinID": 10,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_OUT"
    }
  }
}

*/

#if defined(DINPUTS) || defined(DOUTPUTS)
struct dpin
{
    String pinName;
    String pinType;
    String pinID;
    int8_t pinInitialState;
    int8_t pinConnectState;
    int8_t pinDisconnectState;
    String halPinDirection;
    int8_t pinCurrentState;
};
const int ELEMENT_COUNT_MAX = 30;
typedef Array<dpin,ELEMENT_COUNT_MAX> dpin_array_t;
#endif

#ifdef DINPUTS
dpin_array_t dinput_arr;
#endif

#ifdef DOUTPUTS
dpin_array_t doutput_arr;
#endif

void onConfig(const char* conf) {
    #ifdef DEBUG
    Serial.println("ON CONFIG!");
    #endif
    #ifdef DINPUTS 
      //dinput_vect.clear();
      StaticJsonDocument<200> filter;
      filter["DIGITAL_INPUTS"] = true;
      StaticJsonDocument<400> doc;
      DeserializationError error = deserializeJson(doc, conf, DeserializationOption::Filter(filter));
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      for (JsonPair DIGITAL_INPUTS_item : doc["DIGITAL_INPUTS"].as<JsonObject>()) {
      dpin d = (dpin){.pinName = DIGITAL_INPUTS_item.value()["pinName"],
        .pinType = DIGITAL_INPUTS_item.value()["pinType"],
        .pinID =  DIGITAL_INPUTS_item.value()["pinID"],
        .pinInitialState =  DIGITAL_INPUTS_item.value()["pinInitialState"],
        .pinConnectState = DIGITAL_INPUTS_item.value()["pinConnectState"],
        .pinDisconnectState = DIGITAL_INPUTS_item.value()["pinDisconnectState"],
        .halPinDirection = DIGITAL_INPUTS_item.value()["halPinDirection"]
      };
      dinput_arr.push_back(d);
    #endif
    #ifdef DOUTPUTS
      for (JsonPair DIGITAL_OUTPUTS_item : doc["DIGITAL_OUTPUTS"].as<JsonObject>()) {
      dpin d = (dpin){.pinName = DIGITAL_INPUTS_item.value()["pinName"],
        .pinType = DIGITAL_INPUTS_item.value()["pinType"],
        .pinID =  DIGITAL_INPUTS_item.value()["pinID"],
        .pinInitialState =  DIGITAL_INPUTS_item.value()["pinInitialState"],
        .pinConnectState = DIGITAL_INPUTS_item.value()["pinConnectState"],
        .pinDisconnectState = DIGITAL_INPUTS_item.value()["pinDisconnectState"],
        .halPinDirection = DIGITAL_INPUTS_item.value()["halPinDirection"]
      };
      doutput_arr.push_back(d);
      }
    #endif
      //dinput_vect.push_back(d);
      //const char* DIGITAL_INPUTS_item_key = DIGITAL_INPUTS_item.key().c_str(); // "din.3", "din.4", "din.5", ...
      //Serial.print("KEY:");
      //Serial.println(d.pinName);
      //const char* DIGITAL_INPUTS_item_value_pinName = DIGITAL_INPUTS_item.value()["pinName"]; // "din.3", ...
      //const char* DIGITAL_INPUTS_item_value_pinType = DIGITAL_INPUTS_item.value()["pinType"];
      //int DIGITAL_INPUTS_item_value_pinID = DIGITAL_INPUTS_item.value()["pinID"]; // 3, 4, 5, 6, 7, 8
      //int DIGITAL_INPUTS_item_value_pinInitialState = DIGITAL_INPUTS_item.value()["pinInitialState"]; // -1, ...
      //int DIGITAL_INPUTS_item_value_pinConnectState = DIGITAL_INPUTS_item.value()["pinConnectState"]; // -1, ...
      //int DIGITAL_INPUTS_item_value_pinDisconnectState = DIGITAL_INPUTS_item.value()["pinDisconnectState"];
      //const char* DIGITAL_INPUTS_item_value_halPinDirection = DIGITAL_INPUTS_item.value()["halPinDirection"];

    }
    /*
      "din.3": {
      "pinName": "din.3",
      "pinType": "DIGITAL_INPUT",
      "pinID": 3,
      "pinInitialState": -1,
      "pinConnectState": -1,
      "pinDisconnectState": -1,
      "halPinDirection": "HAL_IN"
    },
    */
    
    // Print the result
    //serializeJsonPretty(doc, Serial);
    //String json(config)

}
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
  serialClient.RegisterConfigCallback(onConfig);
  digitalWrite(LED_BUILTIN, LOW);// Signal startup success to builtin LED
  serialClient.DoWork(); 
}

void loop() {
  serialClient.DoWork(); 
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
