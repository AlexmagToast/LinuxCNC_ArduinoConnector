#if !defined(RAPIDCHANGEATC_H_)
#define RAPIDCHANGEATC_H_
#include <Arduino.h>

// Constants defined for clarity
//#define DIRECTION_PIN     0
//#define STEP_PIN          1
//#define ACTION_PIN        2

#define SIGNAL_CLOSE      0
#define SIGNAL_OPEN       1

#define DIRECTION_CLOSE   1
#define DIRECTION_OPEN    0

#define INCREMENT_CLOSE  -1
#define INCREMENT_OPEN    1

#define CLOSED_STATE      0
#define OPEN_STATE        1
#define CLOSING_STATE     2
#define OPENING_STATE     3

#define CLOSED_STEPS      0
#define OPEN_STEPS        800

#define STEP_DELAY        1200

// variables to manage state - initialized in a closed state
int stepCounter = CLOSED_STEPS;
int coverState = CLOSED_STATE;
int stepIncrement = INCREMENT_CLOSE;

void setup_rapidchange() {
  // Define pins  
  pinMode(ACTION_PIN, INPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  // Initialize step and direction pins
  digitalWrite(DIRECTION_PIN, DIRECTION_CLOSE);
  digitalWrite(STEP_PIN, LOW);
}

void do_rapidchange_work() {

  // Check if we've received a new signal
  // If we have a new close signal
  if (digitalRead(ACTION_PIN) == SIGNAL_CLOSE && (coverState == OPEN_STATE || coverState == OPENING_STATE)) {
    // Set closing state  
    coverState = CLOSING_STATE;
    stepIncrement = INCREMENT_CLOSE;
    digitalWrite(DIRECTION_PIN, DIRECTION_CLOSE);

  // Or if have a new open signal
  } else if (digitalRead(ACTION_PIN) == SIGNAL_OPEN && (coverState == CLOSED_STATE || coverState == CLOSING_STATE)) {
    // Set opening state
    coverState = OPENING_STATE;
    stepIncrement = INCREMENT_OPEN;
    digitalWrite(DIRECTION_PIN, DIRECTION_OPEN);
  }

  // Handle movement if necessary
  // If we are in the process of opening or closing, let's step
  if (coverState == OPENING_STATE || coverState == CLOSING_STATE) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY);

    // And record where we are
    stepCounter = stepCounter + stepIncrement;
  }
  
  // If we are fully closed or fully open, set the appropriate state
  if (stepCounter == CLOSED_STEPS) {
    coverState = CLOSED_STATE;
  } else if (stepCounter == OPEN_STEPS) {
    coverState = OPEN_STATE;
  }
}
#endif