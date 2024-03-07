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
    SerialConnection(uint16_t retryPeriod, uint32_t& fm)
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

  void SendMessage( protocol::IMessage& m)
  {
    Serial.println("SEND MESSAGE");
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
        #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
          MsgPacketizer::feed(data, size);
        #endif
        #ifdef INTEGRATED_CALLBACKS
          feed(data, size);
        #endif

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
    //#ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    _doSerialRecv();
    //#endif
    

    
    
    if(!subscribed)
    {
      #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
      MsgPacketizer::subscribe_manual((uint8_t)MT_HANDSHAKE,
          [&](const protocol::HandshakeMessage& n) {
              _onHandshakeMessage(n);
          });

       MsgPacketizer::subscribe_manual((uint8_t)MT_CONFIG,
          [&](const protocol::ConfigMessage& n) {
           // DEBUG_DEV.print("GOT CONFIG MSG");
              _onConfigMessage(n);
          });
             
      MsgPacketizer::subscribe_manual((uint8_t)MT_HEARTBEAT,
          [&](const protocol::HeartbeatMessage& n) {
              _onHeartbeatMessage(n);
          });
         
      MsgPacketizer::subscribe_manual((uint8_t)MT_PINCHANGE,
          [&](const protocol::PinChangeMessage& n) {
              _onPinChangeMessage(n);
          });
          

      #endif
      /*
      #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
      MsgPacketizer::subscribe(COM_DEV, MT_HANDSHAKE,
          [&](const protocol::HandshakeMessage& n) {
            //DEBUG_DEV.print("GOT MESSAGE HS");
            //DEBUG_DEV.flush();
            //Serial1.println(3);
            //DEBUG_DEV.flush();
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
            DEBUG_DEV.print("GOT CONFIG MSG");
              _onConfigMessage(n);
          });
      */
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
            DEBUG_DEV.print("GOT MESSAGE index= ");
            DEBUG_DEV.println(index);
            DEBUG_DEV.flush();
            

            // send back data as array manually
            //MsgPacketizer::send(Serial, send_index, sz, i, f, s);
        });
        */
        //#endif
        subscribed = 1;
    }
    #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    MsgPacketizer::update();
    #endif
    
  }



  virtual void _sendHandshakeMessage()
  { 
    #ifdef INTEGRATED_CALLBACKS
    /*
      //COM_DEV.println("HANDSHAKE SEND");
      #ifdef INTEGRATED_CALLBACKS_LOWMEMORY
      uint8_t buffer[30];
      #else
      uint8_t buffer[64];
      #endif
      //for( int x = 0; x<sizeof(buffer); x++)
      //{
      //  buffer[x] = 0xFF;
      // }
      JsonDocument doc; 
      hm.toJSON(doc);
      size_t sz = serializeMsgPack(doc, (uint8_t*)&buffer[1], sizeof(buffer)-1);
      COM_DEV.println(sz);
      //printBuffer(buffer, sz+1);
      sz = cobs::encode(buffer, sz+1);
      buffer[sz] = 0x00;
      //printBuffer(buffer, sz+1);
      */
      #ifdef INTEGRATED_CALLBACKS_LOWMEMORY
        uint8_t buffer[30];
      #else
        uint8_t buffer[64];
      #endif
      size_t sz = _getHandshakeMessage(buffer, sizeof(buffer));
      COM_DEV.write(buffer, sz);
      COM_DEV.flush();
    #endif
    /*
    #ifdef INTEGRATED_CALLBACKS
      //HandshakeMessage& hmm = _getHandshakeMessage();
      //const auto& packet = MsgPacketizer::encode(MT_HANDSHAKE, hmm.protocolVersion, hmm.featureMap, hmm.timeout);
      const auto& packet = MsgPacketizer::encode(MT_HANDSHAKE, _getHandshakeMessage());
      COM_DEV.write(packet.data.data(), packet.data.size());
      COM_DEV.flush();
    #endif
    */
  }

  virtual void _sendHeartbeatMessage()
  { 
    //COM_DEV.println("SEND HB!");
    //COM_DEV.flush();
    #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(COM_DEV, MT_HEARTBEAT, _getHeartbeatMessage());
    COM_DEV.flush();
    #endif
        #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    //MsgPacketizer::send(this->_client, this->_mi, hm);
      MsgPacketizer::send(COM_DEV, MT_HANDSHAKE, _getHandshakeMessage());
      COM_DEV.flush();
    #endif
    #ifdef INTEGRATED_CALLBACKS

      uint8_t buffer[25];
      
      size_t sz = _getHeartbeatMessage(buffer, sizeof(buffer));
      //printBuffer(buffer, 25);
      COM_DEV.write(buffer, sz);
      COM_DEV.flush();
    #endif

  }

  virtual void _sendPinChangeMessage()
  {
    #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    //DEBUG_DEV.println("SENDING PING CHANGE MESSAGE!");
    MsgPacketizer::send(COM_DEV, MT_PINCHANGE, _getPinChangeMessage());
    COM_DEV.flush();
    #endif
    #ifdef INTEGRATED_CALLBACKS

      uint8_t buffer[256];
      
      size_t sz = _getPinChangeMessage(buffer, sizeof(buffer));
      //printBuffer(buffer, 25);
      COM_DEV.write(buffer, sz);
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
    #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    MsgPacketizer::send(COM_DEV, MT_DEBUG, _getDebugMessage(message));
    COM_DEV.flush();
    #endif
  }
  #endif
  
  uint8_t subscribed = false;
};
#endif