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

  #ifdef INTEGRATED_CALLBACKS
  void onMessage(uint8_t* d, const size_t& size)
  {
    //const auto& d = Packetizer::decode(MT_HANDSHAKE, d, size);//_getHandshakeMessage());
    //const auto& p_out = Packetizer::decode(d, size);
    //if(p_out.data.size() > 0)
    //{
    //  MsgPacketizer::feed(p_out.data.data(), p_out.data.size());
    //}
    
    /*
    switch(p_out.index)
    {
      case MT_HANDSHAKE:
      {
        HandshakeMessage hmt;
        //hmt.protocolVersion = p_out.data
        MsgPack::Unpacker unpacker;//(buffer+1, decodedLength);
        unpacker.feed(p_out.data.data(), p_out.data.size());
        // manually deserialize packet and modify
        MsgPack::arr_size_t sz;
        unpacker.deserialize(sz, hmt.protocolVersion, hmt.featureMap );//sz, pv, fm, t, ps, uid, dp, ai, ao);
        //_onHandshakeMessage(hmt);
        break;
      }
    }
    */
    /*
    Serial.print("INDEX: ");
    Serial.println(p_out.index);    
    Serial.print("DECODED SIZE: ");
    Serial.println(p_out.data.size());
    Serial.println();
    Serial.print("decoded = ");
    for (const auto& p : p_out.data) {
        Serial.print(p, HEX);
        Serial.print(" ");
    }
    Serial.println();
    //SERIAL_DEV.println("CALLBACK!");
    //SERIAL_DEV.flush();
    */
  }

  #endif

  #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
  void _doSerialRecv()
  {
    const size_t size = COM_DEV.available();
    if (size) {
        //COM_DEV.print("ON READ, SIZE=");
        //COM_DEV.println(size);

        uint8_t* data = new uint8_t[size];
        COM_DEV.readBytes((char*)data, size);
        MsgPacketizer::feed(data, size);

        // feed your binary data to MsgPacketizer manually
        // if data has successfully received and decoded,
        // subscribed callback will be called
        //MsgPacketizer::feed(data, size);
        //RXBuffer::feed(data, size);

        delete[] data;
    }
  }
  #endif

  virtual void _onDoWork()
  {
    #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
      _doSerialRecv();
    #endif
    

    
    
    if(!subscribed)
    {
      #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
      MsgPacketizer::subscribe_manual((uint8_t)MT_HANDSHAKE,
          [&](const protocol::HandshakeMessage& n) {
              _onHandshakeMessage(n);
          });

       MsgPacketizer::subscribe_manual((uint8_t)MT_CONFIG,
          [&](const protocol::ConfigMessage& n) {
           // SERIAL_DEV.print("GOT CONFIG MSG");
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
            //SERIAL_DEV.print("GOT MESSAGE HS");
            //SERIAL_DEV.flush();
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
            SERIAL_DEV.print("GOT CONFIG MSG");
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
            SERIAL_DEV.print("GOT MESSAGE index= ");
            SERIAL_DEV.println(index);
            SERIAL_DEV.flush();
            

            // send back data as array manually
            //MsgPacketizer::send(Serial, send_index, sz, i, f, s);
        });
        */
        //#endif
        subscribed = 1;
    }
    MsgPacketizer::update();
    
  }
/*
  struct HandshakeMessage {
      uint8_t protocolVersion = PROTOCOL_VERSION;
      uint64_t featureMap;
      uint32_t timeout;
      //uint16_t maxMsgSize;
      uint32_t profileSignature = 0; // 0 indicates no config, >0 indicates an existing config
      String uid;
      uint8_t   digitalPins = 0;
      uint8_t analogInputs = 0;
      uint8_t   analogOutputs = 0;
  }hm;
*/
  virtual void _sendHandshakeMessage()
  { 
    //#ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    //MsgPacketizer::send(this->_client, this->_mi, hm);
      MsgPacketizer::send(COM_DEV, MT_HANDSHAKE, _getHandshakeMessage());
      COM_DEV.flush();
    //#endif
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
    //#ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(COM_DEV, MT_HEARTBEAT, _getHeartbeatMessage());
    COM_DEV.flush();
    //#endif
  }

  virtual void _sendPinChangeMessage()
  {
    //#ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    //SERIAL_DEV.println("SENDING PING CHANGE MESSAGE!");
    MsgPacketizer::send(COM_DEV, MT_PINCHANGE, _getPinChangeMessage());
    COM_DEV.flush();
    //#endif
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
    //#ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    MsgPacketizer::send(COM_DEV, MT_DEBUG, _getDebugMessage(message));
    COM_DEV.flush();
    //#endif
  }
  #endif
  
  uint8_t subscribed = false;
};
#endif