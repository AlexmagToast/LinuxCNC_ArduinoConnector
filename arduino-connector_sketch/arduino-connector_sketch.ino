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
//#include <Arduino.h>
#include "Config.h"

#ifdef INTEGRATED_CALLBACKS
#include "RXBuffer.h"
#endif

//#define MSGPACKETIZER_ENABLE_STREAM
//#define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE 7
//#define PACKETIZER_MAX_PACKET_QUEUE_SIZE 5
#include <MsgPacketizer.h>
#include <ArduinoJson.h>
#ifdef ENABLE_FEATUREMAP
#include "FeatureMap.h"
#endif
//#include <EEPROM.h>
//#include <FastCRC.h>
//#include <UUID.h>
#include "SerialConnection.h"
#include "ConfigManager.h"
#include "IOProcessor.h"



#ifdef ENABLE_FEATUREMAP
featureMap fm;
SerialConnection serialClient(SERIAL_RX_TIMEOUT, fm.features);
#else
uint64_t f = 0;
SerialConnection serialClient(SERIAL_RX_TIMEOUT, f);
#endif



void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize builtin LED for error feedback/diagnostics 

  //SERIAL_DEV.begin(19200);
  COM_DEV.begin(115200);
  SERIAL_DEV.begin(115200);
  //while (!Serial) {

 //   ; // wait for serial port to connect. Needed for native USB port only
  //}
  delay(SERIAL_STARTUP_DELAY);
  #ifdef DEBUG
    SERIAL_DEV.println(F("STARTING UP"));
    //SERIAL_DEV.println("HERE WE GO");
    SERIAL_DEV.flush();
  #endif
  /*
  if( EEPROM.length() == 0 )
  {
    #ifdef DEBUG
      SERIAL_DEV.println("EEPROM.length() reported zero bytes, setting to default of 1024 using .begin()..");
    #endif
    EEPROM.begin(EEPROM_DEFAULT_SIZE);
  }


  #ifdef DEBUG
    SERIAL_DEV.print("EEPROM length: ");
    SERIAL_DEV.println(EEPROM.length());
  #endif


  if( EEPROM.length() == 0 )
  {
      #ifdef DEBUG
        SERIAL_DEV.println("Error. EEPROM.length() reported zero bytes.");
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
      SERIAL_DEV.println("EEPROM HEADER MISSING, GENERATING NEW UID..");
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
    SERIAL_DEV.print("Wrote header value = 0x");
    SERIAL_DEV.print(epd.header, HEX);
    SERIAL_DEV.print(" to EEPROM and uid value = ");
    SERIAL_DEV.print((char*)epd.uid);
    SERIAL_DEV.print(" to EEPROM.");
    #endif
    
  }
  else
  {
    #ifdef DEBUG
      SERIAL_DEV.println("\nEEPROM HEADER DUMP");
      SERIAL_DEV.print("Head = 0x");
      SERIAL_DEV.println(epd.header, HEX);
      SERIAL_DEV.print("Config Version = 0x");
      SERIAL_DEV.println(epd.configVersion, HEX);
    #endif

    if(epd.configVersion != EEPROM_CONFIG_FORMAT_VERSION)
    {
      #ifdef DEBUG
        SERIAL_DEV.print("Error. Expected EEPROM Config Version: 0x");
        SERIAL_DEV.print(EEPROM_CONFIG_FORMAT_VERSION);
        SERIAL_DEV.print(", got: 0x");
        SERIAL_DEV.println(epd.configVersion);
      #endif

      while (true)
      {
        do_blink_sequence(1, 250, 250);
      // Loop forver as the expected config version does not match!
      }
    }
    #ifdef DEBUG
      SERIAL_DEV.print("Config Length = 0x");
      SERIAL_DEV.println(epd.configLen, HEX);
      SERIAL_DEV.print("Config CRC = 0x");
      SERIAL_DEV.println(epd.configCRC, HEX);
    #endif

    serialClient.setUID(epd.uid);
   
  }
  */
  #ifndef EEPROM_ENABLED
    //String uuid("ND");
    serialClient.setUID(uuid.c_str());
  #endif

  
  serialClient.RegisterConfigCallback(Callbacks::onConfig);
  serialClient.RegisterCSCallback(Callbacks::onConnectionStageChange);
  serialClient.RegisterPinChangeCallback(Callbacks::onPinChange);
  //#ifdef INTEGRATED_CALLBACKS
  //RXBuffer::init(RX_BUFFER_SIZE);
  //#endif
  digitalWrite(LED_BUILTIN, LOW);// Signal startup success to builtin LED
  serialClient.DoWork(); 
}


void loop() {
  
  serialClient.DoWork(); 
  unsigned long currentMills = millis();
  
  #ifdef DINPUTS
  
  if(ConfigManager::GetDigitalInputsReady() == 1)
  {
    String output; // Used below to output Io update messages
    JsonDocument doc;
    JsonArray pa; 

    //pa.clear();
    //doc.clear();
    pa = doc["pa"].to<JsonArray>();

    for( int x = 0; x < ConfigManager::GetDigitalInputPinsLen(); x++ )
    {
      
      ConfigManager::dpin & pin = ConfigManager::GetDigitalInputPins()[x];
      int v = digitalRead(atoi(pin.pinID.c_str()));
  
      if(pin.pinCurrentState != v && (currentMills - pin.t) >= pin.debounce)
      {
        #ifdef DEBUG_VERBOSE
        SERIAL_DEV.print(F("DINPUTS PIN CHANGE!"));
        SERIAL_DEV.print(F("PIN:"));
        SERIAL_DEV.println(pin.pinID);
        SERIAL_DEV.print(F("Current value: "));
        SERIAL_DEV.println(pin.pinCurrentState);
        SERIAL_DEV.print(F("New value: "));
        SERIAL_DEV.println(v);
        #endif
        
        pin.pinCurrentState = v;
        pin.t = currentMills;
        
        // send update out
        //serialClient

        //doc.clear();

        JsonObject pa_0 = pa.add<JsonObject>();
        pa_0["lid"] = x;
        pa_0["pid"] = atoi(pin.pinID.c_str());
        pa_0["v"] = v;
        
        //doc.shrinkToFit();  // optional

        
      }

    }
    if(pa.size() > 0)
    {
      output = "";
      serializeJson(doc, output);
      #ifdef DEBUG_VERBOSE
      SERIAL_DEV.print(F("JSON = "));
      SERIAL_DEV.println(output);
      #endif
      uint8_t seqID = 0;
      uint8_t resp = 0; // Future TODO: Consider requiring ACK/NAK, maybe.
      uint8_t f = DINPUTS;
      //String o = String(output.c_str());
      serialClient.SendPinChangeMessage(f, seqID, resp, output);
      pa.clear();
    }
  }
  //for (dpin & pin : ConfigManager.GetDigitalInputPins())

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
/*
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
*/