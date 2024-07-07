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
#ifndef CONFIG_H_
#define CONFIG_H_
#pragma once


#define DEBUG                       0
#define DEBUG_VERBOSE               1
#define FEATUREMAP                  2
#define LOWMEM                      3

#define DINPUTS                     4
#define DOUTPUTS                    5 
#define AINPUTS                     6
#define AOUTPUTS                    7                    
//#define SINPUTS                   3                     

//#define PWMOUTPUTS                4
//#define AINPUTS                   5   
//#define DALLAS_TEMP_SENSOR        6
//#define LPOTIS                    7
//#define BINSEL                    8
//#define QUADENC                   9
//#define JOYSTICK                  10
//#define STATUSLED                 11
//#define DLED                      12
//#define KEYPAD                    13
//#define EEPROM_ENABLED            14


#ifdef FEATUREMAP
#define ENABLE_FEATUREMAP
#endif

#ifdef LOWMEM
#define INTEGRATED_CALLBACKS_LOWMEMORY
#endif

const uint16_t SERIAL_STARTUP_DELAY = 5000; // In milliseconds
const uint16_t SERIAL_RX_TIMEOUT = 5000; // In milliseconds. On handhshake, the python side is told to use this value, times two, to determine connection timeouts. 


// Error Codes
const uint32_t ERR_NONE = 0x00000000;
const uint32_t ERR_NOT_IMPLEMENTED_BY_FEATURE = 0x00000001;
const uint32_t ERR_INVALID_FEATURE_ID = 0x00000002;
const uint32_t ERR_FEATURE_NOT_REGISTERED = 0x00000003;
const uint32_t ERR_INVALID_PIN_ID = 0x00000004;
const uint32_t ERR_INVALID_PARAMETER = 0x00000005;
const uint32_t ERR_INVALID_JSON = 0x00000006;

const uint16_t RX_BUFFER_SIZE = 256;

#define COM_DEV Serial
#include "ArduinoJson.h"
#include "SerialConnection.h"

SerialConnection serialClient(SERIAL_RX_TIMEOUT);
#define DEBUG_DEV serialClient

#endif