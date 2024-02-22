#ifndef _RAPIDCHANGE_H_
#define _RAPIDCHANGE_H_ 
//code for dust cover made by jean-philippe and guillaume for rapidchange atc//
#include <Stepper.h>

const int stepsPerRevolution = 200;  // Steps per revolution for the stepper motor
Stepper stepper(stepsPerRevolution, D5, D6, D7, D8); // GPIO pins for motor control

int switch1Pin = D1; // GPIO pin for switch 1

int relayPin = D4;  // GPIO pin for relay

int actionPin = D3;
 

void rc_setup() {
  stepper.setSpeed(150);  // Motor speed
  //digitalWrite(actionPin, HIGH);
  pinMode(switch1Pin, INPUT_PULLUP);  
  //pinMode(switch2Pin, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Relay is off by default
  pinMode(actionPin, OUTPUT);
  digitalWrite(actionPin, LOW);  // Action pin is off by default
  
}

void rc_loop() {
  static bool switch1Pressed = false;
  
  // State of switch 1
  bool switch1State = digitalRead(switch1Pin);

  if (switch1State == LOW) {  // Switch 1 is active
    if (!switch1Pressed) {   // Activate with switch 1
      Serial.println("Switch 1 pressed. Stepper moving forward.");
      Serial.flush();
      stepper.step(-600);  // Number of steps to open
      switch1Pressed = true;
    }
  } else {
    if (switch1Pressed) {  // Deactivate if it was previously activated
      Serial.println("Switch 1 released. Stepper moving backward.");
      Serial.flush();
      stepper.step(600);  // Move back the same number of steps
      switch1Pressed = false;

    }
  }

 
}
#endif