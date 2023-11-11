#ifndef CONFIG_H_
#define CONFIG_H_

#define ENABLE_FEATUREMAP
#define DEBUG                     0
#define DEBUG_PROTOCOL_VERBOSE    1
//s#define INPUTS                    2                       
//#define SINPUTS                   3                      
//#define OUTPUTS                   4
//#define PWMOUTPUTS                5
//#define AINPUTS                   6   
//#define DALLAS_TEMP_SENSOR        7
//#define LPOTIS                    8
//#define BINSEL                    9
//#define QUADENC                   10
//#define JOYSTICK                  11
//#define STATUSLED                 12
//#define DLED                      13
//#define KEYPAD                    14
//#define SERIAL_TO_LINUXCNC        15 // Only select ONE option for the connection type
//#define ETHERNET_UDP_TO_LINUXCNC  16
//#define ETHERNET_TCP_TO_LINUXCNC 17 // FUTURE
//#define WIFI_TCP_TO_LINUXCNC      18 // FUTURE
//#define WIFI_UDP_TO_LINUXCNC     19 // FUTURE
#define WIFI_UDP_ASYNC_TO_LINUXCNC 20
//#define MEMORY_MONITOR              21 // Requires https://github.com/mpflaga/Arduino-MemoryFree/

//################################################### SERIAL CONNECTION OPTIONS ###################################################
#define DEFAULT_SERIAL_BAUD_RATE 115200
#define SERIAL_START_DELAY 3000 // To avoid initial serial output failing to arrive during debugging.
//#define ENABLE_SERIAL2 TRUE // For future

const uint16_t RX_BUFFER_SIZE = 512; // Serial, TCP and UDP connections utilize this constant for their RX buffers



const uint8_t BOARD_INDEX = 0; // Each board connecting to the server should have a differnet index number.

#ifdef SERIAL_TO_LINUXCNC
const uint16_t SERIAL_RX_TIMEOUT = 5000; // This value is used by the Serial-version of the Connection object as the amount of time beween retries of messages such as MT_HANDSHAKE and 2*SERIAL_RX_TIMEOUT as the connection timeout period. MINIMUM RECOMMENDED TIMEOUT = 1000.  Highly recommended that the timeout be set to 1000ms or greater.
#endif
//################################################### ETHERNET CONNECTION OPTIONS ###################################################
// Requires an Arduino / Shield that is compatible with the Arduino Ethernet Library
// Tested and working models:
//      - Ethernet Network Shield W5100
#ifdef ETHERNET_UDP_TO_LINUXCNC
#include <SPI.h>
#include <Ethernet.h>
#endif

#if defined(ETHERNET_UDP_TO_LINUXCNC) || defined(WIFI_UDP_ASYNC_TO_LINUXCNC)
#define DHCP 0// 1 for DHCP, 0 for static.  DHCP support is highly expiremental and leaving this option disabled (i.e., using a static IP address) is recommended.
const uint16_t UDP_RX_TIMOUT = 2500;
const int UDP_RX_PORT = 54321;
const int UDP_TX_PORT = 54321;


// Should you want to have multiple arduiono boards connecting to the same server, remember to change the IP address (if using static IPs) and MAC address of each Arduino to be destinct

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte ARDUINO_MAC[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

#if defined(WIFI_UDP_ASYNC_TO_LINUXCNC)
  #define DHCP 1 // Future TODO: Enable static IP over Wifi
#endif

#if DHCP == 0
  IPAddress ARDUINO_IP(192, 168, 1, 88);
#endif

  //IPAddress SERVER_IP(192, 168, 1, 2);
  const char* SERVER_IP = "192.168.1.2";

// 10 = Most Boards
// 5 = MKR ETH Shield
// 0 = Teensy 2.0
// 20 = Teensy++ 2.0
// 15 = ESP8266 with Adafruit Featherwing Ethernet
// 33 =  ESP32 with Adafruit Featherwing Ethernet
#define ETHERNET_INIT_PIN 10 // Most Arduino shields


#endif
//###################################################IO's###################################################

                 
#ifdef INPUTS //Use Arduino IO's as Inputs. Define how many Inputs you want in total and then which Pins you want to be Inputs.
  const int Inputs = 2;               //number of inputs using internal Pullup resistor. (short to ground to trigger)
  int InPinmap[] = {2, 3};
#endif

                                       //Use Arduino IO's as Toggle Inputs, which means Inputs (Buttons for example) keep HIGH State after Release and Send LOW only after beeing Pressed again. 
                                       //Define how many Toggle Inputs you want in total and then which Pins you want to be Toggle Inputs.
#ifdef SINPUTS
  const int sInputs = 1;              //number of inputs using internal Pullup resistor. (short to ground to trigger)
  int sInPinmap[] = {10};
#endif

                    //Use Arduino IO's as Outputs. Define how many Outputs you want in total and then which Pins you want to be Outputs.
#ifdef OUTPUTS
  const int Outputs = 1;              //number of outputs
  int OutPinmap[] = {4};
#endif

                    //Use Arduino PWM Capable IO's as PWM Outputs. Define how many  PWM Outputs you want in total and then which Pins you want to be  PWM Outputs.
#ifdef PWMOUTPUTS
  const int PwmOutputs = 2;              //number of outputs
  int PwmOutPinmap[] = {12,11};
#endif

                 //Use Arduino ADC's as Analog Inputs. Define how many Analog Inputs you want in total and then which Pins you want to be Analog Inputs.
                                        //Note that Analog Pin numbering is different to the Print on the PCB.
#ifdef AINPUTS
  const int AInputs = 1; 
  int AInPinmap[] = {0};                //Potentiometer for SpindleSpeed override
  int smooth = 200;                     //number of samples to denoise ADC, try lower numbers on your setup 200 worked good for me.
#endif


#ifdef DALLAS_TEMP_SENSOR
// Required Libaries: DallasTemperature, OneWire
// In the US, Dallas-Compatible temp sensors can be found on Amazon at: https://www.amazon.com/gp/product/B08V93CTM2/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
// The current listing is titled: Gikfun DS18B20 Waterproof Digital Temperature Sensor with Adapter Module for Arduino (Pack of 3 Sets) EK1183
// The nice thing about this sensor option is the provided pull-up circuitry and filter PCB (I believe its a filter cap, but I could be wrong)
  #include <OneWire.h>
  #include <DallasTemperature.h>

  const int TmpSensors = 2;  // The Dallas-compatible sesnsors can share a pin and be indexed by an int value when reading.
  // This version is expecting 1 sensor per pin. Future todo: add multi sensors per-pin support
  int TmpSensorMap[] = {2,3};
  DallasTemperature * TmpSensorControlMap[TmpSensors];
  #define TEMP_OUTPUT_C 1 // 1 to output in C, any other value to output in F
#endif




                       
/*This is a special mode of AInputs. My machine had originally Selector Knobs with many Pins on the backside to select different Speed Settings.
I turned them into a "Potentiometer" by connecting all Pins with 10K Resistors in series. Then i applied GND to the first and 5V to the last Pin.
Now the Selector is part of an Voltage Divider and outputs different Voltage for each Position. This function generates Pins for each Position in Linuxcnc Hal.

It can happen, that when you switch position, that the selector is floating for a brief second. This might be detected as Position 0. 
This shouldn't be an issue in most usecases, but think about that in your application.



Connect it to an Analog In Pin of your Arduino and define how many of these you want. 
Then in the Array, {which Pin, How many Positions}
Note that Analog Pin numbering is different to the Print on the PCB.                                        

*/

#ifdef LPOTIS
  const int LPotis = 2; 
  const int LPotiPins[LPotis][2] = {
                    {1,9},             //Latching Knob Spindle Overdrive on A1, has 9 Positions
                    {2,4}              //Latching Knob Feed Resolution on A2, has 4 Positions
                    };
  int margin = 20;                      //giving it some margin so Numbers dont jitter, make this number smaller if your knob has more than 50 Positions
#endif



                //Support of an Rotating Knob that was build in my Machine. It encodes 32 Positions with 5 Pins in Binary. This will generate 32 Pins in LinuxCNC Hal.
#ifdef BINSEL
  const int BinSelKnobPins[] = {2,6,4,3,5};  //1,2,4,8,16
#endif


          
//Support for Quadrature Encoders. Define Pins for A and B Signals for your encoders. Visit https://www.pjrc.com/teensy/td_libs_Encoder.html for further explanation.
// Download Zip from here: https://github.com/PaulStoffregen/Encoder and import as Library to your Arduino IDE. 
#ifdef QUADENC
  #include <Encoder.h>
  #define QUADENCS 2  //how many Rotary Encoders do you want?
  
    // Encoders have 2 signals, which must be connected to 2 pins. There are three options.

    //Best Performance: Both signals connect to interrupt pins.
    //Good Performance: First signal connects to an interrupt pin, second to a non-interrupt pin.
    //Low Performance: Both signals connect to non-interrupt pins, details below. 

    //Board	            Interrupt Pins	            LED Pin(do not use)
    //Teensy 4.0 - 4.1	All Digital Pins	          13
    //Teensy 3.0 - 3.6	All Digital Pins	          13
    //Teensy LC	        2 - 12, 14, 15, 20 - 23	    13
    //Teensy 2.0	      5, 6, 7, 8	                11
    //Teensy 1.0	      0, 1, 2, 3, 4, 6, 7, 16	
    //Teensy++ 2.0	    0, 1, 2, 3, 18, 19, 36, 37  6
    //Teensy++ 1.0	    0, 1, 2, 3, 18, 19, 36, 37	
    //Arduino Due	      All Digital Pins	          13
    //Arduino Uno	      2, 3	                      13
    //Arduino Leonardo	0, 1, 2, 3	                13
    //Arduino Mega	    2, 3, 18, 19, 20, 21	      13
    //Sanguino	        2, 10, 11	                  0

Encoder Encoder0(2,3);      //A,B Pin
Encoder Encoder1(31,33);    //A,B Pin
//Encoder Encoder2(A,B);
//Encoder Encoder3(A,B);
//Encoder Encoder4(A,B);                      
  const int QuadEncSig[] = {2,2};   //define wich kind of Signal you want to generate. 
                                  //1= send up or down signal (typical use for selecting modes in hal)
                                  //2= send position signal (typical use for MPG wheel)
  const int QuadEncMp[] = {4,4};   //some Rotary encoders send multiple Electronical Impulses per mechanical pulse. How many Electrical impulses are send for each mechanical Latch?            

#endif

                  //Support of an Rotating Knob that was build in my Machine. It encodes 32 Positions with 5 Pins in Binary. This will generate 32 Pins in LinuxCNC Hal.
#ifdef JOYSTICK
const int JoySticks = 1;             // Number of potentiometers connected
const int JoyStickPins[JoySticks*2] = {0, 1}; // Analog input pins for the potentiometers
const int middleValue = 512;        // Middle value of the potentiometer
const int deadband = 20;            // Deadband range around the middleValue
const float scalingFactor = 0.01;   // Scaling factor to control the impact of distanceFromMiddle
#endif






//The Software will detect if there is an communication issue. When you power on your machine, the Buttons etc won't work, till LinuxCNC is running. THe StatusLED will inform you about the State of Communication.
// Slow Flash = Not Connected
// Steady on = connected
// short Flash = connection lost. 

// if connection is lost, something happened. (Linuxcnc was closed for example or USB Connection failed.) It will recover when Linuxcnc is restartet. (you could also run "unloadusr arduino", "loadusr arduino" in Hal)
// Define an Pin you want to connect the LED to. it will be set as Output indipendand of the OUTPUTS function, so don't use Pins twice.
// If you use Digital LED's such as WS2812 or PL9823 (only works if you set up the DLED settings below) you can also define a position of the LED. In this case StatLedPin will set the number of the Digital LED Chain. 


#ifdef STATUSLED
  const int StatLedPin = 13;                //Pin for Status LED
  const int StatLedErrDel[] = {1000,10};   //Blink Timing for Status LED Error (no connection)
  const int DLEDSTATUSLED = 0;              //set to 1 to use Digital LED instead. set StatLedPin to the according LED number in the chain.
#endif


                                        
                       
/* Instead of connecting LED's to Output pins, you can also connect digital LED's such as WS2812 or PL9823. 
This way you can have how many LED's you want and also define it's color with just one Pin.

DLEDcount defines, how many Digital LED's you want to control. Count from 0. For Each LED an output Pin will be generated in LinuxCNC hal.
To use this funcion you need to have the Adafruit_NeoPixel.h Library installed in your Arduino IDE.

In LinuxCNC you can set the Pin to HIGH and LOW, for both States you can define an color per LED. 
This way, you can make them glow or shut of, or have them Change color, from Green to Red for example. 

DledOnColors defines the color of each LED when turned "on". For each LED set {Red,Green,Blue} with Numbers from 0-255. 
depending on the Chipset of your LED's Colors might be in a different order. You can try it out by setting {255,0,0} for example. 

You need to define a color to DledOffColors too. Like the Name suggests it defines the color of each LED when turned "off".
If you want the LED to be off just define {0,0,0}, .


If you use STATUSLED, it will also take the colors of your definition here.
*/

#ifdef DLED
  #include <Adafruit_NeoPixel.h>

  const int DLEDcount = 8;              //How Many DLED LED's are you going to connect?
  const int DLEDPin = 4;                  //Where is DI connected to?
  const int DLEDBrightness = 70;         //Brightness of the LED's 0-100%
 
  int DledOnColors[DLEDcount][3] = {
                  {0,0,255},
                  {255,0,0},
                  {0,255,0},
                  {0,255,0},
                  {0,255,0},
                  {0,255,0},
                  {0,255,0},
                  {0,255,0}
                  };

  int DledOffColors[DLEDcount][3] = {
                  {0,0,0},
                  {0,0,0},
                  {255,0,0},
                  {255,0,0},
                  {255,0,0},
                  {0,0,255},
                  {0,0,255},
                  {0,0,255}
                };


Adafruit_NeoPixel strip(DLEDcount, DLEDPin, NEO_GRB + NEO_KHZ800);//Color sequence is different for LED Chipsets. Use RGB for WS2812  or GRB for PL9823.


#endif
/*
Matrix Keypads are supported. The input is NOT added as HAL Pin to LinuxCNC. Instead it is inserted to Linux as Keyboard direktly. 
So you could attach a QWERT* Keyboard to the arduino and you will be able to write in Linux with it (only while LinuxCNC is running!)
*/

#ifdef KEYPAD
const int numRows = 4;  // Define the number of rows in the matrix 
const int numCols = 4;  // Define the number of columns in the matrix

// Define the pins connected to the rows and columns of the matrix
const int rowPins[numRows] = {2, 3, 4, 5};
const int colPins[numCols] = {6, 7, 8, 9};
int keys[numRows][numCols] = {0};
int lastKey= -1;
#endif


//#define MULTIPLEXLEDS // Special mode for Multiplexed LEDs. This mode is experimental and implemented to support Matrix Keyboards with integrated Key LEDs.
// check out this thread on LinuxCNC Forum for context. https://forum.linuxcnc.org/show-your-stuff/49606-matrix-keyboard-controlling-linuxcnc
// for Each LED an Output Pin is generated in LinuxCNC.

//If your Keyboard shares pins with the LEDs, you have to check polarity. 
//rowPins[numRows] = {} are Pullup Inputs
//colPins[numCols] = {} are GND Pins
//the matrix keyboard described in the thread shares GND Pins between LEDs and KEys, therefore LedGndPins[] and colPins[numCols] = {} use same Pins. 

#ifdef MULTIPLEXLEDS

const int numVccPins = 8;      // Number of rows in the matrix
const int numGndPins = 8;      // Number of columns in the matrix
const int LedVccPins[] = {30,31,32,33,34,35,36,37}; // Arduino pins connected to rows
const int LedGndPins[] = {40,41,42,43,44,45,46,47}; // Arduino pins connected to columns

// Define the LED matrix
int ledStates[numVccPins*numGndPins] = {0};

unsigned long previousMillis = 0;
const unsigned long interval = 500; // Time (in milliseconds) per LED display

int currentLED = 0;
#endif


//###Misc Settings###
const int timeout = 10000;   // timeout after 10 sec not receiving Stuff
const int debounceDelay = 50;


//#######################################   END OF CONFIG     ###########################
#endif // #define CONFIG_H_