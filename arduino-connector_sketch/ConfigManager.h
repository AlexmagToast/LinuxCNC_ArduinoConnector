#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_
#include "Config.h"
//#include <ArxContainer.h>

#if defined(DINPUTS) || defined(DOUTPUTS)
struct dpin
{
    String pinID;
    int8_t pinInitialState;
    int8_t pinConnectState;
    int8_t pinDisconnectState;
    uint16_t debounce;
    uint8_t inputPullup;
    uint8_t logicalID;
    int8_t pinCurrentState;
    unsigned long t;
};

//const int ELEMENT_COUNT_MAX = 30;
//int storage_array[ELEMENT_COUNT_MAX];
//Vector<int> vector(storage_array);
//vector.push_back(77);


//const int ELEMENT_COUNT_MAX = 15;
//typedef Array<dpin,ELEMENT_COUNT_MAX> dpin_array_t;
//typedef Vector<dpin> dpin_array_t(storage_array);
#endif


class ConfigManager {
public:
  ConfigManager()
  {

  }
  ~ConfigManager()
  {

  }
#ifdef DINPUTS
  void setDigitalInputPin(dpin pin, uint8_t index)
  {
    #ifdef DEBUG
      Serial.print("ConfigManager::setDigitalInputPin, Index=0x");
      Serial.println(index, HEX);
      #ifdef DEBUG_VERBOSE
        Serial.println("----------- START PIN CONFIG DUMP ------------");
        Serial.print("pinID=");
        Serial.println(pin.pinID);
        Serial.print("pinInitialState=");
        Serial.println(pin.pinInitialState);
        Serial.print("pinConnectState=");
        Serial.println(pin.pinConnectState);    
        Serial.print("pinDisconnectState=");
        Serial.println(pin.pinDisconnectState); 
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
  void clearDigitalInputPins()
  {
    if( dinput_arr != NULL )
    {
      delete[] dinput_arr;
      dinput_arr_len = 0;
      dinput_arr_ready = 0;
    }
    dinput_arr = NULL;
  }
  void initDigitalInputPins(size_t size)
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

  dpin * getDigitalInputPins()
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
      dpin d = getDigitalInputPins()[x];
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
  
private:
#ifdef DINPUTS
  //dpin * din_storage_array = NULL; //[ELEMENT_COUNT_MAX];
  //Vector<dpin> dinput_arr; //(din_storage_array);
  //Array<dpin,ELEMENT_COUNT_MAX>  dinput_arr;
  //std::map<String, dpin> dinput_arr;
  //MsgPack::map_t<String, dpin> dinput_arr;
  dpin * dinput_arr = NULL;//new dpin[100];// = NULL;
  size_t dinput_arr_len = 0;
  uint8_t dinput_arr_ready = 0;
#endif

#ifdef DOUTPUTS
  //dpin * dout_storage_array = NULL; //[ELEMENT_COUNT_MAX];
  Vector<dpin> doutput_arr; //(dout_storage_array);
  //Array<dpin,ELEMENT_COUNT_MAX>  doutput_arr;
#endif



};
#endif