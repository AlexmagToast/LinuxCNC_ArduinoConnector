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
#ifndef UDPCLIENT_H_
#define UDPCLIENT_H_

#include <ArduinoJson.h>  // include before MsgPacketizer.h
#include <MsgPacketizer.h>
#include <String.h>
#include "EthernetFuncs.h"
#include "Connection.h"


class UDPClient :virtual public ConnectionBase {
public:
  #if DHCP == 1
    UDPClient(byte* macAddress, const char* serverIP, uint64_t& fm, int rxPort=10001, int txPort, uint32_t retryPeriod)
  : ConnectionBase(retryPeriod, fm), _myMAC(macAddress), _serverIP(serverIP), _rxPort(port), _txPort(port), 
  {    
  }
  #else
    UDPClient(IPAddress& arduinoIP, byte* macAddress, const char* serverIP,  uint64_t& fm, int rxPort, int txPort, uint32_t retryPeriod)
  : ConnectionBase(retryPeriod, fm), _myIP(arduinoIP), _myMAC(macAddress), _serverIP(serverIP), _rxPort(rxPort), _txPort(txPort)
  {
  }
  #endif
  // Virtual interfaces from Connection class
  virtual void onConnect(){}
  virtual void onDisconnect(){}
  virtual void onError(){}
  uint8_t onInit(){
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
        Serial.println(this->IpAddress2String(this->_myIP));
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


  void onDoWork()
  {
    if( _initialized == false)
    {
      this->onInit();
      delay(1000);
      _initialized = true;
    }

    #if DHCP == 1
      if(millis() > this->_dhcpMaintTimer + _retryPeriod)
      {
        do_dhcp_maint();
        this->_dhcpMaintTimer = millis();
      }
    #endif
    

    switch(_myState)
    {
      case CS_DISCONNECTED:
      {
        this->setState(CS_CONNECTING);
        this->sendHandshakeMessage();
        
        this->_timeNow = millis();
        /*
        #ifdef DEBUG
          Serial.print("DEBUG: UDP disconnected, retrying connection to ");
          Serial.print(this->IpAddress2String(this->_serverIP));
          Serial.print(":");
          Serial.print(this->_txPort);
          Serial.print(" in ");
          Serial.print(_retryPeriod/1000);
          Serial.println(" seconds...");
        #endif
        */
        break;
      }
      case CS_CONNECTING:
      {
        //Serial.println(_retryPeriod);
        if(millis() > this->_timeNow + _retryPeriod)
        {
          // TIMED OUT
          this->setState(CS_ERROR);
          /*
          if(!this->Connect())
          {
           
            return 1;
          }
          else
          {
           this->setState(UDP_CONNECTED);
          }
          */
        }
        break;
      }
      case CS_CONNECTED:
      {
        
          int packetSize = _udpClient.parsePacket();
          if (packetSize) {
            Serial.print("Received packet of size ");
            Serial.println(packetSize);
            Serial.print("From ");
            IPAddress remote = _udpClient.remoteIP();
            for (int i=0; i < 4; i++) {
              Serial.print(remote[i], DEC);
              if (i < 3) {
                Serial.print(".");
              }
            }
            Serial.print(", port ");
            Serial.println(_udpClient.remotePort());
          }
          
          break;
      }
      case CS_ERROR:
      {
        this->setState(CS_DISCONNECTED);
        //return 0;
      }
    }
        // must be called to trigger callback and publish data
    MsgPacketizer::update();
  }
  
  private:
  void sendHandshakeMessage()
  {
    HandshakeMessage hm;
    hm.featureMap = this->_featureMap;
    #ifdef DEBUG
      Serial.println("DEBUG: ---- HANDSHAKE MESSAGE DUMP ----");
      Serial.print("DEBUG: Protocol Version: 0x");
      Serial.println(hm.protocolVersion, HEX);
      Serial.print("DEBUG: Feature Map: 0x");
      Serial.println(hm.featureMap, HEX);
      Serial.print("DEBUG: Board Index: ");
      Serial.println(hm.boardIndex);
      Serial.println("DEBUG: ---- END HANDSHAKE MESSAGE DUMP ----");
    #endif
    
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    MsgPacketizer::send(_udpClient, _serverIP, _txPort, MT_HANDSHAKE, hm);
  }

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