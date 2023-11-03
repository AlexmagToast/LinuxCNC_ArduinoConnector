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
#include "Protocol.h"


enum UDPState
{
  UDP_DISCONNECTED = 0,
  UDP_CONNECTING,
  UDP_CONNECTED,
  UDP_DISCONNECTING,
  UDP_ERROR
};

class UDPClient {
public:
  #if DHCP == 1
    UDPClient(byte* macAddress, const char* serverIP, int rxPort=10001,
    int txPort=10001, uint32_t retryPeriod=3000)
  : _myMAC(macAddress), _serverIP(serverIP), _rxPort(port), _txPort(port), _retryPeriod(retryPeriod)
  {    
  }
  #else
    UDPClient(IPAddress& arduinoIP, byte* macAddress, const char* serverIP, int rxPort, int txPort, uint32_t retryPeriod)
  : _myIP(arduinoIP), _myMAC(macAddress), _serverIP(serverIP), _rxPort(rxPort), _txPort(txPort), _retryPeriod(retryPeriod)
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

  uint8_t doWork()
  {
    if( _initialized == false)
    {
      this->_init();
      delay(1000);
      _initialized = true;
    }
    


    //delay(1000);
    


    #if DHCP == 1
      if(millis() > this->_dhcpMaintTimer + _retryPeriod)
      {
        do_dhcp_maint();
        this->_dhcpMaintTimer = millis();
      }
    #endif
    

    switch(_myState)
    {
      case UDP_DISCONNECTED:
      {
        this->setState(UDP_CONNECTING);
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
      case UDP_CONNECTING:
      {
        //Serial.println(_retryPeriod);
        if(millis() > this->_timeNow + _retryPeriod)
        {
          // TIMED OUT
          this->setState(UDP_ERROR);
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
      case UDP_CONNECTED:
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
      case UDP_ERROR:
      {
        this->setState(UDP_DISCONNECTED);
        //return 0;
      }
    }
        // must be called to trigger callback and publish data
    MsgPacketizer::update();
    
    return 0;
  }

  #ifdef DEBUG
  String stateToString(int& state)
  {
    switch(state)
    {
      case UDP_DISCONNECTED:
        return String("UDP_DISCONNECTED");
      case UDP_CONNECTING:
        return String("UDP_CONNECTING");
      case UDP_CONNECTED:
        return String("UDP_CONNECTED");
      case UDP_DISCONNECTING:
        return String("UDP_DISCONNECTING");
      case UDP_ERROR:
        return String("UDP_ERROR");
      default:
        return String("UDP_UNKNOWN_STATE");
    }
  }
  #endif
  void setState(int new_state)
  {
    #ifdef DEBUG
      Serial.print("DEBUG: UDP client transitioning from current state of [");
      Serial.print(this->stateToString(this->_myState));
      Serial.print("] to [");
      Serial.print(this->stateToString(new_state));
      Serial.println("]");
    #endif
    this->_myState = new_state;
  }
  
  private:

  uint8_t _init()
  {
    #ifdef DEBUG
      Serial.println("DEBUG: UDPClient::_init() called..");
    #endif
    _hm.featureMap = featureMap; // Set featureMap as its dynamically generated at runtime.

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

  void sendHandshakeMessage()
  {
    HandshakeMessage hm;
    hm.featureMap = featureMap;
    #ifdef DEBUG2
      Serial.println("DEBUG: ---- HANDSHAKE MESSAGE DUMP ----");
      Serial.print("DEBUG: Protocol Version: 0x");
      Serial.println(hm.protocolVersion, HEX);
      Serial.print("DEBUG: Feature Map: ");
      Serial.println(hm.featureMap, HEX);
      Serial.print("DEBUG: Board Index: ");
      Serial.println(hm.boardIndex);
      Serial.println("DEBUG: ---- END HANDSHAKE MESSAGE DUMP ----");
    #endif
    
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    //MsgPacketizer::send(_udpClient, _serverIP, _txPort, MT_HANDSHAKE, hm);

   //0 _udpClient.beginPacket(_udpClient.remoteIP(), _udpClient.remotePort());
   // _udpClient.write("hello");
   // _udpClient.endPacket();
    sendNTPpacket(_serverIP);
    delay(1000);
    //MsgPacketizer::update();
    //int packetSize = _udpClient.parsePacket();
    //delay(1000);
    
  }
  
 // send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, 48);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  _udpClient.beginPacket(address, 123); // NTP requests are to port 123
  _udpClient.write(packetBuffer, NTP_PACKET_SIZE);
  _udpClient.endPacket();
}
  int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

  byte packetBuffer[48]; //buffer to hold incoming and outgoing packets

  EthernetUDP _udpClient;
  int _myState = UDP_DISCONNECTED;
  char _rxBuffer[UDP_TX_PACKET_MAX_SIZE];
  const char * _serverIP;
  int _rxPort;
  int _txPort;
  uint32_t _retryPeriod = 0;
  uint32_t _timeNow = 0;
  uint8_t _initialized = false;

  HandshakeMessage _hm;

  byte * _myMAC;
  #if DHCP == 0
    IPAddress & _myIP;
  #else
    uint32_t _dhcpMaintTimer = 0;
  #endif
};
#endif