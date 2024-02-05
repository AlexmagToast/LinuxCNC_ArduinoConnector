#ifndef CONNECTION_H_
#define CONNECTION_H_
#pragma once
#include "Protocol.h"
#include <ArduinoJson.h>


enum ConnectionState
{
  CS_DISCONNECTED = 0,
  CS_CONNECTING,
  CS_CONNECTED,
  CS_RECONNECTED,
  CS_DISCONNECTING,
  CS_CONNECTION_TIMEOUT,
  CS_ERROR
};

class ConnectionBase {

  using m_cmcb = void (*)(const protocol::ConfigMessage&);
  using m_pcmcb = void (*)(const protocol::PinChangeMessage&);
  using m_cscb = void (*)(int);

public:
  ConnectionBase(uint16_t retryPeriod, uint64_t& fm) : _retryPeriod(retryPeriod), _featureMap(fm)
  {

  }

  void RegisterConfigCallback(m_cmcb act)
  {
    _configAction = act;
  
  }

  void RegisterPinChangeCallback(m_pcmcb act)
  {
    _pinChangeAction = act;
  }

  void RegisterCSCallback(m_cscb act)
  {
    _csAction = act;
  }

  virtual ~ConnectionBase(){}

  #ifdef DEBUG
  virtual void SendDebugMessage(String& message)
  {
    _sendDebugMessage(message);
  }
  #endif

  int& GetState()
  {
    return _myState;
  }

  void setUID(const char* uid)
  {
    _uid = uid;
  }

/*
  virtual void SendPinStatusMessage(char sig, int pin, int state)
  {
    String status = String(sig);
    status += String(pin);
    status += ":";
    status += String(state);
    protocol::pm.status = status;
    protocol::pm.status += " ";
    _sendPinStatusMessage();
  }
*/
  virtual void SendPinChangeMessage(uint8_t& featureID, uint8_t& seqID, uint8_t& responseReq, String& message)
  {
    //Serial.println("SENDING PIN MESSAGE!");
    
    protocol::pcm.featureID = featureID;
    protocol::pcm.responseReq = responseReq;
    protocol::pcm.message = message;
    protocol::pcm.seqID = seqID;
    
    _sendPinChangeMessage();
    
  }


  void DoWork()
  {
    if( _initialized == false)
    {
      this->_onInit();
      _initialized = true;
    }

    switch(_myState)
    {
      case CS_CONNECTION_TIMEOUT:
      case CS_DISCONNECTED:
      {
        this->_setState(CS_CONNECTING);
        this->_sendHandshakeMessage();
        _resendTimer = millis();
        _receiveTimer = millis(); 
        _handshakeReceived = 0;
        _heartbeatReceived = 0;
        _commandReceived = 0;
        break;
      }
      case CS_CONNECTING:
      {
        if(_getHandshakeReceived())
        {
          this->_setState(CS_CONNECTED);
          _resendTimer = millis();
          _receiveTimer = millis();
          return;
        }
        else if(millis() > this->_resendTimer + _retryPeriod)
        {
          // TIMED OUT
          this->_setState(CS_ERROR);
          return;
        }
        break;
      }
      case CS_RECONNECTED:
         this->_setState(CS_CONNECTED);
         return;
      case CS_CONNECTED:
      {
        if(_getHandshakeReceived())
        {
          this->_setState(CS_RECONNECTED);
          _resendTimer = millis();
          _receiveTimer = millis();
          return;
        }
        else if ( millis() > this->_resendTimer + (_retryPeriod) )
        {
          _sendHeartbeatMessage();
          _resendTimer = millis();
        }
        else if ( _getHeartbeatReceived() ) // Heartbeat received, reset timeout
        {
          _receiveTimer = millis();
        }
        else if ( millis() > this->_receiveTimer + (_retryPeriod*2) )
        {
          this->_setState(CS_CONNECTION_TIMEOUT);
          return;
        }

        break;
      }
      case CS_ERROR:
      {
        this->_setState(CS_DISCONNECTED);
        return;
      }
    }
    this->_onDoWork();
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
      case CS_RECONNECTED:
        return String("CS_RECONNECTED");
      case CS_DISCONNECTING:
        return String("CS_DISCONNECTING");
      case CS_ERROR:
        return String("CS_ERROR");
      case CS_CONNECTION_TIMEOUT:
        return String("CS_CONNECTION_TIMEOUT");
      default:
        return String("CS_UNKNOWN_STATE");
    }
  }
  #endif
/*
  uint8_t CommandReceived()
  {
    return _commandReceived;
  }

  const protocol::CommandMessage& GetReceivedCommand()
  {
    uint8_t r = _commandReceived;
    if (_commandReceived)
      _commandReceived = 0; // Clear flag.  Crude, but works.
    return protocol::cm;
  }
*/
protected:
  virtual uint8_t _onInit();
  virtual void _onConnect();
  virtual void _onDisconnect();
  virtual void _onError();
  virtual void _onDoWork();


  virtual void _sendHandshakeMessage();

  virtual void _sendHeartbeatMessage();

  virtual void _sendPinChangeMessage();
                //_sendPinChangeMessage
  #ifdef DEBUG
  virtual void _sendDebugMessage(String& message);
  #endif

  void _onHandshakeMessage(const protocol::HandshakeMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- RX HANDSHAKE MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Protocol Version: 0x");
      Serial.println(n.protocolVersion, HEX);
      Serial.print("ARDUINO DEBUG: Profile Signature: ");
      Serial.println(n.profileSignature);
      //Serial.print("ARDUINO DEBUG: Board Index: ");
      //Serial.println(n.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- RX END HANDSHAKE MESSAGE DUMP ----");
      #endif
      _handshakeReceived = 1;
  }

  void _onHeartbeatMessage(const protocol::HeartbeatMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- RX HEARTBEAT MESSAGE DUMP ----");
      //Serial.print("ARDUINO DEBUG: Board Index: ");
      //Serial.println(n.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- RX END HEARTBEAT MESSAGE DUMP ----");
      #endif
      _heartbeatReceived = 1;
  }

  /*
  void _onCommandMessage(const protocol::CommandMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- RX COMMAND MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Command: ");
      Serial.println(n.cmd);
      //Serial.print("ARDUINO DEBUG: Board Index: ");
      //Serial.println(n.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- RX END COMMAND MESSAGE DUMP ----");
      #endif
      protocol::cm.cmd = n.cmd;
      //protocol::cm.boardIndex = n.boardIndex-1;
      _commandReceived = 1;
  }
  */
  void _onConfigMessage(const protocol::ConfigMessage& n)
  {
      if(_configAction != NULL)
      {
        _configAction(n);
      }
      _receiveTimer = millis(); // Don't let the heartbeat timeout elapse just because the arduino is busy processing config
  }
  void _onPinChangeMessage(const protocol::PinChangeMessage& n)
  {
    #ifdef DEBUG_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- RX PINCHANGE MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: FEATURE ID: ");
      Serial.println(n.featureID);  
      Serial.print("ARDUINO DEBUG: SEQ ID: ");
      Serial.println(n.seqID);  
      Serial.print("ARDUINO DEBUG: RESPONSE REQ: ");
      Serial.println(n.responseReq);       
      Serial.print("ARDUINO DEBUG: MESSAGE: ");
      Serial.println(n.message);  
      Serial.println("ARDUINO DEBUG: ---- RX END PINCHANGE MESSAGE DUMP ----");
    #endif
      if(_pinChangeAction != NULL)
      {
        _pinChangeAction(n);
      }
      _receiveTimer = millis(); // Don't let the heartbeat timeout elapse just because the arduino is busy processing messages like this one
  }
  void _setState(int new_state)
  {
    #ifdef DEBUG
      Serial.flush();
      Serial.print("ARDUINO DEBUG: Connection transitioning from current state of [");
      Serial.print(this->stateToString(this->_myState));
      Serial.print("] to [");
      Serial.print(this->stateToString(new_state));
      Serial.println("]");
      Serial.flush();
    #endif
    this->_myState = new_state;
    if( this->_csAction != NULL )
    {
      this->_csAction(new_state);
    }
  }

  protocol::HandshakeMessage& _getHandshakeMessage()
  {
    protocol::hm.featureMap = this->_featureMap;
    protocol::hm.timeout = _retryPeriod * 2;

    protocol::hm.uid = _uid;
    #ifdef DEBUG_VERBOSE   
      Serial.println("ARDUINO DEBUG: ---- TX HANDSHAKE MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Protocol Version: 0x");
      Serial.println(protocol::hm.protocolVersion, HEX);
      //Serial.print("ARDUINO DEBUG: Feature Map: 0x");
      //Serial.println(protocol::hm.featureMap, HEX);
      Serial.print("ARDUINO DEBUG: Timeout: ");
      Serial.println(protocol::hm.timeout);
      //Serial.print("ARDUINO DEBUG: MaxMsgSize: ");
      //Serial.println(protocol::hm.maxMsgSize);
      Serial.print("ARDUINO DEBUG: ProfileSignature: ");
      Serial.println(protocol::hm.profileSignature);
      //Serial.print("ARDUINO DEBUG: Board Index: ");
     //Serial.println(protocol::hm.boardIndex);
      Serial.print("ARDUINO DEBUG: Board UID: ");
      Serial.println(protocol::hm.uid);
      Serial.println("ARDUINO DEBUG: ---- TX END HANDSHAKE MESSAGE DUMP ----");
    #endif
    return protocol::hm;
  }

  protocol::HeartbeatMessage& _getHeartbeatMessage()
  {
    #ifdef DEBUG_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- TX HEARTBEAT MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Board Index: ");
      Serial.println(protocol::hb.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- TX END HEARTBEAT MESSAGE DUMP ----");
    #endif
    return protocol::hb;
  }

  protocol::PinChangeMessage& _getPinChangeMessage()
  {
    #ifdef DEBUG_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- TX PINCHANGE MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: FEATURE ID: ");
      Serial.println(protocol::pcm.featureID);  
      Serial.print("ARDUINO DEBUG: ACK REQ: ");
      Serial.println(protocol::pcm.responseReq);       
      Serial.print("ARDUINO DEBUG: MESSAGE: ");
      Serial.println(protocol::pcm.message);  
      Serial.println("ARDUINO DEBUG: ---- TX END PINCHANGE MESSAGE DUMP ----");
    #endif
    return protocol::pcm;
  }
/*
  protocol::PinStatusMessage& _getPinStatusMessage()
  {
    #ifdef DEBUG_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- TX PINSTATUS MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: STATUS: ");
      Serial.println(protocol::pm.status);      
      //Serial.print("ARDUINO DEBUG: Board Index: ");
      //Serial.println(protocol::pm.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- TX END PINSTATUS MESSAGE DUMP ----");
    #endif
    return protocol::pm;
  }
*/
  #ifdef DEBUG
  protocol::DebugMessage& _getDebugMessage(String& message)
  {
    protocol::dm.message = message;
    // No need to wrap in DEBUG define as the entire method is only compiled in when DEBUG is defined
    Serial.println("ARDUINO DEBUG: ---- TX DEBUG MESSAGE DUMP ----");
    //Serial.print("ARDUINO DEBUG: Board Index: ");
    //Serial.println(protocol::dm.boardIndex);
    Serial.print("ARDUINO DEBUG: Message: ");
    Serial.println(protocol::dm.message);
    Serial.println("ARDUINO DEBUG: ---- TX END DEBUG MESSAGE DUMP ----");
    return protocol::dm;
  }
  #endif
  

  uint8_t _getHandshakeReceived()
  {
    uint8_t r = _handshakeReceived;
    if (_handshakeReceived)
      _handshakeReceived = 0;
    return r;
  }
  uint8_t _getHeartbeatReceived()
  {
    uint8_t r = _heartbeatReceived;
    if (_heartbeatReceived)
      _heartbeatReceived = 0;
    return r;
  }

  
  uint16_t _retryPeriod = 0;
  uint32_t _resendTimer = 0;
  uint32_t _receiveTimer = 0;
  uint8_t _initialized = false;
  uint32_t _featureMap = 1;
  int _myState = CS_DISCONNECTED;
  //char _rxBuffer[RX_BUFFER_SIZE];
  //byte _txBuffer[RX_BUFFER_SIZE];
  // Flip-flops used to signal the presence of received message types
  uint8_t _handshakeReceived = 0;
  uint8_t _heartbeatReceived = 0;
  uint8_t _commandReceived = 0;
  const char * _uid;
  
  private:
    m_cmcb _configAction = NULL;
    m_cscb _csAction = NULL;
    m_pcmcb _pinChangeAction = NULL;


};
#endif