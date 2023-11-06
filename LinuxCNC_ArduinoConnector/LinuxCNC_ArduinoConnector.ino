/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com 2022

  This Software is used as IO Expansion for LinuxCNC. Here i am using a Mega 2560.

  It is NOT intended for timing and security relevant IO's. Don't use it for Emergency Stops or Endstop switches!

  You can create as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
  You can also generate "virtual Pins" by using latching Potentiometers, which are connected to one analog Pin, but are read in Hal as individual Pins.

  Currently the Software Supports: 
  - analog Inputs
  - latching Potentiometers
  - 1 binary encoded selector Switch
  - digital Inputs
  - digital Outputs
  - Matrix Keypad
  - Multiplexed LEDs
  - Quadrature encoders
  - Joysticks
  - Dallas-compatible Temperature Sensors
  
  The Send and receive Protocol is <Signal><PinNumber>:<Pin State>
  To begin Transmitting Ready is send out and expects to receive E: to establish connection. Afterwards Data is exchanged.
  Data is only send everythime it changes once.

  Inputs & Toggle Inputs  = 'I' -write only  -Pin State: 0,1
  Outputs                 = 'O' -read only   -Pin State: 0,1
  PWM Outputs             = 'P' -read only   -Pin State: 0-255
  Digital LED Outputs     = 'D' -read only   -Pin State: 0,1
  Analog Inputs           = 'A' -write only  -Pin State: 0-1024
  Latching Potentiometers = 'L' -write only  -Pin State: 0-max Position
  binary encoded Selector = 'K' -write only  -Pin State: 0-32
  rotary encoder          = 'R' -write only  -Pin State: up/ down / -2147483648 to 2147483647
  joystick                = 'R' -write only  -Pin State: up/ down / -2147483648 to 2147483647
  multiplexed LEDs        = 'M' -read only   -Pin State: 0,1
  temperature reading     = 'T' -read only   -Current Reading in C or F

Keyboard Input:
  Matrix Keypad           = 'M' -write only  -Pin State: 0,1

Communication Status      = 'E' -read/Write  -Pin State: 0:0

  The Keyboard is encoded in the Number of the Key in the Matrix. The according Letter is defined in the receiving end in the Python Skript.
  Here you only define the Size of the Matrix.

  Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal. If the Signal is not received again, the Status LED will Flash.
  The Board will still work as usual and try to send it's data, so this feature is only to inform the User.

  Copyright (c) 2023 Alexander Richter

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
/*
  Library dependencies (use Arduino Library Manager)
  MsgPacketizer >= V0.5.0 (REQUIRED, including all dependencies)
  
*/
/*
  Library dependencies (use Arduino Library Manager)
  MsgPacketizer >= V0.5.0 (REQUIRED, including all dependencies)
  
*/
#include "Version.h"

#include "Config.h"
#include "IOInterface.h"

#ifdef ENABLE_FEATUREMAP
  #include "FeatureMap.h"
  featureMap fm;
#endif

#ifdef MEMORY_MONITOR
#include <MemoryFree.h>
#include <pgmStrToRAM.h>
#endif


#ifdef ETHERNET_UDP_TO_LINUXCNC
  #include "UDPClient.h"
  #if DHCP == 1
    UDPClient _client(ARDUINO_MAC, SERVER_IP, fm.features, UDP_RX_PORT, UDP_TX_PORT, UDP_RX_TIMOUT);
  #else
    UDPClient _client(ARDUINO_IP, ARDUINO_MAC, SERVER_IP,  fm.features, UDP_RX_PORT, UDP_TX_PORT, UDP_RX_TIMOUT);
  #endif
  
#endif

#ifdef SERIAL_TO_LINUXCNC
  #include "SerialClient.h"
  SerialClient _client(SERIAL_RX_TIMEOUT, fm.features);
#endif


void setup() {

Serial.begin(DEFAULT_SERIAL_BAUD_RATE);
//Serial.println("Dumping Feature Map to Serial..");
#ifdef SERIAL_START_DELAY
  delay(SERIAL_START_DELAY);
#endif

#ifdef ENABLE_FEATUREMAP
  Serial.println("Dumping Feature Map to Serial..");
  fm.DumpFeatureMapToSerial();
#endif
#ifdef DEBUG
#ifdef MEMORY_MONITOR
  Serial.print(F("DEBUG: FREE RAM: "));
  Serial.println(freeMemory());
#endif
#endif
//_client.Init();
//_client.DoWork();  // At least ONE option must be selected in the config for connection to LinuxCNC (UDP/TCP/WIFI/SERIAL) - Otherwise, _client will be undefined and errors will be generated at compile.


#ifdef INPUTS
//setting Inputs with internal Pullup Resistors
  for(int i= 0; i<Inputs;i++){
    pinMode(InPinmap[i], INPUT_PULLUP);
    oldInState[i] = -1;
    }
#endif

#ifdef SINPUTS
//setting Inputs with internal Pullup Resistors
  for(int i= 0; i<sInputs;i++){
    pinMode(sInPinmap[i], INPUT_PULLUP);
    soldInState[i] = -1;
    togglesinputs[i] = 0;
  }
    
#endif
#ifdef AINPUTS

  for(int i= 0; i<AInputs;i++){
    pinMode(AInPinmap[i], INPUT);
    oldAinput[i] = -1;
    }
#endif
#ifdef OUTPUTS
  for(int o= 0; o<Outputs;o++){
    pinMode(OutPinmap[o], OUTPUT);
    oldOutState[o] = 0;
    }
#endif

#ifdef PWMOUTPUTS
  for(int o= 0; o<PwmOutputs;o++){
    pinMode(PwmOutPinmap[o], OUTPUT);
    oldOutPWMState[o] = 0;
    }
#endif

#ifdef DALLAS_TEMP_SENSOR

  for(int o= 0; o<TmpSensors;o++){
    pinMode(TmpSensorMap[o], OUTPUT);
    inTmpSensorState[o] = -1;
    // Setup a oneWire instance to communicate with any OneWire devices
    OneWire * oneWire = new OneWire(TmpSensorMap[o]);

    // Pass our oneWire reference to Dallas Temperature sensor 
    DallasTemperature * sensors = new DallasTemperature(oneWire);
    TmpSensorControlMap[o] = sensors;
    }
#endif

#ifdef STATUSLED
  pinMode(StatLedPin, OUTPUT);
#endif

#ifdef BINSEL
  pinMode(BinSelKnobPins[0], INPUT_PULLUP);
  pinMode(BinSelKnobPins[1], INPUT_PULLUP);
  pinMode(BinSelKnobPins[2], INPUT_PULLUP);
  pinMode(BinSelKnobPins[3], INPUT_PULLUP);
  pinMode(BinSelKnobPins[4], INPUT_PULLUP);
#endif

#ifdef DLED
  initDLED();
#endif

#ifdef KEYPAD
for(int col = 0; col < numCols; col++) {
  for (int row = 0; row < numRows; row++) {
    keys[row][col] = row * numRows + col;
  }
}
#endif


//Setup Serial
  //Serial.begin(DEFAULT_SERIAL_BAUD_RATE);
  //while (!Serial){}
  //comalive();
}

int prior_state = -1; //#ConnectionState::CS_DISCONNECTED;
void commUpdate(){
  const int new_state = _client.GetState();
  if( prior_state != new_state )
  {
    #ifdef DEBUG
      Serial.print("DEBUG: ConnectionState changed from: [");
      Serial.print(_client.stateToString(prior_state));
      Serial.print("] to [");
      Serial.print(_client.stateToString(_client.GetState()));
      Serial.println("]");
    #endif
    if (new_state == CS_DISCONNECTED)
      #ifdef STATUSLED
        StatLedErr(500,200);
      #endif
      #ifdef STATUSLED
        if(DLEDSTATUSLED == 1){
          #ifdef DLED
            controlDLED(StatLedPin, 1);
          #endif
        }
        else{
          digitalWrite(StatLedPin, HIGH);
        }
      #endif 
    if (new_state == CS_CONNECTED)
    {
      #ifdef STATUSLED
        StatLedErr(1000,1000);
      #endif
    }
    prior_state = new_state;
  }
  if (_client.GetState() != ConnectionState::CS_CONNECTED)
  {
    // Todo: Figure out what to do when not connected
  }
  else
  {
    if( _client.CommandReceived() == true )
    {
      auto cm = _client.GetReceivedCommand();
      for(auto b : cm.cmd)
      {
        pushCommand(b);
      }
    }
    //readCommands();
  }


}

void loop() {
  commUpdate();

  _client.DoWork(); // At least ONE option must be selected in the config for connection to LinuxCNC (UDP/TCP/WIFI/SERIAL) - Otherwise, _client will be undefined and errors will be generated at compile.


#ifdef INPUTS
  readInputs(); //read Inputs & send data
#endif
#ifdef DALLAS_TEMP_SENSOR
  readTmpInputs();
#endif
#ifdef SINPUTS
  readsInputs(); //read Inputs & send data
#endif
#ifdef AINPUTS
  readAInputs();  //read Analog Inputs & send data
#endif
#ifdef LPOTIS
  readLPoti(); //read LPotis & send data
#endif
#ifdef BINSEL
  readAbsKnob(); //read ABS Encoder & send data
#endif

#ifdef KEYPAD
  readKeypad(); //read Keyboard & send data
#endif

#ifdef QUADENC
  readEncoders(); //read Encoders & send data
#endif

#ifdef JOYSTICK
  readJoySticks(); //read Encoders & send data
#endif
#ifdef MULTIPLEXLEDS
  multiplexLeds();// cycle through the 2D LED Matrix}
#endif
  if( statusMessageReady == 1 )
  {
    _client.SendPinStatusMessage(statusMessage);
    statusMessageReady = 0;
  }
}
