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
#ifndef CONNECTION_H_
#define CONNECTION_H_
#pragma once
#include "Arduino.h"
#include "Config.h"
#include "Protocol.h"
#include "RXBuffer.h"
#include "Cobs.h"


#ifdef ENABLE_FEATUREMAP
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


class ConnectionBase : public RXBuffer, public Stream {
  using m_cmcb = void (*)(protocol::ConfigMessage&);
  using m_pcmcb = void (*)(const protocol::PinChangeMessage&);
  using m_cscb = void (*)(int);

public:

  ConnectionBase(uint16_t retryPeriod) : RXBuffer(), _retryPeriod(retryPeriod)
  {
    _buffer = new char[_bufferSize];
  }
  //virtual void onMessage(uint8_t* d, const size_t& size)=0;

  ~ConnectionBase()
  {
    delete[] _buffer;
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

  virtual void SendMessage( protocol::IMessage& m)
  {}
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
    //this->println("SENDING PIN MESSAGE!");
    
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
          _connectedTime = millis();
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

  // Stream class methods
virtual size_t write(uint8_t byte) override {
  #ifndef DEBUG
  return 1;
  #endif

  if (byte == '\r') {
    return 1; // Ignore \r characters
  }
  
  if (byte == '\n') {
    // Construct the debug message from the buffer up to but not including the \n character
    String debugMessage = String(_buffer).substring(0, _bufferIndex);
    #ifdef DEBUG
    _sendDebugMessage(debugMessage);
    #endif
    _bufferIndex = 0; // Reset buffer index after calling _sendDebugMessage
    return 1;
  }
  
  if (_bufferIndex < _bufferSize) {
    _buffer[_bufferIndex++] = byte;
    return 1;
  }
  
  return 0;  // Buffer is full
}
  virtual int read() override {
    // Implement your read logic here
    // Example: read a byte from a buffer
    if (_bufferIndex > 0) {
      return _buffer[--_bufferIndex];  // Simplistic read logic
    }
    return -1;  // Return -1 if none available
  }

  virtual int available() override {
    // Implement your logic to return the number of bytes available for reading
    // Example: return the number of bytes in the buffer
    return _bufferIndex;
  }

  virtual int peek() override {
    // Implement your peek logic here
    // Example: return the next byte in the buffer without removing it
    if (_bufferIndex > 0) {
      return _buffer[_bufferIndex - 1];
    }
    return -1;
  }

  virtual void flush() override {
    // Implement your flush logic here
    // Example: clear the buffer
    _bufferIndex = 0;
  }

protected:
  virtual uint8_t _onInit()
  {
    return 0;
  }
  virtual void _onConnect()
  {

  }
  virtual void _onDisconnect()
  {

  }
  virtual void _onError()
  {

  }
  virtual void _onDoWork()
  {

  }


  virtual void _sendHandshakeMessage()
  {

  }

  virtual void _sendHeartbeatMessage()
  {

  }

  virtual void _sendPinChangeMessage(){

  }
                //_sendPinChangeMessage
  #ifdef DEBUG
  virtual void _sendDebugMessage(String& message)
  {
    
  }
  #endif

  void _onHandshakeMessage(const protocol::HandshakeMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      this->println(F("-RX HANDSHAKE MESSAGE DUMP-"));
      this->print(F("Protocol Version: 0x"));
      this->println(n.protocolVersion);
      this->print(F("Profile Signature:"));
      this->println(n.profileSignature);
      this->println(F(" - RX END HANDSHAKE MESSAGE DUMP -"));
      #endif
      _handshakeReceived = 1;
  }

  void _onHeartbeatMessage(const protocol::HeartbeatMessage& n)
  {
      //#ifdef DEBUG_VERBOSE
      //this->println(F("RX HEARTBEAT MESSAGE"));
      //#endif
      _heartbeatReceived = 1;
  }

  /*
  void _onCommandMessage(const protocol::CommandMessage& n)
  {
      #ifdef DEBUG_VERBOSE
      this->println(" ---- RX COMMAND MESSAGE DUMP ----");
      this->print(" Command: ");
      this->println(n.cmd);
      //this->print(" Board Index: ");
      //this->println(n.boardIndex);
      this->println(" ---- RX END COMMAND MESSAGE DUMP ----");
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
      this->println(F(" - RX PINCHANGE MESSAGE DUMP -"));
      this->print(F("FEATURE ID:"));
      this->println(n.featureID);  
      this->print(F("SEQ ID:"));
      this->println(n.seqID);  
      this->print(F("RESPONSE REQ:"));
      this->println(n.responseReq);       
      this->print(F("MESSAGE:"));
      this->println(n.message);  
      this->println(F(" - RX END PINCHANGE MESSAGE DUMP -"));
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
      this->print(F("Connection transitioning from current state of ["));
      this->print(this->stateToString(this->_myState));
      this->print(F("] to ["));
      this->print(this->stateToString(new_state));
      this->println(F("]"));
      this->flush();
    #endif
    this->_myState = new_state;
    if( this->_csAction != NULL )
    {
      this->_csAction(new_state);
    }
  }

  void printBuffer(uint8_t* buffer, size_t size) {
    for (uint8_t i = 0; i < size; i++) {
      if (buffer[i] < 0x10) {
        this->print(0);
      }
      this->print(buffer[i], HEX);
      this->print(" ");
    }
    this->println("");
  }

   
  virtual void onMessage(uint8_t* d, const size_t& size)
  {
    JsonDocument doc;
    #ifdef DEBUG_VERBOSE
      this->println(F("ENCODED RX="));
      printBuffer(d, size);
    #endif
    size_t sz = cobs::decode(d, size-1);
    #ifdef DEBUG_VERBOSE
      this->println(F("DECODED RX="));
      printBuffer(d, sz);
    #endif
  
    DeserializationError error = deserializeMsgPack(doc, (int8_t*)&d[1], sz);
    if (error) {
      #ifdef DEBUG
      this->print(F("deserializeJson() failed: "));
      this->println(error.f_str());
      this->flush();
      #endif
      return;
    }


    uint16_t mt = doc[F("mt")];
    
    #ifdef DEBUG_VERBOSE
      this->println(F("JSON RX="));
      serializeJson(doc, *this);
      this->println(F(""));
    #endif
    
    //this->println(mt);
    switch(mt)
    {
      case protocol::MessageTypes::MT_INVITE_SYNC:
      {
        #ifdef DEBUG
          this->println(F("RX MT_INVITE_SYNC"));
        #endif
        //if(_myState == ConnectionState::CS_CONNECTED)
        //{
          // Trigger a reconnect if the python side sends a handshake message when we 'think' we are already connected.
          #ifdef DEBUG
          this->println(F("RX MT_INVITE_SYNC, RESTARTING CONNECTION LOOP!"));
          #endif
          this->_setState(CS_DISCONNECTED);
        //}
        break;
      }
      case protocol::MessageTypes::MT_HANDSHAKE:
      {
        #ifdef DEBUG
          this->println(F("RX MT_HANDSHAKE"));
        #endif
        protocol::HandshakeMessage hmm;
        hmm.fromJSON(doc);
        _onHandshakeMessage(hmm);
        break;
      }
      case protocol::MessageTypes::MT_HEARTBEAT:
      {
        #ifdef DEBUG
          this->println(F("RX MT_HEARTBEAT"));
        #endif
        //protocol::HeartbeatMessage h;
        //hmm.fromJSON(doc);
        _onHeartbeatMessage(protocol::hb);
        break;
      }
      case protocol::MessageTypes::MT_CONFIG:
      {
        #ifdef DEBUG
          this->println(F("RX MT_CONFIG"));

        #endif
        protocol::ConfigMessage ccf;
        ccf.fromJSON(doc);
        #ifdef DEBUG_VERBOSE
            this->println(F("CONFIG MESSAGE DUMP:"));
            this->print(F("SEQ:"));
            this->println(ccf.seq);
            this->print(F("TOTAL:"));
            this->println(ccf.total);
            this->print(F("CONFIG STRING:"));
            this->println(ccf.configString);
        #endif
        _onConfigMessage(ccf);
        break;
      }
      case protocol::MessageTypes::MT_PINCHANGE:
      {
        #ifdef DEBUG
          this->println(F("RX MT_PINCHANGE"));
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
    
      this->println(F("TX HANDSHAKE MESSAGE DUMP"));
      this->print(F("Protocol Version: 0x"));
      this->println(protocol::hm.protocolVersion, HEX);
      //this->print(" Feature Map: 0x");
      //this->println(protocol::hm.featureMap, HEX);
      this->print(F("Timeout:"));
      this->println(protocol::hm.timeout);
      //this->print(" MaxMsgSize: ");
      //this->println(protocol::hm.maxMsgSize);
      this->print(F("ProfileSignature:"));
      this->println(protocol::hm.profileSignature);
      //this->print(" Board Index: ");
     //this->println(protocol::hm.boardIndex);
      #ifndef INTEGRATED_CALLBACKS_LOWMEMORY
      this->print(F("Board UID:"));
      this->println(protocol::hm.uid);
      #endif
      this->println(F("TX END HANDSHAKE MESSAGE DUMP"));
    
    #endif

    JsonDocument doc;  
    protocol::hm.toJSON(doc);
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    return sz;
  }

  size_t _getHeartbeatMessage(uint8_t * buffer, size_t size)
  {
    JsonDocument doc;
    /*
    //unsigned long diff = millis() - _connectedTime;
    unsigned long runMillis = millis();
    unsigned long allSeconds=runMillis/1000;
    int runDays = allSeconds/86400;
    int secsRemaining = allSeconds%86400;

    int runHours=secsRemaining/3600;
    secsRemaining=secsRemaining%3600;

    int runMinutes=secsRemaining/60;
    int runSeconds=secsRemaining%60;
    char buf[32];
    sprintf(buf,"%02d:%02d:%02d:%02d", runDays, runHours,runMinutes,runSeconds);
    */
    protocol::hb.mcuUptime = millis() / 1000 / 60;
    protocol::hb.toJSON(doc);
    //doc["ut"] = diff;
    #ifdef DEBUG_VERBOSE
      this->println(F("TX HEARTBEAT MESSAGE DUMP"));
      this->print(F("MCU Uptime: "));
      this->println(protocol::hb.mcuUptime);
      this->println(F("TX END HEARTBEAT MESSAGE DUMP"));
    #endif
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    return sz;
  }

  size_t _getPinChangeMessage(uint8_t * buffer, size_t size)
  {

    JsonDocument doc;
    protocol::pcm.toJSON(doc);
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    #ifdef DEBUG_VERBOSE
      this->println(F("TX PINCHANGE MESSAGE DUMP"));
      this->print(F("FEATURE ID:"));
      this->println(protocol::pcm.featureID);  
      this->print(F("ACK REQ:"));
      this->println(protocol::pcm.responseReq);       
      this->print(F("MESSAGE:"));
      this->println(protocol::pcm.message);  
      this->println(F("TX END PINCHANGE MESSAGE DUMP"));
    #endif
    return sz;
  }


/*
  protocol::PinStatusMessage& _getPinStatusMessage()
  {
    #ifdef DEBUG_VERBOSE
      this->println(" ---- TX PINSTATUS MESSAGE DUMP ----");
      this->print(" STATUS: ");
      this->println(protocol::pm.status);      
      //this->print(" Board Index: ");
      //this->println(protocol::pm.boardIndex);
      this->println(" ---- TX END PINSTATUS MESSAGE DUMP ----");
    #endif
    return protocol::pm;
  }
*/
  #ifdef DEBUG
  size_t _getDebugMessage(uint8_t * buffer, size_t size, String& message)
  {
    /*
    protocol::dm.message = message;
    // No need to wrap in DEBUG define as the entire method is only compiled in when DEBUG is defined
    this->println(F("- TX DEBUG MESSAGE DUMP -"));
    //this->print(" Board Index: ");
    //this->println(protocol::dm.boardIndex);
    this->print(F(" Message: "));s
    this->println(protocol::dm.message);
    this->println(F("- TX END DEBUG MESSAGE DUMP -"));
    return protocol::dm;
    */

    JsonDocument doc;
    protocol::dm.message = message;
    protocol::dm.toJSON(doc);
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    #ifdef DEBUG_VERBOSE
    this->println(F("X DEBUG MESSAGE DUMP"));
    //this->print(" Board Index: ");
    //this->println(protocol::dm.boardIndex);
    this->print(F(" Message: "));
    this->println(protocol::dm.message);
    this->println(F("TX END DEBUG MESSAGE DUMP"));
    #endif
    return sz;
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
  int _myState = CS_DISCONNECTED;
  unsigned long _connectedTime = 0;
  // Flip-flops used to signal the presence of received message types
  uint8_t _handshakeReceived = 0;
  uint8_t _heartbeatReceived = 0;
  uint8_t _commandReceived = 0;
  const char * _uid;
  
  private:
    m_cmcb _configAction = NULL;
    m_cscb _csAction = NULL;
    m_pcmcb _pinChangeAction = NULL;

    char* _buffer;  // Pointer to the buffer
    size_t _bufferIndex;  // Current index in the buffer
    size_t _bufferSize = 255;  // Size of the buffer
};
#endif