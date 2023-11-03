#ifndef CONNECTION_H_
#define CONNECTION_H_
#include "Protocol.h"

enum ConnectionState
{
  CS_DISCONNECTED = 0,
  CS_CONNECTING,
  CS_CONNECTED,
  CS_DISCONNECTING,
  CS_ERROR
};

class ConnectionBase {
public:
  ConnectionBase(uint32_t retryPeriod, uint64_t& fm) : _retryPeriod(retryPeriod), _featureMap(fm)
  {

  }
  virtual uint8_t onInit();
  virtual void onConnect();
  virtual void onDisconnect();
  virtual void onError();
  virtual void onDoWork();

protected:
  #ifdef DEBUG

  String IpAddress2String(const IPAddress& ipAddress)
  {
    return String(ipAddress[0]) + String(".") +\
    String(ipAddress[1]) + String(".") +\
    String(ipAddress[2]) + String(".") +\
    String(ipAddress[3]); 
  }
  String stateToString(int& state)
  {
    switch(state)
    {
      case CS_DISCONNECTED:
        return String("UDP_DISCONNECTED");
      case CS_CONNECTING:
        return String("UDP_CONNECTING");
      case CS_CONNECTED:
        return String("UDP_CONNECTED");
      case CS_DISCONNECTING:
        return String("UDP_DISCONNECTING");
      case CS_ERROR:
        return String("UDP_ERROR");
      default:
        return String("UDP_UNKNOWN_STATE");
    }
  }
  #endif
  void setState(int new_state)
  {
    #ifdef DEBUG
      Serial.print("DEBUG: Connection transitioning from current state of [");
      Serial.print(this->stateToString(this->_myState));
      Serial.print("] to [");
      Serial.print(this->stateToString(new_state));
      Serial.println("]");
    #endif
    this->_myState = new_state;
  }

  uint32_t _retryPeriod = 0;
  uint32_t _timeNow = 0;
  uint8_t _initialized = false;
  uint64_t& _featureMap;
  int _myState = CS_DISCONNECTED;
  char _rxBuffer[RX_BUFFER_SIZE];

  // Message allocations
  HandshakeMessage _hm;
};
#endif