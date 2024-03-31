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

        virtual uint8_t InitFeaturePin(uint8_t fid, uint8_t lid, uint8_t pid, JsonDocument& json, String& fail_reason, Pin ** p)
        {
            DigitalPin * dp = new DigitalPin();
            dp->fid = fid;
            dp->lid = lid;
            dp->pid = pid;
            //{"mt":7,"fi":4,"se":85,"to":99,"cs":{"fi":4,"id":87,"li":85,"is":-1,"cs":-1,"ds":-1,"pd":5,"ip":true}}
            if(json.containsKey("is"))
            {
                dp->pinInitialState = json["is"];
            }
            else
            {
                dp->pinInitialState = -1;
            }
            if(json.containsKey("cs"))
            {
                dp->pinConnectedState = json["cs"];
            }
            else
            {
                dp->pinConnectedState = -1;
            }
            if(json.containsKey("ds"))
            {
                dp->pinDisconnectedState = json["ds"];
            }
            else
            {
                dp->pinDisconnectedState = -1;
            }
            if(json.containsKey("pd"))
            {
                dp->debounce = json["pd"];
            }
            else
            {
                dp->debounce = 0;
            }
            if(json.containsKey("ip"))
            {
                if (json["ip"] == true)
                {
                    dp->inputPullup = 1;
                }
                else
                {
                    dp->inputPullup = 0;
                }
                //dp->inputPullup = json["ip"];
            }
            else
            {
                dp->inputPullup = 0;
            }
            #ifdef DEBUG
                DEBUG_DEV.print("DigitalInputs::InitFeaturePin: ");
                DEBUG_DEV.print("fid: ");
                DEBUG_DEV.print(fid);
                DEBUG_DEV.print(", lid: ");
                DEBUG_DEV.print(lid);
                DEBUG_DEV.print(", pid: ");
                DEBUG_DEV.print(pid);
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print(", is: ");
                    DEBUG_DEV.print(dp->pinInitialState);
                    DEBUG_DEV.print(", cs: ");
                    DEBUG_DEV.print(dp->pinConnectedState);
                    DEBUG_DEV.print(", ds: ");
                    DEBUG_DEV.print(dp->pinDisconnectedState);
                    DEBUG_DEV.print(", pd: ");
                    DEBUG_DEV.print(dp->debounce);
                    DEBUG_DEV.print(", ip: ");
                    DEBUG_DEV.println(dp->inputPullup);
                #endif
            #endif
            //dp->pinConnectedState = 1;
            //dp->pinDisconnectedState = 0;
            //dp->debounce = 0;
            //dp->inputPullup = 0;
            //dp->pinCurrentState = 0;
            //dp->t = 0;
            *p = dp;
            fail_reason = "";
            return 0;
        }

    };
    #endif
}
#endif