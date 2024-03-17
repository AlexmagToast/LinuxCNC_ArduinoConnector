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
#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_
#include "Config.h"

namespace ConfigManager
{
  #if defined(AINPUTS) || defined(AOUTPUTS)
  struct apin
  {
      String pinID;
      int pinInitialState;
      int pinConnectedState;
      int pinDisconnectedState;
      uint16_t pinSmoothing;
      int16_t pinMaxValue;
      int16_t pinMinValue;
      uint8_t logicalID;
      int pinCurrentState;
      unsigned long t;
  };
  #endif
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
        DEBUG_DEV.println("----------- START DPIN CONFIG DUMP ------------");
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
        DEBUG_DEV.println("----------- END DPIN CONFIG DUMP ------------");
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
        DEBUG_DEV.println("----------- START DPIN CONFIG DUMP ------------");
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
        DEBUG_DEV.println("----------- END DPIN CONFIG DUMP ------------");
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

#ifdef AINPUTS
    apin * ainput_arr = NULL;//new dpin[100];// = NULL;
    size_t ainput_arr_len = 0;
    uint8_t ainput_arr_ready = 0;

  void SetAnalogInputPin(apin pin, uint8_t index)
  {
    #ifdef DEBUG
      DEBUG_DEV.print(F("ConfigManager::SetAnalogInputPin, Index=0x"));
      DEBUG_DEV.println(index, HEX);
      #ifdef DEBUG_VERBOSE
        DEBUG_DEV.println("----------- START APIN CONFIG DUMP ------------");
        DEBUG_DEV.print("pinID=");
        DEBUG_DEV.println(pin.pinID);
        DEBUG_DEV.print("pinInitialState=");
        DEBUG_DEV.println(pin.pinInitialState);
        DEBUG_DEV.print("pinConnectedState=");
        DEBUG_DEV.println(pin.pinConnectedState);    
        DEBUG_DEV.print("pinDisconnectedState=");
        DEBUG_DEV.println(pin.pinDisconnectedState); 
        DEBUG_DEV.print("pinSmoothing=");
        DEBUG_DEV.println(pin.pinSmoothing); 
        DEBUG_DEV.print("pinMaxValue=");
        DEBUG_DEV.println(pin.pinMaxValue); 
        DEBUG_DEV.print("pinMinValue=");
        DEBUG_DEV.println(pin.pinMinValue); 
        DEBUG_DEV.print("logicalID=");
        DEBUG_DEV.println(pin.logicalID); 
        DEBUG_DEV.print("pinCurrentState=");
        DEBUG_DEV.println(pin.pinCurrentState); 
        DEBUG_DEV.print("t=");
        DEBUG_DEV.println(pin.t);
        DEBUG_DEV.println("----------- END PIN CONFIG DUMP ------------");
      #endif     
    #endif
    ainput_arr[index] = pin;
  }
  void ClearAnalogInputPins()
  {
    if( ainput_arr != NULL )
    {
      delete[] ainput_arr;
      ainput_arr_len = 0;
      ainput_arr_ready = 0;
    }
    ainput_arr = NULL;
  }
  void InitAnalogInputPins(size_t size)
  {
    #ifdef DEBUG
      DEBUG_DEV.print(F("ConfigManager::InitAnalogInputPins, Size=0x"));
      DEBUG_DEV.println(size, HEX);
    #endif
    if( ainput_arr != NULL )
    {
      delete[] ainput_arr;
      ainput_arr_len = 0;
      ainput_arr_ready = 0;
    }
    ainput_arr = new apin[size];
    ainput_arr_len = size;
  }

  apin * GetAnalogInputPins()
  {
    return ainput_arr;
  }

  size_t & GetAnalogInputPinsLen()
  {
    return ainput_arr_len;
  }

  uint8_t GetAnalogInputsReady()
  {
    if(ainput_arr_ready == 1)
      return 1;
    if(ainput_arr == NULL)
    {
      return 0;
    }
    for( int x = 0; x < GetAnalogInputPinsLen(); x++)
    {
      apin apin = GetAnalogInputPins()[x];
      if(apin.pinID.length() == 0)
      {
        //DEBUG_DEV.print("NOT READY PIN = ");
        //DEBUG_DEV.println(x);
        return 0;
      }
    }
    ainput_arr_ready = 1;
    return 1;
  }
#endif

#ifdef AOUTPUTS
  apin * aoutput_arr = NULL;//new dpin[100];// = NULL;
  size_t aoutput_arr_len = 0;
  uint8_t aoutput_arr_ready = 0;

  void SetAnalogOutputPin(apin pin, uint8_t index)
  {
    #ifdef DEBUG
      DEBUG_DEV.print(F("ConfigManager::SetAnalogOutputPin, Index=0x"));
      DEBUG_DEV.println(index, HEX);
      #ifdef DEBUG_VERBOSE
        DEBUG_DEV.println("----------- START DPIN CONFIG DUMP ------------");
        DEBUG_DEV.print("pinID=");
        DEBUG_DEV.println(pin.pinID);
        DEBUG_DEV.print("pinInitialState=");
        DEBUG_DEV.println(pin.pinInitialState);
        DEBUG_DEV.print("pinConnectedState=");
        DEBUG_DEV.println(pin.pinConnectedState);    
        DEBUG_DEV.print("pinDisconnectedState=");
        DEBUG_DEV.println(pin.pinDisconnectedState); 
        DEBUG_DEV.print("pinSmoothing=");
        DEBUG_DEV.println(pin.pinSmoothing); 
        DEBUG_DEV.print("pinMaxValue=");
        DEBUG_DEV.println(pin.pinMaxValue); 
        DEBUG_DEV.print("pinMinValue=");
        DEBUG_DEV.println(pin.pinMinValue); 
        DEBUG_DEV.print("logicalID=");
        DEBUG_DEV.println(pin.logicalID); 
        DEBUG_DEV.print("pinCurrentState=");
        DEBUG_DEV.println(pin.pinCurrentState); 
        DEBUG_DEV.print("t=");
        DEBUG_DEV.println(pin.t);
        DEBUG_DEV.println("----------- END DPIN CONFIG DUMP ------------");
      #endif     
    #endif
    aoutput_arr[index] = pin;
  }
  void ClearAnalogOutputPins()
  {
    if( aoutput_arr != NULL )
    {
      delete[] aoutput_arr;
      aoutput_arr_len = 0;
      aoutput_arr_ready = 0;
    }
    aoutput_arr = NULL;
  }
  void InitAnalogOutputPins(size_t size)
  {
    #ifdef DEBUG
      DEBUG_DEV.print(F("ConfigManager::InitAnalogOutputPins, Size=0x"));
      DEBUG_DEV.println(size, HEX);
    #endif
    if( aoutput_arr != NULL )
    {
      delete[] aoutput_arr;
      aoutput_arr_len = 0;
      aoutput_arr_ready = 0;
    }
    aoutput_arr = new apin[size];
    aoutput_arr_len = size;

  }

  apin * GetAnalogOutputPins()
  {
    return aoutput_arr;
  }

  size_t & GetAnalogOutputPinsLen()
  {
    return aoutput_arr_len;
  }

  uint8_t GetAnalogOutputsReady()
  {
    if(aoutput_arr_ready == 1)
      return 1;
    if(aoutput_arr == NULL)
    {
      return 0;
    }
    for( int x = 0; x < GetAnalogOutputPinsLen(); x++)
    {
      apin a = GetAnalogOutputPins()[x];
      if(a.pinID.length() == 0)
      {
        //DEBUG_DEV.print("NOT READY PIN = ");
        //DEBUG_DEV.println(x);
        return 0;
      }
    }
    aoutput_arr_ready = 1;
    return 1;
  }
#endif

}
#endif