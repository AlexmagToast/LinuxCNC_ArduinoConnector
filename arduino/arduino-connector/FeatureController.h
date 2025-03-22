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
#ifndef FEATURE_CONTROLLER_H_
#define FEATURE_CONTROLLER_H_
#include <Arduino.h>
#include "ArduinoJson.h"
#include "Config.h"
#include "Protocol.h"

/** Default loop frequency in milliseconds between successive feature loop executions */
const int DEFAULT_LOOP_FREQUENCY = 50;

/**
 * @namespace SetupEventOptions
 * @brief Defines when feature setup methods should be called
 */
namespace SetupEventOptions
{
    const uint8_t PostConfigSync = 0;              ///< Default. Only call Setup after config sync success
    const uint8_t PostStart = 1;                   ///< Call setup after start (regardless of config sync success)
    const uint8_t PostStartAndPostConfigSync = 2;  ///< Call setup after start and after config sync success
}

/**
 * @namespace LoopEventOptions
 * @brief Defines when feature loop methods should be called
 */
namespace LoopEventOptions
{
    const uint8_t PostConfigSync = 0;  ///< Default. Only call loop after config sync success
    const uint8_t PostStart = 1;       ///< Call loop after start (regardless of config sync success)
}

/**
 * @struct Pin
 * @brief Represents a physical or virtual pin in the system
 */
struct Pin
{
    uint8_t fid;     ///< Feature ID
    uint8_t lid;     ///< Local ID within feature
    int8_t mid;      ///< ID mapped from defines, -1 if unmapped
    String pid;      ///< Pin identifier string
};

typedef Pin* PinPtr;  ///< Pointer to a Pin structure

/**
 * @class IFeature
 * @brief Interface for feature implementations
 * 
 * Defines the interface that all features must implement to be managed
 * by the FeatureController.
 */
class IFeature
{
    public:
       /**
        * @brief Initialize the feature
        * 
        * Called when the feature should be set up.
        */
       virtual void setup() = 0;
       
       /**
        * @brief Main execution loop for the feature
        * 
        * Called periodically based on the loop frequency.
        */
       virtual void loop() = 0;
       
       /**
        * @brief Get the time of last execution
        * @return Milliseconds timestamp of last execution
        */
       virtual unsigned long GetLastExecMilli() = 0;
       
       /**
        * @brief Output debug information
        * @param message Debug message to output
        */
       virtual void Debug(String message) = 0;
       
       /**
        * @brief Get the loop frequency
        * @return Loop frequency in milliseconds
        */
       virtual int GetLoopFreq() = 0;
       
       /**
        * @brief Set the time of last execution
        * @param milliseconds Timestamp to set
        */
       virtual void SetLastExecMilli(unsigned long milliseconds) = 0;
       
       /**
        * @brief Check if feature is ready
        * @return True if feature is ready, false otherwise
        */
       virtual bool FeatureReady() = 0;
       
       /**
        * @brief Get the feature ID
        * @return Feature ID
        */
       virtual uint8_t GetFeatureID() = 0;
       
       /**
        * @brief Get the feature name
        * @return Feature name
        */
       virtual String GetFeatureName() = 0;
       
       /**
        * @brief Set the feature array index
        * @param index Index in the feature array
        */
       virtual void SetFeatureArrayIndex(uint8_t index) = 0;
       
       /**
        * @brief Handle configuration message
        * @param config Configuration message
        * @param fail_reason Reason for failure if any
        * @return Error code, 0 for success
        */
       virtual uint32_t onConfig(protocol::ConfigMessage* config, String& fail_reason) = 0;
        /**
         * @brief Handle pin change message
         * @param pcm Pin change message
         */
        virtual void onPinChange(const protocol::PinChangeMessage& pcm) = 0;
       /**
        * @brief Get the loop event option
        * @return Loop event option
        */
       virtual uint8_t GetLoopEventOption() = 0;

       /**
        * @brief Initialize a feature pin
        * @param fid Feature ID
        * @param lid Local ID within feature
        * @param pid Pin identifier
        * @param json JSON document with pin configuration
        * @param fail_reason Reason for failure if any
        * @param p Pointer to store created Pin
        * @return Error code, 0 for success
        */
       virtual uint8_t InitFeaturePin(uint8_t fid, uint8_t lid, String& pid, JsonDocument& json, String& fail_reason, Pin** p) = 0;

    protected:
        /**
         * @brief Set feature ready state
         * @param ready Ready state to set
         */
        virtual void SetFeatureReady(bool ready) = 0;
        
        /**
         * @brief Called when connection is established
         */
        virtual void onConnected() = 0;
        

        
        /**
         * @brief Called when connection is lost
         */
        virtual void onDisconnected() = 0;
        
        /**
         * @brief Get array of pins
         * @return Array of pin pointers
         */
        virtual PinPtr* GetPins() = 0;
        
        /**
         * @brief Get number of pins
         * @return Pin count
         */
        virtual uint16_t GetPinCount() = 0;
};

typedef IFeature* FeaturePtr;  ///< Pointer to an IFeature

/**
 * @class FeatureController
 * @brief Manages a collection of features and provides methods to execute their loops and setups
 *
 * The FeatureController class allows registering features, executing their loops, and performing setups.
 * It also provides methods to handle configuration messages for the registered features.
 */
class FeatureController {
public:
    /**
     * @brief Constructor
     */
    FeatureController()
    {
    }

    /**
     * @brief Destructor
     */
    ~FeatureController()
    {
    }

    /**
     * @brief Get number of registered features
     * @return Number of registered features
     */
    size_t GetRegisteredFeatureCount()
    {
        return _currentFeatureCount;
    }

    /**
     * @brief Execute the loop method for all registered features
     * 
     * Executes the loop method for features whose execution interval has elapsed
     * and that are either ready or have appropriate loop event options.
     */
    void ExecuteFeatureLoops()
    {
        unsigned long currentMills = millis();
        for(int x = 0; x < _currentFeatureCount; x++)
        {
            if(currentMills - _features[x]->GetLastExecMilli() >= _features[x]->GetLoopFreq())
            {
                if(_features[x]->FeatureReady() || _features[x]->GetLoopEventOption() == LoopEventOptions::PostStart)
                {
                    _features[x]->loop();
                    _features[x]->SetLastExecMilli(millis());
                }
            }
        }
    }

    /**
     * @brief Execute the setup method for all registered features
     */
    void ExcecuteFeatureSetups()
    {
        for(int x = 0; x < _currentFeatureCount; x++)
        {
            _features[x]->setup();
        }
    }

    /**
     * @brief Register a feature
     * @param f Feature to register
     * @return Index of the registered feature
     * 
     * Registers a feature and returns an index value for the feature.
     * The index avoids having to iterate over the feature array.
     */
    int RegisterFeature(IFeature* f)
    {
        if(_features == NULL)
        {
            #ifdef DEBUG
                DEBUG_DEV.println(F("FeatureController::RegisterFeature: Allocating memory for features"));
            #endif
            _features = new FeaturePtr[_defaultToAlloc];
            _allocatedFeatureCount = _defaultToAlloc;
        }

        if(_currentFeatureCount + 1 > _allocatedFeatureCount)
        {
            #ifdef DEBUG
                DEBUG_DEV.println(F("FeatureController::RegisterFeature: Allocating more memory for features"));
            #endif
            // need to allocate more space
            FeaturePtr* tmp = new FeaturePtr[_allocatedFeatureCount+_defaultToAlloc];
            for(int x = 0; x < _currentFeatureCount; x++)
            {
                tmp[x] = _features[x];
            }
            delete[] _features;
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
        _features[_currentFeatureCount] = f;
        
        return _currentFeatureCount++;
    }

    /**
     * @brief Handle config message
     * @param cm Configuration message
     * 
     * Processes a configuration message by finding the target feature
     * and forwarding the message to it. Sends acknowledgment or error
     * messages as appropriate.
     */
    void OnConfig(protocol::ConfigMessage& cm)
    {
        if(_features == NULL)
        {
            #ifdef DEBUG
                DEBUG_DEV.println(F("Error. No features registered!"));
            #endif
            return;
        }
        
        for(int x = 0; x < _currentFeatureCount; x++)
        {
            if(_features[x]->GetFeatureID() == cm.featureID)
            {   
                String reason;
                uint32_t r = _features[x]->onConfig(&cm, reason);
                if(r == 0)
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

    /**
     * @brief Handle pin change message
     * @param pcm Pin change message
     */
    void OnPinChange(const protocol::PinChangeMessage& pcm) {
        for(int x = 0; x < _currentFeatureCount; x++)
        {
            if(_features[x]->GetFeatureID() == pcm.featureID)
            {
                _features[x]->onPinChange(pcm);
            }
        }
    }

private:
    FeaturePtr* _features = NULL;              ///< Array of feature pointers
    size_t _allocatedFeatureCount = 0;         ///< Number of allocated feature slots
    size_t _defaultToAlloc = 2;                ///< Default number of slots to allocate at once
    size_t _currentFeatureCount = 0;           ///< Current number of registered features
}featureController;

/**
 * @class Feature
 * @brief Base class for implementing features
 * 
 * The Feature class provides a common implementation of the IFeature interface.
 * Derived classes can override methods to customize behavior.
 */
class Feature : public IFeature {
public:
    /**
     * @brief Constructor
     * @param featureID Unique identifier for the feature
     * @param featureName Name of the feature
     * @param loopFrequency How often the loop method should be called (in milliseconds)
     * @param setupEventOption When the setup method should be called
     * @param loopEventOption When the loop method should be called
     */
    Feature(const uint8_t featureID, 
        String featureName, 
        const size_t loopFrequency=DEFAULT_LOOP_FREQUENCY, 
        const uint8_t setupEventOption=SetupEventOptions::PostConfigSync,
        const uint8_t loopEventOption=LoopEventOptions::PostConfigSync)
    {
        _featureID = featureID;
        _loopFrequency = loopFrequency;
        _featureName = featureName;
        _setupEventOption = setupEventOption;
        _loopEventOption = loopEventOption;
        
        #ifdef DEBUG
            DEBUG_DEV.print(F("Feature::Feature: Registering feature "));
            DEBUG_DEV.print(featureID);
            DEBUG_DEV.print(F(" with name "));
            DEBUG_DEV.println(featureName);
        #endif
        
        if(_setupEventOption == SetupEventOptions::PostStart || 
            _setupEventOption == SetupEventOptions::PostStartAndPostConfigSync)
        {
            setup();
        }
    }
    
    /**
     * @brief Destructor
     */
    ~Feature()
    {
    }

    /**
     * @brief Setup the feature
     * 
     * Override this method to implement feature-specific setup.
     */
    virtual void setup() override
    {
    }

    /**
     * @brief Execute feature logic
     * 
     * Override this method to implement feature-specific logic that
     * should run periodically.
     */
    virtual void loop() override
    {
    }

    /**
     * @brief Output debug information
     * @param message Debug message
     * 
     * Override this method to implement feature-specific debug output.
     */
    virtual void Debug(String message) override
    {
    }

    /**
     * @brief Get the time of last execution
     * @return Milliseconds timestamp of last execution
     */
    virtual unsigned long GetLastExecMilli() override
    {
        return _lastExecutedMillis;
    }

    /**
     * @brief Set the time of last execution
     * @param ms Timestamp to set
     */
    virtual void SetLastExecMilli(unsigned long ms) override
    {
        _lastExecutedMillis = ms;
    }

    /**
     * @brief Get the feature ID
     * @return Feature ID
     */
    virtual uint8_t GetFeatureID() override
    {
        return _featureID;
    }

    /**
     * @brief Get the loop frequency
     * @return Loop frequency in milliseconds
     */
    virtual int GetLoopFreq() override
    {
        return _loopFrequency;
    }

    /**
     * @brief Set the loop frequency
     * @param freq New frequency in milliseconds
     */
    virtual void SetLoopFreq(int freq)
    {
        _loopFrequency = freq;
    }

    /**
     * @brief Set the feature name
     * @param name New feature name
     */
    virtual void SetFeatureName(String name)
    {
        _featureName = name;
    }

    /**
     * @brief Get the feature name
     * @return Feature name
     */
    virtual String GetFeatureName() override
    {
        return _featureName;
    }

    /**
     * @brief Set the feature array index
     * @param index Index in the feature array
     */
    virtual void SetFeatureArrayIndex(uint8_t index) override
    {
        _featureArrayIndex = index;
    }  

    /**
     * @brief Get the loop event option
     * @return Loop event option
     */
    virtual uint8_t GetLoopEventOption() override
    {
        return _loopEventOption;
    }

    /**
     * @brief Check if feature is ready
     * @return True if feature is ready, false otherwise
     */
    virtual bool FeatureReady() override
    {
        return _featureReady;
    }
    
    /**
     * @brief Handle pin change message
     * @param pcm Pin change message
     */

    virtual void onPinChange(const protocol::PinChangeMessage& pcm) {
    }

protected:
    /**
     * @brief Handle configuration message
     * @param config Configuration message
     * @param fail_reason Reason for failure if any
     * @return Error code, 0 for success
     * 
     * Default implementation initializes pins based on the configuration
     * and sets the feature ready state when all pins are configured.
     */
    virtual uint32_t onConfig(protocol::ConfigMessage* config, String& fail_reason) override
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
        uint8_t lid = pinDoc["li"];
        String pid = pinDoc["id"];
        
        // Initialize the pin based on configuration
        Pin* p;
        uint8_t code = InitFeaturePin(config->featureID, lid, pid, pinDoc, fail_reason, &p);
        if (code != 0)
        {
            return code;
        }
        
        AddPin(p, config->seq);
        
        // Set feature ready if all pins are initialized
        if (config->seq == config->total - 1)
        {
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

    /**
     * @brief Called when connection is established
     */
    virtual void onConnected() override
    {
    }
    
    /**
     * @brief Called when connection is lost
     */
    virtual void onDisconnected() override
    {
    }

    /**
     * @brief Initialize pins array
     * @param size Number of pins to allocate
     */
    virtual void InitPins(const size_t& size)
    {
        if(_pins != NULL)
        {
            delete[] _pins;
        }
        _pinCount = 0;
        _pins = new PinPtr[size];
    }

    /**
     * @brief Add a pin to the pins array
     * @param p Pin to add
     * @param index Index in the array
     */
    virtual void AddPin(Pin* p, uint8_t index)
    {
        _pins[index] = p;
        _pinCount++;
    }

    /**
     * @brief Get a pin by index
     * @param index Index of the pin
     * @return Pin pointer or NULL if not found
     */
    virtual Pin* GetPin(uint8_t index)
    {
        return _pins[index];
    }

    /**
     * @brief Get the array of pins
     * @return Array of pin pointers
     */
    virtual PinPtr* GetPins() override
    {
        return _pins;
    }

    /**
     * @brief Set feature ready state
     * @param b Ready state to set
     */
    virtual void SetFeatureReady(bool b) override
    {
        _featureReady = b;
    }

    /**
     * @brief Get the feature array index
     * @return Feature array index
     */
    uint8_t GetFeatureArrayIndex()
    {
        return _featureArrayIndex;
    }

    /**
     * @brief Get number of pins
     * @return Pin count
     */
    uint16_t GetPinCount() override
    {
        return _pinCount;
    }

private:
    int _loopFrequency = 5000;                ///< How often loop should be called (milliseconds)
    unsigned long _lastExecutedMillis = 0;     ///< When the loop was last executed
    uint8_t _featureID = 0;                    ///< Unique identifier for the feature
    PinPtr* _pins = NULL;                      ///< Array of pin pointers
    uint16_t _pinCount = 0;                    ///< Number of pins
    bool _featureReady = false;                ///< Whether the feature is ready
    bool _configSynced = false;                ///< Whether configuration has been synchronized
    uint8_t _featureArrayIndex = 0;            ///< Index in the feature array
    String _featureName;                       ///< Name of the feature
    uint8_t _setupEventOption = SetupEventOptions::PostConfigSync;  ///< When setup should be called
    uint8_t _loopEventOption = LoopEventOptions::PostConfigSync;    ///< When loop should be called
};

/**
 * @namespace Callbacks
 * @brief Contains callback functions for various events
 */
namespace Callbacks
{
    /**
     * @brief Configuration message callback
     * @param cm Configuration message
     * 
     * Called when a configuration message is received.
     */
    void onConfig(protocol::ConfigMessage& cm) {
        #ifdef DEBUG_VERBOSE
            DEBUG_DEV.print(F("::onConfig called, featureID = "));
            DEBUG_DEV.print((int)cm.featureID);
            DEBUG_DEV.print(F(" Seq = "));
            DEBUG_DEV.print(cm.seq);
            DEBUG_DEV.print(F(" Total = "));
            DEBUG_DEV.println(cm.total);
            DEBUG_DEV.print("Config: ");
            DEBUG_DEV.println(cm.configString);
        #endif
        featureController.OnConfig(cm);
    }

    /**
     * @brief Pin change message callback
     * @param pcm Pin change message
     * 
     * Called when a pin change message is received.
     */
    void onPinChange(const protocol::PinChangeMessage& pcm) {
        #ifdef DEBUG_VERBOSE
            DEBUG_DEV.print(F("::onPinChange called, featureID = "));
            DEBUG_DEV.println((int)pcm.featureID);
        #endif
        featureController.OnPinChange(pcm);
    }
}
#endif