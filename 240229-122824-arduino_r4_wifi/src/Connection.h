#ifndef CONNECTION_H_
#define CONNECTION_H_
#pragma once
#include "Protocol.h"
#ifdef INTEGRATED_CALLBACKS
#include "RXBuffer.h"
#include "Cobs.h"
#include "FeatureMap.h"
#endif


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

#ifdef INTEGRATED_CALLBACKS
class ConnectionBase : public RXBuffer {
#else
class ConnectionBase {
#endif

  using m_cmcb = void (*)(protocol::ConfigMessage&);
  using m_pcmcb = void (*)(const protocol::PinChangeMessage&);
  using m_cscb = void (*)(int);

public:
#ifdef INTEGRATED_CALLBACKS
  ConnectionBase(uint16_t retryPeriod) : RXBuffer(), _retryPeriod(retryPeriod)
  {

  }
  //virtual void onMessage(uint8_t* d, const size_t& size)=0;
#else
  ConnectionBase(uint16_t retryPeriod, uint32_t& fm) : _retryPeriod(retryPeriod)
  {

  }
#endif

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

  virtual void SendMessage( protocol::IMessage& m) = 0;
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
    //DEBUG_DEV.println("SENDING PIN MESSAGE!");
    
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
        return String(F("CS_DISCONNECTED"));
      case CS_CONNECTING:
        return String(F("CS_CONNECTING"));
      case CS_CONNECTED:
        return String(F("CS_CONNECTED"));
      case CS_RECONNECTED:
        return String(F("CS_RECONNECTED"));
      case CS_DISCONNECTING:
        return String(F("CS_DISCONNECTING"));
      case CS_ERROR:
        return String(F("CS_ERROR"));
      case CS_CONNECTION_TIMEOUT:
        return String(F("CS_CONNECTION_TIMEOUT"));
      default:
        return String(F("CS_UNKNOWN_STATE"));
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
      //Serial1.println("GOT HS MESSAGE");
      //Serial1.flush();
      #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(F("-RX HANDSHAKE MESSAGE DUMP-"));
      DEBUG_DEV.print(F("Protocol Version: 0x"));
      DEBUG_DEV.println(n.protocolVersion, HEX);
      DEBUG_DEV.print(F("Profile Signature:"));
      DEBUG_DEV.println(n.profileSignature);
      //DEBUG_DEV.print(" Board Index: ");
      //DEBUG_DEV.println(n.boardIndex);
      DEBUG_DEV.println(F(" - RX END HANDSHAKE MESSAGE DUMP -"));
      #endif
      _handshakeReceived = 1;
  }

  void _onHeartbeatMessage(const protocol::HeartbeatMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(F(" - RX HEARTBEAT MESSAGE DUMP -"));
      //DEBUG_DEV.print(" Board Index: ");
      //DEBUG_DEV.println(n.boardIndex);
      DEBUG_DEV.println(F(" - RX END HEARTBEAT MESSAGE DUMP -"));
      #endif
      _heartbeatReceived = 1;
  }

  /*
  void _onCommandMessage(const protocol::CommandMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(" ---- RX COMMAND MESSAGE DUMP ----");
      DEBUG_DEV.print(" Command: ");
      DEBUG_DEV.println(n.cmd);
      //DEBUG_DEV.print(" Board Index: ");
      //DEBUG_DEV.println(n.boardIndex);
      DEBUG_DEV.println(" ---- RX END COMMAND MESSAGE DUMP ----");
      #endif
      protocol::cm.cmd = n.cmd;
      //protocol::cm.boardIndex = n.boardIndex-1;
      _commandReceived = 1;
  }
  */
  void _onConfigMessage(protocol::ConfigMessage& n)
  {
      if(_configAction != NULL)
      {
        _configAction(n);
      }
      _receiveTimer = millis(); // Don';t let the heartbeat timeout elapse just because the arduino is busy processing config
  }
  void _onPinChangeMessage(const protocol::PinChangeMessage& n)
  {
    #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(F(" - RX PINCHANGE MESSAGE DUMP -"));
      DEBUG_DEV.print(F("FEATURE ID:"));
      DEBUG_DEV.println(n.featureID);  
      DEBUG_DEV.print(F("SEQ ID:"));
      DEBUG_DEV.println(n.seqID);  
      DEBUG_DEV.print(F("RESPONSE REQ:"));
      DEBUG_DEV.println(n.responseReq);       
      DEBUG_DEV.print(F("MESSAGE:"));
      DEBUG_DEV.println(n.message);  
      DEBUG_DEV.println(F(" - RX END PINCHANGE MESSAGE DUMP -"));
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
      //DEBUG_DEV.flush();
      DEBUG_DEV.print(F("Connection transitioning from current state of ["));
      DEBUG_DEV.print(this->stateToString(this->_myState));
      DEBUG_DEV.print(F("] to ["));
      DEBUG_DEV.print(this->stateToString(new_state));
      DEBUG_DEV.println(F("]"));
      DEBUG_DEV.flush();
    #endif
    this->_myState = new_state;
    if( this->_csAction != NULL )
    {
      this->_csAction(new_state);
    }
  }




  #ifdef INTEGRATED_CALLBACKS
  void printBuffer(uint8_t* buffer, size_t size) {
    for (uint8_t i = 0; i < size; i++) {
      if (buffer[i] < 0x10) {
        Serial.print(0);
      }
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
    COM_DEV.flush();
  }

   
  virtual void onMessage(uint8_t* d, const size_t& size)
  {
    JsonDocument doc;
    #ifdef DEBUG_VERBOSE
      COM_DEV.println(F("ENCODED RX="));
      printBuffer(d, size);
    #endif
    size_t sz = cobs::decode(d, size-1);
    #ifdef DEBUG_VERBOSE
      COM_DEV.println(F("DECODED RX="));
      printBuffer(d, sz);
    #endif
  
    DeserializationError error = deserializeMsgPack(doc, (int8_t*)&d[1], sz);
    if (error) {
      #ifdef DEBUG
      DEBUG_DEV.print(F("deserializeJson() failed: "));
      DEBUG_DEV.println(error.f_str());
      COM_DEV.flush();
      #endif
      return;
    }


    uint16_t mt = doc[F("mt")];
    
    #ifdef DEBUG_VERBOSE
      COM_DEV.println(F("JSON RX="));
      serializeJson(doc, DEBUG_DEV);
      COM_DEV.println(F(""));
      COM_DEV.flush();
    #endif
    
    //DEBUG_DEV.println(mt);
    switch(mt)
    {
      case protocol::MessageTypes::MT_HANDSHAKE:
      {
        #ifdef DEBUG
          DEBUG_DEV.println(F("RX MT_HANDSHAKE"));
        #endif

        if(_myState == ConnectionState::CS_CONNECTED)
        {
          // Trigger a reconnect if the python side sends a handshake message when we 'think' we are already connected.
          DEBUG_DEV.println(F("RX MT_HANDSHAKE, RESTARTING CONNECTION LOOP!"));
          this->_setState(CS_DISCONNECTED);
        }
        else
        {
          protocol::HandshakeMessage hmm;
          hmm.fromJSON(doc);
          _onHandshakeMessage(hmm);
        }
        break;
      }
      case protocol::MessageTypes::MT_HEARTBEAT:
      {
        #ifdef DEBUG
          DEBUG_DEV.println(F("RX MT_HEARTBEAT"));
        #endif
        //protocol::HeartbeatMessage h;
        //hmm.fromJSON(doc);
        _onHeartbeatMessage(protocol::hb);
        break;
      }
      case protocol::MessageTypes::MT_CONFIG:
      {
        #ifdef DEBUG
          DEBUG_DEV.println(F("RX MT_CONFIG"));
        #endif
        protocol::ConfigMessage ccf;
        ccf.fromJSON(doc);
        _onConfigMessage(ccf);
        break;
      }
      case protocol::MessageTypes::MT_PINCHANGE:
      {
        #ifdef DEBUG
          DEBUG_DEV.println(F("RX MT_PINCHANGE"));
        #endif
        protocol::PinChangeMessage p;
        p.fromJSON(doc);
        _onPinChangeMessage(p);
        break;
      }
    }
    
  }
  size_t _jsonToMsgPack(JsonDocument& doc, uint8_t * buffer, size_t s)
  {
    size_t sz = serializeMsgPack(doc, (uint8_t*)&buffer[1], s-1);
    //COM_DEV.println(sz);
    sz = cobs::encode(buffer, sz+1);
    buffer[sz] = 0x00;
    return sz+1;
  }

  size_t _getHandshakeMessage(uint8_t * buffer, size_t size)
  {
    protocol::hm.featureMap = fm.features;//this->_featureMap;
    protocol::hm.timeout = _retryPeriod * 2;
    #ifndef INTEGRATED_CALLBACKS_LOWMEMORY
    protocol::hm.uid = _uid;
    #endif
    #ifdef DEBUG_VERBOSE  
    
      DEBUG_DEV.println(F("-TX HANDSHAKE MESSAGE DUMP-"));
      DEBUG_DEV.print(F("Protocol Version: 0x"));
      DEBUG_DEV.println(protocol::hm.protocolVersion, HEX);
      //DEBUG_DEV.print(" Feature Map: 0x");
      //DEBUG_DEV.println(protocol::hm.featureMap, HEX);
      DEBUG_DEV.print(F("Timeout:"));
      DEBUG_DEV.println(protocol::hm.timeout);
      //DEBUG_DEV.print(" MaxMsgSize: ");
      //DEBUG_DEV.println(protocol::hm.maxMsgSize);
      DEBUG_DEV.print(F("ProfileSignature:"));
      DEBUG_DEV.println(protocol::hm.profileSignature);
      //DEBUG_DEV.print(" Board Index: ");
     //DEBUG_DEV.println(protocol::hm.boardIndex);
      #ifndef INTEGRATED_CALLBACKS_LOWMEMORY
      DEBUG_DEV.print(F("Board UID:"));
      DEBUG_DEV.println(protocol::hm.uid);
      #endif
      DEBUG_DEV.println(F("-TX END HANDSHAKE MESSAGE DUMP-"));
    
    #endif

    JsonDocument doc;  
    protocol::hm.toJSON(doc);
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    return sz;
  }

  size_t _getHeartbeatMessage(uint8_t * buffer, size_t size)
  {
    #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(F("- TX HEARTBEAT MESSAGE DUMP -"));
      DEBUG_DEV.print(F("Board Index:"));
      DEBUG_DEV.println(protocol::hb.boardIndex);
      DEBUG_DEV.println(F("- TX END HEARTBEAT MESSAGE DUMP -"));
    #endif
    JsonDocument doc;
    protocol::hb.toJSON(doc);
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    return sz;
  }

  size_t _getPinChangeMessage(uint8_t * buffer, size_t size)
  {
    #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(F("-TX PINCHANGE MESSAGE DUMP -"));
      DEBUG_DEV.print(F("FEATURE ID:"));
      DEBUG_DEV.println(protocol::pcm.featureID);  
      DEBUG_DEV.print(F("ACK REQ:"));
      DEBUG_DEV.println(protocol::pcm.responseReq);       
      DEBUG_DEV.print(F("MESSAGE:"));
      DEBUG_DEV.println(protocol::pcm.message);  
      DEBUG_DEV.println(F("- TX END PINCHANGE MESSAGE DUMP -"));
    #endif
    JsonDocument doc;
    protocol::pcm.toJSON(doc);
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    return sz;
  }

  #endif

  #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
  protocol::HandshakeMessage& _getHandshakeMessage()
  {
    protocol::hm.featureMap = this->_featureMap;
    protocol::hm.timeout = _retryPeriod * 2;

    protocol::hm.uid = _uid;
    #ifdef DEBUG_VERBOSE  
    
      DEBUG_DEV.println(F("- TX HANDSHAKE MESSAGE DUMP -"));
      DEBUG_DEV.print(F("Protocol Version: 0x"));
      DEBUG_DEV.println(protocol::hm.protocolVersion, HEX);
      //DEBUG_DEV.print(" Feature Map: 0x");
      //DEBUG_DEV.println(protocol::hm.featureMap, HEX);
      DEBUG_DEV.print(F("Timeout:"));
      DEBUG_DEV.println(protocol::hm.timeout);
      //DEBUG_DEV.print(" MaxMsgSize: ");
      //DEBUG_DEV.println(protocol::hm.maxMsgSize);
      DEBUG_DEV.print(F("ProfileSignature:"));
      DEBUG_DEV.println(protocol::hm.profileSignature);
      //DEBUG_DEV.print(" Board Index: ");
     //DEBUG_DEV.println(protocol::hm.boardIndex);
      DEBUG_DEV.print(F("Board UID:"));
      DEBUG_DEV.println(protocol::hm.uid);
      DEBUG_DEV.println(F("- TX END HANDSHAKE MESSAGE DUMP -"));
    
    #endif
    return protocol::hm;
  }
    protocol::HeartbeatMessage& _getHeartbeatMessage()
  {
    #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(F("- TX HEARTBEAT MESSAGE DUMP -"));
      DEBUG_DEV.print(F("Board Index:"));
      DEBUG_DEV.println(protocol::hb.boardIndex);
      DEBUG_DEV.println(F("- TX END HEARTBEAT MESSAGE DUMP -"));
    #endif
    return protocol::hb;
  }

  protocol::PinChangeMessage& _getPinChangeMessage()
  {
    #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(F("-TX PINCHANGE MESSAGE DUMP -"));
      DEBUG_DEV.print(F("FEATURE ID:"));
      DEBUG_DEV.println(protocol::pcm.featureID);  
      DEBUG_DEV.print(F("ACK REQ:"));
      DEBUG_DEV.println(protocol::pcm.responseReq);       
      DEBUG_DEV.print(F("MESSAGE:"));
      DEBUG_DEV.println(protocol::pcm.message);  
      DEBUG_DEV.println(F("- TX END PINCHANGE MESSAGE DUMP -"));
    #endif
    return protocol::pcm;
  }
  #endif

/*
  protocol::PinStatusMessage& _getPinStatusMessage()
  {
    #ifdef DEBUG_VERBOSE
      DEBUG_DEV.println(" ---- TX PINSTATUS MESSAGE DUMP ----");
      DEBUG_DEV.print(" STATUS: ");
      DEBUG_DEV.println(protocol::pm.status);      
      //DEBUG_DEV.print(" Board Index: ");
      //DEBUG_DEV.println(protocol::pm.boardIndex);
      DEBUG_DEV.println(" ---- TX END PINSTATUS MESSAGE DUMP ----");
    #endif
    return protocol::pm;
  }
*/
  #ifdef DEBUG
  protocol::DebugMessage& _getDebugMessage(String& message)
  {
    protocol::dm.message = message;
    // No need to wrap in DEBUG define as the entire method is only compiled in when DEBUG is defined
    DEBUG_DEV.println(F("- TX DEBUG MESSAGE DUMP -"));
    //DEBUG_DEV.print(" Board Index: ");
    //DEBUG_DEV.println(protocol::dm.boardIndex);
    DEBUG_DEV.print(F(" Message: "));
    DEBUG_DEV.println(protocol::dm.message);
    DEBUG_DEV.println(F("- TX END DEBUG MESSAGE DUMP -"));
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
  //uint32_t _featureMap = 1;
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