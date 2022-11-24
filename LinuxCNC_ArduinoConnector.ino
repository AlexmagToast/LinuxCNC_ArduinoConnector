/*
This Software is used as IO Expansion for LinuxCNC. Here i am using an Mega 2560.
It can use as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
I also generate "virtual Pins" by using latching Potentiometers, which are connected to one analog Pin, but are read in Hal as individual Pins.


The Send Protocol is <Signal><Pin Number>:<Pin State>

Inputs are encoded with Letter 'I'
Keep alive Signal is send with Letter 'E'
Outputs are encoded with Letter 'O'
PWM Outputs are encoded with Letter 'P'
Analog Inputs are encoded with Letter 'A'
Latching Potentiometers are encoded with Letter 'L'
Absolute Encoder input is encoded with Letter 'K'
*/


//###IO's###
#define INPUTS                        
#ifdef INPUTS
  const int Inputs = 16;               //number of inputs using internal Pullup resistor. (short to ground to trigger)
  int InPinmap[] = {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48};
#endif


#define OUTPUTS                       
#ifdef OUTPUTS
  const int Outputs = 9;              //number of outputs
  int OutPinmap[] = {10,9,8,7,6,5,4,3,2,21};
#endif

#define PWMOUTPUTS                      
#ifdef PWMOUTPUTS
  const int PwmOutput = 2;              //number of outputs
  int PwmOutPinmap[] = {12,11};
#endif

#define AINPUTS                         
#ifdef AINPUTS
  const int AInputs = 1; 
  int AInPinmap[] = {A3};                //Potentiometer for SpindleSpeed override
#endif

#define LPOTIS                          
#ifdef LPOTIS
  const int LPotis = 2; 
  int LPotiPins[LPotis][2] = {
                    {A1,8},             //Latching Knob Spindle Overdrive on A1, has 9 Positions
                    {A2,3}              //Latching Knob Feed Resolution on A2, has 4 Positions
                    };
#endif

#define ABSENCODER                      
#ifdef ABSENCODER
  const int AbsEncPins[] = {27,28,31,29,30};  //1,2,4,8,16
#endif


#define STATUSLED                       
#ifdef STATUSLED
  const int StatLedPin = 13;                //Pin for Status LED
  const int StatLedErrDel[] = {1000,10};   //Blink Timing for Status LED Error (no connection)

#endif

//###Misc Settings###
const int timeout = 10000;   // timeout after 10 sec not receiving Stuff

#define DEBUG

//Variables for Saving States
#ifdef INPUTS
  int InState[Inputs];
  int oldInState[Inputs];
#endif
#ifdef OUTPUTS
  int OutState[Outputs];
  int oldOutState[Outputs];
#endif
#ifdef AINPUTS
  int oldAinput[AInputs];
#endif
#ifdef LPOTIS
  int oldLpoti[LPotis];
#endif
#ifdef ABSENCODER
  int oldAbsEncState;
#endif



//### global Variables setup###
//Diese variablen nicht von außen anfassen
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


#ifdef ABSENCODER
  pinMode(AbsEncPins[0], INPUT_PULLUP);
  pinMode(AbsEncPins[1], INPUT_PULLUP);
  pinMode(AbsEncPins[2], INPUT_PULLUP);
  pinMode(AbsEncPins[3], INPUT_PULLUP);
  pinMode(AbsEncPins[4], INPUT_PULLUP);
#endif

#ifdef INPUTS
//setting Inputs with internal Pullup Resistors
  for(int i= 0; i<Inputs;i++){
    pinMode(InPinmap[i], INPUT_PULLUP);
    oldInState[i] = -1;
    }
#endif

#ifdef OUTPUTS
  for(int o= 0; o<Outputs;o++){
    pinMode(OutPinmap[o], OUTPUT);
    oldOutState[o] = 0;
    }
#endif


#ifdef STATUSLED
  pinMode(StatLedPin, OUTPUT);
#endif


//Setup Serial
  Serial.begin(115200);
  while (!Serial){
    #ifdef STATUSLED
      StatLedErr();
    #endif
  }
  if (Serial){
    delay(1000);
    flushSerial();
    Serial.println("Ready");
    }

}


void loop() {
  while (!Serial){
    #ifdef STATUSLED
      StatLedErr();
    #endif
  }

readCommands();
#ifdef INPUTS
//  readInputs();
#endif



}


void comalive(){
  if(millis() - lastcom > timeout){
    StatLedErr();
  }
}

void StatLedErr(){
  unsigned long newMillis = millis();

  if (newMillis - oldmillis >= StatLedErrDel[0]){

      digitalWrite(StatLedPin, HIGH);
    } 
  if (newMillis - oldmillis >= StatLedErrDel[0]+StatLedErrDel[1]){{
      digitalWrite(StatLedPin, LOW);
      oldmillis = newMillis;
    }
  }

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
/*
void readData(){
  int pin = 0;
  int state = 0;
  byte terminated = false;

  if (Serial.available() > 0) {
    
    char inChar = Serial.read();
    Serial.println(inChar);
    if (inChar == 'o'){
      Serial.println("O erkannt");
      while (!terminated && comalive()){
        
        inChar = Serial.read();
        if (inChar == ':'){
      }
      
    }

    if (inChar == 'p'){
      Serial.println("p erkannt");
      sig = 'p';
    }

  }
}
}

*/
void writeOutputs(){
    for(int x = 0; x<Outputs;x++){
    digitalWrite(OutPinmap[x], OutState[x]);
    }
}


int readLPoti(int Pin,int Pos, int Stat){
  int var = analogRead(Pin)+20; //giving it some margin so Numbers dont jitter
  Pos = 1024/Pos;
  var = var/Pos;
   return (Stat);
}

int readAIn(int Pin){
   unsigned long var = 0;
   for(int i= 0;i<500; i++){
      var = var+ analogRead(Pin);
   }
   var = var / 500;
   return (var);
}

void readInputs(){
    for(int i= 0;i<Inputs; i++){
      int State = digitalRead(InPinmap[i]);
      if(InState[i]!= State){
        InState[i] = State;
        sendData('I',InPinmap[i],InState[i]);
      }
  }
}


/*
int readAbsKnob(){
  int var = 0;
  if(digitalRead(DI0)==1){
    var += 1;
  }
  if(digitalRead(DI1)==1){
    var += 2;
  }
  if(digitalRead(DI2)==1){
    var += 4;
  }  
  if(digitalRead(DI3)==1){
    var += 8;
  }  
  if(digitalRead(DI4)==1){
    var += 16;
  }
  if(var != oldvar){
    Serial.print("AK:");
    Serial.println(var);
    }
  oldvar = var;
  return (var);
}
*/

void commandReceived(char cmd, uint16_t io, uint16_t value){
  switch(state){
    case
  }      Serial.print(cmd);
        Serial.print(io);
        Serial.print(":");
        Serial.println(value);

}

int isCmdChar(char cmd){
  if(cmd == 'O'||cmd == 'P'||cmd == 'E') {return true;}
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

                }else{
                    Serial.print("Ungültiges zeichen: ");
                    Serial.println(current);
                }
                break;
            case STATE_VALUE:
                if(isDigit(current)){
                    inputbuffer[bufferIndex++] = current;
                }else if(current == '\n'){
                    inputbuffer[bufferIndex] = 0;
                    value = atoi(inputbuffer);
                    commandReceived(cmd, io, value);
                    state = STATE_CMD;
                }else{
                    Serial.print("Ungültiges zeichen: ");
                    Serial.println(current);
                }
                break;
        }

    }
}
