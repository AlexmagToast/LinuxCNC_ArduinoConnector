#ifndef CONNECTION_H_
#define CONNECTION_H_
#pragma once
#include "Protocol.h"


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
    //SERIAL_DEV.println("SENDING PIN MESSAGE!");
    
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
  /*
  String IpAddress2String(const IPAddress& ipAddress)
  {
    return String(ipAddress[0]) + String(".") +\
    String(ipAddress[1]) + String(".") +\
    String(ipAddress[2]) + String(".") +\
    String(ipAddress[3]); 
  }
  */
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
      Serial1.println("GOT HS MESSAGE");
      Serial1.flush();
      #ifdef DEBUG_VERBOSE
      SERIAL_DEV.println(" ---- RX HANDSHAKE MESSAGE DUMP ----");
      SERIAL_DEV.print(" Protocol Version: 0x");
      SERIAL_DEV.println(n.protocolVersion, HEX);
      SERIAL_DEV.print(" Profile Signature: ");
      SERIAL_DEV.println(n.profileSignature);
      //SERIAL_DEV.print(" Board Index: ");
      //SERIAL_DEV.println(n.boardIndex);
      SERIAL_DEV.println(" ---- RX END HANDSHAKE MESSAGE DUMP ----");
      #endif
      _handshakeReceived = 1;
  }

  void _onHeartbeatMessage(const protocol::HeartbeatMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      SERIAL_DEV.println(" ---- RX HEARTBEAT MESSAGE DUMP ----");
      //SERIAL_DEV.print(" Board Index: ");
      //SERIAL_DEV.println(n.boardIndex);
      SERIAL_DEV.println(" ---- RX END HEARTBEAT MESSAGE DUMP ----");
      #endif
      _heartbeatReceived = 1;
  }

  /*
  void _onCommandMessage(const protocol::CommandMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      SERIAL_DEV.println(" ---- RX COMMAND MESSAGE DUMP ----");
      SERIAL_DEV.print(" Command: ");
      SERIAL_DEV.println(n.cmd);
      //SERIAL_DEV.print(" Board Index: ");
      //SERIAL_DEV.println(n.boardIndex);
      SERIAL_DEV.println(" ---- RX END COMMAND MESSAGE DUMP ----");
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
      SERIAL_DEV.println(" ---- RX PINCHANGE MESSAGE DUMP ----");
      SERIAL_DEV.print(" FEATURE ID: ");
      SERIAL_DEV.println(n.featureID);  
      SERIAL_DEV.print(" SEQ ID: ");
      SERIAL_DEV.println(n.seqID);  
      SERIAL_DEV.print(" RESPONSE REQ: ");
      SERIAL_DEV.println(n.responseReq);       
      SERIAL_DEV.print(" MESSAGE: ");
      SERIAL_DEV.println(n.message);  
      SERIAL_DEV.println(" ---- RX END PINCHANGE MESSAGE DUMP ----");
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
      SERIAL_DEV.flush();
      SERIAL_DEV.print(" Connection transitioning from current state of [");
      SERIAL_DEV.print(this->stateToString(this->_myState));
      SERIAL_DEV.print("] to [");
      SERIAL_DEV.print(this->stateToString(new_state));
      SERIAL_DEV.println("]");
      SERIAL_DEV.flush();
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
      SERIAL_DEV.println(" ---- TX HANDSHAKE MESSAGE DUMP ----");
      SERIAL_DEV.print(" Protocol Version: 0x");
      SERIAL_DEV.println(protocol::hm.protocolVersion, HEX);
      //SERIAL_DEV.print(" Feature Map: 0x");
      //SERIAL_DEV.println(protocol::hm.featureMap, HEX);
      SERIAL_DEV.print(" Timeout: ");
      SERIAL_DEV.println(protocol::hm.timeout);
      //SERIAL_DEV.print(" MaxMsgSize: ");
      //SERIAL_DEV.println(protocol::hm.maxMsgSize);
      SERIAL_DEV.print(" ProfileSignature: ");
      SERIAL_DEV.println(protocol::hm.profileSignature);
      //SERIAL_DEV.print(" Board Index: ");
     //SERIAL_DEV.println(protocol::hm.boardIndex);
      SERIAL_DEV.print(" Board UID: ");
      SERIAL_DEV.println(protocol::hm.uid);
      SERIAL_DEV.println(" ---- TX END HANDSHAKE MESSAGE DUMP ----");
    #endif
    return protocol::hm;
  }

  protocol::HeartbeatMessage& _getHeartbeatMessage()
  {
    #ifdef DEBUG_VERBOSE
      SERIAL_DEV.println(" ---- TX HEARTBEAT MESSAGE DUMP ----");
      SERIAL_DEV.print(" Board Index: ");
      SERIAL_DEV.println(protocol::hb.boardIndex);
      SERIAL_DEV.println(" ---- TX END HEARTBEAT MESSAGE DUMP ----");
    #endif
    return protocol::hb;
  }

  protocol::PinChangeMessage& _getPinChangeMessage()
  {
    #ifdef DEBUG_VERBOSE
      SERIAL_DEV.println(" ---- TX PINCHANGE MESSAGE DUMP ----");
      SERIAL_DEV.print(" FEATURE ID: ");
      SERIAL_DEV.println(protocol::pcm.featureID);  
      SERIAL_DEV.print(" ACK REQ: ");
      SERIAL_DEV.println(protocol::pcm.responseReq);       
      SERIAL_DEV.print(" MESSAGE: ");
      SERIAL_DEV.println(protocol::pcm.message);  
      SERIAL_DEV.println(" ---- TX END PINCHANGE MESSAGE DUMP ----");
    #endif
    return protocol::pcm;
  }
/*
  protocol::PinStatusMessage& _getPinStatusMessage()
  {
    #ifdef DEBUG_VERBOSE
      SERIAL_DEV.println(" ---- TX PINSTATUS MESSAGE DUMP ----");
      SERIAL_DEV.print(" STATUS: ");
      SERIAL_DEV.println(protocol::pm.status);      
      //SERIAL_DEV.print(" Board Index: ");
      //SERIAL_DEV.println(protocol::pm.boardIndex);
      SERIAL_DEV.println(" ---- TX END PINSTATUS MESSAGE DUMP ----");
    #endif
    return protocol::pm;
  }
*/
  #ifdef DEBUG
  protocol::DebugMessage& _getDebugMessage(String& message)
  {
    protocol::dm.message = message;
    // No need to wrap in DEBUG define as the entire method is only compiled in when DEBUG is defined
    SERIAL_DEV.println(" ---- TX DEBUG MESSAGE DUMP ----");
    //SERIAL_DEV.print(" Board Index: ");
    //SERIAL_DEV.println(protocol::dm.boardIndex);
    SERIAL_DEV.print(" Message: ");
    SERIAL_DEV.println(protocol::dm.message);
    SERIAL_DEV.println(" ---- TX END DEBUG MESSAGE DUMP ----");
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