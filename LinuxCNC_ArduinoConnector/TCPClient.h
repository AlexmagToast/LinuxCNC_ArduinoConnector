/*
TCPClient for Arduino

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
#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_
#include <ArduinoJson.h>  // include before MsgPacketizer.h
#include <MsgPacketizer.h>
#include <String.h>
#include "EthernetFuncs.h"
#include "Protocol.h"


enum TcpState
{
  TCP_DISCONNECTED = 0,
  TCP_CONNECTING,
  TCP_CONNECTED,
  TCP_DISCONNECTING,
  TCP_ERROR
};

class TCPClient {
public:
  #if DHCP == 1
    TCPClient(byte* macAddress, IPAddress& serverIP, uint16_t port=10001, uint32_t retryPeriod=3000)
  : _myMAC(macAddress), _serverIP(serverIP), _port(port), _retryPeriod(retryPeriod)
  {    
  }
  #else
    TCPClient(IPAddress& arduinoIP, byte* macAddress, IPAddress& serverIP, uint16_t port=10001, uint32_t retryPeriod=3000)
  : _myIP(arduinoIP), _myMAC(macAddress), _serverIP(serverIP), _port(port), _retryPeriod(retryPeriod)
  {
  }
  #endif

  String IpAddress2String(const IPAddress& ipAddress)
  {
    return String(ipAddress[0]) + String(".") +\
    String(ipAddress[1]) + String(".") +\
    String(ipAddress[2]) + String(".") +\
    String(ipAddress[3]); 
  }

  uint8_t isConnected()
  {
    return _client.connected();
  }
  const uint8_t msg_index = 0x12;
  


  uint8_t doWork()
  {



    //delay(1000);

    // must be called to trigger callback and publish data
    MsgPacketizer::update();
    #if DHCP == 1
      if(millis() > this->_dhcpMaintTimer + _retryPeriod /*reusing reconnect period for DCHP maint- may be wiser to have a seperate value in later version*/)
      {
        do_dhcp_maint();
        this->_dhcpMaintTimer = millis();
      }
    #endif

    switch(_myState)
    {
      case TCP_DISCONNECTED:
      {
        this->_client.stop();
        this->setState(TCP_CONNECTING);
        
        //if (_client != NULL)
        //  delete _client;
        //_client = new EthernetClient();
        this->_init();
        this->_timeNow = millis();
        #ifdef DEBUG
          Serial.print("DEBUG: TCP disconnected, retrying connection to ");
          Serial.print(this->IpAddress2String(this->_serverIP));
          Serial.print(":");
          Serial.print(this->_port);
          Serial.print(" in ");
          Serial.print(_retryPeriod/1000);
          Serial.println(" seconds...");
        #endif
        break;
      }
      case TCP_CONNECTING:
      {
        if(millis() > this->_timeNow + _retryPeriod)
        {
          if(!this->Connect())
          {
            this->setState(TCP_ERROR);
            return 1;
          }
          else
          {
           this->setState(TCP_CONNECTED);
          }
        }
        break;
      }
      case TCP_CONNECTED:
      {
        if(!this->isConnected())
        {
           this->setState(TCP_ERROR);
           return 1;
        }
        else
        {
            if (this->_client.available()) {
              char c = this->_client.read();
              Serial.print(c);
            }
            break;
        }
      }
      case TCP_ERROR:
      {
        this->setState(TCP_DISCONNECTED);
        return 0;
      }
    }
    return 0;
  }

  #ifdef DEBUG
  String stateToString(int& state)
  {
    switch(state)
    {
      case TCP_DISCONNECTED:
        return String("TCP_DISCONNECTED");
      case TCP_CONNECTING:
        return String("TCP_CONNECTING");
      case TCP_CONNECTED:
        return String("TCP_CONNECTED");
      case TCP_DISCONNECTING:
        return String("TCP_DISCONNECTING");
      case TCP_ERROR:
        return String("TCP_ERROR");
      default:
        return String("TCP_UNKNOWN_STATE");
    }
  }
  #endif
  void setState(int new_state)
  {
    #ifdef DEBUG
      Serial.print("DEBUG: TCP client transitioning from current state of [");
      Serial.print(this->stateToString(this->_myState));
      Serial.print("] to [");
      Serial.print(this->stateToString(new_state));
      Serial.println("]");
    #endif
    this->_myState = new_state;
  }
  private:

  void sendHandshakeMessage()
  {
    // TODO: Move this featuremap init to a spot that is not called constantly
    _hm.featureMap = featureMap;
    //hm.boardIndex = BOARD_INDEX;
    #ifdef DEBUG1
      Serial.println("DEBUG: ---- HANDSHAKE MESSAGE DUMP ----");
      Serial.print("DEBUG: Protocol Version: 0x");
      Serial.println(_hm.protocolVersion, HEX);
      Serial.print("DEBUG: Feature Map: ");
      Serial.println(_hm.featureMap, HEX);
      Serial.print("DEBUG: Board Index: ");
      Serial.println(_hm.boardIndex);
    #endif
    MsgPacketizer::send(this->_client, MT_HANDSHAKE, _hm);
  }
  uint8_t _init()
  {
    // disable SD card if one in the slot
    //pinMode(10,OUTPUT);
    //digitalWrite(10,HIGH);
   
    //Ethernet.init(ETHERNET_INIT_PIN);
    #ifdef DEBUG
      #if DHCP == 1
        Serial.println("DEBUG: Initializing Ethernet. DHCP = Enabled");
      #else
        Serial.print("DEBUG: Initializing Ethernet. DHCP = False. Static IP = ");
        Serial.println(this->IpAddress2String(this->_myIP));
      #endif
    #endif
    #if DHCP == 1
      #ifdef USE_ETHERNET_SHIELD_DELAY
        delay(JANKY_ETHERNET_SHIELD_DELAY);
      #endif
      if (Ethernet.begin(this->_myMAC) == 0) {
        #ifdef DEBUG
          Serial.print("DEBUG: Failed to configure Ethernet using DHCP");
        #endif

        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
          #ifdef DEBUG
              Serial.println("DEBUG: Ethernet shield was not found.  Sorry, can't run without hardware. :(");
          #endif


        } else if (Ethernet.linkStatus() == LinkOFF) {
          #ifdef DEBUG
              Serial.println("DEBUG: Ethernet cable is not connected.");
          #endif
          
        }
        // no point in carrying on, so do nothing forevermore:

        while (true) {

          delay(1);
      }
      }
    #else
      
      Ethernet.begin(this->_myMAC, this->_myIP); // Per Arduino documentation, only DHCP versio of .begin returns an int.
      // Ethernet with useful options
      // Ethernet.begin(mac, ip, dns, gateway, subnet); // full
      // Ethernet.setRetransmissionCount(4); // default: 8[times]
      // Ethernet.setRetransmissionTimeout(50); // default: 200[ms]

      
    #endif
    #ifdef DEBUG
      Serial.print("DEBUG: My IP address: ");
      Serial.println(Ethernet.localIP());
    #endif

    return 1;
  }
  
  uint8_t Connect()
  {
    //if ( this->isConnected() ) {
   //   return 0;
   // }
    //if (!this->Init())
    //  return 0;
    this->_client.setConnectionTimeout(TCP_CONNECTION_TIMEOUT);
    if (this->_client.connect(_serverIP, _port)) {
      return 1;
    } else {
      return 0;
    }
  }

  HandshakeMessage _hm;
  int _myState = TCP_DISCONNECTED;
  EthernetClient _client;
  IPAddress & _serverIP;
  uint16_t _port;
  uint32_t _retryPeriod = 0;
  uint32_t _timeNow = 0;

  
  byte * _myMAC;
  #if DHCP == 0
    IPAddress & _myIP;
  #else
    uint32_t _dhcpMaintTimer = 0;
  #endif
};
#endif