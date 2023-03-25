/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com 2022

  This Software is used as IO Expansion for LinuxCNC. Here i am using a Mega 2560.

  It is NOT intended for timing and security relevant IO's. Don't use it for Emergency Stops or Endstop switches!

  You can create as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
  You can also generate "virtual Pins" by using latching Potentiometers, which are connected to one analog Pin, but are read in Hal as individual Pins.

  Currently the Software provides: 
  - analog Inputs
  - latching Potentiometers
  - 1 absolute encoder input
  - digital Inputs
  - digital Outputs

  The Send and receive Protocol is <Signal><PinNumber>:<Pin State>
  To begin Transmitting Ready is send out and expects to receive E: to establish connection. Afterwards Data is exchanged.
  Data is only send everythime it changes once.

  Inputs                  = 'I' -write only  -Pin State: 0,1
  Outputs                 = 'O' -read only   -Pin State: 0,1
  PWM Outputs             = 'P' -read only   -Pin State: 0-255
  Digital LED Outputs     = 'D' -read only   -Pin State: 0,1
  Analog Inputs           = 'A' -write only  -Pin State: 0-1024
  Latching Potentiometers = 'L' -write only  -Pin State: 0-max Position
  Absolute Encoder input  = 'K' -write only  -Pin State: 0-32



  Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/




//###IO's###
//#define INPUTS                        
#ifdef INPUTS
  const int Inputs = 16;               //number of inputs using internal Pullup resistor. (short to ground to trigger)
  int InPinmap[] = {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48};
#endif


//#define OUTPUTS                       
#ifdef OUTPUTS
  const int Outputs = 9;              //number of outputs
  int OutPinmap[] = {10,9,8,7,6,5,4,3,2,21};
#endif

//#define PWMOUTPUTS                      
#ifdef PWMOUTPUTS
  const int PwmOutputs = 2;              //number of outputs
  int PwmOutPinmap[] = {12,11};
#endif

//#define AINPUTS                         
#ifdef AINPUTS
  const int AInputs = 1; 
  int AInPinmap[] = {1};                //Potentiometer for SpindleSpeed override
  int smooth = 200;                      //number of samples to denoise ADC, try lower numbers on your setup
#endif

//#define LPOTIS                          
#ifdef LPOTIS
  const int LPotis = 2; 
  int LPotiPins[LPotis][2] = {
                    {2,9},             //Latching Knob Spindle Overdrive on A1, has 9 Positions
                    {3,4}              //Latching Knob Feed Resolution on A2, has 4 Positions
                    };
  int margin = 20;                      //giving it some margin so Numbers dont jitter, make this number smaller if your knob has more than 50 Positions
#endif

//#define ABSENCODER                      
#ifdef ABSENCODER
  const int AbsEncPins[] = {27,28,31,29,30};  //1,2,4,8,16
#endif


#define STATUSLED                       
#ifdef STATUSLED
  const int StatLedPin = 0;                //Pin for Status LED
  const int StatLedErrDel[] = {1000,10};   //Blink Timing for Status LED Error (no connection)
  const int DLEDSTATUSLED = 1;              //set to 1 to use Digital LED instead. set StatLedPin to the according LED number in the chain.
#endif

/* Instead of connecting LED's to Output pins, you can also connect digital LED's such as DLED or PL9823. 
DLEDcount defines, how many Digital LED's you want to control. For Each a output Pin will be generated in LinuxCNC hal. There are two modes supported.
You set the color here. Set the Parameter as {Greeb,Red,Blue}. When LinuxCNC sends the state = 1, the LED will be set to the specified color. State = 0 will shut the LED to the specified off color. 
This allows you to either turn the LED On at any specified color or to flip color to show Status change. (Red and Green for example)
 
*/
#define DLED                       
#ifdef DLED
  #include <Adafruit_NeoPixel.h>

  const int DLEDcount = 8;              //How Many DLED LED's are you going to connect?
  const int DLEDPin = 4;

 
  int DledOnColors[DLEDcount][3] = {
                  {255,0,0},
                  {0,0,255},
                  {0,255,0},
                  {0,255,0},
                  {0,255,0},
                  {0,255,0},
                  {0,255,0},
                  {0,255,0}
                  };

  int DledOffColors[DLEDcount][3] = {
                  {0,0,0},
                  {0,255,0},
                  {255,0,0},
                  {255,0,0},
                  {255,0,0},
                  {0,0,255},
                  {0,0,255},
                  {0,0,255}
                };


Adafruit_NeoPixel strip(DLEDcount, DLEDPin, NEO_GRB + NEO_KHZ800);//Color sequence is different for LED Chipsets. Use RGB for WS2812  or GRB for PL9823.


#endif




//###Misc Settings###
const int timeout = 10000;   // timeout after 10 sec not receiving Stuff

//#define DEBUG

//Variables for Saving States
#ifdef INPUTS
  int InState[Inputs];
  int oldInState[Inputs];
#endif
#ifdef OUTPUTS
  int OutState[Outputs];
  int oldOutState[Outputs];
#endif
#ifdef PWMOUTPUTS
  int OutPWMState[PwmOutputs];
  int oldOutPWMState[PwmOutputs];
#endif
#ifdef AINPUTS
  int oldAinput[AInputs];
#endif
#ifdef LPOTIS
  int Lpoti[LPotis];
  int oldLpoti[LPotis];
#endif
#ifdef ABSENCODER
  int oldAbsEncState;
#endif



//### global Variables setup###
//Please don't touch them
unsigned long oldmillis = 0;
unsigned long newcom = 0;
unsigned long lastcom = 0;

#define STATE_CMD 0
#define STATE_IO 1
#define STATE_VALUE 2


byte state = STATE_CMD;
char inputbuffer[5];
byte bufferIndex = 0;
char cmd = 0;
uint16_t io = 0;
uint16_t value = 0;



void setup() {

#ifdef INPUTS
//setting Inputs with internal Pullup Resistors
  for(int i= 0; i<Inputs;i++){
    pinMode(InPinmap[i], INPUT_PULLUP);
    oldInState[i] = -1;
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
#ifdef STATUSLED
  pinMode(StatLedPin, OUTPUT);
#endif

#ifdef ABSENCODER
  pinMode(AbsEncPins[0], INPUT_PULLUP);
  pinMode(AbsEncPins[1], INPUT_PULLUP);
  pinMode(AbsEncPins[2], INPUT_PULLUP);
  pinMode(AbsEncPins[3], INPUT_PULLUP);
  pinMode(AbsEncPins[4], INPUT_PULLUP);
#endif

#ifdef DLED
  initDLED();
#endif

//Setup Serial
  Serial.begin(115200);
  while (!Serial){}
  while (lastcom == 0){
    readCommands();
    flushSerial();
    Serial.println("E0:0");
    #ifdef STATUSLED
      StatLedErr(1000,1000);
    #endif
    }

}


void loop() {

  readCommands(); //receive and execute Commands 
  comalive(); //if nothing is received for 10 sec. blink warning LED 


#ifdef INPUTS
  readInputs(); //read Inputs & send data
#endif
#ifdef AINPUTS
  readAInputs();  //read Analog Inputs & send data
#endif
#ifdef LPOTIS
  readLPoti(); //read LPotis & send data
#endif
#ifdef ABSENCODER
  readAbsKnob(); //read ABS Encoder & send data
#endif


}


void comalive(){
  #ifdef STATUSLED
    if(millis() - lastcom > timeout){
      StatLedErr(1000,10);
    }
    else{
      if(DLEDSTATUSLED){controlDLED(StatLedPin, 1);}
      else{digitalWrite(StatLedPin, HIGH);}
    }
  #endif
}



void sendData(char sig, int pin, int state){
        Serial.print(sig);
        Serial.print(pin);
        Serial.print(":");
        Serial.println(state);
}

void flushSerial(){
  while (Serial.available() > 0) {
  Serial.read();
  }
}

#ifdef STATUSLED
void StatLedErr(int offtime, int ontime){
  unsigned long newMillis = millis();
  
  if (newMillis - oldmillis >= offtime){
      
      if(DLEDSTATUSLED){controlDLED(StatLedPin, 1);}
      else{digitalWrite(StatLedPin, HIGH);}
    } 
  if (newMillis - oldmillis >= offtime+ontime){{
      if(DLEDSTATUSLED){controlDLED(StatLedPin, 0);}
      else{digitalWrite(StatLedPin, LOW);}
      oldmillis = newMillis;
    }
  }

}
#endif

#ifdef OUTPUTS
void writeOutputs(int Pin, int Stat){
  digitalWrite(Pin, Stat);
}
#endif

#ifdef PWMOUTPUTS
void writePwmOutputs(int Pin, int Stat){
  analogWrite(Pin, Stat);
}

#endif

#ifdef DLED
void initDLED(){
  strip.begin();
  strip.setBrightness(50);
  
    for (int i = 0; i < DLEDcount; i++) {
    strip.setPixelColor(i, strip.Color(DledOffColors[i][0],DledOffColors[i][1],DledOffColors[i][2]));
  }
  strip.show();
  #ifdef DEBUG
    Serial.print("DLED initialised");
  #endif
}

void controlDLED(int Pin, int Stat){
  if(Stat == 1){
    strip.setPixelColor(Pin, strip.Color(DledOnColors[Pin][0],DledOnColors[Pin][1],DledOnColors[Pin][2]));
    #ifdef DEBUG
      Serial.print("DLED No.");
      Serial.print(Pin);
      Serial.print(" set to:");
      Serial.println(Stat);
    #endif
  }
  else{strip.setPixelColor(Pin, strip.Color(DledOffColors[Pin][0],DledOffColors[Pin][1],DledOffColors[Pin][2]));
    #ifdef DEBUG
      Serial.print("DLED No.");
      Serial.print(Pin);
      Serial.print(" set to:");
      Serial.println(Stat);
    #endif
  }
  
  strip.show();
  

}
#endif

#ifdef LPOTI
int readLPoti(){
    for(int i= 0;i<LPotis; i++){
      int var = analogRead(LPotiPins[i][0])+margin;
      int pos = 1024/(LPotiPins[i][1]-1);
      var = var/pos;
      if(oldLpoti[i]!= var){
        oldLpoti[i] = var;
        sendData('L', LPotiPins[i][0],oldLpoti[i]);
      }
    }
}


#endif


#ifdef AINPUTS
int readAInputs(){
   unsigned long var = 0;
   for(int i= 0;i<AInputs; i++){
      int State = analogRead(AInPinmap[i]);
      for(int i= 0;i<smooth; i++){// take couple samples to denoise signal
        var = var+ analogRead(AInPinmap[i]);
      }
      var = var / smooth;
      if(oldAinput[i]!= var){
        oldAinput[i] = var;
        sendData('A',AInPinmap[i],oldAinput[i]);
      }
    }
}
#endif
#ifdef INPUTS
void readInputs(){
    for(int i= 0;i<Inputs; i++){
      int State = digitalRead(InPinmap[i]);
      if(InState[i]!= State){
        InState[i] = State;
        sendData('I',InPinmap[i],InState[i]);
      }
    }
}
#endif

#ifdef ABSENCODER
int readAbsKnob(){
  int var = 0;
  if(digitalRead(AbsEncPins[0])==1){
    var += 1;
  }
  if(digitalRead(AbsEncPins[1])==1){
    var += 2;
  }
  if(digitalRead(AbsEncPins[2])==1){
    var += 4;
  }  
  if(digitalRead(AbsEncPins[3])==1){
    var += 8;
  }  
  if(digitalRead(AbsEncPins[4])==1){
    var += 16;
  }
  if(var != oldAbsEncState){
    Serial.print("K0:");
    Serial.println(var);
    }
  oldAbsEncState = var;
  return (var);
}
#endif

void commandReceived(char cmd, uint16_t io, uint16_t value){
  #ifdef OUTPUTS
  if(cmd == 'O'){
    writeOutputs(io,value);
  }
  #endif
  #ifdef PWMOUTPUTS
  if(cmd == 'P'){
    writePwmOutputs(io,value);
  }
  #endif
  if(cmd == 'E'){
    lastcom=millis();
  }
  #ifdef DLED
  if(cmd == 'D'){
    controlDLED(io,value);
  }
  #endif

  #ifdef DEBUG
    Serial.print("I Received= ");
    Serial.print(cmd);
    Serial.print(io);
    Serial.print(":");
    Serial.println(value);
  #endif
}

int isCmdChar(char cmd){
  if(cmd == 'O'||cmd == 'P'||cmd == 'E'||cmd == 'L') {return true;}
  else{return false;}
}

void readCommands(){
    byte current;
    while(Serial.available() > 0){
        current = Serial.read();
        switch(state){
            case STATE_CMD:
                if(isCmdChar(current)){
                    cmd = current;
                    state = STATE_IO;
                    bufferIndex = 0;
                }
                break;
            case STATE_IO:
                if(isDigit(current)){
                    inputbuffer[bufferIndex++] = current;
                }else if(current == ':'){
                    inputbuffer[bufferIndex] = 0;
                    io = atoi(inputbuffer);
                    state = STATE_VALUE;
                    bufferIndex = 0;

                }
                else{
                    #ifdef DEBUG
                    Serial.print("Ungültiges zeichen: ");
                    Serial.println(current);
                    #endif
                }
                break;
            case STATE_VALUE:
                if(isDigit(current)){
                    inputbuffer[bufferIndex++] = current;
                }
                else if(current == '\n'){
                    inputbuffer[bufferIndex] = 0;
                    value = atoi(inputbuffer);
                    commandReceived(cmd, io, value);
                    state = STATE_CMD;
                }
                else{
                  #ifdef DEBUG
                  Serial.print("Ungültiges zeichen: ");
                  Serial.println(current);
                  #endif
                
                }
                break;
        }

    }
}
