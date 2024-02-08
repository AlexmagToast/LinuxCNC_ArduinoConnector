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
      DEBUG_DEV.print(F("ConfigManager::SetDigitalInputPin, Index=0x"));
      DEBUG_DEV.println(index, HEX);
      #ifdef DEBUG_VERBOSE
        DEBUG_DEV.println("----------- START PIN CONFIG DUMP ------------");
        DEBUG_DEV.print("pinID=");
        DEBUG_DEV.println(pin.pinID);
        DEBUG_DEV.print("pinInitialState=");
        DEBUG_DEV.println(pin.pinInitialState);
        DEBUG_DEV.print("pinConnectedState=");
        DEBUG_DEV.println(pin.pinConnectedState);    
        DEBUG_DEV.print("pinDisconnectedState=");
        DEBUG_DEV.println(pin.pinDisconnectedState); 
        DEBUG_DEV.print("debounce=");
        DEBUG_DEV.println(pin.debounce); 
        DEBUG_DEV.print("inputPullup=");
        DEBUG_DEV.println(pin.inputPullup); 
        DEBUG_DEV.print("logicalID=");
        DEBUG_DEV.println(pin.logicalID); 
        DEBUG_DEV.print("pinCurrentState=");
        DEBUG_DEV.println(pin.pinCurrentState); 
        DEBUG_DEV.print("t=");
        DEBUG_DEV.println(pin.t);
        DEBUG_DEV.println("----------- END PIN CONFIG DUMP ------------");
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
      DEBUG_DEV.print(F("ConfigManager::initDigialInputPins, Size=0x"));
      DEBUG_DEV.println(size, HEX);
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
        //DEBUG_DEV.print("NOT READY PIN = ");
        //DEBUG_DEV.println(x);
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
      DEBUG_DEV.print(F("ConfigManager::SetDigitalOutputPin, Index=0x"));
      DEBUG_DEV.println(index, HEX);
      #ifdef DEBUG_VERBOSE
        DEBUG_DEV.println("----------- START PIN CONFIG DUMP ------------");
        DEBUG_DEV.print("pinID=");
        DEBUG_DEV.println(pin.pinID);
        DEBUG_DEV.print("pinInitialState=");
        DEBUG_DEV.println(pin.pinInitialState);
        DEBUG_DEV.print("pinConnectedState=");
        DEBUG_DEV.println(pin.pinConnectedState);    
        DEBUG_DEV.print("pinDisconnectedState=");
        DEBUG_DEV.println(pin.pinDisconnectedState); 
        DEBUG_DEV.print("debounce=");
        DEBUG_DEV.println(pin.debounce); 
        DEBUG_DEV.print("inputPullup=");
        DEBUG_DEV.println(pin.inputPullup); 
        DEBUG_DEV.print("logicalID=");
        DEBUG_DEV.println(pin.logicalID); 
        DEBUG_DEV.print("pinCurrentState=");
        DEBUG_DEV.println(pin.pinCurrentState); 
        DEBUG_DEV.print("t=");
        DEBUG_DEV.println(pin.t);
        DEBUG_DEV.println("----------- END PIN CONFIG DUMP ------------");
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
      DEBUG_DEV.print(F("ConfigManager::InitDigitalOutputPins, Size=0x"));
      DEBUG_DEV.println(size, HEX);
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
        //DEBUG_DEV.print("NOT READY PIN = ");
        //DEBUG_DEV.println(x);
        return 0;
      }
    }
    doutput_arr_ready = 1;
    return 1;
  }
#endif
}
#endif