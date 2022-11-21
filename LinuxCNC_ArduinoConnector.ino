//###IO's###
#define POTI
#ifdef POTI
  const int PotiNo = 1; 
  int PotiPins[] = {A3};                //Knob Potentiometer for SpindleSpeed in manual mode
#endif

#define LPOTI
#ifdef LPOTI
  const int LPotiNo = 2; 
  int LPotiPins[LPotiNo][2] = {
                    {A1,8},             //Latching Knob Spindle Overdrive on A1, has 9 Positions
                    {A2,3}              //Latching Knob Feed Resolution on A2, has 4 Positions
                    };
#endif

#define ABSENCODER
#ifdef ABSENCODER
  int AbsEncPins[] = {27,28,31,29,30};  //1,2,4,8,16
#endif


#define INPUTS
#ifdef INPUTS
  const int InputNo = 16;               //number of inputs using internal Pullup resistor. (short to ground to trigger)
  int InPins[] = {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48};
#endif


#define OUTPUTS
#ifdef OUTPUTS
  const int OutputNo = 22;              //number of outputs
  int OutPins[] = {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48};
#endif

#define STATUSLED
#ifdef STATUSLED
  const int StatLedPin = 13;                //Pin for Status LED
  const int StatLedErrDel[] = {1000,10};   //Blink Timing for Status LED Error (no connection)

#endif



//Variables for Saving States
#ifdef POTI 
  int oldPoti[PotiNo];
#endif
#ifdef LPOTI 
  int oldLpoti[LPotiNo];
#endif
#ifdef ABSENCODER
  int oldAbsEncState;
#endif
#ifdef INPUTS 
  int InState[InputNo];
  int oldInState[InputNo];
#endif
#ifdef OUTPUTS 
  int OutState[OutputNo];
  int oldOutState[OutputNo];
#endif

int FirstSend = 0;
unsigned long oldmillis = 0;




void setup() {


#ifdef ABSENCODER
  pinMode(AbsEncPins[0], INPUT_PULLUP);
  pinMode(AbsEncPins[1], INPUT_PULLUP);
  pinMode(AbsEncPins[2], INPUT_PULLUP);
  pinMode(AbsEncPins[3], INPUT_PULLUP);
  pinMode(AbsEncPins[4], INPUT_PULLUP);
#endif

#ifdef INPUT
//setting Inputs with internal Pullup Resistors
  for(int i= 0; i<InputNo;i++){
    pinMode(InPins[i], INPUT_PULLUP);
    oldInState[i] = -1;
    }
#endif
#ifdef OUTPUT
  for(int o= 0; o<OutputNo;o++){
    pinMode(OutPins[o], OUTPUT);
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
  if (Serial){delay(1000);}
}


void loop() {
  while (!Serial){
    #ifdef STATUSLED
      StatLedErr();
    #endif
  }




}




int StatLedErr(){
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


int writeOutputs(){
    for(int x = 0; x<OutputNo;x++){
    digitalWrite(OutPins[x], OutState[x]);
    }
}


int readLPoti(int Pin,int Pos, int Stat){
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


/*
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

*/
