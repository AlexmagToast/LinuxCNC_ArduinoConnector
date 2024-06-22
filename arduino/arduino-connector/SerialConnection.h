
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
#ifndef SERIALCONNECTION_H_
#define SERIALCONNECTION_H_
#include "Protocol.h"
#pragma once

#include <string.h>
#include "Connection.h"




using namespace protocol;
/**
 * @class SerialConnection
 * @brief Represents a serial connection for communication with an Arduino device.
 * 
 * The SerialConnection class is a concrete implementation of the ConnectionBase class,
 * providing functionality to establish a serial connection with an Arduino device and
 * send/receive messages over the serial interface.
 */
class SerialConnection : public ConnectionBase {
public:

    // Future TODO: Support selection of a different Serial interface other than just the default 'Serial'
    SerialConnection(uint16_t retryPeriod)
  : ConnectionBase(retryPeriod)
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

  virtual void SendMessage( protocol::IMessage& m)
  {
    JsonDocument doc;
    m.toJSON(doc);
    uint8_t buffer[128];
    size_t size = sizeof(buffer);
    size_t sz = _jsonToMsgPack(doc, buffer, size);
    COM_DEV.write(buffer, sz);
    COM_DEV.flush();
  }

  protected:
  
  void _doSerialRecv()
  {
    const size_t size = COM_DEV.available();
    if (size) {
        //DEBUG_DEV.print("ON READ, SIZE=");
        //DEBUG_DEV.println(size);

        uint8_t* data = new uint8_t[size];
        COM_DEV.readBytes((char*)data, size);

        feed(data, size);


        // feed your binary data to MsgPacketizer manually
        // if data has successfully received and decoded,
        // subscribed callback will be called
        //MsgPacketizer::feed(data, size);
        //RXBuffer::feed(data, size);

        delete[] data;
    }
  }
  

  virtual void _onDoWork()
  {
    
    _doSerialRecv();
    
    if(!subscribed)
    {
        subscribed = 1;
    }
    
  }



  virtual void _sendHandshakeMessage()
  { 
    
    #ifdef INTEGRATED_CALLBACKS_LOWMEMORY
      uint8_t buffer[30];
    #else
      uint8_t buffer[64];
    #endif
    size_t sz = _getHandshakeMessage(buffer, sizeof(buffer));
    COM_DEV.write(buffer, sz);
    COM_DEV.flush();
    
  }

  virtual void _sendHeartbeatMessage()
  { 
    uint8_t buffer[25];
    
    size_t sz = _getHeartbeatMessage(buffer, sizeof(buffer));
    //printBuffer(buffer, 25);
    COM_DEV.write(buffer, sz);
    COM_DEV.flush();
  }

  virtual void _sendPinChangeMessage()
  {

    uint8_t buffer[256];
    
    size_t sz = _getPinChangeMessage(buffer, sizeof(buffer));
    //printBuffer(buffer, 25);
    COM_DEV.write(buffer, sz);
    COM_DEV.flush();


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
    uint8_t buffer[256];
    
    //size_t sz = _getDebugMessage(buffer, sizeof(buffer));
    size_t sz = _getDebugMessage(buffer, sizeof(buffer), message);
    //printBuffer(buffer, 25);
    COM_DEV.write(buffer, sz);
    // write nul character to serial
    //COM_DEV.write((uint8_t)0x00);
    COM_DEV.flush();
    //COM_DEV.write(0x00, 1);
    //Serial.flush();

  }
  #endif
  
  uint8_t subscribed = false;
};
#endif