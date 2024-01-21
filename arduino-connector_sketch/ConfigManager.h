#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_
#include "Config.h"
//#include <ArxContainer.h>

#if defined(DINPUTS) || defined(DOUTPUTS)
struct dpin
{
    String pinName;
    String pinType;
    String pinID;
    int8_t pinInitialState;
    int8_t pinConnectState;
    int8_t pinDisconnectState;
    String halPinDirection;
    uint16_t debounce;
    uint8_t inputPullup;
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
    dinput_arr[index] = pin;
  }
  void clearDigitalInputPins()
  {
    if( dinput_arr != NULL )
    {
      delete[] dinput_arr;
    }
    dinput_arr = NULL;
  }
  void initDigitalInputPins(size_t size)
  {
    if( dinput_arr != NULL )
    {
      delete[] dinput_arr;
    }
    dinput_arr = new dpin[size];
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
#endif

#ifdef DOUTPUTS
  //dpin * dout_storage_array = NULL; //[ELEMENT_COUNT_MAX];
  Vector<dpin> doutput_arr; //(dout_storage_array);
  //Array<dpin,ELEMENT_COUNT_MAX>  doutput_arr;
#endif



};
#endif