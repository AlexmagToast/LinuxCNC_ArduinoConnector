#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_
#include "Config.h"
#include <Vector.h>

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

  
private:



}
#endif