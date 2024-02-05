#pragma once
#ifndef FEATUREMAP_H_
#define FEATUREMAP_H_

#ifdef ENABLE_FEATUREMAP
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
    SERIAL_DEV.print("DEBUG: Feature Map = 0x");
    SERIAL_DEV.println((uint32_t)this->features, HEX);
    int flag = 0;
    #ifdef DEBUG
      SERIAL_DEV.println("------------------- Feature Map Decode Dump ------------------- ");
      // Debug is obviously defined if we got here, but use the map for consistency.
      flag = bitRead(this->features, DEBUG);
      SERIAL_DEV.print("[ID0");
      SERIAL_DEV.print(DEBUG);
      SERIAL_DEV.print("]");
      SERIAL_DEV.print(" DEBUG = ");
      SERIAL_DEV.println(flag);
      #ifdef DINPUTS
        flag = bitRead(this->features, DINPUTS);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(DINPUTS);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" INPUTS = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef SINPUTS
        flag = bitRead(this->features, SINPUTS);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(SINPUTS);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" SINPUTS = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef DOUTPUTS
        flag = bitRead(this->features, DOUTPUTS);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(DOUTPUTS);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" OUTPUTS = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef PWMOUTPUTS
        flag = bitRead(this->features, PWMOUTPUTS);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(PWMOUTPUTS);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" PWMOUTPUTS = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef AINPUTS
        flag = bitRead(this->features, AINPUTS);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(AINPUTS);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" AINPUTS = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef DALLAS_TEMP_SENSOR
        flag = bitRead(this->features, DALLAS_TEMP_SENSOR);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(DALLAS_TEMP_SENSOR);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" DALLAS_TEMP_SENSOR = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef LPOTIS
        flag = bitRead(this->features, LPOTIS);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(LPOTIS);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" LPOTIS = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef BINSEL
        flag = bitRead(this->features, BINSEL);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(BINSEL);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" BINSEL = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef QUADENC
        flag = bitRead(this->features, QUADENC);
        SERIAL_DEV.print("[ID0");
        SERIAL_DEV.print(QUADENC);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" QUADENC = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef JOYSTICK
        flag = bitRead(this->features, JOYSTICK);
        SERIAL_DEV.print("[ID");
        SERIAL_DEV.print(JOYSTICK);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" JOYSTICK = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef STATUSLED
        flag = bitRead(this->features, STATUSLED);
        SERIAL_DEV.print("[ID");
        SERIAL_DEV.print(STATUSLED);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" STATUSLED = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef DLED
        flag = bitRead(this->features, DLED);
        SERIAL_DEV.print("[ID");
        SERIAL_DEV.print(DLED);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" DLED = ");
        SERIAL_DEV.println(flag);
      #endif
      #ifdef KEYPAD
        flag = bitRead(this->features, KEYPAD);
        SERIAL_DEV.print("[ID");
        SERIAL_DEV.print(KEYPAD);
        SERIAL_DEV.print("]");
        SERIAL_DEV.print(" KEYPAD = ");
        SERIAL_DEV.println(flag);
      #endif


      SERIAL_DEV.println("------------------- End Feature Map Decode Dump ------------------- ");
    #endif
  }
};
#endif
#endif //#define FEATUREMAP_H_

