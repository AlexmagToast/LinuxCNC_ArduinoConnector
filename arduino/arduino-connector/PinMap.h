/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com &
  Ken Thompson (not THAT Ken Thompson), https://github.com/KennethThompson
  
  MIT License
  Copyright (c) 2023-2025 Alexander Richter & Ken Thompson

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
#ifndef PIN_MAP_H
#define PIN_MAP_H

#include <Arduino.h>

int convertPinString(const char* pinStr) {
  if (pinStr == NULL || *pinStr == '\0') {
    return -1;  // Invalid input
  }

  // Check if the input is just a number
  if (isdigit(pinStr[0]) || (pinStr[0] == '-' && isdigit(pinStr[1]))) {
    return atoi(pinStr);
  }

  int pinNum = -1;

  // Handle 'D' or 'A' pins
  if (pinStr[0] == 'D' || pinStr[0] == 'A') {
    pinNum = atoi(pinStr + 1);
    if (pinNum < 0 || pinNum > 50) {
      return -1;  // Invalid pin number
    }
  }

  // Handle 'D' pins
  if (pinStr[0] == 'D') {
    switch (pinNum) {
    #if defined(D0) || __has_include(<pins_arduino.h>)
      case 0: return D0;
    #else
      case 0: return 0;
    #endif
    #if defined(D1) || __has_include(<pins_arduino.h>)
      case 1: return D1;
    #else
      case 1: return 1;
    #endif
    #if defined(D2) || __has_include(<pins_arduino.h>)
      case 2: return D2;
    #else
      case 2: return 2;
    #endif
    #if defined(D3) || __has_include(<pins_arduino.h>)
      case 3: return D3;
    #else
      case 3: return 3;
    #endif
    #if defined(D4) || __has_include(<pins_arduino.h>)
      case 4: return D4;
    #else
      case 4: return 4;
    #endif
    #if defined(D5) || __has_include(<pins_arduino.h>)
      case 5: return D5;
    #else
      case 5: return 5;
    #endif
    #if defined(D6) || __has_include(<pins_arduino.h>)
      case 6: return D6;
    #else
      case 6: return 6;
    #endif
    #if defined(D7) || __has_include(<pins_arduino.h>)
      case 7: return D7;
    #else
      case 7: return 7;
    #endif
    #if defined(D8) || __has_include(<pins_arduino.h>)
      case 8: return D8;
    #else
      case 8: return 8;
    #endif
    #if defined(D9) || __has_include(<pins_arduino.h>)
      case 9: return D9;
    #else
      case 9: return 9;
    #endif
    #if defined(D10) || __has_include(<pins_arduino.h>)
      case 10: return D10;
    #else
      case 10: return 10;
    #endif
    
    }
    return pinNum;  // Return the number if pin is not defined
  }

  // Handle 'A' pins
  if (pinStr[0] == 'A') {
    switch (pinNum) {
      #ifdef PIN_A0 || __has_include(<pins_arduino.h>)
      case 0: return PIN_A0;
      #endif
      #ifdef PIN_A1 || __has_include(<pins_arduino.h>)
      case 1: return PIN_A1;
      #endif
      #ifdef PIN_A2 || __has_include(<pins_arduino.h>)
      case 2: return PIN_A2;
      #endif
      #ifdef PIN_A3 || __has_include(<pins_arduino.h>)
      case 3: return PIN_A3;
      #endif
      #ifdef PIN_A4 || __has_include(<pins_arduino.h>)
      case 4: return PIN_A4;
      #endif
      #ifdef PIN_A5 || __has_include(<pins_arduino.h>)
      case 5: return PIN_A5;
      #endif
      #ifdef PIN_A6 || __has_include(<pins_arduino.h>)
      case 6: return PIN_A6;
      #endif
      #ifdef PIN_A7 || __has_include(<pins_arduino.h>)
      case 7: return PIN_A7;
      #endif
      #ifdef PIN_A8 || __has_include(<pins_arduino.h>)
      case 8: return PIN_A8;
      #endif
      #ifdef PIN_A9 || __has_include(<pins_arduino.h>)
      case 9: return PIN_A9;
      #endif
      #ifdef PIN_A10 || __has_include(<pins_arduino.h>)
      case 10: return PIN_A10;
      #endif
      #ifdef PIN_A11 || __has_include(<pins_arduino.h>)
      case 11: return PIN_A11;
      #endif
      #ifdef PIN_A12 || __has_include(<pins_arduino.h>)
      case 12: return PIN_A12;
      #endif
      #ifdef PIN_A13 || __has_include(<pins_arduino.h>)
      case 13: return PIN_A13;
      #endif
      #ifdef PIN_A14 || __has_include(<pins_arduino.h>)
      case 14: return PIN_A14;
      #endif
      #ifdef A15 || __has_include(<pins_arduino.h>)
      case 15: return A15;
      #endif
    }
    return pinNum;  // Return the number if pin is not defined
  }

  return -1;  // Pin not found or not defined
}

#endif // PIN_MAP_H