/*
UDPClient for Arduino

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
#ifndef UDPCLIENT_H_
#define UDPCLIENT_H_

//#include <ArduinoJson.h>  // include before MsgPacketizer.h
#include <MsgPacketizer.h>
#include <String.h>
#include "EthernetFuncs.h"
#include "Connection.h"

using namespace protocol;
class UDPClient :virtual public ConnectionBase {
public:
  #if DHCP == 1
    UDPClient(byte* macAddress, const char* serverIP, uint64_t& fm, int rxPort=10001, int txPort, uint16_t retryPeriod)
  : ConnectionBase(retryPeriod, fm), _myMAC(macAddress), _serverIP(serverIP), _rxPort(port), _txPort(port), 
  {    
  }
  #else
    UDPClient(IPAddress& arduinoIP, byte* macAddress, const char* serverIP,  uint64_t& fm, int rxPort, int txPort, uint16_t retryPeriod)
  : ConnectionBase(retryPeriod, fm), _myIP(arduinoIP), _myMAC(macAddress), _serverIP(serverIP), _rxPort(rxPort), _txPort(txPort)
  {
  }
  #endif
  // Virtual interfaces from Connection class
  virtual void _onConnect(){}
  virtual void _onDisconnect(){}
  virtual void _onError(){}
  uint8_t _onInit(){
    #ifdef DEBUG
      Serial.println("DEBUG: UDPClient::onInit() called..");
    #endif
    // disable SD card if one in the slot
    //pinMode(4,OUTPUT);
    //digitalWrite(4,HIGH);
    Ethernet.init(ETHERNET_INIT_PIN);
    delay(1000);
    #ifdef DEBUG
      #if DHCP == 1
        Serial.println("DEBUG: Initializing Ethernet. DHCP = Enabled");
      #else
        Serial.print("DEBUG: Initializing Ethernet. DHCP = False. Static IP = ");
        Serial.println(this->_IpAddress2String(this->_myIP));
      #endif
    #endif
    #if DHCP == 1
      if (Ethernet.begin(this->_myMAC) == 0) {
        #ifdef DEBUG
          Serial.print("DEBUG: Failed to configure Ethernet using DHCP");
        #endif
      }
      #else
        Ethernet.begin(this->_myMAC, this->_myIP);
        delay(1000);
    #endif

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      #ifdef DEBUG
          Serial.println("DEBUG: Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      #endif
      // no point in carrying on, so do nothing forevermore:
      while (true) {
        delay(1);
      }

    } else if (Ethernet.linkStatus() == LinkOFF) {
      #ifdef DEBUG
          Serial.println("DEBUG: Ethernet cable is not connected.");
      #endif
    }

    #ifdef DEBUG
      Serial.print("DEBUG: My IP address: ");
      Serial.println(Ethernet.localIP());
    #endif
      // start UDP
    this->_udpClient.begin(_rxPort);

    #ifdef DEBUG
      Serial.println("DEBUG: UDPClient::_init() completed");
    #endif
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
      
      MsgPacketizer::subscribe(_udpClient, MT_HANDSHAKE,
          [&](const protocol::HandshakeMessage& n) {
              _onHandshakeMessage(n);
          });
      MsgPacketizer::subscribe(_udpClient, MT_HEARTBEAT,
          [&](const protocol::HeartbeatMessage& n) {
              _onHeartbeatMessage(n);
          });
        subscribed = 1;
    }
    MsgPacketizer::update();
  }

  virtual void _sendHandshakeMessage()
  { 
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(_udpClient, _serverIP, _txPort, MT_HANDSHAKE, _getHandshakeMessage());
  }

  virtual void _sendHeartbeatMessage()
  { 
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(_udpClient, _serverIP, _txPort, MT_HEARTBEAT, _getHeartbeatMessage());
  }
  
  #ifdef DEBUG
  virtual void _sendDebugMessage(String& message)
  {
    MsgPacketizer::send(_udpClient, _serverIP, _txPort, MT_DEBUG, _getDebugMessage(message));
  }
  #endif
  
  uint8_t subscribed = false;
  EthernetUDP _udpClient;
  const char * _serverIP;
  int _rxPort;
  int _txPort;

  byte * _myMAC;
  #if DHCP == 0
    IPAddress & _myIP;
  #else
    uint32_t _dhcpMaintTimer = 0;
  #endif
};
#endif