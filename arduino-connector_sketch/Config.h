
#ifndef CONFIG_H_
#define CONFIG_H_
#pragma once
#define ENABLE_FEATUREMAP

#define ENABLE_MSGPACKETIZER_CALLBACKS
//#define INTEGRATED_CALLBACKS

#define DEBUG                       0
#define DINPUTS                     1
#define DOUTPUTS                    2                      
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
//#define EEPROM_ENABLED              14


#ifndef EEPROM_ENABLED
  String uuid("ND");

#endif


#define DEBUG_VERBOSE
const uint16_t SERIAL_STARTUP_DELAY = 5000; // In milliseconds
const uint16_t SERIAL_RX_TIMEOUT = 10000;

#ifdef INTEGRATED_CALLBACKS
const uint16_t RX_BUFFER_SIZE = 128;
#endif

#define SERIAL_DEV Serial
#define COM_DEV Serial

#endif
