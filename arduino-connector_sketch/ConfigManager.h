#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_
#include "Config.h"
//#include <ArxContainer.h>

namespace ConfigManager
{
  #if defined(DINPUTS) || defined(DOUTPUTS)
  struct dpin
  {
      String pinID;
      int8_t pinInitialState;
      int8_t pinConnectedState;
      int8_t pinDisconnectedState;
      uint16_t debounce;
      uint8_t inputPullup;
      uint8_t logicalID;
      int8_t pinCurrentState;
      unsigned long t;
  };
  #endif

#ifdef DINPUTS
    dpin * dinput_arr = NULL;//new dpin[100];// = NULL;
    size_t dinput_arr_len = 0;
    uint8_t dinput_arr_ready = 0;

  void SetDigitalInputPin(dpin pin, uint8_t index)
  {
    #ifdef DEBUG
      Serial.print("ConfigManager::SetDigitalInputPin, Index=0x");
      Serial.println(index, HEX);
      #ifdef DEBUG_VERBOSE
        Serial.println("----------- START PIN CONFIG DUMP ------------");
        Serial.print("pinID=");
        Serial.println(pin.pinID);
        Serial.print("pinInitialState=");
        Serial.println(pin.pinInitialState);
        Serial.print("pinConnectedState=");
        Serial.println(pin.pinConnectedState);    
        Serial.print("pinDisconnectedState=");
        Serial.println(pin.pinDisconnectedState); 
        Serial.print("debounce=");
        Serial.println(pin.debounce); 
        Serial.print("inputPullup=");
        Serial.println(pin.inputPullup); 
        Serial.print("logicalID=");
        Serial.println(pin.logicalID); 
        Serial.print("pinCurrentState=");
        Serial.println(pin.pinCurrentState); 
        Serial.print("t=");
        Serial.println(pin.t);
        Serial.println("----------- END PIN CONFIG DUMP ------------");
      #endif     
    #endif
    dinput_arr[index] = pin;
  }
  void ClearDigitalInputPins()
  {
    if( dinput_arr != NULL )
    {
      delete[] dinput_arr;
      dinput_arr_len = 0;
      dinput_arr_ready = 0;
    }
    dinput_arr = NULL;
  }
  void InitDigitalInputPins(size_t size)
  {
    #ifdef DEBUG
      Serial.print("ConfigManager::initDigialInputPins, Size=0x");
      Serial.println(size, HEX);
    #endif
    if( dinput_arr != NULL )
    {
      delete[] dinput_arr;
      dinput_arr_len = 0;
      dinput_arr_ready = 0;
    }
    dinput_arr = new dpin[size];
    dinput_arr_len = size;
  }

  dpin * GetDigitalInputPins()
  {
    return dinput_arr;
  }

  size_t & GetDigitalInputPinsLen()
  {
    return dinput_arr_len;
  }

  uint8_t GetDigitalInputsReady()
  {
    if(dinput_arr_ready == 1)
      return 1;
    if(dinput_arr == NULL)
    {
      return 0;
    }
    for( int x = 0; x < GetDigitalInputPinsLen(); x++)
    {
      dpin d = GetDigitalInputPins()[x];
      if(d.pinID.length() == 0)
      {
        //Serial.print("NOT READY PIN = ");
        //Serial.println(x);
        return 0;
      }
    }
    dinput_arr_ready = 1;
    return 1;
  }
#endif

#ifdef DOUTPUTS
  dpin * doutput_arr = NULL;//new dpin[100];// = NULL;
  size_t doutput_arr_len = 0;
  uint8_t doutput_arr_ready = 0;

  void SetDigitalOutputPin(dpin pin, uint8_t index)
  {
    #ifdef DEBUG
      Serial.print("ConfigManager::SetDigitalOutputPin, Index=0x");
      Serial.println(index, HEX);
      #ifdef DEBUG_VERBOSE
        Serial.println("----------- START PIN CONFIG DUMP ------------");
        Serial.print("pinID=");
        Serial.println(pin.pinID);
        Serial.print("pinInitialState=");
        Serial.println(pin.pinInitialState);
        Serial.print("pinConnectedState=");
        Serial.println(pin.pinConnectedState);    
        Serial.print("pinDisconnectedState=");
        Serial.println(pin.pinDisconnectedState); 
        Serial.print("debounce=");
        Serial.println(pin.debounce); 
        Serial.print("inputPullup=");
        Serial.println(pin.inputPullup); 
        Serial.print("logicalID=");
        Serial.println(pin.logicalID); 
        Serial.print("pinCurrentState=");
        Serial.println(pin.pinCurrentState); 
        Serial.print("t=");
        Serial.println(pin.t);
        Serial.println("----------- END PIN CONFIG DUMP ------------");
      #endif     
    #endif
    doutput_arr[index] = pin;
  }
  void ClearDigitalOutputPins()
  {
    if( doutput_arr != NULL )
    {
      delete[] doutput_arr;
      doutput_arr_len = 0;
      doutput_arr_ready = 0;
    }
    doutput_arr = NULL;
  }
  void InitDigitalOutputPins(size_t size)
  {
    #ifdef DEBUG
      Serial.print("ConfigManager::InitDigitalOutputPins, Size=0x");
      Serial.println(size, HEX);
    #endif
    if( doutput_arr != NULL )
    {
      delete[] doutput_arr;
      doutput_arr_len = 0;
      doutput_arr_ready = 0;
    }
    doutput_arr = new dpin[size];
    doutput_arr_len = size;

  }

  dpin * GetDigitalOutputPins()
  {
    return doutput_arr;
  }

  size_t & GetDigitalOutputPinsLen()
  {
    return doutput_arr_len;
  }

  uint8_t GetDigitalOutputsReady()
  {
    if(doutput_arr_ready == 1)
      return 1;
    if(doutput_arr == NULL)
    {
      return 0;
    }
    for( int x = 0; x < GetDigitalOutputPinsLen(); x++)
    {
      dpin d = GetDigitalOutputPins()[x];
      if(d.pinID.length() == 0)
      {
        //Serial.print("NOT READY PIN = ");
        //Serial.println(x);
        return 0;
      }
    }
    doutput_arr_ready = 1;
    return 1;
  }
#endif
}
#endif