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

  Inputs & Toggle Inputs  = 'I' -write only  -Pin State: 0,1
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




//###################################################IO's###################################################


#define INPUTS                       //Use Arduino IO's as Inputs. Define how many Inputs you want in total and then which Pins you want to be Inputs.
#ifdef INPUTS
  const int Inputs = 5;               //number of inputs using internal Pullup resistor. (short to ground to trigger)
  int InPinmap[] = {37,38,39,40,41};
#endif

                                       //Use Arduino IO's as Toggle Inputs, which means Inputs (Buttons for example) keep HIGH State after Release and Send LOW only after beeing Pressed again. 
#define SINPUTS                        //Define how many Toggle Inputs you want in total and then which Pins you want to be Toggle Inputs.
#ifdef SINPUTS
  const int sInputs = 5;              //number of inputs using internal Pullup resistor. (short to ground to trigger)
  int sInPinmap[] = {32,33,34,35,36};
#endif

#define OUTPUTS                     //Use Arduino IO's as Outputs. Define how many Outputs you want in total and then which Pins you want to be Outputs.
#ifdef OUTPUTS
  const int Outputs = 9;              //number of outputs
  int OutPinmap[] = {10,9,8,7,6,5,4,3,2,21};
#endif

#define PWMOUTPUTS                     //Use Arduino PWM Capable IO's as PWM Outputs. Define how many  PWM Outputs you want in total and then which Pins you want to be  PWM Outputs.
#ifdef PWMOUTPUTS
  const int PwmOutputs = 2;              //number of outputs
  int PwmOutPinmap[] = {12,11};
#endif

#define AINPUTS                       //Use Arduino ADC's as Analog Inputs. Define how many Analog Inputs you want in total and then which Pins you want to be Analog Inputs.
                                        //Note that Analog Pin numbering is different to the Print on the PCB.
#ifdef AINPUTS
  const int AInputs = 1; 
  int AInPinmap[] = {1};                //Potentiometer for SpindleSpeed override
  int smooth = 200;                     //number of samples to denoise ADC, try lower numbers on your setup 200 worked good for me.
#endif


//#define LPOTIS                       
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
  int LPotiPins[LPotis][2] = {
                    {2,9},             //Latching Knob Spindle Overdrive on A1, has 9 Positions
                    {3,4}              //Latching Knob Feed Resolution on A2, has 4 Positions
                    };
  int margin = 20;                      //giving it some margin so Numbers dont jitter, make this number smaller if your knob has more than 50 Positions
#endif

#define ABSENCODER                   //Support of an Rotating Knob that was build in my Machine. It encodes 32 Positions with 5 Pins in Binary. This will generate 32 Pins in LinuxCNC Hal.
#ifdef ABSENCODER
  const int AbsEncPins[] = {27,28,31,29,30};  //1,2,4,8,16
#endif


//#define STATUSLED                   //The Software will detect if there is an communication issue. When you power on your machine, the Buttons etc won't work, till LinuxCNC is running. THe StatusLED will inform you about the State of Communication.
                                      // Slow Flash = Not Connected
                                      // Steady on = connected
                                      // short Flash = connection lost. 

                                      // if connection is lost, something happened. (Linuxcnc was closed for example or USB Connection failed.) It will recover when Linuxcnc is restartet. (you could also run "unloadusr arduino", "loadusr arduino" in Hal)
                                      // Define an Pin you want to connect the LED to. it will be set as Output indipendand of the OUTPUTS function, so don't use Pins twice.
                                      // If you use Digital LED's such as WS2812 or PL9823 (only works if you set up the DLED settings below) you can also define a position of the LED. In this case StatLedPin will set the number of the Digital LED Chain. 
#ifdef STATUSLED
  const int StatLedPin = 5;                //Pin for Status LED
  const int StatLedErrDel[] = {1000,10};   //Blink Timing for Status LED Error (no connection)
  const int DLEDSTATUSLED = 1;              //set to 1 to use Digital LED instead. set StatLedPin to the according LED number in the chain.
#endif


                                        
//#define DLED                       
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
                  {255,0,0},
                  {0,255,255},
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


//#define DEBUG

//###Misc Settings###
const int timeout = 10000;   // timeout after 10 sec not receiving Stuff
const int debounceDelay = 50;


//Variables for Saving States
#ifdef INPUTS
  int InState[Inputs];
  int oldInState[Inputs];
  unsigned long lastInputDebounce[Inputs];
#endif
#ifdef SINPUTS
  int sInState[sInputs];
  int soldInState[sInputs];
  int togglesinputs[sInputs];
  unsigned long lastsInputDebounce[sInputs];
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
#ifdef SINPUTS
  readsInputs(); //read Inputs & send data
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
  strip.setBrightness(DLEDBrightness);
  
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
      if(InState[i]!= State && millis()- lastInputDebounce[i] > debounceDelay){
        InState[i] = State;
        sendData('I',InPinmap[i],InState[i]);
      
      lastInputDebounce[i] = millis();
      }
    }
}
#endif
#ifdef SINPUTS
void readsInputs(){
  for(int i= 0;i<sInputs; i++){
    sInState[i] = digitalRead(sInPinmap[i]);
    if (sInState[i] != soldInState[i] && millis()- lastsInputDebounce[i] > debounceDelay){
      // Button state has changed and debounce delay has passed
      
      if (sInState[i] == LOW || soldInState[i]== -1) { // Stuff after || is only there to send States at Startup
        // Button has been pressed
        togglesinputs[i] = !togglesinputs[i];  // Toggle the LED state
      
        if (togglesinputs[i]) {
          sendData('I',sInPinmap[i],togglesinputs[i]);  // Turn the LED on
        } 
        else {
          sendData('I',sInPinmap[i],togglesinputs[i]);   // Turn the LED off
        }
      }
      soldInState[i] = sInState[i];
      lastsInputDebounce[i] = millis();
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
  #ifdef DLED
  if(cmd == 'D'){
    controlDLED(io,value);
  }
  #endif
  if(cmd == 'E'){
    lastcom=millis();
  }


  #ifdef DEBUG
    Serial.print("I Received= ");
    Serial.print(cmd);
    Serial.print(io);
    Serial.print(":");
    Serial.println(value);
  #endif
}

int isCmdChar(char cmd){
  if(cmd == 'O'||cmd == 'P'||cmd == 'E'||cmd == 'D') {return true;}
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
