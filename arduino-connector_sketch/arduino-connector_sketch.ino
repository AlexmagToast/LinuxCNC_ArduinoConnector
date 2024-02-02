/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com &
  Ken Thompson (not THAT Ken Thompson), https://github.com/KennethThompson
  
  MIT License
  Copyright (c) 2023 Alexander Richter & Ken Thompson

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
#include "ConfigManager.h"
#include "IOProcessor.h"

featureMap fm;
SerialConnection serialClient(SERIAL_RX_TIMEOUT, fm.features);


void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize builtin LED for error feedback/diagnostics 

  Serial.begin(115200);
  //while (!Serial) {

 //   ; // wait for serial port to connect. Needed for native USB port only
  //}
  delay(SERIAL_STARTUP_DELAY);
  #ifdef DEBUG
    Serial.println("ARDUINO DEBUG: STARTING UP.. ");
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
  serialClient.RegisterConfigCallback(Callbacks::onConfig);
  serialClient.RegisterCSCallback(Callbacks::onConnectionStageChange);
  serialClient.RegisterPinChangeCallback(Callbacks::onPinChange);
  digitalWrite(LED_BUILTIN, LOW);// Signal startup success to builtin LED
  serialClient.DoWork(); 
}


void loop() {
  serialClient.DoWork(); 
  unsigned long currentMills = millis();
  
  #ifdef DINPUTS
  
  if(configManager.GetDigitalInputsReady() == 1)
  {
    String output;
    JsonDocument doc;
    JsonArray pa; //= doc["pa"].to<JsonArray>();
    //Serial.println("READY");
    for( int x = 0; x < configManager.GetDigitalInputPinsLen(); x++ )
    {
      
      dpin & pin = configManager.getDigitalInputPins()[x];
      int v = digitalRead(atoi(pin.pinID.c_str()));
  
      if(pin.pinCurrentState != v && (currentMills - pin.t) >= pin.debounce)
      {
        #ifdef DEBUG_VERBOSE
        Serial.print("DINPUTS PIN CHANGE! ");
        Serial.print("PIN:");
        Serial.println(pin.pinID);
        Serial.print("Current value: ");
        Serial.println(pin.pinCurrentState);
        Serial.print("New value: ");
        Serial.println(v);
        #endif
        
        pin.pinCurrentState = v;
        pin.t = currentMills;
        
        // send update out
        //serialClient

        //doc.clear();
        
        if(pa.isNull())
        {
          pa = doc["pa"].to<JsonArray>();
        }
        JsonObject pa_0 = pa.add<JsonObject>();
        pa_0["lid"] = x;
        pa_0["pid"] = atoi(pin.pinID.c_str());
        pa_0["v"] = v;
        
        //doc.shrinkToFit();  // optional

        
      }
      if(pa.size() > 0)
      {
        serializeJson(doc, output);
        Serial.print("JSON = ");
        Serial.println(output);
        uint8_t seqID = 0;
        uint8_t resp = 0; // Future TODO: Consider requiring ACK/NAK, maybe.
        uint8_t f = DINPUTS;
        //String o = String(output.c_str());
        serialClient.SendPinChangeMessage(f, seqID, resp, output);
      }

    }
  }
  //for (dpin & pin : configManager.getDigitalInputPins())

  #endif
  
/*
  #ifdef DOUTPUTS
  for (dpin pin : doutput_arr)
  {
    //Serial << element << " ";
  }
  #endif
*/
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
