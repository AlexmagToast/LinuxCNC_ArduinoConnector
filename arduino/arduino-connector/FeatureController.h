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
#ifndef FEATURE_CONTROLLER_H_
#define FEATURE_CONTROLLER_H_
#include <Arduino.h>
#include "ArduinoJson.h"
#include "Config.h"
#include "Protocol.h"

const int DEFAULT_LOOP_FREQUENCY = 50;

struct Pin
{
    uint8_t fid;
    uint8_t lid;
    uint8_t pid;
};

typedef Pin* PinPtr;


class IFeature
{
    public:
       virtual void setup() = 0;
       virtual void loop() = 0;
       virtual unsigned long GetLastExecMilli() = 0;
       virtual void Debug(String) = 0;
       virtual int GetLoopFreq() = 0;
       virtual void SetLastExecMilli(unsigned long) = 0;
       virtual bool FeatureReady() = 0;
       virtual void SetFeatureReady(bool);
       virtual uint8_t GetFeatureID() = 0;
       virtual bool onConfig(protocol::ConfigMessage*, String& fail_reason) = 0;

    protected:
        
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;
};

typedef IFeature* FeaturePtr;

/**
 * @class FeatureController
 * @brief The FeatureController class manages a collection of features and provides methods to execute their loops and setups.
 *
 * The FeatureController class allows registering features, executing their loops, and performing setups. It also provides a method to handle configuration messages for the registered features.
 */
class FeatureController {
public:
    FeatureController()
    {
        
    }

    ~FeatureController()
    {

    }

    size_t GetRegisteredFeatureCount()
    {
        return _currentFeatureCount;
    }

    void ExecuteFeatureLoops()
    {
        unsigned long currentMills = millis();
        for(int x = 0; x < _currentFeatureCount; x++)
        {
            if( currentMills - _features[x]->GetLastExecMilli() >= _features[x]->GetLoopFreq() &&
                _features[x]->FeatureReady())
            {
                _features[x]->loop();
                _features[x]->SetLastExecMilli(millis());
            }
        }
    }

    void ExcecuteFeatureSetups()
    {
        for(int x = 0; x < _currentFeatureCount; x++)
        {
            _features[x]->setup();
        }
    }

    // Registers a feature and returns an index value for the feature
    // Returns array position of feature to avoid having to iterate over feature array
    int RegisterFeature(IFeature * f)
    {
        if( _features == NULL )
        {
            _features = new FeaturePtr[_defaultToAlloc];
            _allocatedFeatureCount = _defaultToAlloc;
        }
        else
        {
            if( _currentFeatureCount + 1 > _allocatedFeatureCount )
            {
                // need to allocate more space
                FeaturePtr * tmp = new FeaturePtr[_allocatedFeatureCount+_defaultToAlloc];
                for (int x = 0; x < _currentFeatureCount; x++ )
                {
                    tmp[x] = _features[x];
                }
                delete [] _features;
                _features = tmp;
            }
            _features[_currentFeatureCount] = f;//new FeaturePtr();
            return _currentFeatureCount++;
        }
    }

    void OnConfig( protocol::ConfigMessage& cm)
    {
        if( _features == NULL )
        {
            #ifdef DEBUG
                DEBUG_DEV.println(F("Error. No features registered!"));
            #endif
            return;
        }
        for(int x = 0; x < _currentFeatureCount; x++)
        {
            if( _features[x]->GetFeatureID() == cm.featureID )
            {   
                String reason;
                if( _features[x]->onConfig(&cm, reason) )
                {
                    protocol::ConfigMessageAck ack;
                    ack.featureID = cm.featureID;
                    ack.seq = cm.seq;
                    ack.featureArrIndex = x;
                    serialClient.SendMessage(ack);
                }
                else
                {
                    protocol::ConfigMessageNak nak;
                    nak.featureID = cm.featureID;
                    nak.seq = cm.seq;
                    nak.errorCode = 10;
                    nak.errorString = "Feature did not accept config";
                    serialClient.SendMessage(nak);
                }
                return;
            }
            else
            {
                #ifdef DEBUG
                    DEBUG_DEV.print(F("Feature not found for featureID: "));
                    DEBUG_DEV.println(cm.featureID);
                #endif
            }
        }
        /*
        protocol::ConfigMessageNak nak;
        nak.featureID = cm.featureID;
        nak.seq = cm.seq;
        //ack.featureArrIndex = 33;
        nak.errorCode = 10;
        nak.errorString = "TEST FAIL";
        serialClient.SendMessage(nak);
        */
    }


private:
    FeaturePtr * _features = NULL;
    size_t _allocatedFeatureCount = 0;
    size_t _defaultToAlloc = 2; /* TODO: Move this to Config.h or some better place*/
    size_t _currentFeatureCount = 0;
}featureController;

/**
 * @class Feature
 * @brief Represents a feature in the system.
 * 
 * The `Feature` class is a base class for implementing features in the system. It provides
 * common functionality and methods that can be overridden by derived classes to customize
 * the behavior of the feature.
 */
class Feature : public IFeature {
public:
    Feature(const uint8_t featureID, const size_t loopFrequency=DEFAULT_LOOP_FREQUENCY)
    {
        _featureID = featureID;
        _loopFrequency = loopFrequency;
        _featureArrayIndex = featureController.RegisterFeature(this);
    }
    
    ~Feature()
    {

    }

    virtual void setup()
    {

    }

    virtual void loop()
    {

    }

    virtual void Debug(String message)
    {

    }

    virtual unsigned long GetLastExecMilli()
    {
        return _lastExecutedMillis;
    }

    virtual void SetLastExecMilli(unsigned long ms)
    {
        _lastExecutedMillis = ms;
    }

    uint8_t GetFeatureID()
    {
        return _featureID;
    }

    int GetLoopFreq()
    {
        return _loopFrequency;
    }

protected:
    /*
        Events
    */
    // onConfig gets called when a config message is received from the python host
    virtual bool onConfig(protocol::ConfigMessage * config, String& fail_reason)
    {
        fail_reason = "Feature did not implement onConfig";
        return false;
    }
    // onConnected gets called when the python host has connected and completed handshaking
    virtual void onConnected()
    {

    }
    // onDisconnected gets called when the python host has disconnected
    virtual void onDisconnected()
    {

    }

    virtual void InitPins(const size_t& size)
    {
        if( _pins != NULL )
        {
            delete [] _pins;
        }
        _pins = new PinPtr[size];
    }

    virtual void AddPin(Pin* p, uint8_t index)
    {
        _pins[index] = p;
    }

    virtual Pin* GetPin(uint8_t index)
    {
        return _pins[index];
    }

    virtual bool FeatureReady()
    {
        return _featureReady;
    }

    virtual void SetFeatureReady(bool b)
    {
        _featureReady = b;
    }

    uint8_t GetFeatureArrayIndex()
    {
        return _featureArrayIndex;
    }

private:
    // How often loop should be called for feature
    int _loopFrequency = 5000;
    unsigned long _lastExecutedMillis = 0;
    uint8_t _featureID = 0;
    PinPtr * _pins = NULL;
    bool _featureReady = false;
    uint8_t _featureArrayIndex = 0;
};


namespace Callbacks
{
    void onConfig(protocol::ConfigMessage& cm) {
        #ifdef DEBUG
        DEBUG_DEV.print(F("::onConfig called, featureID = "));
        DEBUG_DEV.print((int)cm.featureID);
        DEBUG_DEV.print(F(" Seq = "));
        DEBUG_DEV.print(cm.seq);
        DEBUG_DEV.print(F(" Total = "));
        DEBUG_DEV.println(cm.total);
        #ifdef DEBUG_VERBOSE
            DEBUG_DEV.print("Config: ");
            DEBUG_DEV.println(cm.configString);
        #endif
        #endif
        featureController.OnConfig(cm);
    }
}
#endif