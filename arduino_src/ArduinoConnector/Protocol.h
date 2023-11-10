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
// The following are otpomizations for low-memory boards such as Arduino Unos.\
// See https://github.com/hideakitai/MsgPacketizer#memory-management-only-for-no-stl-boards
// TODO: Board detect & set these dynamically based on detected board
// max publishing element size in one destination
#define MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE 5
// max destinations to publish
#define MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE 1

// msgpack serialized binary size
#define MSGPACK_MAX_PACKET_BYTE_SIZE 96
// max size of MsgPack::arr_t
#define MSGPACK_MAX_ARRAY_SIZE 5
// max size of MsgPack::map_t
#define MSGPACK_MAX_MAP_SIZE 5
// msgpack objects size in one packet
#define MSGPACK_MAX_OBJECT_SIZE 64

// max number of decoded packet queues
#define PACKETIZER_MAX_PACKET_QUEUE_SIZE 1
// max data bytes in packet
#define PACKETIZER_MAX_PACKET_BINARY_SIZE 96
// max number of callback for one stream
#define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE 3
// max number of streams
#define PACKETIZER_MAX_STREAM_MAP_SIZE 1

#define MSGPACKETIZER_DEBUGLOG_ENABLE
//#define MSGPACKETIZER_ENABLE_STREAM
//#define MSGPACKETIZER_ENABLE_ETHER
#define MSGPACK_MAX_OBJECT_SIZE 64
#include <MsgPacketizer.h>
#define PROTOCOL_VERSION 1 // Server and client must agree on version during handshake
MsgPack::Packer _packer;
namespace protocol
{
  enum MessageTypes
  {
    MT_HEARTBEAT      =   1,
    MT_HANDSHAKE      =   2, 
    MT_COMMAND        =   3,
    MT_PINSTATUS      =   4,
    MT_DEBUG          =   5
  };

  struct HandshakeMessage {
      uint8_t protocolVersion = PROTOCOL_VERSION;
      uint64_t featureMap = 0x1001;
      uint32_t timeout = 5000;
      uint8_t boardIndex = BOARD_INDEX+1;
      MSGPACK_DEFINE(protocolVersion, featureMap, timeout, boardIndex); 
  }hm;

  struct HeartbeatMessage {
      uint8_t boardIndex = BOARD_INDEX+1;
      MSGPACK_DEFINE(boardIndex); 
  }hb;

  struct CommandMessage {
      String cmd;
      int boardIndex = BOARD_INDEX;
      MSGPACK_DEFINE(cmd, boardIndex); 
  }cm;
  
  struct PinStatusMessage {
      String status;
      uint8_t boardIndex = BOARD_INDEX+1;
      MSGPACK_DEFINE(status, boardIndex); 
  }pm;

  #ifdef DEBUG
  struct DebugMessage {
      uint8_t boardIndex = BOARD_INDEX+1;
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
