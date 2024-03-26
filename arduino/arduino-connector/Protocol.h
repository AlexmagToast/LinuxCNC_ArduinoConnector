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
#pragma once
#ifndef PROTOCOL_H_
#define PROTOCOL_H_
#define PROTOCOL_VERSION 1 // Server and client must agree on version during handshake

namespace protocol
{
  enum MessageTypes
  {
    MT_INVITE_SYNC              =   0,
    MT_HEARTBEAT                =   1,
    MT_RESPONSE                 =   2,
    MT_HANDSHAKE                =   3, 
    MT_PINCHANGE                =   4,
    MT_PIN_CHANGE_RESPONSE      =   5,
    MT_DEBUG                    =   6,
    MT_CONFIG                   =   7,
    MT_CONFIG_ACK               =   8,
    MT_CONFIG_NAK               =   9
  };

  enum ResponseTypes
  {
    MT_ACK  = 1,
    MT_NAK  = 2
  };

  struct IMessage
  {
    virtual void fromJSON(const JsonDocument& doc) = 0;
    virtual void toJSON(JsonDocument& doc) = 0;
  };

  struct HandshakeMessage : public IMessage {
      uint8_t protocolVersion = PROTOCOL_VERSION;
      uint32_t featureMap;
      uint32_t timeout;
      uint32_t profileSignature = 0; // 0 indicates no config, >0 indicates an existing config
      #if !defined(INTEGRATED_CALLBACKS_LOWMEMORY)
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
      #endif
      void toJSON(JsonDocument& doc)
      {
        //JsonDocument doc;
        doc[F("mt")] = MessageTypes::MT_HANDSHAKE;
        doc[F("pv")] = protocolVersion;
        doc[F("fm")] = featureMap;
        doc[F("to")] = timeout;
        doc[F("ps")] = profileSignature;
        #if !defined(INTEGRATED_CALLBACKS_LOWMEMORY)
          doc[F("ui")] = uid;
          doc[F("dp")] = digitalPins;
          doc[F("ai")] = analogInputs;
          doc[F("ao")] = analogOutputs;
        #endif
        //return doc;
      }
      void fromJSON(const JsonDocument& doc)
      {
        protocolVersion = doc[F("pv")];
        featureMap = doc[F("fm")];
        timeout = doc[F("to")];
        profileSignature = doc[F("ps")];
        #if !defined(INTEGRATED_CALLBACKS_LOWMEMORY)
          uid = doc[F("ui")];
          digitalPins = doc[F("dp")];
          analogInputs = doc[F("ai")];
          analogOutputs = doc[F("ao")];
        #endif
        //return doc;
      }
  }hm;

  // First ResponseMessage received by the Arduino is in response to the Python side receiving the HandshakeMessage from the Arduiono.  The arduinoIndex value
  // in the ResponseMessage gets used by the Arduino when sending subsequent
  // messages to python, such as via UDP.  This avoids the need to send the full UID in each message to the python side.

  struct PinChangeMessage : IMessage {
      uint8_t featureID;
      uint8_t seqID;  
      uint8_t responseReq; // Indicates if a response is required from recepient
      String message;
      void toJSON(JsonDocument& doc)
      {
        //JsonDocument doc;
        doc[F("mt")] = MessageTypes::MT_PINCHANGE;
        doc[F("fi")] = featureID;
        doc[F("si")] = seqID;
        doc[F("rr")] = responseReq;
        doc[F("ms")] = message;
       // return doc;
      }
      void fromJSON(const JsonDocument& doc)
      {
        featureID = doc[F("fi")];
        seqID = doc[F("si")];
        responseReq = doc[F("rs")];
        String m = doc[F("ms")];
        message = m;
      }
  }pcm;

  /*
  struct PinChangeResponseMessage {
      uint8_t featureID;
      uint8_t seqID;
      uint8_t response; // 1 - success/ACK, 0 - error/NAK
      String message;
  }pcrm;
*/
/*s
  struct ArduinoPropertiesMessage {
    String    uid;
    uint32_t  featureMap;
    uint8_t   digitalPins;
    uint8_t   analogInputs;
    uint8_t   analogOutputs;
    uint16_t  eepromSize;
    uint32_t  eepromCRC;
    MSGPACK_DEFINE(uid, featureMap, digitalPins, analogInputs, analogOutputs, eepromSize, eepromCRC); 
  }apm;
*/
  struct HeartbeatMessage : public IMessage {
    //String mcuUptime;
    uint32_t mcuUptime;
    void toJSON(JsonDocument& doc)
    {
      //JsonDocument doc;
      doc[F("mt")] = MessageTypes::MT_HEARTBEAT;
      doc[F("ut")] = mcuUptime;
      //return doc;
    }
    void fromJSON(const JsonDocument& doc)
    {
      
    }
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
  struct ConfigMessageAck : public IMessage{
    uint8_t featureID;
    uint16_t seq;
    uint8_t featureArrIndex;
    void toJSON(JsonDocument& doc)
    {
      //JsonDocument doc;
      doc[F("mt")] = MessageTypes::MT_CONFIG_ACK;
      doc[F("fi")] = featureID;
      doc[F("se")] = seq;
      doc[F("fa")] = featureArrIndex;
      //return doc;
    }
    void fromJSON(const JsonDocument& doc)
    {
      featureID = doc[F("fi")];
      seq = doc[F("se")];
      featureArrIndex = doc[F("fa")];
    }
  };

  struct ConfigMessageNak : public IMessage {
    uint8_t featureID;
    uint16_t seq;
    uint8_t errorCode;
    String errorString;
    void toJSON(JsonDocument& doc)
    {
      //JsonDocument doc;
      doc[F("mt")] = MessageTypes::MT_CONFIG_NAK;
      doc[F("fi")] = featureID;
      doc[F("se")] = seq;
      doc[F("ec")] = errorCode;
      doc[F("es")] = errorString;
      //return doc;
    }
    void fromJSON(const JsonDocument& doc)
    {
      featureID = doc[F("fi")];
      seq = doc[F("se")];
      errorCode = doc[F("ec")];
      String s = doc[F("es")];
      errorString = s;
    }
  };

  struct ConfigMessage : public IMessage {
      uint8_t featureID;
      uint16_t seq;
      uint16_t total;
      String configString; 
      void toJSON(JsonDocument& doc)
      {
        //JsonDocument doc;
        doc[F("mt")] = MessageTypes::MT_CONFIG;
        doc[F("fi")] = featureID;
        doc[F("se")] = seq;
        doc[F("to")] = total;
        doc[F("cs")] = configString;
        //return doc;
      }
      void fromJSON(const JsonDocument& doc)
      {
        //doc[F("mt")] = MessageTypes::MT_PINCONFIG;
        featureID = doc[F("fi")];//= featureID;
        seq = doc[F("se")];// = sequence;
        total = doc[F("to")];// = total;
        String s = doc[F("cs")];
        configString = s;
      }
  }cfg;

  #ifdef DEBUG
  struct DebugMessage : public IMessage {
    String message;
    void toJSON(JsonDocument& doc)
    {
      //JsonDocument doc;
      doc[F("mt")] = MessageTypes::MT_DEBUG;
      doc[F("ds")] = message;
      //return doc;
    }
    void fromJSON(const JsonDocument& doc)
    {
    }

      //uint8_t boardIndex = BOARD_INDEX+1;
      
  }dm;
  #endif

}
#endif // #define PROTOCOL_H_
