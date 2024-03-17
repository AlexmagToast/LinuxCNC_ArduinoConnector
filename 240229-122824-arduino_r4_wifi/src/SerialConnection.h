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


#include <string.h>
#include "Connection.h"




using namespace protocol;
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

  void SendMessage( protocol::IMessage& m)
  {
    //Serial.println("SEND MESSAGE");
    DEBUG_DEV.println("SEND MESSAGE");
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
        //COM_DEV.print("ON READ, SIZE=");
        //COM_DEV.println(size);

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

  }
  #endif
  
  uint8_t subscribed = false;
};
#endif