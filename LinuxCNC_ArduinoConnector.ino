/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com 2022

  This Software is used as IO Expansion for LinuxCNC.

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

Keyboard Input:
  Matrix Keypad           = 'M' -write only  -Pin State: 0,1

Communication Status      = 'E' -read/Write  -Pin State: 0:0

  The Keyboard is encoded in the Number of the Key in the Matrix. The according Letter is defined in the receiving end in the Python Skript.
  Here you only define the Size of the Matrix.

  Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal. If the Signal is not received again, the Status LED will Flash.
  The Board will still work as usual and try to send it's data, so this feature is only to inform the User.


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

#include "firmware2.h"


//###Misc Settings###
const int debounceDelay = 50;


//Variables for Saving States
#ifdef INPUTS
  int InState[DigitalInputs];
  int oldInState[DigitalInputs];
  unsigned long lastInputDebounce[DigitalInputs];
#endif
#ifdef SINPUTS
  int sInState[sInputs];
  int soldInState[sInputs];
  int togglesinputs[sInputs];
  unsigned long lastsInputDebounce[sInputs];
#endif
#ifdef OUTPUTS
  int OutState[DigitalOutputs];
  int oldOutState[DigitalOutputs];
#endif
#ifdef PWMOUTPUTS
  int OutPWMState[PwmOutputs];
  int oldOutPWMState[PwmOutputs];
#endif
#ifdef AINPUTS
  int oldAinput[ANALOG_INPUTS];
  unsigned long sumAinput[ANALOG_INPUTS];
#endif
#ifdef LPOTIS
  int Lpoti[LPotis];
  int oldLpoti[LPotis];
#endif
#ifdef BINSEL
  int oldAbsEncState;
#endif
#ifdef KEYPAD
  byte KeyState = 0;
#endif
#ifdef MULTIPLEXLEDS
  byte KeyLedStates[numVccPins*numGndPins];
#endif
#if QuadratureEncoders == 1
  const int QuadEncs = 1;
#endif
#if QuadratureEncoders == 2
  const int QuadEncs = 2;
#endif
#if QuadratureEncoders == 3
  const int QuadEncs = 3;
#endif
#if QuadratureEncoders == 4
  const int QuadEncs = 4;
#endif
#if QuadratureEncoders == 5
  const int QuadEncs = 5;
#endif
#ifdef QUADENC
  long EncCount[QuadratureEncoders];
  long OldEncCount[QuadratureEncoders];
#endif
#ifdef DIGITALLEDS
  #include <Adafruit_NeoPixel.h>
  Adafruit_NeoPixel strip(DLedChainLength, DLedPin, NEO_GRB + NEO_KHZ800);//Color sequence is different for LED Chipsets. Use RGB for WS2812  or GRB for PL9823.


#endif

#ifdef JOYSTICK
long counter[Joysticks*2] = {0};      // Initialize an array for the counters
long prevCounter[Joysticks*2] = {0};  // Initialize an array for the previous counters
float incrementFactor[Joysticks*2] = {0.0}; // Initialize an array for the incrementFactors
unsigned long lastUpdateTime[Joysticks*2] = {0}; // Store the time of the last update for each potentiometer

#endif

//### global Variables setup###
//Please don't touch them
unsigned long oldmillis = 0;
unsigned long newcom = 0;
unsigned long lastcom = 0;
int connectionState = 0;

#define STATE_CMD 0
#define STATE_IO 1
#define STATE_VALUE 2


byte state = STATE_CMD;
char inputbuffer[5];
byte bufferIndex = 0;
char cmd = 0;
uint16_t io = 0;
uint16_t value = 0;

// Function Prototypes
void readCommands();
void commandReceived(char cmd, uint16_t io, uint16_t value);
void multiplexLeds();
void readKeypad();
int readAbsKnob();
void readsInputs();
void readInputs();
void readAInputs();
void readLPoti();
void controlDLED(int Pin, int Stat);
void initDLED();
void writePwmOutputs(int Pin, int Stat);
void writeOutputs(int Pin, int Stat);
void StatLedErr(int offtime, int ontime);
void flushSerial();
void sendData(char sig, int pin, int state);
void reconnect();
void comalive();
void readEncoders();
void readJoySticks();

void setup() {

#ifdef INPUTS
//setting Inputs
  for(int i= 0; i<DigitalInputs;i++){
    if(InPinPullups[i] == 0){
      pinMode(InPinmap[i], INPUT);
    }
    else if(InPinPullups[i] == 1){
      pinMode(InPinmap[i], INPUT_PULLUP);
    }
    oldInState[i] = -1;
    }
#endif

#ifdef AINPUTS

  for(int i= 0; i<ANALOG_INPUTS;i++){
    pinMode(AInPinmap[i], INPUT);
    oldAinput[i] = -1;
    sumAinput[i] = 0;
    }
#endif
#ifdef OUTPUTS
  for(int o= 0; o<DigitalOutputs;o++){
    pinMode(OutPinmap[o], OUTPUT);
    oldOutState[o] = OutPinInit[o];
    //todo: add code to set initial state
    }
#endif

#ifdef PWMOUTPUTS
  for(int o= 0; o<PwmOutputs;o++){
    pinMode(PwmOutPinmap[o], OUTPUT);
    oldOutPWMState[o] = PWMPinInit[o];
    //todo: add code to set initial state
    }
#endif
#ifdef STATUSLED
  pinMode(StatusLed, OUTPUT);
#endif

#ifdef BINSEL
  pinMode(BSSPinmap[0], INPUT_PULLUP);
  pinMode(BSSPinmap[1], INPUT_PULLUP);
  pinMode(BSSPinmap[2], INPUT_PULLUP);
  pinMode(BSSPinmap[3], INPUT_PULLUP);
  pinMode(BSSPinmap[4], INPUT_PULLUP);
#endif

#ifdef DIGITALLEDS
  initDLED();
#endif

#ifdef KEYPAD
for(int col = 0; col < numCols; col++) {
  for (int row = 0; row < numRows; row++) {
    keys[row][col] = row * numCols + col;
  }
}
#endif


//Setup Serial
  Serial.begin(CONNECTION_BAUDRATE);
  while (!Serial){}
  comalive();
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
#ifdef BINSEL
  readAbsKnob(); //read ABS Encoder & send data
#endif

#ifdef KEYPAD
  readKeypad(); //read Keyboard & send data
#endif

#ifdef QUADENCS
  readEncoders(); //read Encoders & send data
#endif
#ifdef DIGITALLEDS
  initDLED();
#endif
#ifdef JOYSTICK
  readJoySticks(); //read Encoders & send data
#endif
#ifdef MULTIPLEXLEDS
  multiplexLeds();// cycle through the 2D LED Matrix}
#endif
}

#ifdef JOYSTICK

void readJoySticks() {
  for (int i = 0; i < Joysticks*2; i++) {
    unsigned long currentTime = millis(); // Get the current time

    // Check if it's time to update the counter for this potentiometer
    if (currentTime - lastUpdateTime[i] >= 100) { // Adjust 100 milliseconds based on your needs
      lastUpdateTime[i] = currentTime; // Update the last update time for this potentiometer

      int potValue = analogRead(JSPinmap[i]); // Read the potentiometer value

      // Calculate the distance of the potentiometer value from the middle
      int distanceFromMiddle = potValue - JSCenter[i];

      // Apply deadband to ignore small variations around middleValue
      if (abs(distanceFromMiddle) <= JSDeadband[i]) {
        incrementFactor[i] = 0.0; // Set incrementFactor to 0 within the deadband range
      } else {
        // Apply non-linear scaling to distanceFromMiddle to get the incrementFactor
        incrementFactor[i] = pow((distanceFromMiddle * JSScaling[i]), 3);
      }

      // Update the counter if the incrementFactor has reached a full number
      if (incrementFactor[i] >= 1.0 || incrementFactor[i] <= -1.0) {
        counter[i] += static_cast<long>(incrementFactor[i]); // Increment or decrement the counter by the integer part of incrementFactor
        incrementFactor[i] -= static_cast<long>(incrementFactor[i]); // Subtract the integer part from incrementFactor
      }

      // Check if the counter value has changed
      if (counter[i] != prevCounter[i]) {
        sendData('R',JSPinmap[i],counter[i]);
        // Update the previous counter value with the current counter value
        prevCounter[i] = counter[i];
      }
    }
  }
}
#endif

#ifdef QUADENCS
void readEncoders(){
    if(QuadEncs>=1){
      #if QUADENCS >= 1
        EncCount[0] = Encoder0.read()/QuadEncMp[0];
      #endif
    }
    if(QuadEncs>=2){
      #if QUADENCS >= 2
        EncCount[1] = Encoder1.read()/QuadEncMp[1];
      #endif
    }
    if(QuadEncs>=3){
      #if QUADENCS >= 3
        EncCount[2] = Encoder2.read()/QuadEncMp[2];
      #endif
    }
    if(QuadEncs>=4){
      #if QUADENCS >= 4
        EncCount[3] = Encoder3.read()/QuadEncMp[3];
      #endif
    }
    if(QuadEncs>=5){
      #if QUADENCS >= 5
        EncCount[4] = Encoder4.read()/QuadEncMp[4];
      #endif
    }

    for(int i=0; i<QuadEncs;i++){
      if(QuadEncSig[i]==2){
        if(OldEncCount[i] != EncCount[i]){
          sendData('R',i,EncCount[i]);//send Counter
          OldEncCount[i] = EncCount[i];
        }
      }
      if(QuadEncSig[i]==1){
        if(OldEncCount[i] < EncCount[i]){
        sendData('R',i,1); //send Increase by 1 Signal
        OldEncCount[i] = EncCount[i];
        }
        if(OldEncCount[i] > EncCount[i]){
        sendData('R',i,0); //send Increase by 1 Signal
        OldEncCount[i] = EncCount[i];
        }
      }
    }
}

#endif

void comalive(){
  if(lastcom == 0){ //no connection yet. send E0:0 periodicly and wait for response
    while (lastcom == 0){
      readCommands();
      flushSerial();
      Serial.println("E0:0");
      delay(200);
      #ifdef STATUSLED
        //StatLedErrDel[] = {1000,1000};
      #endif
    }
    connectionState = 1;
    flushSerial();
    #ifdef DEBUG
      Serial.println("first connect");
    #endif
  }
  if(millis() - lastcom > CONNECTION_TIMEOUT){
  #ifdef STATUSLED
     StatLedErr(500,200);
  #endif
      if(connectionState == 1){
        #ifdef DEBUG
          Serial.println("disconnected");
        #endif
        connectionState = 2;
      }

   }
   else{
      connectionState=1;
      #ifdef STATUSLED
        if(UseDLedStatusLed == 1){
          #ifdef DIGITALLEDS
            controlDLED(StatusLed, 1);
          #endif
        }
        else{
          digitalWrite(StatusLed, HIGH);
        }
      #endif
    }
}


void reconnect(){
  #ifdef DEBUG
    Serial.println("reconnected");
    Serial.println("resending Data");
  #endif

  #ifdef INPUTS
    for (int x = 0; x < DigitalInputs; x++){
      InState[x]= -1;
    }
  #endif
  
  #ifdef AINPUTS
    for (int x = 0; x < ANALOG_INPUTS; x++){
      oldAinput[x] = -1;
      sumAinput[x] = 0;
    }
  #endif
  #ifdef LPOTIS
    for (int x = 0; x < LPotis; x++){
      oldLpoti[x] = -1;
    }
  #endif
  #ifdef BINSEL
    oldAbsEncState = -1;
  #endif


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
  #ifdef BINSEL
    readAbsKnob(); //read ABS Encoder & send data
  #endif
  #ifdef MULTIPLEXLEDS
    multiplexLeds(); //Flash LEDS.
  #endif

  connectionState = 1;


}


void sendData(char sig, int pin, int state){
        Serial.print(sig);
        Serial.print(pin);
        Serial.print(":");
        Serial.println(state);
}

//
void sendStates() {
  uint8_t buffer[64];  // Adjust size as needed
  uint8_t pos = 0;
  uint8_t checksum = 0;

  // Start marker
  buffer[pos++] = 0xAA;

  // Section 1: Binary pins
  buffer[pos++] = 0; // Type: binary
  uint8_t binaryPins[] = {1, 0, 1, 1, 0, 0, 1, 1}; // Example binary states
  uint8_t binaryCount = sizeof(binaryPins) / sizeof(binaryPins[0]);
  buffer[pos++] = binaryCount; // Number of binary pins
  uint8_t packedBinary = 0;
  for (uint8_t i = 0; i < binaryCount; i++) {
    packedBinary |= (binaryPins[i] << (i % 8));
    if ((i + 1) % 8 == 0 || i == binaryCount - 1) {
      buffer[pos++] = packedBinary;
      packedBinary = 0; // Reset for the next byte
    }
  }

  // Section 2: Integer pins
  buffer[pos++] = 1; // Type: integer
  uint8_t integerPins[] = {1, 2}; // Pin IDs
  int16_t integerValues[] = {512, 1024};
  uint8_t integerCount = sizeof(integerPins) / sizeof(integerPins[0]);
  buffer[pos++] = integerCount; // Number of integer pins
  for (uint8_t i = 0; i < integerCount; i++) {
    buffer[pos++] = integerPins[i]; // Pin ID
    memcpy(&buffer[pos], &integerValues[i], sizeof(integerValues[i])); // Value
    pos += sizeof(integerValues[i]);
  }

  // Section 3: Float pins
  buffer[pos++] = 2; // Type: float
  uint8_t floatPins[] = {3}; // Pin IDs
  float floatValues[] = {3.14};
  uint8_t floatCount = sizeof(floatPins) / sizeof(floatPins[0]);
  buffer[pos++] = floatCount; // Number of float pins
  for (uint8_t i = 0; i < floatCount; i++) {
    buffer[pos++] = floatPins[i]; // Pin ID
    memcpy(&buffer[pos], &floatValues[i], sizeof(floatValues[i])); // Value
    pos += sizeof(floatValues[i]);
  }

  // Compute checksum
  for (uint8_t i = 1; i < pos; i++) {
    checksum += buffer[i];
  }
  buffer[pos++] = checksum;

  // End marker
  buffer[pos++] = 0xBB;

  // Send message
  Serial.write(buffer, pos);
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
      #ifdef DIGITALLEDS
        if(UseDLedStatusLed == 1){
          controlDLED(StatusLed, 1);}
      #endif
      if(UseDLedStatusLed == 0){digitalWrite(StatusLed, HIGH);}
    }
  if (newMillis - oldmillis >= offtime+ontime){{
      #ifdef DIGITALLEDS
        if(UseDLedStatusLed == 1){
          controlDLED(StatusLed, 0);}
      #endif
      if(UseDLedStatusLed == 0){digitalWrite(StatusLed, LOW);}

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

#ifdef DIGITALLEDS
void initDLED(){
  strip.begin();
  strip.setBrightness(DledBrightness);

    for (int i = 0; i < DLedChainLength; i++) {
    strip.setPixelColor(i, strip.Color(DLedPin_off_values[i][0],DLedPin_off_values[i][1],DLedPin_off_values[i][2]));
    }
  strip.show();
  #ifdef DEBUG
    Serial.print("DIGITALLEDS initialised");
  #endif
}

void controlDLED(int Pin, int Stat){
  if(Stat == 1){

    strip.setPixelColor(Pin, strip.Color(DLedPin_on_values[Pin][0],DLedPin_on_values[Pin][1],DLedPin_on_values[Pin][2]));
    #ifdef DEBUG
      Serial.print("DIGITALLEDS No.");
      Serial.print(Pin);
      Serial.print(" set to:");
      Serial.println(Stat);

    #endif
    }
    else{

      strip.setPixelColor(Pin, strip.Color(DLedPin_off_values[Pin][0],DLedPin_off_values[Pin][1],DLedPin_off_values[Pin][2]));
      #ifdef DEBUG
        Serial.print("DIGITALLEDS No.");
        Serial.print(Pin);
        Serial.print(" set to:");
        Serial.println(Stat);

      #endif
    }
  strip.show();
  }
#endif

#ifdef LPOTIS
void readLPoti(){
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
void readAInputs() {
  static unsigned int samplecount = 0;
  static int currentInput = 0;

  if (samplecount < AInPinSmoothing[currentInput]) {
    sumAinput[currentInput] += analogRead(AInPinmap[currentInput]);
    samplecount++;
  } else {
    sumAinput[currentInput] /= AInPinSmoothing[currentInput];
    if (oldAinput[currentInput] != sumAinput[currentInput]) {
      oldAinput[currentInput] = sumAinput[currentInput];
      sendData('A', AInPinmap[currentInput], oldAinput[currentInput]);
    }
    sumAinput[currentInput] = 0;
    samplecount = 0;
    currentInput++;
    if (currentInput >= ANALOG_INPUTS) {
      currentInput = 0;
    }
  }
}
#endif

#ifdef INPUTS
void readInputs(){
    for(int i= 0;i<DigitalInputs; i++){
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

#ifdef BINSEL
int readAbsKnob(){
  int var = 0;
  if(digitalRead(BSSPinmap[0])==1){
    var += 1;
  }
  if(digitalRead(BSSPinmap[1])==1){
    var += 2;
  }
  if(digitalRead(BSSPinmap[2])==1){
    var += 4;
  }
  if(digitalRead(BSSPinmap[3])==1){
    var += 8;
  }
  if(digitalRead(BSSPinmap[4])==1){
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

#ifdef KEYPAD
void readKeypad(){
  //detect if Button is Pressed
  for (int col = 0; col < numCols; col++) {
    pinMode(colPins[col], OUTPUT);
    digitalWrite(colPins[col], LOW);
    // Read the state of the row pins
    for (int row = 0; row < numRows; row++) {
      pinMode(rowPins[row], INPUT_PULLUP);
      if (digitalRead(rowPins[row]) == LOW && lastKey != keys[row][col]) {
        // A button has been pressed
        sendData('M',keys[row][col],1);
        lastKey = keys[row][col];
        break;

      }
      if (digitalRead(rowPins[row]) == HIGH && lastKey == keys[row][col]) {
        // The Last Button has been unpressed
        sendData('M',keys[row][col],0);
        lastKey = -1; //reset Key pressed
        break;
      }
    }

    // Set the column pin back to input mode
    pinMode(colPins[col], INPUT);
  }

}
#endif

#ifdef MULTIPLEXLEDS
void multiplexLeds() {
  unsigned long currentMillis = millis();
  //init Multiplex
  #ifdef KEYPAD //if Keyboard is presend disable Pullup Resistors to not mess with LEDs while a Button is pressed.
    for (int row = 0; row < numRows; row++) {
      pinMode(rowPins[row], OUTPUT);
      digitalWrite(rowPins[row], LOW);
    }
  #endif

  for (int i = 0; i < numVccPins; i++) {
    pinMode(LedVccPins[i], OUTPUT);
    digitalWrite(LedVccPins[i], LOW); // Set to LOW to disable all Vcc Pins
  }
  for (int i = 0; i < numGndPins; i++) {
    pinMode(LedGndPins[i], OUTPUT);
    digitalWrite(LedGndPins[i], HIGH); // Set to HIGH to disable all GND Pins
  }

  for(currentLED = 0; currentLED < numVccPins*numGndPins ;currentLED ++){
    if(ledStates[currentLED] == 1){                         //only handle turned on LEDs
      digitalWrite(LedVccPins[currentLED/numVccPins],HIGH); //turn current LED on
      digitalWrite(LedGndPins[currentLED%numGndPins],LOW);

      #ifdef debug
        Serial.print("VCC: ");
        Serial.print(LedVccPins[currentLED/numVccPins]);
        Serial.print(" GND: ");
        Serial.println(LedGndPins[currentLED%numGndPins]);
      #endif

      delayMicroseconds(interval);                          //wait couple ms
      digitalWrite(LedVccPins[currentLED/numVccPins],LOW);  //turn off and go to next one
      digitalWrite(LedGndPins[currentLED%numGndPins],HIGH);
    }
  }
/*
  }
  if(ledStates[currentLED]==0){//If currentLED is Off, manage next one.
    currentLED++;
  }
  if(currentLED >= numVccPins*numGndPins){
      currentLED= 0;
  }
  */
}
#endif

void commandReceived(char cmd, uint16_t io, uint16_t value){
  #ifdef OUTPUTS
  if(cmd == 'O'){
    writeOutputs(io,value);
    lastcom=millis();

  }
  #endif
  #ifdef PWMOUTPUTS
  if(cmd == 'P'){
    writePwmOutputs(io,value);
    lastcom=millis();

  }
  #endif
  #ifdef DIGITALLEDS
  if(cmd == 'D'){
    controlDLED(io,value);
    lastcom=millis();
    #ifdef debug
      Serial.print("DIGITALLEDS:");
      Serial.print(io);
      Serial.print(" State:");
      Serial.println(DLEDstate[io]);
    #endif

  }
  #endif
  #ifdef MULTIPLEXLEDS
    if(cmd == 'M'){
      ledStates[io] = value; // Set the LED state
      lastcom=millis();
      #ifdef DEBUG
        Serial.print("multiplexed Led No:");
        Serial.print(io);
        Serial.print("Set to:");
        Serial.println(ledStates[io]);
      #endif

  }
  #endif


  if(cmd == 'E'){
    lastcom=millis();
    if(connectionState == 2){
     reconnect();
    }
  }


  #ifdef DEBUG
    Serial.print("I Received= ");
    Serial.print(cmd);
    Serial.print(io);
    Serial.print(":");
    Serial.println(value);
  #endif
}


void readCommands(){
    byte current;
    while(Serial.available() > 0){
        current = Serial.read();
        switch(state){
            case STATE_CMD:
                   cmd = current;
                   state = STATE_IO;
                   bufferIndex = 0;
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
