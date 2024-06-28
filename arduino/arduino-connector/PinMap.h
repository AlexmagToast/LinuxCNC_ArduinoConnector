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

  // Handle 'D' pins
  if (pinStr[0] == 'D') {
    int pinNum = atoi(pinStr + 1);
    if (pinNum >= 0 && pinNum <= 50) {
      switch (pinNum) {
        #ifdef D0
        case 0: return D0;
        #endif
        #ifdef D1
        case 1: return D1;
        #endif
        #ifdef D2
        case 2: return D2;
        #endif
        #ifdef D3
        case 3: return D3;
        #endif
        #ifdef D4
        case 4: return D4;
        #endif
        #ifdef D5
        case 5: return D5;
        #endif
        #ifdef D6
        case 6: return D6;
        #endif
        #ifdef D7
        case 7: return D7;
        #endif
        #ifdef D8
        case 8: return D8;
        #endif
        #ifdef D9
        case 9: return D9;
        #endif
        #ifdef D10
        case 10: return D10;
        #endif
        #ifdef D11
        case 11: return D11;
        #endif
        #ifdef D12
        case 12: return D12;
        #endif
        #ifdef D13
        case 13: return D13;
        #endif
        #ifdef D14
        case 14: return D14;
        #endif
        #ifdef D15
        case 15: return D15;
        #endif
        #ifdef D16
        case 16: return D16;
        #endif
        #ifdef D17
        case 17: return D17;
        #endif
        #ifdef D18
        case 18: return D18;
        #endif
        #ifdef D19
        case 19: return D19;
        #endif
        #ifdef D20
        case 20: return D20;
        #endif
        #ifdef D21
        case 21: return D21;
        #endif
        #ifdef D22
        case 22: return D22;
        #endif
        #ifdef D23
        case 23: return D23;
        #endif
        #ifdef D24
        case 24: return D24;
        #endif
        #ifdef D25
        case 25: return D25;
        #endif
        #ifdef D26
        case 26: return D26;
        #endif
        #ifdef D27
        case 27: return D27;
        #endif
        #ifdef D28
        case 28: return D28;
        #endif
        #ifdef D29
        case 29: return D29;
        #endif
        #ifdef D30
        case 30: return D30;
        #endif
        #ifdef D31
        case 31: return D31;
        #endif
        #ifdef D32
        case 32: return D32;
        #endif
        #ifdef D33
        case 33: return D33;
        #endif
        #ifdef D34
        case 34: return D34;
        #endif
        #ifdef D35
        case 35: return D35;
        #endif
        #ifdef D36
        case 36: return D36;
        #endif
        #ifdef D37
        case 37: return D37;
        #endif
        #ifdef D38
        case 38: return D38;
        #endif
        #ifdef D39
        case 39: return D39;
        #endif
        #ifdef D40
        case 40: return D40;
        #endif
        #ifdef D41
        case 41: return D41;
        #endif
        #ifdef D42
        case 42: return D42;
        #endif
        #ifdef D43
        case 43: return D43;
        #endif
        #ifdef D44
        case 44: return D44;
        #endif
        #ifdef D45
        case 45: return D45;
        #endif
        #ifdef D46
        case 46: return D46;
        #endif
        #ifdef D47
        case 47: return D47;
        #endif
        #ifdef D48
        case 48: return D48;
        #endif
        #ifdef D49
        case 49: return D49;
        #endif
        #ifdef D50
        case 50: return D50;
        #endif
        default: return pinNum;  // Return the number if pin is not defined
      }
    }
  }

  // Handle 'A' pins
  if (pinStr[0] == 'A') {
    int pinNum = atoi(pinStr + 1);
    if (pinNum >= 0 && pinNum <= 50) {
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
        #ifdef A16
        case 16: return A16;
        #endif
        #ifdef A17
        case 17: return A17;
        #endif
        #ifdef A18
        case 18: return A18;
        #endif
        #ifdef A19
        case 19: return A19;
        #endif
        #ifdef A20
        case 20: return A20;
        #endif
        #ifdef A21
        case 21: return A21;
        #endif
        #ifdef A22
        case 22: return A22;
        #endif
        #ifdef A23
        case 23: return A23;
        #endif
        #ifdef A24
        case 24: return A24;
        #endif
        #ifdef A25
        case 25: return A25;
        #endif
        #ifdef A26
        case 26: return A26;
        #endif
        #ifdef A27
        case 27: return A27;
        #endif
        #ifdef A28
        case 28: return A28;
        #endif
        #ifdef A29
        case 29: return A29;
        #endif
        #ifdef A30
        case 30: return A30;
        #endif
        #ifdef A31
        case 31: return A31;
        #endif
        #ifdef A32
        case 32: return A32;
        #endif
        #ifdef A33
        case 33: return A33;
        #endif
        #ifdef A34
        case 34: return A34;
        #endif
        #ifdef A35
        case 35: return A35;
        #endif
        #ifdef A36
        case 36: return A36;
        #endif
        #ifdef A37
        case 37: return A37;
        #endif
        #ifdef A38
        case 38: return A38;
        #endif
        #ifdef A39
        case 39: return A39;
        #endif
        #ifdef A40
        case 40: return A40;
        #endif
        #ifdef A41
        case 41: return A41;
        #endif
        #ifdef A42
        case 42: return A42;
        #endif
        #ifdef A43
        case 43: return A43;
        #endif
        #ifdef A44
        case 44: return A44;
        #endif
        #ifdef A45
        case 45: return A45;
        #endif
        #ifdef A46
        case 46: return A46;
        #endif
        #ifdef A47
        case 47: return A47;
        #endif
        #ifdef A48
        case 48: return A48;
        #endif
        #ifdef A49
        case 49: return A49;
        #endif
        #ifdef A50
        case 50: return A50;
        #endif
        default: return pinNum;  // Return the number if pin is not defined
      }
    }
  }

  return -1;  // Pin not found or not defined
}

#endif // PIN_MAP_H