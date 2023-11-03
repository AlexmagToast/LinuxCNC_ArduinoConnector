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

  virtual void sendHandshakeMessage();

  void DoWork()
  {
    if( _initialized == false)
    {
      this->onInit();
      delay(1000);
      _initialized = true;
    }

    #if DHCP == 1
      if(millis() > this->_dhcpMaintTimer + _retryPeriod)
      {
        do_dhcp_maint();
        this->_dhcpMaintTimer = millis();
      }
    #endif
    

    switch(_myState)
    {
      case CS_DISCONNECTED:
      {
        this->setState(CS_CONNECTING);
        this->sendHandshakeMessage();
        
        this->_timeNow = millis();
        /*
        #ifdef DEBUG
          Serial.print("DEBUG: UDP disconnected, retrying connection to ");
          Serial.print(this->IpAddress2String(this->_serverIP));
          Serial.print(":");
          Serial.print(this->_txPort);
          Serial.print(" in ");
          Serial.print(_retryPeriod/1000);
          Serial.println(" seconds...");
        #endif
        */
        break;
      }
      case CS_CONNECTING:
      {
        //Serial.println(_retryPeriod);
        if(millis() > this->_timeNow + _retryPeriod)
        {
          // TIMED OUT
          this->setState(CS_ERROR);
          /*
          if(!this->Connect())
          {
           
            return 1;
          }
          else
          {
           this->setState(UDP_CONNECTED);
          }
          */
        }
        break;
      }
      case CS_CONNECTED:
      {
        /*
          int packetSize = _udpClient.parsePacket();
          if (packetSize) {
            Serial.print("Received packet of size ");
            Serial.println(packetSize);
            Serial.print("From ");
            IPAddress remote = _udpClient.remoteIP();
            for (int i=0; i < 4; i++) {
              Serial.print(remote[i], DEC);
              if (i < 3) {
                Serial.print(".");
              }
            }
            Serial.print(", port ");
            Serial.println(_udpClient.remotePort());
          }
          */
          break;
      }
      case CS_ERROR:
      {
        this->setState(CS_DISCONNECTED);
        //return 0;
      }
    }
    this->onDoWork();
        // must be called to trigger callback and publish data
    //MsgPacketizer::update();
  }
  

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
        return String("CS_DISCONNECTED");
      case CS_CONNECTING:
        return String("CS_CONNECTING");
      case CS_CONNECTED:
        return String("CS_CONNECTED");
      case CS_DISCONNECTING:
        return String("CS_DISCONNECTING");
      case CS_ERROR:
        return String("CS_ERROR");
      default:
        return String("CS_UNKNOWN_STATE");
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