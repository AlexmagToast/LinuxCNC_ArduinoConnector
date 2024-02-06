#include "Protocol.h"
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
#ifndef SERIALCONNECTION_H_
#define SERIALCONNECTION_H_
#ifdef ENABLE_MSGPACKETIZER
#include <MsgPacketizer.h>
#endif
#include <string.h>
#include "Connection.h"

using namespace protocol;
class SerialConnection : public ConnectionBase {
public:
    // Future TODO: Support selection of a different Serial interface other than just the default 'Serial'
    SerialConnection(uint16_t retryPeriod, uint64_t& fm)
  : ConnectionBase(retryPeriod, fm)
  {

  }

  ~SerialConnection(){}



  // Virtual interfaces from Connection class
  virtual void _onConnect(){}
  virtual void _onDisconnect(){}
  virtual void _onError(){}
  uint8_t _onInit(){
        // Future TODO: Support selection of a different Serial interface other than just the default 'Serial'
    return 1;
  }



  protected:

  virtual void _onDoWork()
  {
    //#ifdef ENABLE_MSGPACKETIZER
    if(!subscribed)
    {
      
      MsgPacketizer::subscribe(COM_DEV, MT_HANDSHAKE,
          [&](const protocol::HandshakeMessage& n) {
            SERIAL_DEV.print("GOT MESSAGE HS");
            SERIAL_DEV.flush();
            //Serial1.println(3);
            //SERIAL_DEV.flush();
            _onHandshakeMessage(n);
          });
      MsgPacketizer::subscribe(COM_DEV, MT_HEARTBEAT,
          [&](const protocol::HeartbeatMessage& n) {
              _onHeartbeatMessage(n);
          });
      MsgPacketizer::subscribe(COM_DEV, MT_PINCHANGE,
          [&](const protocol::PinChangeMessage& n) {
              _onPinChangeMessage(n);
          });
      MsgPacketizer::subscribe(COM_DEV, MT_CONFIG,
          [&](const protocol::ConfigMessage& n) {
              _onConfigMessage(n);
          });
      /*
      MsgPacketizer::subscribe(COM_DEV,
        [&](const uint8_t index, MsgPack::Unpacker& unpacker) {
            // input to msgpack
            //MsgPack::arr_size_t sz;
            //int i = 0;
            //float f = 0.f;
            //String s = "";

            // manually deserialize packet and modify
            //unpacker.deserialize(sz, i, f, s);
            //s = s + " " + index;
            SERIAL_DEV.print("GOT MESSAGE index= ");
            SERIAL_DEV.println(index);
            SERIAL_DEV.flush();
            

            // send back data as array manually
            //MsgPacketizer::send(Serial, send_index, sz, i, f, s);
        });
        */
        subscribed = 1;
    }
    MsgPacketizer::update();
    //#endif
  }

  virtual void _sendHandshakeMessage()
  { 
    #ifdef ENABLE_MSGPACKETIZER
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(COM_DEV, MT_HANDSHAKE, _getHandshakeMessage());
    COM_DEV.flush();
    #endif
  }

  virtual void _sendHeartbeatMessage()
  { 
    #ifdef ENABLE_MSGPACKETIZER
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(COM_DEV, MT_HEARTBEAT, _getHeartbeatMessage());
    COM_DEV.flush();
    #endif
  }

  virtual void _sendPinChangeMessage()
  {
    #ifdef ENABLE_MSGPACKETIZER
    //SERIAL_DEV.println("SENDING PING CHANGE MESSAGE!");
    MsgPacketizer::send(COM_DEV, MT_PINCHANGE, _getPinChangeMessage());
    COM_DEV.flush();
    #endif
  }
  /*
  virtual void _sendPinStatusMessage()
  { 
    MsgPacketizer::send(Serial, MT_PINSTATUS, _getPinStatusMessage());
  }
  */
  
  #ifdef DEBUG
  virtual void _sendDebugMessage(String& message)
  {
    #ifdef ENABLE_MSGPACKETIZER
    MsgPacketizer::send(COM_DEV, MT_DEBUG, _getDebugMessage(message));
    COM_DEV.flush();
    #endif
  }
  #endif
  
  uint8_t subscribed = false;
};
#endif