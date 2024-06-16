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

namespace SetupEventOptions
{
    const uint8_t PostConfigSync = 0; // Default. Only call Setup after config sync success
    const uint8_t PostStart = 1; // Call setup after start (regardless of config sync success)
    const uint8_t PostStartAndPostConfigSync = 2; // Call setup after start and after config sync success
}

namespace LoopEventOptions
{
    const uint8_t PostConfigSync = 0; // Default. Only call loop after config sync success
    const uint8_t PostStart = 1; // Call loop after start (regardless of config sync success)
}

struct Pin
{
    uint8_t fid;
    uint8_t lid;
    String pid;
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
       
       virtual uint8_t GetFeatureID() = 0;
       virtual String GetFeatureName() = 0;
       virtual void SetFeatureArrayIndex(uint8_t) = 0;
       virtual uint32_t onConfig(protocol::ConfigMessage*, String& fail_reason) = 0;
       virtual uint8_t GetLoopEventOption() = 0;

       virtual uint8_t InitFeaturePin(uint8_t fid, uint8_t lid, String& pid, JsonDocument& json, String& fail_reason, Pin ** p) = 0;

    protected:
        virtual void SetFeatureReady(bool)=0;
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;
        virtual PinPtr* GetPins() = 0;
        virtual uint16_t GetPinCount() = 0;
        
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
            if( currentMills - _features[x]->GetLastExecMilli() >= _features[x]->GetLoopFreq())
            {
                if ( _features[x]->FeatureReady() || _features[x]->GetLoopEventOption() == LoopEventOptions::PostStart )
                {
                    _features[x]->loop();
                    _features[x]->SetLastExecMilli(millis());
                }
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
            #ifdef DEBUG
                DEBUG_DEV.println(F("FeatureController::RegisterFeature: Allocating memory for features"));
            #endif
            _features = new FeaturePtr[_defaultToAlloc];
            _allocatedFeatureCount = _defaultToAlloc;
        }


        if( _currentFeatureCount + 1 > _allocatedFeatureCount )
        {
            #ifdef DEBUG
                DEBUG_DEV.println(F("FeatureController::RegisterFeature: Allocating more memory for features"));
            #endif
            // need to allocate more space
            FeaturePtr * tmp = new FeaturePtr[_allocatedFeatureCount+_defaultToAlloc];
            for (int x = 0; x < _currentFeatureCount; x++ )
            {
                tmp[x] = _features[x];
            }
            delete [] _features;
            _features = tmp;
        }
        f->SetFeatureArrayIndex(_currentFeatureCount);
        #ifdef DEBUG
            DEBUG_DEV.print(F("FeatureController::RegisterFeature: Registering feature "));
            DEBUG_DEV.print(f->GetFeatureID());
            DEBUG_DEV.print(F(" with name "));
            DEBUG_DEV.println(f->GetFeatureName());
            DEBUG_DEV.print(F("Feature Array Index = "));
            DEBUG_DEV.println(_currentFeatureCount);
        #endif
        _features[_currentFeatureCount] = f;//new FeaturePtr();
        
        return _currentFeatureCount++;
        
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
                uint32_t r = _features[x]->onConfig(&cm, reason);
                if( r == 0 )
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
                    nak.errorCode = r;
                    nak.errorString = reason;
                    serialClient.SendMessage(nak);
                }
                return;
            }

        }

        #ifdef DEBUG
            protocol::ConfigMessageNak nak;
            nak.featureID = cm.featureID;
            nak.seq = cm.seq;
            nak.errorCode = ERR_INVALID_FEATURE_ID;
            nak.errorString = "Feature ID unknown";
            serialClient.SendMessage(nak);
            DEBUG_DEV.print(F("Feature not found for featureID: "));
            DEBUG_DEV.println(cm.featureID);
        #endif
    
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
    Feature(const uint8_t featureID, 
        String featureName, 
        const size_t loopFrequency=DEFAULT_LOOP_FREQUENCY, 
        const uint8_t setupEventOption=SetupEventOptions::PostConfigSync,
        const uint8_t loopEventOption=LoopEventOptions::PostConfigSync)
    {
        #ifdef DEBUG
            DEBUG_DEV.println("DigitalInputs::DigitalInputs");
        #endif
        _featureID = featureID;
        _loopFrequency = loopFrequency;
        _featureName = featureName;
        _setupEventOption = setupEventOption;
        #ifdef DEBUG
            DEBUG_DEV.print(F("Feature::Feature: Registering feature "));
            DEBUG_DEV.print(featureID);
            DEBUG_DEV.print(F(" with name "));
            DEBUG_DEV.println(featureName);
        #endif
        //_featureArrayIndex = featureController.RegisterFeature(this);
        if(_setupEventOption == SetupEventOptions::PostStart || 
            _setupEventOption == SetupEventOptions::PostStartAndPostConfigSync)
        {
            setup();
        }
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

    virtual uint8_t GetFeatureID()
    {
        return _featureID;
    }

    virtual int GetLoopFreq()
    {
        return _loopFrequency;
    }

    virtual void SetLoopFreq(int freq)
    {
        _loopFrequency = freq;
    }

    virtual void SetFeatureName(String name)
    {
        _featureName = name;
    }

    virtual String GetFeatureName()
    {
        return _featureName;
    }

    virtual void SetFeatureArrayIndex(uint8_t index)
    {
        _featureArrayIndex = index;
    }  

    virtual uint8_t GetLoopEventOption()
    {
        return _loopEventOption;
    }

protected:
    /*
        Events
    */
    // onConfig gets called when a config message is received from the python host
    virtual uint32_t onConfig(protocol::ConfigMessage * config, String& fail_reason)
    {
        if (config->total == 0)
        {
            fail_reason = "Error. Feature " + GetFeatureName() + " with ID " + String(config->featureID) + "did not implement onConfig";
            return ERR_NOT_IMPLEMENTED_BY_FEATURE;
        }
        if (config->seq == 0)
        {
            #ifdef DEBUG
                DEBUG_DEV.print(F("Feature::onConfig: Feature ID "));
                DEBUG_DEV.print(config->featureID);
                DEBUG_DEV.println(F(" is initializing pins"));
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print(F("Feature Name = "));
                    DEBUG_DEV.println(GetFeatureName());
                    DEBUG_DEV.print(F("Total = "));
                    DEBUG_DEV.println(config->total);
                    DEBUG_DEV.print(F("Seq = "));
                    DEBUG_DEV.println(config->seq);
                #endif

                //DEBUG_DEV.println(F("Feature::onConfig: seq is 0, initializing pins")); 
            #endif
            InitPins(config->total);
        }   
        JsonDocument pinDoc;
        DeserializationError error = deserializeJson(pinDoc, config->configString);

        // check for deserialization error
        if (error) {
            fail_reason = "Failed to deserialize config string";
            return ERR_INVALID_JSON;
        }
        // Extract lid and pid from pinDoc
        uint8_t lid = pinDoc["l"];
        String pid = pinDoc["p"];
        // AddPin based on config's featureID, lid and pid
        //auto pin = InitFeaturePin(config->featureID, lid, pid, pinDoc);
        Pin * p;
        uint8_t code = InitFeaturePin(config->featureID, lid, pid, pinDoc, fail_reason, &p);
        if (code != 0)
        {
            fail_reason = fail_reason;
            return code;
        }
        AddPin(p, config->seq);
        // Set feautre ready if all pins are initialized
        if (config->seq == config->total - 1)
        {
            // output debug indicating feature ID was set ready
            #ifdef DEBUG
                DEBUG_DEV.print(F("Feature::onConfig: Feature ID "));
                DEBUG_DEV.print(config->featureID);
                DEBUG_DEV.println(F(" is ready"));
                #ifdef DEBUG_VERBOSE
                    DEBUG_DEV.print(F("Feature Name = "));
                    DEBUG_DEV.println(GetFeatureName());
                    DEBUG_DEV.print(F("Total Pins = "));
                    DEBUG_DEV.println(config->total);
                    DEBUG_DEV.print(F("Feature Index = "));
                    DEBUG_DEV.println(_featureArrayIndex);
                #endif
            #endif
            _configSynced = true;
            if(_setupEventOption == SetupEventOptions::PostConfigSync || 
                _setupEventOption == SetupEventOptions::PostStartAndPostConfigSync)
            {
                setup();
            }
        }       
        return ERR_NONE;
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
        _pinCount = 0;
        _pins = new PinPtr[size];
    }

    virtual void AddPin(Pin* p, uint8_t index)
    {
        _pins[index] = p;
        _pinCount++;
    }

    virtual Pin* GetPin(uint8_t index)
    {
        return _pins[index];
    }

    virtual PinPtr* GetPins()
    {
        return _pins;
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

    uint16_t GetPinCount()
    {
        return _pinCount;
    }

   

private:
    // How often loop should be called for feature
    int _loopFrequency = 5000;
    unsigned long _lastExecutedMillis = 0;
    uint8_t _featureID = 0;
    PinPtr * _pins = NULL;
    uint16_t _pinCount = 0;
    bool _featureReady = false;
    bool _configSynced = false;
    uint8_t _featureArrayIndex = 0;
    String _featureName;
    uint8_t _setupEventOption = SetupEventOptions::PostConfigSync;
    uint8_t _loopEventOption = LoopEventOptions::PostConfigSync;
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