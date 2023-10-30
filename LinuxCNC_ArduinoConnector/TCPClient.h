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
//#include <Ethernet.h>
#include <String.h>

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
  TCPClient(IPAddress& serverIP, uint16_t port, uint32_t retryPeriod, uint16_t maxMessageSize=512, uint8_t protocolVersion=1) 
  : _serverIP(serverIP), _port(port), _retryPeriod(retryPeriod), _maxMessageSize(maxMessageSize), _protocolVersion(protocolVersion)
  {
  }

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

  uint8_t doWork()
  {

    switch(_myState)
    {
      case TCP_DISCONNECTED:
      {
        this->setState(TCP_CONNECTING);
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
            if (_client.available()) {
              char c = _client.read();
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

    //if(this->isConnected() == 0)
    //{
     // this->Connect();
    //}
    return 0;
  }

private:

  uint8_t Connect()
  {
    if ( this->isConnected() ) {
      return 0;
    }
    if (_client.connect(_serverIP, _port)) {
      return 1;
    } else {
      return 0;
    }
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

  int _myState = TCP_DISCONNECTED;
  EthernetClient _client;
  IPAddress & _serverIP;
  uint16_t _port;
  uint32_t _retryPeriod = 0;
  uint32_t _timeNow = 0;
  uint16_t _maxMessageSize;
  uint8_t _protocolVersion;
};
#endif