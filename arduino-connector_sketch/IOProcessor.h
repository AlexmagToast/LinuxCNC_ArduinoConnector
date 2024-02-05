#ifndef IOPROCESSOR_H_
#define IOPROCESSOR_H_


namespace Callbacks
{
  void onPinChange(const protocol::PinChangeMessage& pcm) {
      #ifdef DEBUG_VERBOSE
        Serial.print("ARDUINO DEBUG: Callbacks::onPinChange called, featureID = ");
        Serial.print(pcm.featureID);
        Serial.print("ARDUINO DEBUG: Response Req = ");
        Serial.print(pcm.responseReq);
        Serial.print("ARDUINO DEBUG: Message = ");
        Serial.println(pcm.message);
      #endif

      switch (pcm.featureID)
      {
        #ifdef DOUTPUTS
          case DOUTPUTS:
          {
            if(configManager.GetDigitalOutputsReady() == 0)
            {
              #ifdef DEBUG_VERBOSE
                Serial.print("ARDUINO DEBUG: Callbacks::onPinChange: GetDigitalOutputsReady() returned FALSE");
              #endif
              return;
            }

            JsonDocument doc;

            DeserializationError error = deserializeJson(doc, pcm.message);

            if (error) {
              #ifdef DEBUG_VERBOSE
                Serial.print("ARDUINO DEBUG: Callbacks::onPinChange: deserializeJson() of message failed: ");
                Serial.println(error.c_str());
              #endif
              return;
            }

            for (JsonObject pa_item : doc["pa"].as<JsonArray>()) {
              
              int lid = pa_item["lid"]; // 0, 1
              int pid = pa_item["pid"]; // 0, 1
              int v = pa_item["v"]; // 1, 0

              if(lid > configManager.GetDigitalOutputPinsLen())
              {
                #ifdef DEBUG_VERBOSE
                Serial.print("ARDUINO DEBUG: Callbacks::onPinChange: Error. logical pin ID ");
                Serial.print(lid);
                Serial.println(" is invalid.");
                #endif
              }
              else{
                dpin & pin = configManager.getDigitalOutputPins()[lid];
                #ifdef DEBUG_VERBOSE
                Serial.print("DOUTPUTS PIN CHANGE!");
                Serial.print("PIN ID: ");
                Serial.println(pin.pinID);
                Serial.print("PID: ");
                Serial.println(pid);
                Serial.print("LID: ");
                Serial.println(lid);
                Serial.print("Current value:");
                Serial.println(pin.pinCurrentState);
                Serial.print("New value:");
                Serial.println(v);
                #endif
                pin.pinCurrentState = v;
                digitalWrite(atoi(pin.pinID.c_str()), v);
              }
            }
            break;
          }
        #endif
      }
  }

  void onConfig(const protocol::ConfigMessage& cm) {
      #ifdef DEBUG
        Serial.print("::onConfig called, featureID = ");
        Serial.print((int)cm.featureID);
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
      switch ((int)cm.featureID)
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
              break;
            }
            
            dpin d = (dpin){
              .pinID =  doc["pinID"],
              .pinInitialState =  doc["pinInitialState"],
              .pinConnectedState = doc["pinConnectedState"],
              .pinDisconnectedState = doc["pinDisconnectedState"],
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
          break;
        
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
              Serial.print(F("deserializeJson() of DOUTPUTS failed: "));
              Serial.println(error.f_str());
              break;
            }
            
            dpin d = (dpin){
              .pinID =  doc["pinID"],
              .pinInitialState =  doc["pinInitialState"],
              .pinConnectedState = doc["pinConnectedState"],
              .pinDisconnectedState = doc["pinDisconnectedState"],
              .debounce = doc["pinDebounce"],
              .inputPullup = doc["inputPullup"],
              .logicalID = doc["logicalID"],
              .pinCurrentState = 0,
              .t = 0
            };
            pinMode(atoi(d.pinID.c_str()), OUTPUT); 
            configManager.setDigitalOutputPin(d, d.logicalID);
            if(d.pinInitialState != -1)
            {
              digitalWrite(atoi(d.pinID.c_str()), d.pinInitialState);
            }
          }
          break;
        
        #endif
      }

    }

  void onConnectionStageChange(int s) {
    #ifdef DOUTPUTS
        if(configManager.GetDigitalOutputsReady() == 0)
        {
          #ifdef DEBUG_VERBOSE
            Serial.print("ARDUINO DEBUG: Callbacks::onPinChange: GetDigitalOutputsReady() returned FALSE");
          #endif
          return;
        }
        for( int x = 0; x < configManager.GetDigitalOutputPinsLen(); x++)
        {
          dpin & pin = configManager.getDigitalOutputPins()[x];
          if( s == CS_CONNECTED && pin.pinConnectedState != -1 )
          {
            digitalWrite(atoi(pin.pinID.c_str()), pin.pinConnectedState);
          }
          if( s == CS_DISCONNECTED && pin.pinDisconnectedState != -1 )
          {
            digitalWrite(atoi(pin.pinID.c_str()), pin.pinDisconnectedState);
          }
        }

        
    #endif
  }
}


#endif
