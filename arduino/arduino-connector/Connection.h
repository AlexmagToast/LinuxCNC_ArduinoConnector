/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com &
  Ken Thompson (not THAT Ken Thompson), https://github.com/KennethThompson
  
  MIT License
  Copyright (c) 2023-2025 Alexander Richter & Ken Thompson

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

#include "Config.h"
#include "Protocol.h"
#include "RXBuffer.h"
#include "Cobs.h"


#ifdef ENABLE_FEATUREMAP
#include "FeatureMap.h"
#endif


/**
 * @enum ConnectionState
 * @brief Defines the possible states of a connection
 */
enum ConnectionState
{
  CS_DISCONNECTED = 0,     ///< No active connection
  CS_CONNECTING,           ///< Connection attempt in progress
  CS_CONNECTED,            ///< Connection established
  CS_RECONNECTED,          ///< Connection re-established after disconnect
  CS_DISCONNECTING,        ///< Connection being terminated
  CS_CONNECTION_TIMEOUT,   ///< Connection timed out
  CS_ERROR                 ///< Error state
};


/**
 * @class ConnectionBase
 * @brief Base class for implementing connections with message handling capabilities
 *
 * This class provides the core functionality for establishing, maintaining, and 
 * handling communication over various transport layers.
 */
class ConnectionBase : public RXBuffer, public Stream {
  using m_cmcb = void (*)(protocol::ConfigMessage&);
  using m_pcmcb = void (*)(const protocol::PinChangeMessage&);
  using m_cscb = void (*)(int);

public:

  /**
   * @brief Constructor for ConnectionBase
   * @param retryPeriod Time in milliseconds between connection retry attempts
   */
  ConnectionBase(uint16_t retryPeriod) : RXBuffer(), _retryPeriod(retryPeriod)
  {
    _buffer = new char[_bufferSize];
  }

  /**
   * @brief Destructor for ConnectionBase
   */
  ~ConnectionBase()
  {
    delete[] _buffer;
  }

  /**
   * @brief Register a callback for configuration messages
   * @param act Callback function to handle configuration messages
   */
  void RegisterConfigCallback(m_cmcb act)
  {
    _configAction = act;
  
  }

  /**
   * @brief Register a callback for pin change messages
   * @param act Callback function to handle pin change messages
   */
  void RegisterPinChangeCallback(m_pcmcb act)
  {
    _pinChangeAction = act;
  }

  /**
   * @brief Register a callback for connection state changes
   * @param act Callback function to handle connection state changes
   */
  void RegisterCSCallback(m_cscb act)
  {
    _csAction = act;
  }

  #ifdef DEBUG
  /**
   * @brief Send a debug message
   * @param message The debug message to send
   */
  virtual void SendDebugMessage(String& message)
  {
    _sendDebugMessage(message);
  }
  #endif

  /**
   * @brief Get the current connection state
   * @return Reference to the current state
   */
  int& GetState()
  {
    return _myState;
  }

  /**
   * @brief Set the unique identifier for this device
   * @param uid Unique identifier string
   */
  void setUID(const char* uid)
  {
    _uid = uid;
  }

  /**
   * @brief Send a generic message
   * @param m Message to send
   */
  virtual void SendMessage( protocol::IMessage& m)
  {}

  /**
   * @brief Send a pin change message
   * @param featureID ID of the feature
   * @param seqID Sequence ID
   * @param responseReq Response required flag
   * @param message Message content
   */
  virtual void SendPinChangeMessage(uint8_t& featureID, uint8_t& seqID, uint8_t& responseReq, String& message)
  {
    protocol::pcm.featureID = featureID;
    protocol::pcm.responseReq = responseReq;
    protocol::pcm.message = message;
    protocol::pcm.seqID = seqID;
    
    _sendPinChangeMessage();    
  }


  /**
   * @brief Main work function to be called regularly to maintain the connection and process messages
   */
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

  /**
   * @brief Convert connection state to human-readable string
   * @param state State to convert
   * @return String representation of the state
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

  /**
   * @brief Implementation of Stream's write method
   * @param byte Byte to write
   * @return Number of bytes written
   */
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
  /**
   * @brief Implementation of Stream's read method
   * @return Byte read or -1 if none available
   */
  virtual int read() override {
    // Implement your read logic here
    // Example: read a byte from a buffer
    if (_bufferIndex > 0) {
      return _buffer[--_bufferIndex];  // Simplistic read logic
    }
    return -1;  // Return -1 if none available
  }

  /**
   * @brief Implementation of Stream's available method
   * @return Number of bytes available for reading
   */
  virtual int available() override {
    // Implement your logic to return the number of bytes available for reading
    // Example: return the number of bytes in the buffer
    return _bufferIndex;
  }

  /**
   * @brief Implementation of Stream's peek method
   * @return Next byte without removing it, or -1 if none available
   */
  virtual int peek() override {
    // Implement your peek logic here
    // Example: return the next byte in the buffer without removing it
    if (_bufferIndex > 0) {
      return _buffer[_bufferIndex - 1];
    }
    return -1;
  }

  /**
   * @brief Implementation of Stream's flush method
   */
  virtual void flush() override {
    // Implement your flush logic here
    // Example: clear the buffer
    _bufferIndex = 0;
  }

protected:
  /**
   * @brief Called during initialization
   * @return Status code (0 for success)
   */
  virtual uint8_t _onInit()
  {
    return 0;
  }
  /**
   * @brief Called when connection is established
   */
  virtual void _onConnect()
  {

  }
  /**
   * @brief Called when connection is closed
   */
  virtual void _onDisconnect()
  {

  }
  /**
   * @brief Called when an error occurs
   */
  virtual void _onError()
  {

  }
  /**
   * @brief Called during regular processing in DoWork
   */
  virtual void _onDoWork()
  {

  }


  /**
   * @brief Send handshake message to establish connection
   */
  virtual void _sendHandshakeMessage()
  {

  }

  /**
   * @brief Send heartbeat message to maintain connection
   */
  virtual void _sendHeartbeatMessage()
  {

  }

  /**
   * @brief Send pin change message
   */
  virtual void _sendPinChangeMessage(){

  }
                //_sendPinChangeMessage
  #ifdef DEBUG
  /**
   * @brief Send debug message
   * @param message Debug message to send
   */
  virtual void _sendDebugMessage(String& message)
  {
    
  }
  #endif

  /**
   * @brief Handle received handshake message
   * @param n Handshake message
   */
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

  /**
   * @brief Handle received heartbeat message
   * @param n Heartbeat message
   */
  void _onHeartbeatMessage(const protocol::HeartbeatMessage& n)
  {
      //#ifdef DEBUG_VERBOSE
      //this->println(F("RX HEARTBEAT MESSAGE"));
      //#endif
      _heartbeatReceived = 1;
  }


  /**
   * @brief Handle received configuration message
   * @param n Configuration message
   */
  void _onConfigMessage(protocol::ConfigMessage& n)
  {
      if(_configAction != NULL)
      {
        _configAction(n);
      }
      _receiveTimer = millis(); // Don';t let the heartbeat timeout elapse just because the arduino is busy processing config
  }
  /**
   * @brief Handle received pin change message
   * @param n Pin change message
   */
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
  /**
   * @brief Set connection state and trigger callback if registered
   * @param new_state New state to set
   */
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

  /**
   * @brief Print buffer contents for debugging
   * @param buffer Buffer to print
   * @param size Size of buffer
   */
  void printBuffer(uint8_t* buffer, size_t size) {
    for (uint8_t i = 0; i < size; i++) {
      if (buffer[i] < 0x10) {
        this->print(0);
      }
      this->print(buffer[i]);
      this->print(" ");
    }
    this->println("");
  }

   
  /**
   * @brief Process received message
   * @param d Data buffer
   * @param size Buffer size
   */
  virtual void onMessage(uint8_t* d, const size_t& size)
  {
    JsonDocument doc;
    #ifdef DEBUG_VERBOSE_DISABLED // TODO: Determine why these verbose outputs were causing the 8266 to output garbage on debug. Disabled for now.
      this->println(F("ENCODED RX="));
      printBuffer(d, size);
    #endif
    size_t sz = cobs::decode(d, size-1);
    #ifdef DEBUG_VERBOSE_DISABLED
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
          #ifdef DEBUG
          this->println(F("RX MT_INVITE_SYNC, RESTARTING CONNECTION LOOP!"));
          #endif
          this->_setState(CS_DISCONNECTED);
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

        protocol::PinChangeMessage p;
        p.fromJSON(doc);
        /*
        #ifdef DEBUG_VERBOSE
          this->println(F("RX MT_PINCHANGE"));
          this->print(F("FEATURE ID:"));
          this->println(p.featureID);
          this->print(F("RESPONSE REQ:"));
          this->println(p.responseReq);
          this->print(F("MESSAGE:"));
          this->println(p.message);
        #endif
        */
       
        _onPinChangeMessage(p);
        break;
      }
    }
    
  }
  /**
   * @brief Convert JSON document to MessagePack format
   * @param doc JSON document
   * @param buffer Target buffer
   * @param s Buffer size
   * @return Size of encoded data
   */
  size_t _jsonToMsgPack(JsonDocument& doc, uint8_t * buffer, size_t s)
  {
    size_t sz = serializeMsgPack(doc, (uint8_t*)&buffer[1], s-1);
    sz = cobs::encode(buffer, sz+1);
    buffer[sz] = 0x00;
    return sz+1;
  }

  /**
   * @brief Prepare handshake message for sending
   * @param buffer Target buffer
   * @param size Buffer size
   * @return Size of encoded message
   */
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
      this->println(protocol::hm.protocolVersion);
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

  /**
   * @brief Prepare heartbeat message for sending
   * @param buffer Target buffer
   * @param size Buffer size
   * @return Size of encoded message
   */
  size_t _getHeartbeatMessage(uint8_t * buffer, size_t size)
  {
    JsonDocument doc;
    protocol::hb.mcuUptime = millis() / 1000 / 60;
    protocol::hb.toJSON(doc);
    //doc["ut"] = diff;
    #ifdef DEBUG_VERBOSE
      this->println(F("TX HEARTBEAT MESSAGE DUMP"));
      this->println(F("MCU Uptime: "));
      //this->print(protocol::hb.mcuUptime);
      this->println(F("TX END HEARTBEAT MESSAGE DUMP"));
      //this->flush();
    #endif
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    return sz;
  }

  /**
   * @brief Prepare pin change message for sending
   * @param buffer Target buffer
   * @param size Buffer size
   * @return Size of encoded message
   */
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

  #ifdef DEBUG
  /**
   * @brief Prepare debug message for sending
   * @param buffer Target buffer
   * @param size Buffer size
   * @param message Debug message
   * @return Size of encoded message
   */
  size_t _getDebugMessage(uint8_t * buffer, size_t size, String& message)
  {
    JsonDocument doc;
    protocol::dm.message = message;
    protocol::dm.toJSON(doc);
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    return sz;
  }
  #endif
  

  /**
   * @brief Get and clear handshake received flag
   * @return Current flag value
   */
  uint8_t _getHandshakeReceived()
  {
    uint8_t r = _handshakeReceived;
    if (_handshakeReceived)
      _handshakeReceived = 0;
    return r;
  }
  /**
   * @brief Get and clear heartbeat received flag
   * @return Current flag value
   */
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