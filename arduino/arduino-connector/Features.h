/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com &
  Ken Thompson (not THAT Ken Thompson), https://github.com/KennethThompson
  
  MIT License
  Copyright (c) 2023 Alexander Richter & Ken Thompson

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
#ifndef FEATURES_H_
#define FEATURES_H_
#include "FeatureController.h"
#include "Config.h"

namespace Features
{
    #ifdef DINPUTS

    struct DigitalPin: public Pin
    {
        uint32_t ts;

        int8_t pinInitialState;
        int8_t pinConnectedState;
        int8_t pinDisconnectedState;
        uint16_t debounce;
        uint8_t inputPullup;
        int8_t pinCurrentState;
        unsigned long t;
    };

    class DigitalInputs: public Feature
    {
        public:
        DigitalInputs() : Feature(DINPUTS, String("DIGITAL_INPUTS"), DEFAULT_LOOP_FREQUENCY)
        {
            #ifdef DEBUG
                Serial.println("DigitalInputs::DigitalInputs");
                Serial.flush();
            #endif
        }

        virtual void loop()
        {
           // loop through pins and perform reads

        }

        virtual void setup()
        {
            #ifdef DEBUG
                Serial.println("DigitalInputs::setup");
            #endif

            // Perform any setup here.

            // Then set the feature to ready, otherwise it will not be available to process incoming messages or perform local
            // tasks such as pin reads.
            SetFeatureReady(true);
        }

        protected:

        // onConnected gets called when the python host has connected and completed handshaking
        virtual void onConnected()
        {
            #ifdef DEBUG
                Serial.println("DigitalInputs::onConnected");
            #endif
        }

        // onDisconnected gets called when the python host has disconnected
        virtual void onDisconnected()
        {
            #ifdef DEBUG
                Serial.println("DigitalInputs::onDisconnected");
            #endif
        }

        virtual Pin* InitFeaturePin(uint8_t fid, uint8_t lid, uint8_t pid, JsonDocument& json)
        {
            DigitalPin* dp = new DigitalPin();
            dp->fid = fid;
            dp->lid = lid;
            dp->pid = pid;
            dp->pinInitialState = 0;
            dp->pinConnectedState = 1;
            dp->pinDisconnectedState = 0;
            dp->debounce = 0;
            dp->inputPullup = 0;
            dp->pinCurrentState = 0;
            dp->t = 0;
            return dp;
        }

    };
    #endif
}
#endif