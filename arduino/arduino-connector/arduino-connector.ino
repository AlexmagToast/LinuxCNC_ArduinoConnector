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
#include "FeatureController.h"
#include "Features.h"


#ifdef ENABLE_RAPIDCHANGE
#include "RapidChange.h"
#endif

#include <ArduinoJson.h>
#ifdef ENABLE_FEATUREMAP
#include "FeatureMap.h"
#endif
//#include <EEPROM.h>
//#include <FastCRC.h>
//#include <UUID.h>
//#include "SerialConnection.h"
//#include "ConfigManager.h"
//#include "IOProcessor.h"


/*
#ifdef ENABLE_FEATUREMAP
featureMap fm;
SerialConnection serialClient(SERIAL_RX_TIMEOUT, fm.features);
#else
uint32_t f = 0;
SerialConnection serialClient(SERIAL_RX_TIMEOUT, f);
#endif
*/



void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize builtin LED for error feedback/diagnostics 

  //DEBUG_DEV.begin(19200);
  COM_DEV.begin(115200);
  //DEBUG_DEV.begin(115200);
  //while (!Serial) {

 //   ; // wait for serial port to connect. Needed for native USB port only
  //}
  delay(SERIAL_STARTUP_DELAY);
  #ifdef DEBUG
    DEBUG_DEV.println(F("STARTING UP!! V8"));
    //DEBUG_DEV.println("HERE WE GO");
    DEBUG_DEV.flush();
  #endif
  /*
  if( EEPROM.length() == 0 )
  {
    #ifdef DEBUG
      DEBUG_DEV.println("EEPROM.length() reported zero bytes, setting to default of 1024 using .begin()..");
    #endif
    EEPROM.begin(EEPROM_DEFAULT_SIZE);
  }


  #ifdef DEBUG
    DEBUG_DEV.print("EEPROM length: ");
    DEBUG_DEV.println(EEPROM.length());
  #endif


  if( EEPROM.length() == 0 )
  {
      #ifdef DEBUG
        DEBUG_DEV.println("Error. EEPROM.length() reported zero bytes.");
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
      DEBUG_DEV.println("EEPROM HEADER MISSING, GENERATING NEW UID..");
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
    DEBUG_DEV.print("Wrote header value = 0x");
    DEBUG_DEV.print(epd.header, HEX);
    DEBUG_DEV.print(" to EEPROM and uid value = ");
    DEBUG_DEV.print((char*)epd.uid);
    DEBUG_DEV.print(" to EEPROM.");
    #endif
    
  }
  else
  {
    #ifdef DEBUG
      DEBUG_DEV.println("\nEEPROM HEADER DUMP");
      DEBUG_DEV.print("Head = 0x");
      DEBUG_DEV.println(epd.header, HEX);
      DEBUG_DEV.print("Config Version = 0x");
      DEBUG_DEV.println(epd.configVersion, HEX);
    #endif

    if(epd.configVersion != EEPROM_CONFIG_FORMAT_VERSION)
    {
      #ifdef DEBUG
        DEBUG_DEV.print("Error. Expected EEPROM Config Version: 0x");
        DEBUG_DEV.print(EEPROM_CONFIG_FORMAT_VERSION);
        DEBUG_DEV.print(", got: 0x");
        DEBUG_DEV.println(epd.configVersion);
      #endif

      while (true)
      {
        do_blink_sequence(1, 250, 250);
      // Loop forver as the expected config version does not match!
      }
    }
    #ifdef DEBUG
      DEBUG_DEV.print("Config Length = 0x");
      DEBUG_DEV.println(epd.configLen, HEX);
      DEBUG_DEV.print("Config CRC = 0x");
      DEBUG_DEV.println(epd.configCRC, HEX);
    #endif

    serialClient.setUID(epd.uid);
   
  }
  */
  #ifndef EEPROM_ENABLED
    //String uuid("ND");
    serialClient.setUID(uuid.c_str());
  #endif

  #ifdef ENABLE_RAPIDCHANGE
    rc_setup();
  #endif

  
  serialClient.RegisterConfigCallback(Callbacks::onConfig);
  //serialClient.RegisterCSCallback(Callbacks::onConnectionStageChange);
  //serialClient.RegisterPinChangeCallback(Callbacks::onPinChange);
  featureController.ExcecuteFeatureSetups();
  #ifdef DINPUTS
    Features::DigitalInputs * din = new Features::DigitalInputs();
    featureController.RegisterFeature(din);
  #endif
  #ifdef DOUTPUTS
    Features::DigitalOutputs * dout = new Features::DigitalOutputs();
    featureController.RegisterFeature(dout);
  #endif

  digitalWrite(LED_BUILTIN, LOW);// Signal startup success to builtin LED
  serialClient.DoWork(); 
}


void loop() {
  
  serialClient.DoWork(); 
  unsigned long currentMills = millis();

  //Serial.print("REGISTERED FEATURES COUNT = ");
  //Serial.println(featureController.GetRegisteredFeatureCount());
  featureController.ExecuteFeatureLoops();
  //delay(5000);

  #ifdef ENABLE_RAPIDCHANGE
    rc_loop();
  #endif
  

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