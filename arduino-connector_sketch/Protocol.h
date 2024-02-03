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
/*
#define MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE 20
#define MSGPACK_MAX_PACKET_BYTE_SIZE 2048
#define PACKETIZER_MAX_PACKET_BINARY_SIZE 512
#define MSGPACK_MAX_ARRAY_SIZE 512
#define MSGPACK_MAX_OBJECT_SIZE 512
#define MSGPACKETIZER_DEBUGLOG_ENABLE
#define PACKETIZER_MAX_PACKET_QUEUE_SIZE 10
*/
//#define MSGPACKETIZER_ENABLE_STREAM
/*
// The following are otpomizations for low-memory boards such as Arduino Unos.\
// See https://github.com/hideakitai/MsgPacketizer#memory-management-only-for-no-stl-boards
// TODO: Board detect & set these dynamically based on detected board
// max publishing element size in one destination
define MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE 5
// max destinations to publish
#define MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE 1

// msgpack serialized binary size
#define MSGPACK_MAX_PACKET_BYTE_SIZE 96
// max size of MsgPack::arr_t
#define MSGPACK_MAX_ARRAY_SIZE 7
// max size of MsgPack::map_t
#define MSGPACK_MAX_MAP_SIZE 7
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
*/
//#include <MsgPacketizer.h>
#define PROTOCOL_VERSION 1 // Server and client must agree on version during handshake

namespace protocol
{
  enum MessageTypes
  {
    MT_HEARTBEAT                =   1,
    MT_RESPONSE                 =   2,
    MT_HANDSHAKE                =   3, 
    MT_PINCHANGE                =   4,
    MT_PIN_CHANGE_RESPONSE      =   5,
    MT_DEBUG                    =   6,
    MT_CONFIG                   =   7
  };

  enum ResponseTypes
  {
    MT_ACK  = 1,
    MT_NAK  = 2
  };

  struct HandshakeMessage {
      uint8_t protocolVersion = PROTOCOL_VERSION;
      uint64_t featureMap;
      uint32_t timeout;
      uint16_t maxMsgSize;
      uint16_t configVersion; // 0 indicates no config, >0 indicates an existing config
      String uid;
      #ifdef NUM_DIGITAL_PINS
      uint8_t   digitalPins = NUM_DIGITAL_PINS;
      #else
      uint8_t   digitalPins = 0;
      #endif
      #ifdef NUM_ANALOG_INPUTS
      uint8_t   analogInputs = NUM_ANALOG_INPUTS;
      #else
      uint8_t analogInputs = 0;
      #endif
      #ifdef NUM_ANALOG_OUTPUTS
      uint8_t   analogOutputs = NUM_ANALOG_OUTPUTS;
      #else
      uint8_t   analogOutputs = 0;
      #endif
      MSGPACK_DEFINE(protocolVersion, featureMap, timeout, maxMsgSize, configVersion, uid, digitalPins, analogInputs, analogOutputs); 
  }hm;

  // First ResponseMessage received by the Arduino is in response to the Python side receiving the HandshakeMessage from the Arduiono.  The arduinoIndex value
  // in the ResponseMessage gets used by the Arduino when sending subsequent
  // messages to python, such as via UDP.  This avoids the need to send the full UID in each message to the python side.

  struct PinChangeMessage {
      uint8_t featureID;
      uint8_t seqID;  
      uint8_t responseReq; // Indicates if a response is required from recepient
      String message;
      MSGPACK_DEFINE(featureID, seqID, responseReq, message); 
  }pcm;
  
  struct PinChangeResponseMessage {
      uint8_t featureID;
      uint8_t seqID;
      uint8_t response; // 1 - success/ACK, 0 - error/NAK
      String message;
      MSGPACK_DEFINE(featureID, seqID, message); 
  }pcrm;
/*s
  struct ArduinoPropertiesMessage {
    String    uid;
    uint64_t  featureMap;
    uint8_t   digitalPins;
    uint8_t   analogInputs;
    uint8_t   analogOutputs;
    uint16_t  eepromSize;
    uint32_t  eepromCRC;
    MSGPACK_DEFINE(uid, featureMap, digitalPins, analogInputs, analogOutputs, eepromSize, eepromCRC); 
  }apm;
*/
  struct HeartbeatMessage {
      uint8_t boardIndex;
      MSGPACK_DEFINE(boardIndex); 
  }hb;

  /*
  // This message may be deprecated in the near future
  struct CommandMessage {
      String cmd;
      MSGPACK_DEFINE(cmd); 
  }cm;
  */
  /*
  struct PinStatusMessage {
      String status;
      MSGPACK_DEFINE(status); 
  }pm;
  */
  struct ConfigMessage {
      uint64_t featureID;
      uint16_t seq;
      uint16_t total;
      String configString; 
      MSGPACK_DEFINE(featureID, seq, total, configString); 
  }cfg;

  #ifdef DEBUG
  struct DebugMessage {
      //uint8_t boardIndex = BOARD_INDEX+1;
      String message;
      MSGPACK_DEFINE(message); 
  }dm;
  #endif

}
#endif // #define PROTOCOL_H_
