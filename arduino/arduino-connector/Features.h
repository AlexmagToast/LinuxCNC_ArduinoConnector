/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com &
  Ken Thompson (not THAT Ken Thompson), https://github.com/KennethThompson
  
  MIT License
  Copyright (c) 2023-2025 Alexander Richter & Ken Thompson

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
#include "PinMap.h"

namespace Features
{
    
    #if defined(DINPUTS) || defined(DOUTPUTS)
    struct DigitalPin: public Pin
    {
        uint32_t ts; 
        int8_t pinInitialState; // initial state value
        int8_t pinConnectedState; // state when connected
        int8_t pinDisconnectedState; // state when disconnected
        uint16_t debounce; // debounce time in milliseconds
        uint8_t inputPullup; // 1 if input pullup is enabled, 0 otherwise
        int8_t pinCurrentState; // current state of the pin
    };
    #endif
    #ifdef DOUTPUTS
    class DigitalOutputs: public Feature
    {
        public:
        DigitalOutputs() : Feature(DOUTPUTS, String("DIGITAL_OUTPUTS"), DEFAULT_LOOP_FREQUENCY)
        {
            #ifdef DEBUG
                DEBUG_DEV.println("DigitalOutputs::DigitalOutputs");
            #endif
        }

        virtual void loop()
        {
 
        }

        virtual void setup()
        {
            #ifdef DEBUG_VERBOSE
                DEBUG_DEV.println("DigitalOutputs::setup");
            #endif
            SetFeatureReady(true);
        }

        protected:

        // onConnected gets called when the python host has connected and completed handshaking
        virtual void onConnected()
        {
            #ifdef DEBUG_VERBOSE
                DEBUG_DEV.println("DigitalOutputs::onConnected");
            #endif
        }

        // onDisconnected gets called when the python host has disconnected
        virtual void onDisconnected()
        {
            #ifdef DEBUG_VERBOSE
                DEBUG_DEV.println("DigitalOutputs::onDisconnected");
            #endif
        }

        virtual void onPinChange(const protocol::PinChangeMessage& pcm) {
            #ifdef DEBUG_VERBOSE
                DEBUG_DEV.println("DigitalOutputs::onPinChange");
                DEBUG_DEV.print("Feature ID: ");
                DEBUG_DEV.println(pcm.featureID);
                DEBUG_DEV.print("Seq ID: ");
                DEBUG_DEV.println(pcm.seqID);
                DEBUG_DEV.print("Response Required: ");
                DEBUG_DEV.println(pcm.responseReq);
                DEBUG_DEV.print("Message: ");
                DEBUG_DEV.println(pcm.message);
            #endif
             JsonDocument doc; 
             DeserializationError error = deserializeJson(doc, pcm.message);
             if (error) {
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print("Error: ");
                    DEBUG_DEV.println(error.f_str());
                #endif
             }
             else
             {
                if (!doc.containsKey("p") || !doc.containsKey("l") || !doc.containsKey("v"))
                {
                    #ifdef DEBUG
                        DEBUG_DEV.println("ERROR. Missing required keys in JSON");
                    #endif
                    return;
                }


                #ifdef DEBUG_VERBOSE
                    //DEBUG_DEV.println("Message parsed successfully");
                    DEBUG_DEV.print("PID: ");
                    String pid = doc["p"];
                    DEBUG_DEV.println(pid);
                    String lid = doc["l"];
                    DEBUG_DEV.print("LID: ");
                    DEBUG_DEV.println(lid);
                    String value = doc["v"];
                    DEBUG_DEV.print("Value: ");
                    DEBUG_DEV.println(value);
                #endif
                String pin_id = doc["p"];
                unsigned long pin_lid = doc["l"];
                if (pin_lid > GetPinCount())
                {
                    #ifdef DEBUG
                        DEBUG_DEV.print("LID out of range: ");
                        DEBUG_DEV.println(pin_lid);
                    #endif
                }
                else
                {

                    DigitalPin * pin = static_cast<DigitalPin*>(GetPin(pin_lid));
                    #ifdef DEBUG_VERBOSE
                        DEBUG_DEV.print("Pin MID: ");
                        DEBUG_DEV.println(pin->mid);
                        DEBUG_DEV.print("Pin PID: ");
                        DEBUG_DEV.println(pin->pid);
                        DEBUG_DEV.print("Pin LID: ");
                        DEBUG_DEV.println(pin->lid);
                        DEBUG_DEV.print("Pin FID: ");
                        DEBUG_DEV.println(pin->fid);
        
                    #endif
                    if (pin != nullptr)
                    {
                        #ifdef DEBUG_VERBOSE
                            DEBUG_DEV.print("Writing to pin: ");
                            DEBUG_DEV.println(pin->mid);
                            DEBUG_DEV.print("Value: ");
                            DEBUG_DEV.println(value);
                        #endif
                        int value_converted = value.toInt();
                        digitalWrite(pin->mid, value_converted);
                    }
                    else
                    {
                        #ifdef DEBUG
                            DEBUG_DEV.print("Pin not found: ");
                            DEBUG_DEV.println(pin_lid);
                        #endif
                    }
                }
             }
             
        }

        virtual uint8_t InitFeaturePin(uint8_t fid, uint8_t lid, String& pid, JsonDocument& json, String& fail_reason, Pin ** p)
        {
            DigitalPin * dp = new DigitalPin();
            dp->fid = fid;
            dp->lid = lid;

            dp->mid = -1;//convertPinString(atoi(pid.c_str()));
            //dp->pid = pid;
            //{"mt":7,"fi":4,"se":85,"to":99,"cs":{"fi":4,"id":87,"li":85,"is":-1,"cs":-1,"ds":-1,"pd":5,"ip":true}}
            if(json.containsKey("id"))
            {
                String idstring = json[F("id")];
                dp->pid = idstring;
                dp->mid = convertPinString(idstring.c_str());
            }
            else
            {
               // dp->pinInitialState = -1;
               fail_reason = "Missing pin 'id' key in JSON";
               return ERR_INVALID_JSON;
            }
            if(json.containsKey("is"))
            {
                dp->pinInitialState = json["is"];
                if(dp->mid==-1)
                    digitalWrite(atoi(dp->pid.c_str()), dp->pinInitialState);
                else
                    digitalWrite(dp->mid, dp->pinInitialState);
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

            if (dp->mid == -1)
            {
                pinMode(atoi(dp->pid.c_str()), OUTPUT);
            }
            else
            {
                pinMode(dp->mid, OUTPUT);
            }

            
            
            #ifdef DEBUG
                DEBUG_DEV.print(F("DigitalOutputs::InitFeaturePin: "));
                DEBUG_DEV.print(F("fid: "));
                DEBUG_DEV.print(fid);
                DEBUG_DEV.print(F(", lid: "));
                DEBUG_DEV.print(lid);
                DEBUG_DEV.print(F(", pid: "));
                DEBUG_DEV.println(dp->pid);
                DEBUG_DEV.println(F("mid: "));
                DEBUG_DEV.println(dp->mid);
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print(F(", is: "));
                    DEBUG_DEV.print(dp->pinInitialState);
                    DEBUG_DEV.print(F(", cs: "));
                    DEBUG_DEV.print(dp->pinConnectedState);
                    DEBUG_DEV.print(F(", ds: "));
                    DEBUG_DEV.println(dp->pinDisconnectedState);
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

    #ifdef DINPUTS
    class DigitalInputs: public Feature
    {
        public:
        DigitalInputs() : Feature(DINPUTS, String("DIGITAL_INPUTS"), DEFAULT_LOOP_FREQUENCY)
        {
            #ifdef DEBUG
                DEBUG_DEV.println("DigitalInputs::DigitalInputs");
                //Serial.flush();
            #endif
        }

        virtual void loop()
        {
            unsigned long currentMills = millis();
            String output; // Used below to output Io update messages
            JsonDocument doc;
            JsonArray pa = doc.to<JsonArray>();
           // loop through pins and perform reads
            auto pins = GetPins();
            for( int x = 0; x < GetPinCount(); x++ )
            {
                DigitalPin & pin = *static_cast<DigitalPin*>(pins[x]);
                //int ii = convertPinString(pin.pid.c_str());
                //DEBUG_DEV.print("ii = ");
                //DEBUG_DEV.println(ii);
                //int i = convertPinString(pin.pid.c_str());
                int v = 0;
                if (pin.mid != -1)
                {
                    v = digitalRead(pin.mid);
                }
                else
                {
                    v = digitalRead(atoi(pin.pid.c_str()));
                }


                if(pin.pinCurrentState != v && (currentMills - pin.ts) >= pin.debounce)
                {
                    #ifdef DEBUG_VERBOSE
                        DEBUG_DEV.print(F("DINPUTS PIN CHANGE!"));
                        DEBUG_DEV.print(F("PIN:"));
                        DEBUG_DEV.println(pin.pid);
                        DEBUG_DEV.print(F("PIN_MID:"));
                        DEBUG_DEV.println(pin.mid);
                        DEBUG_DEV.print(F("Current value: "));
                        DEBUG_DEV.println(pin.pinCurrentState);
                        DEBUG_DEV.print(F("New value: "));
                        DEBUG_DEV.println(v);
                        
                    #endif
                    //serialClient.println(F("DINPUTS PIN CHANGE!"));
                    pin.pinCurrentState = v;
                    pin.ts = currentMills;

                    // send update out
                    //serialClient

                    //doc.clear();

                    JsonObject pa_0 = pa.add<JsonObject>();
                    pa_0["l"] = x;
                    pa_0["p"] = pin.pid.c_str();
                    pa_0["v"] = v;
                    //doc[F("l")] = x;
                    //doc[F("p")] = atoi(pin.pid.c_str());
                    //doc[F("v")] = v;

                    //doc.shrinkToFit();  // optional
                    //if(pa.size() > 0)
                    //{
 
                    //  pa.clear();
                    //}
                }
            }
            if (pa.size() > 0)
            {

                output = "";
                serializeJson(doc, output);
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print(F("JSON = "));
                    DEBUG_DEV.println(output);
                #endif
                uint8_t seqID = 0;
                uint8_t resp = 0; // Future TODO: Consider requiring ACK/NAK, maybe.
                uint8_t f = DINPUTS;
                //String o = String(output.c_str());
                serialClient.SendPinChangeMessage(f, seqID, resp, output);
            }


        }

        virtual void setup()
        {
            #ifdef DEBUG
                DEBUG_DEV.println("DigitalInputs::setup");
            #endif

            // Perform any setup here.
            auto pins = GetPins();
            for( int x = 0; x < GetPinCount(); x++ )
            {
                DigitalPin & pin = *static_cast<DigitalPin*>(pins[x]);
                // Set pin current state to -1 to trigger initial state update
                pin.pinCurrentState = -1;
            }
            // Then set the feature to ready, otherwise it will not be available to process incoming messages or perform local
            // tasks such as pin reads.
            SetFeatureReady(true);
        }

        protected:

        // onConnected gets called when the python host has connected and completed handshaking
        virtual void onConnected()
        {
            #ifdef DEBUG
                DEBUG_DEV.println("DigitalInputs::onConnected");
            #endif
        }

        // onDisconnected gets called when the python host has disconnected
        virtual void onDisconnected()
        {
            #ifdef DEBUG
                DEBUG_DEV.println("DigitalInputs::onDisconnected");
            #endif
        }

        virtual uint8_t InitFeaturePin(uint8_t fid, uint8_t lid, String& pid, JsonDocument& json, String& fail_reason, Pin ** p)
        {
            DigitalPin * dp = new DigitalPin();
            dp->fid = fid;
            dp->lid = lid;
            dp->mid = -1;

            //dp->pid = pid;
            //{"mt":7,"fi":4,"se":85,"to":99,"cs":{"fi":4,"id":87,"li":85,"is":-1,"cs":-1,"ds":-1,"pd":5,"ip":true}}
            if(json.containsKey("id"))
            {
                String idstring = json[F("id")];
                dp->pid = idstring;
                dp->mid = convertPinString(idstring.c_str());
            }
            else
            {
               // dp->pinInitialState = -1;
               fail_reason = "Missing pin 'id' key in JSON";
               return ERR_INVALID_JSON;
            }
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
                    if(dp->mid==-1)
                        pinMode(atoi(dp->pid.c_str()), INPUT_PULLUP);
                    else
                        pinMode(dp->mid, INPUT_PULLUP);
                    
                }
                else
                {
                    dp->inputPullup = 0;
                    if (dp->mid == -1)
                        pinMode(atoi(dp->pid.c_str()), INPUT_PULLUP);
                    else
                        pinMode(dp->mid, INPUT_PULLUP);    
                }
            }
            else
            {
                dp->inputPullup = 0;
                if (dp->mid == -1)
                    pinMode(atoi(dp->pid.c_str()), INPUT_PULLUP);
                else
                    pinMode(dp->mid, INPUT_PULLUP);
            }
            #ifdef DEBUG
                DEBUG_DEV.print(F("DigitalInputs::InitFeaturePin: "));
                DEBUG_DEV.print(F("fid: "));
                DEBUG_DEV.print(fid);
                DEBUG_DEV.print(F(", lid: "));
                DEBUG_DEV.print(lid);
                DEBUG_DEV.print(F(", pid: "));
                DEBUG_DEV.print(pid);
                DEBUG_DEV.print(F(", mid: "));
                DEBUG_DEV.print(dp->mid);
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print(F(", is: "));
                    DEBUG_DEV.print(dp->pinInitialState);
                    DEBUG_DEV.print(F(", cs: "));
                    DEBUG_DEV.print(dp->pinConnectedState);
                    DEBUG_DEV.print(F(", ds: "));
                    DEBUG_DEV.print(dp->pinDisconnectedState);
                    DEBUG_DEV.print(F(", pd: "));
                    DEBUG_DEV.print(dp->debounce);
                    DEBUG_DEV.print(F(", ip: "));
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
    #if defined(AINPUTS) || defined(AOUTPUTS)
    struct AnalogPin: public Pin
    {
        uint32_t ts; 
        uint32_t pinInitialState; // initial state value
        uint32_t pinConnectedState; // state when connected
        uint32_t pinDisconnectedState; // state when disconnected
        uint32_t pinSmoothing; // smoothing factor
        uint8_t pinSmoothingAlgo; // smoothing algorithm 0: Simple, 1: Exponential, 2: Moving Average
        uint32_t pinMaxValue; // maximum value of the pin
        uint32_t pinMinValue; // minimum value of the pin
        uint32_t pinCurrentState; // current state of the pin
        uint32_t pinValueArray[SMOOTHING_ARRAY_SIZE]; // array of values for smoothing
    };
    #endif
    #ifdef AINPUTS
    class AnalogInputs: public Feature
    {
        public:
        AnalogInputs() : Feature(AINPUTS, String("ANALOG_INPUTS"), DEFAULT_LOOP_FREQUENCY)
        {
            #ifdef DEBUG
                DEBUG_DEV.println("AnalogInputs::AnalogInputs");
            #endif
        }

#ifdef AINPUTS_SMOOTHING_SIMPLE
        // Simple averaging - applies basic averaging to a pre-read value
        uint32_t SimpleAverage(AnalogPin &pin, uint32_t rawValue) {
            // Apply simple averaging (current + new)/2
            uint32_t newValue = (pin.pinCurrentState + rawValue) / 2;
            
            // Constrain to min/max values
            if (newValue > pin.pinMaxValue) newValue = pin.pinMaxValue;
            if (newValue < pin.pinMinValue) newValue = pin.pinMinValue;
            
            // Update current state
            pin.pinCurrentState = newValue;
            
            return newValue;
        }
#endif // AINPUTS_SMOOTHING_SIMPLE

#ifdef AINPUTS_SMOOTHING_MOVING_AVERAGE
        // Moving average - maintains a window of readings and calculates average
        uint32_t MovingAverage(AnalogPin &pin, uint32_t rawValue) {
            // Shift all values in the array
            for (int i = SMOOTHING_ARRAY_SIZE - 1; i > 0; i--) {
                pin.pinValueArray[i] = pin.pinValueArray[i-1];
            }
            
            // Add new value to the beginning
            pin.pinValueArray[0] = rawValue;
            
            // Calculate the average
            uint32_t sum = 0;
            uint8_t count = 0;
            
            // Use smoothing as window size (or default to full array if not specified)
            uint8_t windowSize = (pin.pinSmoothing > 0 && pin.pinSmoothing <= SMOOTHING_ARRAY_SIZE) ? 
                                  pin.pinSmoothing : SMOOTHING_ARRAY_SIZE;
            
            for (int i = 0; i < windowSize; i++) {
                if (pin.pinValueArray[i] > 0 || i == 0) { // Count zero values except the newest
                    sum += pin.pinValueArray[i];
                    count++;
                }
            }
            
            uint32_t newValue = (count > 0) ? sum / count : rawValue;
            
            // Constrain to min/max values
            if (newValue > pin.pinMaxValue) newValue = pin.pinMaxValue;
            if (newValue < pin.pinMinValue) newValue = pin.pinMinValue;
            
            // Update current state
            pin.pinCurrentState = newValue;
            
            return newValue;
        }
#endif // AINPUTS_SMOOTHING_MOVING_AVERAGE

#ifdef AINPUTS_SMOOTHING_EXPONENTIAL
        // Exponential smoothing - weighted average giving more importance to recent readings
        uint32_t ExponentialSmoothing(AnalogPin &pin, uint32_t rawValue) {
            // Calculate alpha (smoothing factor): 0 < alpha < 1
            // Higher pinSmoothing value = slower response (lower alpha)
            float alpha = 1.0;
            if (pin.pinSmoothing > 0) {
                alpha = 1.0 / pin.pinSmoothing;
                if (alpha > 1.0) alpha = 1.0;
            }
            
            // Apply exponential smoothing formula: newValue = alpha * rawValue + (1 - alpha) * oldValue
            uint32_t newValue;
            
            // For first reading, just use the raw value
            if (pin.pinCurrentState == -1) {
                newValue = rawValue;
            } else {
                newValue = (uint32_t)(alpha * rawValue + (1.0 - alpha) * pin.pinCurrentState);
            }
            
            // Constrain to min/max values
            if (newValue > pin.pinMaxValue) newValue = pin.pinMaxValue;
            if (newValue < pin.pinMinValue) newValue = pin.pinMinValue;
            
            // Update current state
            pin.pinCurrentState = newValue;
            
            return newValue;
        }
#endif // AINPUTS_SMOOTHING_EXPONENTIAL

        virtual void loop()
        {
            unsigned long currentMills = millis();
            String output; // Used below to output Io update messages
            JsonDocument doc;
            JsonArray pa = doc.to<JsonArray>();
           // loop through pins and perform reads
            auto pins = GetPins();
            for( int x = 0; x < GetPinCount(); x++ )
            {
                AnalogPin & pin = *static_cast<AnalogPin*>(pins[x]);
                
                uint32_t newValue = 0;
                uint32_t rawValue = 0;
                
                // Read the raw value only once per loop
                if (pin.mid != -1) {
                    rawValue = analogRead(pin.mid);
                } else {
                    rawValue = analogRead(atoi(pin.pid.c_str()));
                }
                
                // Apply smoothing based on algorithm and available implementations
                // Check which algorithms are defined and match with pin.pinSmoothingAlgo
#ifdef AINPUTS_SMOOTHING_EXPONENTIAL
                if (pin.pinSmoothingAlgo == AINPUTS_SMOOTHING_EXPONENTIAL) {
                    newValue = ExponentialSmoothing(pin, rawValue);
                } else
#endif
#ifdef AINPUTS_SMOOTHING_MOVING_AVERAGE
                if (pin.pinSmoothingAlgo == AINPUTS_SMOOTHING_MOVING_AVERAGE) {
                    newValue = MovingAverage(pin, rawValue);
                } else
#endif
#ifdef AINPUTS_SMOOTHING_SIMPLE
                if (pin.pinSmoothingAlgo == AINPUTS_SMOOTHING_SIMPLE) {
                    newValue = SimpleAverage(pin, rawValue);
                } else
#endif
                {
                    // Default fallback if no matching algorithm is available
                    newValue = rawValue;
                }
                
                // If no smoothing algorithm is defined, apply constraints
                if (newValue == 0 && rawValue > 0) {
                    newValue = rawValue;
                }
                
                // Constrain to min/max values
                if (newValue > pin.pinMaxValue) newValue = pin.pinMaxValue;
                if (newValue < pin.pinMinValue) newValue = pin.pinMinValue;

                // If value has changed or it's the first read, add to output array
                if (pin.pinCurrentState != newValue || pin.pinCurrentState == -1) {
                    #ifdef DEBUG_VERBOSE
                        DEBUG_DEV.print(F("AINPUTS PIN CHANGE!"));
                        DEBUG_DEV.print(F("PIN:"));
                        DEBUG_DEV.println(pin.pid);
                        DEBUG_DEV.print(F("Current value: "));
                        DEBUG_DEV.println(pin.pinCurrentState);
                        DEBUG_DEV.print(F("New value: "));
                        DEBUG_DEV.println(newValue);
                    #endif
                    
                    pin.pinCurrentState = newValue;
                    pin.ts = currentMills;

                    // Add to output array
                    JsonObject pa_0 = pa.add<JsonObject>();
                    pa_0["l"] = x;
                    pa_0["p"] = pin.pid.c_str();
                    pa_0["v"] = newValue;
                }
            }
            if (pa.size() > 0)
            {
                output = "";
                serializeJson(doc, output);
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print(F("JSON = "));
                    DEBUG_DEV.println(output);
                #endif
                uint8_t seqID = 0;
                uint8_t resp = 0; // Future TODO: Consider requiring ACK/NAK, maybe.
                uint8_t f = AINPUTS;
                serialClient.SendPinChangeMessage(f, seqID, resp, output);
            }
        }

        virtual void setup()
        {
            #ifdef DEBUG
                DEBUG_DEV.println("AnalogInputs::setup");
            #endif

            auto pins = GetPins();
            for( int x = 0; x < GetPinCount(); x++ )
            {
                AnalogPin & pin = *static_cast<AnalogPin*>(pins[x]);
                // Set pin current state to -1 to trigger initial state update
                pin.pinCurrentState = -1;
            }
            // Then set the feature to ready, otherwise it will not be available to process incoming messages or perform local
            // tasks such as pin reads.
            SetFeatureReady(true);
        }

        protected:

        // onConnected gets called when the python host has connected and completed handshaking
        virtual void onConnected()
        {
            #ifdef DEBUG
                DEBUG_DEV.println("AnalogInputs::onConnected");
            #endif
        }

        // onDisconnected gets called when the python host has disconnected
        virtual void onDisconnected()
        {
            #ifdef DEBUG
                DEBUG_DEV.println("AnalogInputs::onDisconnected");
            #endif
        }

        virtual uint8_t InitFeaturePin(uint8_t fid, uint8_t lid, String& pid, JsonDocument& json, String& fail_reason, Pin ** p)
        {
            AnalogPin * ap = new AnalogPin();
            ap->fid = fid;
            ap->lid = lid;
            ap->mid = -1;

            if(json.containsKey("id"))
            {
                String idstring = json[F("id")];
                ap->pid = idstring;
                ap->mid = convertPinString(idstring.c_str());
            }
            else
            {
               // dp->pinInitialState = -1;
               fail_reason = "Missing pin 'id' key in JSON";
               return ERR_INVALID_JSON;
            }
            if(json.containsKey("is"))
            {
                ap->pinInitialState = json["is"];
            }
            else
            {
                ap->pinInitialState = -1;
            }
            if(json.containsKey("cs"))
            {
                ap->pinConnectedState = json["cs"];
            }
            else
            {
                ap->pinConnectedState = -1;
            }
            if(json.containsKey("ds"))
            {
                ap->pinDisconnectedState = json["ds"];
            }
            else
            {
                ap->pinDisconnectedState = -1;
            }
            if(json.containsKey("ps"))
            {
                ap->pinSmoothing = json["ps"];
            }
            else
            {
                ap->pinSmoothing = 0;
            }
            if(json.containsKey("px"))
            {
                ap->pinSmoothingAlgo = json["px"];
            }
            else
            {
#ifdef AINPUTS_SMOOTHING_SIMPLE
                ap->pinSmoothingAlgo = AINPUTS_SMOOTHING_SIMPLE;
#else
                ap->pinSmoothingAlgo = 0; // Default to 0 if AINPUTS_SMOOTHING_SIMPLE is not defined
#endif
            }
            
            if(json.containsKey("ph"))
            {
                ap->pinMaxValue = json["ph"];
            }
            else
            {
                ap->pinMaxValue = 1024;
            }

            if(json.containsKey("pl"))
            {
                ap->pinMinValue = json["pl"];
            }
            else
            {
                ap->pinMinValue = 0;
            }

            if( json.containsKey("pr"))
            {
                if (json["pr"] > 0)
                {
#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_RENESAS) || defined(ARDUINO_ARCH_MBED) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
                    analogReadResolution(json["pr"]);
#else
                    DEBUG_DEV.println(F("ERROR: analogReadResolution not supported on this platform"));
#endif
                }
                else
                {
                    DEBUG_DEV.println(F("INFO: pin resolution not set, using default number of bits"));
                }

            }
            
            #ifdef DEBUG
                DEBUG_DEV.print(F("AnalogInputs::InitFeaturePin: "));
                DEBUG_DEV.print(F("fid: "));
                DEBUG_DEV.print(fid);
                DEBUG_DEV.print(F(", lid: "));
                DEBUG_DEV.print(lid);
                DEBUG_DEV.print(F(", pid: "));
                DEBUG_DEV.print(pid);
                DEBUG_DEV.print(F(", mid: "));
                DEBUG_DEV.print(ap->mid);
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print(F(", is: "));
                    DEBUG_DEV.print(ap->pinInitialState);
                    DEBUG_DEV.print(F(", cs: "));
                    DEBUG_DEV.print(ap->pinConnectedState);
                    DEBUG_DEV.print(F(", ds: "));
                    DEBUG_DEV.print(ap->pinDisconnectedState);
                    DEBUG_DEV.print(F(", ps: "));
                    DEBUG_DEV.print(ap->pinSmoothing);
                    DEBUG_DEV.print(F(", px: "));
                    DEBUG_DEV.println(ap->pinSmoothingAlgo);
                    DEBUG_DEV.print(F(", ph: "));
                    DEBUG_DEV.print(ap->pinMaxValue);
                    DEBUG_DEV.print(F(", pl: "));
                    DEBUG_DEV.println(ap->pinMinValue);
                #endif
            #endif
            //dp->pinConnectedState = 1;
            //dp->pinDisconnectedState = 0;
            //dp->debounce = 0;
            //dp->inputPullup = 0;
            //dp->pinCurrentState = 0;
            //dp->t = 0;
            *p = ap;
            fail_reason = "";
            return 0;
        }

    };
    #endif
}
#endif