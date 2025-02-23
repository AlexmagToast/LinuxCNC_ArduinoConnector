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
      #ifdef A0
      case 0: return A0;
      #endif
      #ifdef A1
      case 1: return A1;
      #endif
      #ifdef A2
      case 2: return A2;
      #endif
      #ifdef A3
      case 3: return A3;
      #endif
      #ifdef A4
      case 4: return A4;
      #endif
      #ifdef A5
      case 5: return A5;
      #endif
      #ifdef A6
      case 6: return A6;
      #endif
      #ifdef A7
      case 7: return A7;
      #endif
      #ifdef A8
      case 8: return A8;
      #endif
      #ifdef A9
      case 9: return A9;
      #endif
      #ifdef A10
      case 10: return A10;
      #endif
      #ifdef A11
      case 11: return A11;
      #endif
      #ifdef A12
      case 12: return A12;
      #endif
      #ifdef A13
      case 13: return A13;
      #endif
      #ifdef A14
      case 14: return A14;
      #endif
      #ifdef A15
      case 15: return A15;
      #endif
      // ... (cases for A16 to A49)
      #ifdef A50
      case 50: return A50;
      #endif
    }
    return pinNum;  // Return the number if pin is not defined
  }

  return -1;  // Pin not found or not defined
}

#endif // PIN_MAP_H