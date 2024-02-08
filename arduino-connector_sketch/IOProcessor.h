#ifndef IOPROCESSOR_H_
#define IOPROCESSOR_H_


namespace Callbacks
{
  void onPinChange(const protocol::PinChangeMessage& pcm) {
      #ifdef DEBUG_VERBOSE
        DEBUG_DEV.print(" Callbacks::onPinChange called, featureID = ");
        DEBUG_DEV.print(pcm.featureID);
        DEBUG_DEV.print(" Response Req = ");
        DEBUG_DEV.print(pcm.responseReq);
        DEBUG_DEV.print(" Message = ");
        DEBUG_DEV.println(pcm.message);
      #endif

      switch (pcm.featureID)
      {
        #ifdef DOUTPUTS
          case DOUTPUTS:
          {
            if(ConfigManager::GetDigitalOutputsReady() == 0)
            {
              #ifdef DEBUG_VERBOSE
                DEBUG_DEV.println(F(" Callbacks::onPinChange: GetDigitalOutputsReady() returned FALSE"));
              #endif
              return;
            }

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, pcm.message);

            if (error) {
              #ifdef DEBUG_VERBOSE
                DEBUG_DEV.print(F(" Callbacks::onPinChange: deserializeJson() of message failed: "));
                DEBUG_DEV.println(error.c_str());
              #endif
              return;
            }

            for (JsonObject pa_item : doc["pa"].as<JsonArray>()) {
              
              int lid = pa_item["lid"]; // 0, 1
              int pid = pa_item["pid"]; // 0, 1
              int v = pa_item["v"]; // 1, 0

              if(lid > ConfigManager::GetDigitalOutputPinsLen())
              {
                #ifdef DEBUG_VERBOSE
                DEBUG_DEV.print(" Callbacks::onPinChange: Error. logical pin ID ");
                DEBUG_DEV.print(lid);
                DEBUG_DEV.println(" is invalid.");
                #endif
              }
              else{
                ConfigManager::dpin & pin = ConfigManager::GetDigitalOutputPins()[lid];
                #ifdef DEBUG_VERBOSE
                DEBUG_DEV.print("DOUTPUTS PIN CHANGE!");
                DEBUG_DEV.print("PIN ID: ");
                DEBUG_DEV.println(pin.pinID);
                DEBUG_DEV.print("PID: ");
                DEBUG_DEV.println(pid);
                DEBUG_DEV.print("LID: ");
                DEBUG_DEV.println(lid);
                DEBUG_DEV.print("Current value:");
                DEBUG_DEV.println(pin.pinCurrentState);
                DEBUG_DEV.print("New value:");
                DEBUG_DEV.println(v);
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
        DEBUG_DEV.print(F("::onConfig called, featureID = "));
        DEBUG_DEV.print((int)cm.featureID);
        DEBUG_DEV.print(F(" Seq = "));
        DEBUG_DEV.print(cm.seq);
        DEBUG_DEV.print(F(" Total = "));
        DEBUG_DEV.println(cm.total);
        #ifdef DEBUG_VERBOSE
          DEBUG_DEV.print("Config: ");
          DEBUG_DEV.println(cm.configString);
        #endif
      #endif
      //DEBUG_DEV.print("Size of Config: ");
      //DEBUG_DEV.println(strlen(conf));
      switch ((int)cm.featureID)
      {
        #ifdef DINPUTS
        
          case DINPUTS:
          {
            if(cm.seq == 0)
            {
              ConfigManager::InitDigitalInputPins(cm.total);
            }
            JsonDocument doc;

            DeserializationError error = deserializeJson(doc, cm.configString);

            if (error) {
              #ifdef DEBUG
              DEBUG_DEV.print(F("deserializeJson() of DINPUTS failed: "));
              DEBUG_DEV.println(error.f_str());
              #endif
              break;
            }
            
            ConfigManager::dpin d = (ConfigManager::dpin){
              .pinID =  doc["id"],
              .pinInitialState =  doc["is"],
              .pinConnectedState = doc["cs"],
              .pinDisconnectedState = doc["ds"],
              .debounce = doc["pd"],
              .inputPullup = doc["ip"],
              .logicalID = doc["li"],
              .pinCurrentState = 0,
              .t = 0
            };
            if (d.inputPullup == 1)
            {
              pinMode(atoi(d.pinID.c_str()), INPUT_PULLUP);
            }
            else { pinMode(atoi(d.pinID.c_str()), INPUT); }
            ConfigManager::SetDigitalInputPin(d, d.logicalID);
            
          }
          break;
        
        #endif
        #ifdef DOUTPUTS
        
          case DOUTPUTS:
          {
            if(cm.seq == 0)
            {
              ConfigManager::InitDigitalOutputPins(cm.total);
            }
            JsonDocument doc;

            DeserializationError error = deserializeJson(doc, cm.configString);

            if (error) {
              #ifdef DEBUG
              DEBUG_DEV.print(F("deserializeJson() of DOUTPUTS failed: "));
              DEBUG_DEV.println(error.f_str());
              #endif
              break;
            }
            
            ConfigManager::dpin d = (ConfigManager::dpin){
              .pinID =  doc["id"],
              .pinInitialState =  doc["is"],
              .pinConnectedState = doc["cs"],
              .pinDisconnectedState = doc["ds"],
              .debounce = doc["pd"],
              .inputPullup = doc["ip"],
              .logicalID = doc["li"],
              .pinCurrentState = 0,
              .t = 0
            };
            pinMode(atoi(d.pinID.c_str()), OUTPUT); 
            ConfigManager::SetDigitalOutputPin(d, d.logicalID);
            if(d.pinInitialState != -1)
            {
              digitalWrite(atoi(d.pinID.c_str()), d.pinInitialState);
            }
          }
          break;
        
        #endif
        #ifdef AINPUTS
        
          case AINPUTS:
          {
            if(cm.seq == 0)
            {
              ConfigManager::InitAnalogInputPins(cm.total);
            }
            JsonDocument doc;

            DeserializationError error = deserializeJson(doc, cm.configString);

            if (error) {
              #ifdef DEBUG
              DEBUG_DEV.print(F("deserializeJson() of AINPUTS failed: "));
              DEBUG_DEV.println(error.f_str());
              #endif
              break;
            }

            ConfigManager::apin a = (ConfigManager::apin){
              .pinID =  doc["id"],
              .pinInitialState =  doc["is"],
              .pinConnectedState = doc["cs"],
              .pinDisconnectedState = doc["ds"],
              .pinSmoothing = doc["ps"],
              .pinMaxValue = doc["pm"],
              .pinMinValue = doc["pn"],
              .logicalID = doc["li"],
              .pinCurrentState = 0,
              .t = 0
            };
            pinMode(atoi(a.pinID.c_str()), INPUT);
            ConfigManager::SetAnalogInputPin(a, a.logicalID);
          }
          break;
        
        #endif
        #ifdef AOUTPUTS
        
          case AOUTPUTS:
          {
            if(cm.seq == 0)
            {
              ConfigManager::InitAnalogOutputPins(cm.total);
            }
            JsonDocument doc;

            DeserializationError error = deserializeJson(doc, cm.configString);

            if (error) {
              #ifdef DEBUG
              DEBUG_DEV.print(F("deserializeJson() of AOUTPUTS failed: "));
              DEBUG_DEV.println(error.f_str());
              #endif
              break;
            }
            
            ConfigManager::apin a = (ConfigManager::apin){
              .pinID =  doc["pinID"],
              .pinInitialState =  doc["is"],
              .pinConnectedState = doc["cs"],
              .pinDisconnectedState = doc["ds"],
              .pinSmoothing = doc["ps"],
              .pinMaxValue = doc["pm"],
              .pinMinValue = doc["pn"],
              .logicalID = doc["li"],
              .pinCurrentState = 0,
              .t = 0
            };
            pinMode(atoi(a.pinID.c_str()), OUTPUT); 
            ConfigManager::SetAnalogOutputPin(a, a.logicalID);
            if(a.pinInitialState != -1)
            {
              analogWrite(atoi(a.pinID.c_str()), a.pinInitialState);
            }
          }
          break;
        
        #endif
      }

    }

  void onConnectionStageChange(int s) {
    #ifdef DOUTPUTS
        if(ConfigManager::GetDigitalOutputsReady() == 0)
        {
          #ifdef DEBUG_VERBOSE
            DEBUG_DEV.println(F("Callbacks::onPinChange: GetDigitalOutputsReady() returned FALSE"));
          #endif
          return;
        }
        for( int x = 0; x < ConfigManager::GetDigitalOutputPinsLen(); x++)
        {
          ConfigManager::dpin & pin = ConfigManager::GetDigitalOutputPins()[x];
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
