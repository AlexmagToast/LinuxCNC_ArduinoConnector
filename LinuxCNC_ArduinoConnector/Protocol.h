/*
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
#ifndef PROTOCOL_H_
#define PROTOCOL_H_
// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>
#define PROTOCOL_VERSION 1 // Server and client must agree on version during handshake

namespace protocol
{
  enum MessageTypes
  {
    MT_HEARTBEAT      =   0,
    MT_HANDSHAKE      =   1, 
    MT_COMMAND        =   2,
    MT_PINSTATUS      =   3,
    MT_DEBUG          =   4
  };

  struct HandshakeMessage {
      uint8_t protocolVersion = PROTOCOL_VERSION;
      //uint16_t messageType = MT_HANDSHAKE;
      uint64_t featureMap = 0;
      uint8_t boardIndex = BOARD_INDEX;
      MSGPACK_DEFINE(protocolVersion, featureMap, boardIndex); 
  }hm;

  struct HeartbeatMessage {
      uint8_t boardIndex = BOARD_INDEX;
      MSGPACK_DEFINE(boardIndex); 
  }hb;
  
  #ifdef DEBUG
  struct DebugMessage {
      uint8_t boardIndex = BOARD_INDEX;
      String message;
      MSGPACK_DEFINE(boardIndex, message); 
  }dm;
  #endif
  
  /*
  // save the current data alignment setting to the stack
  // and set data alignment to 1 byte
  struct MessageHeader
  {
    uint8_t protocolVersion = PROTOCOL_VERSION;
    uint8_t messageType;
    uint16_t messagelength; // length of proceeding bytes including 2 byte CRC value at the end of each frame
  };

  uint8_t make_message(uint8_t * messageType, uint8_t * data, uint16_t * dataLen)
  {
    StaticJsonDocument<MAX_MESSAGE_LENGTH-2> doc;
      DeserializationError error = deserializeMsgPack(doc, input);

    // Test if parsing succeeded.
    if (error) {
      Serial.print("deserializeMsgPack() failed: ");
      Serial.println(error.f_str());
      return NULL;
    }


  }
  */
}
#endif // #define PROTOCOL_H_
