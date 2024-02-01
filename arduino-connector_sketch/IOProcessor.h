#ifndef IOPROCESSOR_H_
#define IOPROCESSOR_H_


namespace Callbacks
{
  void onPinChange(const protocol::PinChangeMessage& pcm) {
      #ifdef DEBUG
        Serial.print("::onPinChange called, featureID = ");
        Serial.print(pcm.featureID);
        Serial.print(" Response Req = ");
        Serial.print(pcm.responseReq);
        Serial.print(" Message = ");
        Serial.println(pcm.message);
      #endif
  }

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

    }

  void onConnectionStageChange(int s) {
  }
}


#endif
