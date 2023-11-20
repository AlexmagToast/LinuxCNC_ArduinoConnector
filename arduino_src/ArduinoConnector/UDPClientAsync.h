/*
UDPClientAsync for Arduino

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
#ifndef UDPCLIENTASYNC_H_
#define UDPCLIENTASYNC_H_
#include "Secret.h"
//#include <ArduinoJson.h>  // include before MsgPacketizer.h
#include <MsgPacketizer.h>
#include <string.h>
#include "WiFi.h"
#include "AsyncUDP.h"
#include "Connection.h"

using namespace protocol;
class UDPClientAsync : public ConnectionBase {
public:
  #if DHCP == 1
    UDPClientAsync(byte* macAddress, const char* serverIP, uint64_t& fm, int rxPort, int txPort, uint16_t retryPeriod)
  : ConnectionBase(retryPeriod, fm), _myMAC(macAddress), _serverIP(serverIP), _rxPort(rxPort), _txPort(txPort) 
  {    
  }
  #else
    UDPClientAsync(IPAddress& arduinoIP, byte* macAddress, const char* serverIP,  uint64_t& fm, int rxPort, int txPort, uint16_t retryPeriod)
  : ConnectionBase(retryPeriod, fm), _myIP(arduinoIP), _myMAC(macAddress), _serverIP(serverIP), _rxPort(rxPort), _txPort(txPort)
  {
  }
  #endif
  // Virtual interfaces from Connection class
  virtual void _onConnect(){}
  virtual void _onDisconnect(){
    if (WiFi.status() != WL_CONNECTED)
      _initialized = false; // reset wifi connection
  }
  virtual void _onError(){}
  uint8_t _onInit(){
    #ifdef DEBUG
      Serial.println("ARDUINO DEBUG: UDPClientAsync::onInit() called..");
    #endif
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(2000);
    WiFi.begin(ssid, password);
    Serial.print("ARDUINO DEBUG: Connecting to WiFi, SSID: ");
    Serial.print(ssid);
    int retryCount = 10; // 10 seconds of retry before resetting the connection
    while (WiFi.status() != WL_CONNECTED) {
      if (retryCount-- == 0)
      {
        Serial.print("ARDUINO DEBUG: Wifi connection timeout. Restarting connection.. ");
        return 0;
      }
      Serial.print('.');
      delay(1000);
    }
    #ifdef DEBUG
    Serial.print("ARDUINO DEBUG: Wifi assigned IP address: ");
    Serial.println(WiFi.localIP());
    #endif
    destip.fromString(_serverIP);
    //delay(5000);
    if(_udpClient.listen(_txPort)) 
    {
        #ifdef DEBUG_PROTOCOL_VERBOSE
        Serial.println("UDP connected");
        #endif
        _udpClient.onPacket([](AsyncUDPPacket packet) 
        {
        
            #ifdef DEBUG_PROTOCOL_VERBOSE
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            //Serial.write(packet.data(), packet.length());
            //Serial.println();
            for( int x = 0; x < packet.length(); x++ )
            {
              Serial.print("[0x");
              Serial.print(packet.data()[x], HEX);
              Serial.print("]");
            }
            Serial.println("");
            Serial.print("Received byte total: ");
            Serial.println( packet.length() );
            #endif
            
            MsgPacketizer::feed(packet.data(), packet.length());
            //MsgPacketizer::update();
        });
    }
  
  
        
    #ifdef DEBUG
      Serial.println("ARDUINO DEBUG: UDPClient::_init() completed");
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
      MsgPacketizer::subscribe_manual((uint8_t)MT_HANDSHAKE,
          [&](const protocol::HandshakeMessage& n) {
              _onHandshakeMessage(n);
          });
      MsgPacketizer::subscribe_manual((uint8_t)MT_HEARTBEAT,
          [&](const protocol::HeartbeatMessage& n) {
              _onHeartbeatMessage(n);
          });
      MsgPacketizer::subscribe_manual((uint8_t)MT_COMMAND,
          [&](const protocol::CommandMessage& n) {
              _onCommandMessage(n);
          });
        subscribed = 1;
    } 
    MsgPacketizer::update();
  }

  virtual void _sendHandshakeMessage()
  {
    const auto& packet = MsgPacketizer::encode(MT_HANDSHAKE, _getHandshakeMessage());
    _udpClient.writeTo((uint8_t*)packet.data.data(), packet.data.size(), destip, _txPort);
    //MsgPacketizer::send(_udpClient, _serverIP, _txPort, MT_HANDSHAKE, _getHandshakeMessage());
    //byte b[RX_BUFFER_SIZE];
    //size_t packetsize = _getHandshakeMessagePacked(_txBuffer);
  
    
    /*
    _getHandshakeMessage();
    MsgPack::Packer packer;
    packer.serialize(protocol::hm);
    uint8_t size = packer.size();
    _crc.add((uint8_t*)packer.data(), size);
    uint8_t crc = _crc.calc();
    uint8_t t = MT_HANDSHAKE;
    uint8_t eot = 0x00;
    _crc.restart();

    #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.print("START PACKER DUMP FOR MT_HANDSHAKE");
      Serial.print("SIZE: 0x");
      Serial.println(size, HEX);
      Serial.print("TYPE: 0x");
      Serial.println(t, HEX);
      for( int x = 0; x < packer.size(); x++ )
      {
        Serial.print("[0x");
        Serial.print(packer.data()[x], HEX);
        Serial.print("]");

      }
      Serial.print("CRC: ");
      Serial.println(crc, HEX);
      Serial.println("END PACKER DUMP");
    #endif

    byte outData[RX_BUFFER_SIZE];
    memcpy(outData, (void*)&size, sizeof(byte));
    memcpy(((byte*)outData)+1, (void*)&t, sizeof(byte));
    memcpy(((byte*)outData)+2, packer.data(), packer.size());
    memcpy(((byte*)outData)+2+packer.size(), (void*)&crc, sizeof(byte));
    memcpy(((byte*)outData)+2+packer.size()+1, (void*)&eot, sizeof(byte));
    uint8_t total = 4 + packer.size(); // 1 byte for length, 1 byte for message type/index, payload bytes, 1 byte for CRC, and 1 byte for null/0x00 terminator
    */

  }

  virtual void _sendHeartbeatMessage()
  { 
    const auto& packet = MsgPacketizer::encode(MT_HEARTBEAT, _getHeartbeatMessage());
    _udpClient.writeTo((uint8_t*)packet.data.data(), packet.data.size(), destip, _txPort);
    //size_t packetsize = _getHeartbeatMessagePacked(_txBuffer);
  
    //_udpClient.writeTo((uint8_t*)_txBuffer, packetsize, destip, _txPort);
   // size_t packetsize = _getHeartbeatMessagePacked(_txBuffer);
  
    //_udpClient.writeTo((uint8_t*)_txBuffer, packetsize, destip, _txPort);
    //_getHeatbeatMessage();
    //MsgPacketizer::send(this->_client, this->_mi, hm);
    //MsgPacketizer::send(_udpClient, _serverIP, _txPort, MT_HEARTBEAT, _getHeartbeatMessage());
  }
  
  #ifdef DEBUG
  virtual void _sendDebugMessage(String& message)
  {
    const auto& packet = MsgPacketizer::encode(MT_DEBUG, _getDebugMessage(message));
    _udpClient.writeTo((uint8_t*)packet.data.data(), packet.data.size(), destip, _txPort);
    //size_t packetsize = _getDebugMessagePacked(_txBuffer, message);
  
    //_udpClient.writeTo((uint8_t*)_txBuffer, packetsize, destip, _txPort);
   // MsgPacketizer::send(_udpClient, _serverIP, _txPort, MT_DEBUG, _getDebugMessage(message));
  }
  
  virtual void _sendPinStatusMessage()
  { 
    const auto& packet = MsgPacketizer::encode(MT_PINSTATUS, _getPinStatusMessage());
    _udpClient.writeTo((uint8_t*)packet.data.data(), packet.data.size(), destip, _txPort);
    //size_t packetsize = _getPinStatusMessagePacked(_txBuffer);
  
    //_udpClient.writeTo((uint8_t*)_txBuffer, packetsize, destip, _txPort);
    //MsgPacketizer::send(Serial, MT_PINSTATUS, _getPinStatusMessage());
  }

  #endif
  
  uint8_t subscribed = false;
  AsyncUDP _udpClient;
  const char * _serverIP;
  int _rxPort;
  int _txPort;
  IPAddress destip;

  byte * _myMAC;
  #if DHCP == 0
    IPAddress & _myIP;
  #else
    uint32_t _dhcpMaintTimer = 0;
  #endif
};
#endif