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
#include "ConfigManager.h"
//#include <ArxContainer.h>
//#include <Vector.h>
//using namespace arx;
featureMap fm;
SerialConnection serialClient(SERIAL_RX_TIMEOUT, fm.features);
ConfigManager configManager;
//std::map<String, int> mp;// {{"one", 1}, {"two", 2}, {"four", 4}};

void onConfig(const protocol::ConfigMessage& cm) {
      #ifdef DEBUG
        Serial.print("::onConfig called, featureID = ");
        Serial.print(cm.featureID);
        Serial.print(" Seq = ");
        Serial.print(cm.seq);
        Serial.print(" Total = ");
        Serial.println(cm.total);
        #ifdef DEBUG_VERBOSE
          Serial.print("Config: ");
          Serial.println(cm.configString);
        #endif
      #endif
      //Serial.print("Size of Config: ");
      //Serial.println(strlen(conf));
      switch (cm.featureID)
      {
        #ifdef DINPUTS
        
          case DINPUTS:
          {
            if(cm.seq == 0)
            {
              configManager.initDigitalInputPins(cm.total);
            }
            JsonDocument doc;

            DeserializationError error = deserializeJson(doc, cm.configString);

            if (error) {
              Serial.print(F("deserializeJson() of DINPUTS failed: "));
              Serial.println(error.f_str());
              return;
            }
            
              dpin d = (dpin){
                .pinID =  doc["pinID"],
                .pinInitialState =  doc["pinInitialState"],
                .pinConnectState = doc["pinConnectState"],
                .pinDisconnectState = doc["pinDisconnectState"],
                .debounce = doc["pinDebounce"],
                .inputPullup = doc["inputPullup"],
                .logicalID = doc["logicalID"],
                .pinCurrentState = 0,
                .t = 0
              };
              if (d.inputPullup == 1)
              {
                pinMode(atoi(d.pinID.c_str()), INPUT_PULLUP);
              }
              else { pinMode(atoi(d.pinID.c_str()), INPUT); }
              configManager.setDigitalInputPin(d, d.logicalID);
              
            
          }
        
        #endif
        #ifdef DOUTPUTS
        
          case DOUTPUTS:
          {
            if(cm.seq == 0)
            {
              configManager.initDigitalOutputPins(cm.total);
            }
            JsonDocument doc;

            DeserializationError error = deserializeJson(doc, cm.configString);

            if (error) {
              Serial.print(F("deserializeJson() of DINPUTS failed: "));
              Serial.println(error.f_str());
              return;
            }
            
              dpin d = (dpin){
                .pinID =  doc["pinID"],
                .pinInitialState =  doc["pinInitialState"],
                .pinConnectState = doc["pinConnectState"],
                .pinDisconnectState = doc["pinDisconnectState"],
                .debounce = doc["pinDebounce"],
                .inputPullup = doc["inputPullup"],
                .logicalID = doc["logicalID"],
                .pinCurrentState = 0,
                .t = 0
              };
              pinMode(atoi(d.pinID.c_str()), OUTPUT); 
              configManager.setDigitalOutputPin(d, d.logicalID);
              
            
          }
        
        #endif
      }
      /*
      #ifdef DINPUTS
        { 
          
          //din_storage_array = new dpin[]; //[ELEMENT_COUNT_MAX];
          //Vector<dpin> * dinput_arr = NULL; //(din_storage_array);
          //dinput_arr = new 
          //dinput_arr.clear();
          JsonDocument filter;
          filter["DIGITAL_INPUTS"] = true;
          //filter["DIGITAL_INPUTS_COUNT"] = true;
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, conf, DeserializationOption::Filter(filter));
          if (error) {
            #ifdef DEBUG
            Serial.print("deserializeJson() of DIGITAL_INPUTS failed: ");
            Serial.println(error.c_str());
            #endif
            return;
          }
          //int DIGITAL_INPUTS_COUNT = doc["DIGITAL_INPUTS_COUNT"];
          //if( din_storage_array != NULL )
          //{
          //  delete din_storage_array;
          //  delete dinput_arr;
          //}
          //dpin din_storage_array[DIGITAL_INPUTS_COUNT];
          //Vector<dpin> dinput_arr();//din_storage_array);
          //dinput_arr.setStorage(din_storage_array);

          //for (JsonPair DIGITAL_INPUTS_item : doc["DIGITAL_INPUTS"].as<JsonObject>()) {
            dpin d = (dpin){
              .pinID =  doc["pinID"],
              .pinInitialState =  doc["pinInitialState"],
              .pinConnectState = doc["pinConnectState"],
              .pinDisconnectState = doc["pinDisconnectState"],
              .debounce = doc["pinDebounce"],
              .inputPullup = doc["inputPullup"],
              .pinCurrentState = 0,
              .t = 0
            };
            if (d.inputPullup == 1)
            {
              pinMode(atoi(d.pinID.c_str()), INPUT_PULLUP);
            }
            else { pinMode(atoi(d.pinID.c_str()), INPUT); }
            //dinput_arr->push_back(d);
          }
          
        }
      #endif
      #ifdef DOUTPUTS
      {
        
        doutput_arr.clear();
        
        StaticJsonDocument<200> filter;
        filter["DIGITAL_OUTPUTS"] = true;
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, conf, DeserializationOption::Filter(filter));
        if (error) {
          #ifdef DEBUG
          Serial.print("deserializeJson() of DIGITAL_OUTPUTS  failed: ");
          Serial.println(error.c_str());
          #endif
          return;
        }
        
        for (JsonPair DIGITAL_OUTPUTS_item : doc["DIGITAL_OUTPUTS"].as<JsonObject>()) {
          dpin d = (dpin){.pinName = DIGITAL_OUTPUTS_item.value()["pinName"],
            .pinType = DIGITAL_OUTPUTS_item.value()["pinType"],
            .pinID =  DIGITAL_OUTPUTS_item.value()["pinID"],
            .pinInitialState =  DIGITAL_OUTPUTS_item.value()["pinInitialState"],
            .pinConnectState = DIGITAL_OUTPUTS_item.value()["pinConnectState"],
            .pinDisconnectState = DIGITAL_OUTPUTS_item.value()["pinDisconnectState"],
            .halPinDirection = DIGITAL_OUTPUTS_item.value()["halPinDirection"]
          };
          
          
          doutput_arr.push_back(d);
          
        }
        
        
      }  
      #endif
      */
  }

void onConnectionStageChange(int s) {
}


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
  serialClient.RegisterConfigCallback(onConfig);
  serialClient.RegisterCSCallback(onConnectionStageChange);
  digitalWrite(LED_BUILTIN, LOW);// Signal startup success to builtin LED
  serialClient.DoWork(); 
}

void loop() {
  serialClient.DoWork(); 
  unsigned long currentMills = millis();
  
  #ifdef DINPUTS
  
  if(configManager.GetDigitalInputsReady() == 1)
  {
    //Serial.println("READY");
    for( int x = 0; x < configManager.GetDigitalInputPinsLen(); x++ )
    {
      dpin & pin = configManager.getDigitalInputPins()[x];
      int v = digitalRead(atoi(pin.pinID.c_str()));
      if(pin.pinCurrentState != v && (currentMills - pin.t) >= pin.debounce)
      {
        #ifdef DEBUG
        Serial.print("PIN CHANGE! ");
        Serial.print("PIN: ");
        Serial.print(pin.pinID);
        Serial.print(" Current value: ");
        Serial.print(pin.pinCurrentState);
        Serial.print(" New value: ");
        Serial.println(v);
        #endif
        pin.pinCurrentState = v;
        pin.t = currentMills;
        // send update out
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
