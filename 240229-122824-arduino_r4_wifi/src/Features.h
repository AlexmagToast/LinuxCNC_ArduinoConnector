#ifndef FEATURES_H_
#define FEATURES_H_
#include "FeatureController.h"
#include "Config.h"

namespace Features
{
    #ifdef DINPUTS
    class DigitalInputs: public Feature
    {
        public:
        DigitalInputs() : Feature(DINPUTS, DEFAULT_LOOP_FREQUENCY)
        {
        }

        virtual void loop()
        {
        }

        virtual void setup()
        {
        }

        protected:
        // onConfig gets called when a config message is received from the python host
        virtual bool onConfig(protocol::ConfigMessage * config)
        {
            return true;
        }
        // onConnected gets called when the python host has connected and completed handshaking
        virtual void onConnected()
        {

        }
        // onDisconnected gets called when the python host has disconnected
        virtual void onDisconnected()
        {

        }
    }din;
    #endif
}
#endif