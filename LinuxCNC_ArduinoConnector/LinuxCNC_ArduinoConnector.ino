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


#include "Config.h"
#include "EthernetFuncs.h"

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
#ifdef DALLAS_TEMP_SENSOR
  double inTmpSensorState[TmpSensors];
  //int oldInTempSensorState[TmpSensors];
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
#ifdef BINSEL
  int oldAbsEncState;
#endif
#ifdef KEYPAD
  byte KeyState = 0;
#endif
#ifdef MULTIPLEXLEDS
  byte KeyLedStates[numVccPins*numGndPins];
#endif
#if QUADENCS == 1 
  const int QuadEncs = 1;  
#endif
#if QUADENCS == 2 
  const int QuadEncs = 2;  
#endif
#if QUADENCS == 3 
  const int QuadEncs = 3;  
#endif
#if QUADENCS == 4 
  const int QuadEncs = 4;  
#endif
#if QUADENCS == 5 
  const int QuadEncs = 5;  
#endif
#ifdef QUADENC
  long EncCount[QuadEncs];
  long OldEncCount[QuadEncs];
#endif


#ifdef JOYSTICK
long counter[JoySticks*2] = {0};      // Initialize an array for the counters
long prevCounter[JoySticks*2] = {0};  // Initialize an array for the previous counters
float incrementFactor[JoySticks*2] = {0.0}; // Initialize an array for the incrementFactors
unsigned long lastUpdateTime[JoySticks*2] = {0}; // Store the time of the last update for each potentiometer
  
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

byte reconnected = 2;
byte state = STATE_CMD;
char inputbuffer[5];
byte bufferIndex = 0;
char cmd = 0;
uint16_t io = 0;
uint16_t value = 0;

void setup() {

#ifdef ETHERNET_TO_LINUXCNC
  Serial.begin(DEFAULT_SERIAL_BAUD_RATE);

  while (!Serial) {

    ; // wait for serial port to connect. Needed for native USB port only

  }
  #ifdef DEBUG
    #if DHCP == 1
      Serial.println("Starting up.. DHCP = Enabled");
    #else
      Serial.print("Starting up.  DHCP = False. Static IP = ");
      //Serial.println(ip.toString());
    #endif
  #endif
  #if DHCP == 1
    if (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP");

      if (Ethernet.hardwareStatus() == EthernetNoHardware) {

        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");

      } else if (Ethernet.linkStatus() == LinkOFF) {

        Serial.println("Ethernet cable is not connected.");

      }
      // no point in carrying on, so do nothing forevermore:

      while (true) {

        delay(1);
    }
    }
  #else
    Ethernet.begin(mac, ip); // Per Arduino documentation, only DHCP version of .begin returns an int.
  #endif
  #ifdef DEBUG
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
  #endif
  
  
#endif

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
  comalive();
}


void loop() {

  readCommands(); //receive and execute Commands 
  comalive(); //if nothing is received for 10 sec. blink warning LED 

#ifdef ETHERNET_TO_LINUXCNC
  #if DHCP == 1
    do_dhcp_maintain();
  #endif
#endif
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
}

#ifdef JOYSTICK

void readJoySticks() {
  for (int i = 0; i < JoySticks*2; i++) {
    unsigned long currentTime = millis(); // Get the current time

    // Check if it's time to update the counter for this potentiometer
    if (currentTime - lastUpdateTime[i] >= 100) { // Adjust 100 milliseconds based on your needs
      lastUpdateTime[i] = currentTime; // Update the last update time for this potentiometer

      int potValue = analogRead(JoyStickPins[i]); // Read the potentiometer value

      // Calculate the distance of the potentiometer value from the middle
      int distanceFromMiddle = potValue - middleValue;

      // Apply deadband to ignore small variations around middleValue
      if (abs(distanceFromMiddle) <= deadband) {
        incrementFactor[i] = 0.0; // Set incrementFactor to 0 within the deadband range
      } else {
        // Apply non-linear scaling to distanceFromMiddle to get the incrementFactor
        incrementFactor[i] = pow((distanceFromMiddle * scalingFactor), 3);
      }

      // Update the counter if the incrementFactor has reached a full number
      if (incrementFactor[i] >= 1.0 || incrementFactor[i] <= -1.0) {
        counter[i] += static_cast<long>(incrementFactor[i]); // Increment or decrement the counter by the integer part of incrementFactor
        incrementFactor[i] -= static_cast<long>(incrementFactor[i]); // Subtract the integer part from incrementFactor
      }

      // Check if the counter value has changed
      if (counter[i] != prevCounter[i]) {
        sendData('R',JoyStickPins[i],counter[i]);
        // Update the previous counter value with the current counter value
        prevCounter[i] = counter[i];
      }
    }
  }
}
#endif

#ifdef QUADENC
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
        StatLedErr(1000,1000);
      #endif
    }  
    connectionState = 1;
    flushSerial();
    #ifdef DEBUG
      Serial.println("first connect");
    #endif
  }
  if(millis() - lastcom > timeout){  
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
        if(DLEDSTATUSLED == 1){
          #ifdef DLED
            controlDLED(StatLedPin, 1);
          #endif
        }
        else{
          digitalWrite(StatLedPin, HIGH);
          if (reconnected == 0){
            reconnect()
          }
        }
      #endif 
    }
}


void reconnect(){
  #ifdef DEBUG
    Serial.println("reconnected");
  #endif
  #ifdef DEBUG
    Serial.println("resending Data");
  #endif
    
  #ifdef INPUTS
    for (int x = 0; x < Inputs; x++){
      InState[x]= -1;
    }
  #endif
  #ifdef SINPUTS
    for (int x = 0; x < sInputs; x++){
      soldInState[x]= -1;
      togglesinputs[x] = 0;
    }
  #endif
  #ifdef AINPUTS
    for (int x = 0; x < AInputs; x++){
      oldAinput[x] = -1;
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
  
  #ifdef DALLAS_TEMP_SENSOR
    readTmpInputs(); //read Inputs & send data
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
  #ifdef SERIAL_TO_LINUXCNC
      Serial.print(sig);
      Serial.print(pin);
      Serial.print(":");
      Serial.println(state);
  #endif
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
      #ifdef DLED
        if(DLEDSTATUSLED == 1){
          controlDLED(StatLedPin, 1);}
      #endif
      if(DLEDSTATUSLED == 0){digitalWrite(StatLedPin, HIGH);}
    } 
  if (newMillis - oldmillis >= offtime+ontime){{
      #ifdef DLED
        if(DLEDSTATUSLED == 1){
          controlDLED(StatLedPin, 0);}
      #endif
      if(DLEDSTATUSLED == 0){digitalWrite(StatLedPin, LOW);}
            
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
    else{

      strip.setPixelColor(Pin, strip.Color(DledOffColors[Pin][0],DledOffColors[Pin][1],DledOffColors[Pin][2]));
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

#ifdef LPOTIS
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
   
   for(int i= 0;i<AInputs; i++){
      unsigned long var = 0;
      for(int d= 0;d<smooth; d++){// take couple samples to denoise signal
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
#ifdef DALLAS_TEMP_SENSOR
void readTmpInputs(){
    for(int i= 0;i<TmpSensors; i++){
      DallasTemperature * sensor = TmpSensorControlMap[i];
      sensor->requestTemperatures(); 
      int v = sensor->getTempCByIndex(0); // Future Todo: Add in support for multiple sensors per pin.
      // The sensor is returning a double. Future Todo: Enable an option that ouputs the double value if desired
      int v_f = (v * 9/5) + 32; // Perform conversion to Farenheit if output in F is toggled on below
      
      if(inTmpSensorState[i]!= v){
        inTmpSensorState[i] = v;
        
        #if TEMP_OUTPUT_C == 1
          sendData('T',TmpSensorMap[i],inTmpSensorState[i]);
        #else
          sendData('T',TmpSensorMap[i],v_f);
        #endif
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
  if(digitalRead(BinSelKnobPins[0])==1){
    var += 1;
  }
  if(digitalRead(BinSelKnobPins[1])==1){
    var += 2;
  }
  if(digitalRead(BinSelKnobPins[2])==1){
    var += 4;
  }  
  if(digitalRead(BinSelKnobPins[3])==1){
    var += 8;
  }  
  if(digitalRead(BinSelKnobPins[4])==1){
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
        row = numRows;

      }
      if (digitalRead(rowPins[row]) == HIGH && lastKey == keys[row][col]) {
        // The Last Button has been unpressed
        sendData('M',keys[row][col],0);
        lastKey = -1; //reset Key pressed
        row = numRows;
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
      digitalWrite(LedGndPins[currentLED%numVccPins],LOW);
      
      Serial.print("VCC: ");
      Serial.print(LedVccPins[currentLED/numVccPins]);
      Serial.print(" GND: ");
      Serial.println(LedGndPins[currentLED%numVccPins]);
      
      delayMicroseconds(interval);                          //wait couple ms
      digitalWrite(LedVccPins[currentLED/numVccPins],LOW);  //turn off and go to next one
      digitalWrite(LedGndPins[currentLED%numVccPins],HIGH);
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
  #ifdef DLED
  if(cmd == 'D'){
    controlDLED(io,value);
    lastcom=millis();
    #ifdef debug
      Serial.print("DLED:");
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
    #ifdef SERIAL_TO_LINUXCNC
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
                      Serial.print("Invalid character: ");
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
                    Serial.print("Invalid character: ");
                    Serial.println(current);
                    #endif
                  
                  }
                  break;
          }

      }
      #endif
}
