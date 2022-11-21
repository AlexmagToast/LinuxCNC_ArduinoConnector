

//###IO's###

//###Analog In###
int SpOd[] = {A1,8,-1}; //Knob Spindle Overdrive on A1, has 4 Pos, and initialised with 0
int FdRes[] = {A2,3,-1};//Knob Feed Resolution on A2, has 9 Pos, and initialised with 0
const int SpSp = A3;//Knob Potentiometer for SpindleSpeed in manual mode

//###Digital In###
//Absolute encoder Knob
const int DI0 = 27;//1
const int DI1 = 28;//2
const int DI2 = 31;//4
const int DI3 = 29;//8
const int DI4 = 30;//16

//Buttons
const int Buttons = 17; //Number of Buttons (Inputs)
int Button[] = {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48};

//Digital Out###
const int Outputs = 22; //Number of Outputs (i use them for LEDs in Buttons)
int OutPin[] = {13,12,11,10,9,8,7,6,5,4,3,2,22,23,24,25,26};






//Variables for Saving States
int oldvar = -1;
int SpSpvar = -1;
int ButtonState[Buttons];
int OutState[Outputs];
int FdOdSt = -1;
int SpOdSt = -1;
int SpSpSt = -1;


void setup() {

  pinMode(DI0, INPUT_PULLUP);
  pinMode(DI1, INPUT_PULLUP);
  pinMode(DI2, INPUT_PULLUP);
  pinMode(DI3, INPUT_PULLUP);
  pinMode(DI4, INPUT_PULLUP);

//setting Buttons as Inputs with internal Pullup Resistors
  for(int i= 0; i<Buttons;i++){
    pinMode(Button[i], INPUT_PULLUP);
    ButtonState[i] = -1;
    }
    
  for(int o= 0; o<Outputs;o++){
    pinMode(OutPin[o], OUTPUT);
    OutState[o] = 0;
    }
//Setup Serial
  Serial.begin(9600);
  while (!Serial){
  }
  if (Serial){delay(1000);}
}


void loop() {

  listenSerial();
  readAbsKnob();
  readPoti(SpSp);
  
  SpOd[2] = readLPoti(SpOd[0],SpOd[1],SpOd[2]);
  FdRes[2] = readLPoti(FdRes[0],FdRes[1],FdRes[2]);
  writeOutputs();
  readInputs();
  delay(10);

}


int listenSerial(){
    long rec=0;
    if(Serial.available()){
      rec = Serial.parseInt();
      if(rec >= 10 && rec % 2){
        rec --;
        rec = rec/10;
          if(rec < Buttons){
            OutState[rec]=1;
          }
        }
      if(rec >= 10 && !(rec % 2)){
        rec = rec/10;
          if(rec < Buttons){
            OutState[rec]=0;
          }
      }
      rec= 0;
    }
}

int writeOutputs(){
    for(int x = 0; x<Outputs;x++){
    digitalWrite(OutPin[x], OutState[x]);
    }
}

  
int readLPoti(int Pin,int Pos,int Stat){
  int var = analogRead(Pin)+20; //giving it some margin so Numbers dont jitter
  Pos = 1024/Pos;
  var = var/Pos;
  if(var != Stat){
    Stat = var;
    Serial.print("LP");
    Serial.print(Pin);
    Serial.print(":");
    Serial.println(Stat);
    
  }
  return (Stat);
}



int readPoti(int Pin){
   unsigned long var = 0;;
   for(int i= 0;i<500; i++){
      var = var+ analogRead(Pin);
   }
   var = var / 500;
   if (SpSpSt!=var){
    Serial.print("Pt");
    Serial.print(Pin);
    Serial.print(":");
    Serial.println(var);
    SpSpSt = var;
   }
   
   return (var);
}




int readInputs(){
    for(int i= 0;i<Buttons; i++){
      int State = digitalRead(Button[i]);

      if(ButtonState[i]!= State){
        ButtonState[i] = State;
        Serial.print("I");
        Serial.print(i);
        Serial.print(":");
        Serial.println(ButtonState[i]);
      }
      
  }
}




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
