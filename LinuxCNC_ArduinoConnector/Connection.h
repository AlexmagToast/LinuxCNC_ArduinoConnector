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
  ConnectionBase(uint16_t retryPeriod, uint64_t& fm) : _retryPeriod(retryPeriod), _featureMap(fm)
  {

  }
  virtual uint8_t onInit();
  virtual void onConnect();
  virtual void onDisconnect();
  virtual void onError();
  virtual void onDoWork();

  virtual void sendHandshakeMessage();

  virtual void sendHeartbeatMessage();

  virtual void sendMessage()
  {

  }

  uint8_t getHandshakeReceived()
  {
    uint8_t r = handshakeReceived;
    if (handshakeReceived)
      handshakeReceived = 0;
    return r;
  }

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
        if(getHandshakeReceived())
        {
          this->setState(CS_CONNECTED);
          this->_timeNow = millis();
        }
        else if(millis() > this->_timeNow + _retryPeriod)
        {
          // TIMED OUT
          this->setState(CS_ERROR);

        }
        break;
      }
      case CS_CONNECTED:
      {
        if ( millis() > this->_timeNow + _heartbeatPeriod )
        {
          sendHeartbeatMessage();
          this->_timeNow = millis();
        }
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

  void onHandshakeMessage(const protocol::HandshakeMessage& n)
  {
      #ifdef DEBUG
      Serial.println("DEBUG: ---- RX HANDSHAKE MESSAGE DUMP ----");
      Serial.print("DEBUG: Protocol Version: 0x");
      Serial.println(n.protocolVersion, HEX);
      Serial.print("DEBUG: Feature Map: 0x");
      Serial.println(n.featureMap, HEX);
      Serial.print("DEBUG: Board Index: ");
      Serial.println(n.boardIndex);
      Serial.println("DEBUG: ---- RX END HANDSHAKE MESSAGE DUMP ----");
      #endif
      handshakeReceived = 1;
  }
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

  protocol::HandshakeMessage& getHandshakeMessage()
  {
    _hm.featureMap = this->_featureMap;
    #ifdef DEBUG
      Serial.println("DEBUG: ---- TX HANDSHAKE MESSAGE DUMP ----");
      Serial.print("DEBUG: Protocol Version: 0x");
      Serial.println(_hm.protocolVersion, HEX);
      Serial.print("DEBUG: Feature Map: 0x");
      Serial.println(_hm.featureMap, HEX);
      Serial.print("DEBUG: Board Index: ");
      Serial.println(_hm.boardIndex);
      Serial.println("DEBUG: ---- TX END HANDSHAKE MESSAGE DUMP ----");
    #endif
    return _hm;
  }

  protocol::HeartbeatMessage& getHeartbeatMessage()
  {
    #ifdef DEBUG
      Serial.println("DEBUG: ---- TX HEARTBEAT MESSAGE DUMP ----");
      Serial.print("DEBUG: Board Index: ");
      Serial.println(_hb.boardIndex);
      Serial.println("DEBUG: ---- TX END HEARTBEAT MESSAGE DUMP ----");
    #endif
    return _hb;
  }

  uint16_t _retryPeriod = 0;
  uint32_t _timeNow = 0;
  uint8_t _initialized = false;
  uint64_t& _featureMap;
  int _myState = CS_DISCONNECTED;
  char _rxBuffer[RX_BUFFER_SIZE];
  uint8_t handshakeReceived = 0;
  uint16_t _heartbeatPeriod = 3000;

  // Message allocations
  protocol::HandshakeMessage _hm;
  protocol::HeartbeatMessage _hb;
};
#endif