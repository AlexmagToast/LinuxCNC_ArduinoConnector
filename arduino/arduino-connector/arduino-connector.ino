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
#include <Arduino.h>
#include "Config.h"
#include "FeatureController.h"
#include "Features.h"

#include <ArduinoJson.h>
#ifdef ENABLE_FEATUREMAP
#include "FeatureMap.h"
#endif


void setup() {
  COM_DEV.begin(115200);
  delay(SERIAL_STARTUP_DELAY);
  #ifdef DEBUG
    DEBUG_DEV.println(F("STARTING UP!!"));
    //DEBUG_DEV.println("HERE WE GO");
    DEBUG_DEV.flush();
  #endif
  
  serialClient.RegisterConfigCallback(Callbacks::onConfig);
  featureController.ExcecuteFeatureSetups();
  #ifdef DINPUTS
    Features::DigitalInputs * din = new Features::DigitalInputs();
    featureController.RegisterFeature(din);
  #endif
  #ifdef DOUTPUTS
    Features::DigitalOutputs * dout = new Features::DigitalOutputs();
    featureController.RegisterFeature(dout);
  #endif

  serialClient.DoWork(); 
}


void loop() {
  
  serialClient.DoWork(); 
  unsigned long currentMills = millis();

  featureController.ExecuteFeatureLoops();
}
