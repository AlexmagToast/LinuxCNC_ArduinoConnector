#pragma once
#ifndef FEATUREMAP_H_
#define FEATUREMAP_H_


/**
 * Sets featureMap with currently enabled and disabled features.
 *
 * The featureMap is to enable the publication of which features have been enabled/disabled to the linuxcnc/python host.  This
 * can allow the Python/LinuxCNC side to output helpful information during debugging, e.g., "Error. X-command failed. The Y feature is disabled on Arduino"
 * The featureMap may also be useful in future features, such as the ability to display what features are enabled via the UI of LinuxCNC.  Why would this be
 * Another use case for debugging is during the initial testing using the halrun command.  If the user enters a command for a disabled feature,
 * the python side can catch the attempt before passing the command to the Arduino and instruct the user to enable the feature on the Arduino to use the feature. useful one may ask?  The answer is that over time new features will be added, and it will be difficult to remember if your Arduino has that version which
 * X feature or not.  
 */
struct featureMap
{
  uint64_t features;
  featureMap()
  {
    features = 0;
    #ifdef DEBUG
      bitSet(this->features, DEBUG);
    #endif
    #ifdef INPUTS
      bitSet(this->features, INPUTS);
    #endif
    #ifdef SINPUTS
      bitSet(this->features, SINPUTS);
    #endif
    #ifdef OUTPUTS
      bitSet(this->features, OUTPUTS);
    #endif
    #ifdef PWMOUTPUTS
      bitSet(this->features, PWMOUTPUTS);
    #endif
    #ifdef AINPUTS
      bitSet(this->features, AINPUTS);
    #endif
    #ifdef DALLAS_TEMP_SENSOR
      bitSet(this->features, DALLAS_TEMP_SENSOR);
    #endif
    #ifdef LPOTIS
      bitSet(this->features, LPOTIS);
    #endif
    #ifdef BINSEL
      bitSet(this->features, BINSEL);
    #endif
    #ifdef QUADENC
      bitSet(this->features, QUADENC);
    #endif
    #ifdef JOYSTICK
      bitSet(this->features, JOYSTICK);
    #endif
    #ifdef STATUSLED
      bitSet(this->features, STATUSLED);
    #endif
    #ifdef DLED
      bitSet(this->features, DLED);
    #endif
    #ifdef KEYPAD
      bitSet(this->features, KEYPAD);
    #endif
    #ifdef SERIAL_TO_LINUXCNC
      bitSet(this->features, SERIAL_TO_LINUXCNC);
    #endif
    #ifdef ETHERNET_UDP_TO_LINUXCNC
      bitSet(this->features, ETHERNET_UDP_TO_LINUXCNC);
    #endif
    #ifdef ETHERNET_TCP_TO_LINUXCNC
      bitSet(this->features, ETHERNET_TCP_TO_LINUXCNC);
    #endif
    #ifdef WIFI_UDP_TO_LINUXCNC
      bitSet(this->features, WIFI_UDP_TO_LINUXCNC);
    #endif
    #ifdef WIFI_TCP_TO_LINUXCNC
      bitSet(this->features, WIFI_TCP_TO_LINUXCNC);
    #endif
    #ifdef MEMORY_MONITOR
      bitSet(this->features, MEMORY_MONITOR);
    #endif
  }

  /**
  * GetFeatureStatus returns 1 when specified feature is enabled, 0 otherwise
  *
  */

  int GetFeatureStatus(int f)
  {
    return bitRead(features, f);
  }

  void DumpFeatureMapToSerial()
  {
    Serial.print("DEBUG: Feature Map = 0x");
    Serial.println(this->features, HEX);
    int flag = 0;
    #ifdef DEBUG
      Serial.println("------------------- Feature Map Decode Dump ------------------- ");
      // Debug is obviously defined if we got here, but use the map for consistency.
      flag = bitRead(this->features, DEBUG);
      Serial.print("[ID0");
      Serial.print(DEBUG);
      Serial.print("]");
      Serial.print(" DEBUG = ");
      Serial.println(flag);
      #ifdef DINPUTS
        flag = bitRead(this->features, DINPUTS);
        Serial.print("[ID0");
        Serial.print(INPUTS);
        Serial.print("]");
        Serial.print(" INPUTS = ");
        Serial.println(flag);
      #endif
      #ifdef SINPUTS
        flag = bitRead(this->features, SINPUTS);
        Serial.print("[ID0");
        Serial.print(SINPUTS);
        Serial.print("]");
        Serial.print(" SINPUTS = ");
        Serial.println(flag);
      #endif
      #ifdef DOUTPUTS
        flag = bitRead(this->features, DOUTPUTS);
        Serial.print("[ID0");
        Serial.print(OUTPUTS);
        Serial.print("]");
        Serial.print(" OUTPUTS = ");
        Serial.println(flag);
      #endif
      #ifdef PWMOUTPUTS
        flag = bitRead(this->features, PWMOUTPUTS);
        Serial.print("[ID0");
        Serial.print(PWMOUTPUTS);
        Serial.print("]");
        Serial.print(" PWMOUTPUTS = ");
        Serial.println(flag);
      #endif
      #ifdef AINPUTS
        flag = bitRead(this->features, AINPUTS);
        Serial.print("[ID0");
        Serial.print(AINPUTS);
        Serial.print("]");
        Serial.print(" AINPUTS = ");
        Serial.println(flag);
      #endif
      #ifdef DALLAS_TEMP_SENSOR
        flag = bitRead(this->features, DALLAS_TEMP_SENSOR);
        Serial.print("[ID0");
        Serial.print(DALLAS_TEMP_SENSOR);
        Serial.print("]");
        Serial.print(" DALLAS_TEMP_SENSOR = ");
        Serial.println(flag);
      #endif
      #ifdef LPOTIS
        flag = bitRead(this->features, LPOTIS);
        Serial.print("[ID0");
        Serial.print(LPOTIS);
        Serial.print("]");
        Serial.print(" LPOTIS = ");
        Serial.println(flag);
      #endif
      #ifdef BINSEL
        flag = bitRead(this->features, BINSEL);
        Serial.print("[ID0");
        Serial.print(BINSEL);
        Serial.print("]");
        Serial.print(" BINSEL = ");
        Serial.println(flag);
      #endif
      #ifdef QUADENC
        flag = bitRead(this->features, QUADENC);
        Serial.print("[ID0");
        Serial.print(QUADENC);
        Serial.print("]");
        Serial.print(" QUADENC = ");
        Serial.println(flag);
      #endif
      #ifdef JOYSTICK
        flag = bitRead(this->features, JOYSTICK);
        Serial.print("[ID");
        Serial.print(JOYSTICK);
        Serial.print("]");
        Serial.print(" JOYSTICK = ");
        Serial.println(flag);
      #endif
      #ifdef STATUSLED
        flag = bitRead(this->features, STATUSLED);
        Serial.print("[ID");
        Serial.print(STATUSLED);
        Serial.print("]");
        Serial.print(" STATUSLED = ");
        Serial.println(flag);
      #endif
      #ifdef DLED
        flag = bitRead(this->features, DLED);
        Serial.print("[ID");
        Serial.print(DLED);
        Serial.print("]");
        Serial.print(" DLED = ");
        Serial.println(flag);
      #endif
      #ifdef KEYPAD
        flag = bitRead(this->features, KEYPAD);
        Serial.print("[ID");
        Serial.print(KEYPAD);
        Serial.print("]");
        Serial.print(" KEYPAD = ");
        Serial.println(flag);
      #endif


      Serial.println("------------------- End Feature Map Decode Dump ------------------- ");
    #endif
  }
};
#endif //#define FEATUREMAP_H_

