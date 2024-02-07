#ifndef IOPROCESSOR_H_
#define IOPROCESSOR_H_


namespace Callbacks
{
  void onPinChange(const protocol::PinChangeMessage& pcm) {
      #ifdef DEBUG_VERBOSE
        SERIAL_DEV.print(" Callbacks::onPinChange called, featureID = ");
        SERIAL_DEV.print(pcm.featureID);
        SERIAL_DEV.print(" Response Req = ");
        SERIAL_DEV.print(pcm.responseReq);
        SERIAL_DEV.print(" Message = ");
        SERIAL_DEV.println(pcm.message);
      #endif

      switch (pcm.featureID)
      {
        #ifdef DOUTPUTS
          case DOUTPUTS:
          {
            if(ConfigManager::GetDigitalOutputsReady() == 0)
            {
              #ifdef DEBUG_VERBOSE
                SERIAL_DEV.println(F(" Callbacks::onPinChange: GetDigitalOutputsReady() returned FALSE"));
              #endif
              return;
            }

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, pcm.message);

            if (error) {
              #ifdef DEBUG_VERBOSE
                SERIAL_DEV.print(F(" Callbacks::onPinChange: deserializeJson() of message failed: "));
                SERIAL_DEV.println(error.c_str());
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
                SERIAL_DEV.print(" Callbacks::onPinChange: Error. logical pin ID ");
                SERIAL_DEV.print(lid);
                SERIAL_DEV.println(" is invalid.");
                #endif
              }
              else{
                ConfigManager::dpin & pin = ConfigManager::GetDigitalOutputPins()[lid];
                #ifdef DEBUG_VERBOSE
                SERIAL_DEV.print("DOUTPUTS PIN CHANGE!");
                SERIAL_DEV.print("PIN ID: ");
                SERIAL_DEV.println(pin.pinID);
                SERIAL_DEV.print("PID: ");
                SERIAL_DEV.println(pid);
                SERIAL_DEV.print("LID: ");
                SERIAL_DEV.println(lid);
                SERIAL_DEV.print("Current value:");
                SERIAL_DEV.println(pin.pinCurrentState);
                SERIAL_DEV.print("New value:");
                SERIAL_DEV.println(v);
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
        SERIAL_DEV.print(F("::onConfig called, featureID = "));
        SERIAL_DEV.print((int)cm.featureID);
        SERIAL_DEV.print(F(" Seq = "));
        SERIAL_DEV.print(cm.seq);
        SERIAL_DEV.print(F(" Total = "));
        SERIAL_DEV.println(cm.total);
        #ifdef DEBUG_VERBOSE
          SERIAL_DEV.print("Config: ");
          SERIAL_DEV.println(cm.configString);
        #endif
      #endif
      //SERIAL_DEV.print("Size of Config: ");
      //SERIAL_DEV.println(strlen(conf));
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
              SERIAL_DEV.print(F("deserializeJson() of DINPUTS failed: "));
              SERIAL_DEV.println(error.f_str());
              #endif
              break;
            }
            
            ConfigManager::dpin d = (ConfigManager::dpin){
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
              SERIAL_DEV.print(F("deserializeJson() of DOUTPUTS failed: "));
              SERIAL_DEV.println(error.f_str());
              #endif
              break;
            }
            
            ConfigManager::dpin d = (ConfigManager::dpin){
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
            ConfigManager::SetDigitalOutputPin(d, d.logicalID);
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
        if(ConfigManager::GetDigitalOutputsReady() == 0)
        {
          #ifdef DEBUG_VERBOSE
            SERIAL_DEV.println(F("Callbacks::onPinChange: GetDigitalOutputsReady() returned FALSE"));
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
