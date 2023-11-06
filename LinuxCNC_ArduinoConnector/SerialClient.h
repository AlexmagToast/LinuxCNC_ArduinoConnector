/*
SerialCient for Arduino

Developed in connection with the LinuxCNC_ArduinoConnector Project Located at https://github.com/AlexmagToast/LinuxCNC_ArduinoConnector

Copyright (c) 2023 Kenneth Thompson, https://github.com/KennethThompson?

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
#pragma once
#ifndef SERIALCLIENT_H_
#define SERIALCLIENT_H_

//#include <ArduinoJson.h>  // include before MsgPacketizer.h
#include <MsgPacketizer.h>
#include <string.h>
#include "Connection.h"

using namespace protocol;
class SerialClient :virtual public ConnectionBase {
public:
    // Future TODO: Support selection of a different Serial interface other than just the default 'Serial'
    SerialClient(uint16_t retryPeriod, uint64_t& fm)
  : ConnectionBase(retryPeriod, fm)
  {

  }

  // Virtual interfaces from Connection class
  virtual void _onConnect(){}
  virtual void _onDisconnect(){}
  virtual void _onError(){}
  uint8_t _onInit(){
        // Future TODO: Support selection of a different Serial interface other than just the default 'Serial'
    return 1;
  }

  //void onConnect();
  //void onDisconnect();
  //void onError();



  protected:

  virtual void _onDoWork()
  {
    if(!subscribed)
    {
      
      MsgPacketizer::subscribe(Serial, MT_HANDSHAKE,
          [&](const protocol::HandshakeMessage& n) {
              _onHandshakeMessage(n);
          });
      MsgPacketizer::subscribe(Serial, MT_HEARTBEAT,
          [&](const protocol::HeartbeatMessage& n) {
              _onHeartbeatMessage(n);
          });
      MsgPacketizer::subscribe(Serial, MT_COMMAND,
          [&](const protocol::CommandMessage& n) {
              _onCommandMessage(n);
          });
        subscribed = 1;
    }
    MsgPacketizer::update();
  }

  virtual void _sendHandshakeMessage()
  { 
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(Serial, MT_HANDSHAKE, _getHandshakeMessage());
    Serial.flush();
  }

  virtual void _sendHeartbeatMessage()
  { 
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(Serial, MT_HEARTBEAT, _getHeartbeatMessage());
    Serial.flush();
  }
  virtual void _sendPinStatusMessage()
  { 
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(Serial, MT_PINSTATUS, _getPinStatusMessage());
    Serial.flush();
  }
  #ifdef DEBUG
  virtual void _sendDebugMessage(String& message)
  {
    MsgPacketizer::send(Serial, MT_DEBUG, _getDebugMessage(message));
    Serial.flush();
  }
  #endif
  
  uint8_t subscribed = false;
};
#endif