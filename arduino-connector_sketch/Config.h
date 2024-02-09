
#ifndef CONFIG_H_
#define CONFIG_H_
#pragma once


#define DEBUG                       0
//#define DEBUG_VERBOSE               1
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


//#define ENABLE_MSGPACKETIZER_CALLBACKS
#define INTEGRATED_CALLBACKS
#ifdef LOWMEM
#define INTEGRATED_CALLBACKS_LOWMEMORY
#endif

#ifdef INTEGRATED_CALLBACKS_LOWMEMORY
  #define INTEGRATED_CALLBACKS
  #ifdef ENABLE_MSGPACKETIZER_CALLBACKS
    #undef ENABLE_MSGPACKETIZER_CALLBACKS
  #endif
#endif

#ifndef EEPROM_ENABLED
  String uuid("ND");
#endif


//#define DEBUG_VERBOSE
const uint16_t SERIAL_STARTUP_DELAY = 5000; // In milliseconds
const uint16_t SERIAL_RX_TIMEOUT = 10000;

#ifdef INTEGRATED_CALLBACKS
const uint16_t RX_BUFFER_SIZE = 256;
#endif

#define DEBUG_DEV Serial
#define COM_DEV Serial

#endif
