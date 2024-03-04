#ifndef FEATURE_CONTROLLER_H_
#define FEATURE_CONTROLLER_H_
#include <Arduino.h>
#include "ArduinoJson.h"
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

    protected:
        virtual bool onConfig(protocol::ConfigMessage*) = 0;
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;
};

typedef IFeature* FeaturePtr;

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


private:
    FeaturePtr * _features = NULL;
    size_t _allocatedFeatureCount = 0;
    size_t _defaultToAlloc = 2; /* TODO: Move this to Config.h or some better place*/
    size_t _currentFeatureCount = 0;
}featureController;

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
    virtual bool onConfig(protocol::ConfigMessage * config)
    {
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

    uint8_t _featureArrayIndex = 0;

    virtual bool FeatureReady()
    {
        return _featureReady;
    }

    virtual void SetFeatureReady(bool b)
    {
        _featureReady = b;
    }

private:
    // How often loop should be called for feature
    int _loopFrequency = 5000;
    unsigned long _lastExecutedMillis = 0;
    uint8_t _featureID = 0;
    PinPtr * _pins = NULL;
    bool _featureReady = false;
};
#endif