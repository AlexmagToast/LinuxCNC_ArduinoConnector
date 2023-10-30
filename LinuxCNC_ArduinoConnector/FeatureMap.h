#ifndef FEATUREMAP_H_
#define FEATUREMAP_H_
uint64_t featureMap;

/**
 * Sets featureMap with currently enabled and disabled features.
 *
 * The featureMap is to enable the publication of which features have been enabled/disabled to the linuxcnc/python host.  This
 * can allow the Python/LinuxCNC side to output helpful information during debugging, e.g., "Error. X-command failed. The Y feature is disabled on Arduino"
 * The featureMap may also be useful in future features, such as the ability to display what features are enabled via the UI of LinuxCNC.  Why would this be
 * Another use case for debugging is during the initial testing using the halrun command.  If the user enters a command for a disabled feature,
 * the python side can catch the attempt before passing the command to the Arduino and instruct the user to enable the feature on the Arduino to use the feature.
 * useful one may ask?  The answer is that over time new features will be added, and it will be difficult to remember if your Arduino has that version which
 * X feature or not.  
 */

void initFeatureMap()
{
  featureMap = 0;
  #ifdef DEBUG
    bitSet(featureMap, DEBUG);
  #endif
  #ifdef INPUTS
    bitSet(featureMap, INPUTS);
  #endif
  #ifdef SINPUTS
    bitSet(featureMap, SINPUTS);
  #endif
  #ifdef OUTPUTS
    bitSet(featureMap, OUTPUTS);
  #endif
  #ifdef PWMOUTPUTS
    bitSet(featureMap, PWMOUTPUTS);
  #endif
  #ifdef AINPUTS
    bitSet(featureMap, AINPUTS);
  #endif
  #ifdef DALLAS_TEMP_SENSOR
    bitSet(featureMap, DALLAS_TEMP_SENSOR);
  #endif
  #ifdef LPOTIS
    bitSet(featureMap, LPOTIS);
  #endif
  #ifdef BINSEL
    bitSet(featureMap, BINSEL);
  #endif
  #ifdef QUADENC
    bitSet(featureMap, QUADENC);
  #endif
  #ifdef JOYSTICK
    bitSet(featureMap, JOYSTICK);
  #endif
  #ifdef STATUSLED
    bitSet(featureMap, STATUSLED);
  #endif
  #ifdef DLED
    bitSet(featureMap, DLED);
  #endif
  #ifdef KEYPAD
    bitSet(featureMap, KEYPAD);
  #endif
  #ifdef SERIAL_TO_LINUXCNC
    bitSet(featureMap, SERIAL_TO_LINUXCNC);
  #endif
  #ifdef ETHERNET_TO_LINUXCNC
    bitSet(featureMap, ETHERNET_TO_LINUXCNC);
  #endif
  #ifdef WIFI_TO_LINUXCNC
    bitSet(featureMap, WIFI_TO_LINUXCNC);
  #endif
}
/**
 * GetFeatureStatus returns 1 when specified feature is enabled, 0 otherwise
 *
 */

int GetFeatureStatus(int f)
{
  return bitRead(featureMap, f);
}

void DumpFeatureMapToSerial()
{
  Serial.print("Feature Map = 0x");
  Serial.println(featureMap, HEX);
  int flag = 0;
  #ifdef DEBUG
    Serial.println("------------------- Feature Map Decode Dump ------------------- ");
    // Debug is obviously defined if we got here, but use the map for consistency.
    flag = bitRead(featureMap, DEBUG);
    Serial.print("[ID0");
    Serial.print(DEBUG);
    Serial.print("]");
    Serial.print(" DEBUG = ");
    Serial.println(flag);
    #ifdef INPUTS
      flag = bitRead(featureMap, INPUTS);
      Serial.print("[ID0");
      Serial.print(INPUTS);
      Serial.print("]");
      Serial.print(" INPUTS = ");
      Serial.println(flag);
    #endif
    #ifdef SINPUTS
      flag = bitRead(featureMap, SINPUTS);
      Serial.print("[ID0");
      Serial.print(SINPUTS);
      Serial.print("]");
      Serial.print(" SINPUTS = ");
      Serial.println(flag);
    #endif
    #ifdef OUTPUTS
      flag = bitRead(featureMap, OUTPUTS);
      Serial.print("[ID0");
      Serial.print(OUTPUTS);
      Serial.print("]");
      Serial.print(" OUTPUTS = ");
      Serial.println(flag);
    #endif
    #ifdef PWMOUTPUTS
      flag = bitRead(featureMap, PWMOUTPUTS);
      Serial.print("[ID0");
      Serial.print(PWMOUTPUTS);
      Serial.print("]");
      Serial.print(" PWMOUTPUTS = ");
      Serial.println(flag);
    #endif
    #ifdef AINPUTS
      flag = bitRead(featureMap, AINPUTS);
      Serial.print("[ID0");
      Serial.print(AINPUTS);
      Serial.print("]");
      Serial.print(" AINPUTS = ");
      Serial.println(flag);
    #endif
    #ifdef DALLAS_TEMP_SENSOR
      flag = bitRead(featureMap, DALLAS_TEMP_SENSOR);
      Serial.print("[ID0");
      Serial.print(DALLAS_TEMP_SENSOR);
      Serial.print("]");
      Serial.print(" DALLAS_TEMP_SENSOR = ");
      Serial.println(flag);
    #endif
    #ifdef LPOTIS
      flag = bitRead(featureMap, LPOTIS);
      Serial.print("[ID0");
      Serial.print(LPOTIS);
      Serial.print("]");
      Serial.print(" LPOTIS = ");
      Serial.println(flag);
    #endif
    #ifdef BINSEL
      flag = bitRead(featureMap, BINSEL);
      Serial.print("[ID0");
      Serial.print(BINSEL);
      Serial.print("]");
      Serial.print(" BINSEL = ");
      Serial.println(flag);
    #endif
    #ifdef QUADENC
      flag = bitRead(featureMap, QUADENC);
      Serial.print("[ID0");
      Serial.print(QUADENC);
      Serial.print("]");
      Serial.print(" QUADENC = ");
      Serial.println(flag);
    #endif
    #ifdef JOYSTICK
      flag = bitRead(featureMap, JOYSTICK);
      Serial.print("[ID");
      Serial.print(JOYSTICK);
      Serial.print("]");
      Serial.print(" JOYSTICK = ");
      Serial.println(flag);
    #endif
    #ifdef STATUSLED
      flag = bitRead(featureMap, STATUSLED);
      Serial.print("[ID");
      Serial.print(STATUSLED);
      Serial.print("]");
      Serial.print(" STATUSLED = ");
      Serial.println(flag);
    #endif
    #ifdef DLED
      flag = bitRead(featureMap, DLED);
      Serial.print("[ID");
      Serial.print(DLED);
      Serial.print("]");
      Serial.print(" DLED = ");
      Serial.println(flag);
    #endif
    #ifdef KEYPAD
      flag = bitRead(featureMap, KEYPAD);
      Serial.print("[ID");
      Serial.print(KEYPAD);
      Serial.print("]");
      Serial.print(" KEYPAD = ");
      Serial.println(flag);
    #endif
    #ifdef SERIAL_TO_LINUXCNC
      flag = bitRead(featureMap, SERIAL_TO_LINUXCNC);
      Serial.print("[ID");
      Serial.print(SERIAL_TO_LINUXCNC);
      Serial.print("]");
      Serial.print(" SERIAL_TO_LINUXCNC = ");
      Serial.println(flag);
    #endif
    #ifdef ETHERNET_TO_LINUXCNC
      flag = bitRead(featureMap, ETHERNET_TO_LINUXCNC);
      Serial.print("[ID");
      Serial.print(ETHERNET_TO_LINUXCNC);
      Serial.print("]");
      Serial.print(" ETHERNET_TO_LINUXCNC = ");
      Serial.println(flag);
    #endif
    #ifdef WIFI_TO_LINUXCNC
      flag = bitRead(featureMap, WIFI_TO_LINUXCNC);
      Serial.print("[ID");
      Serial.print(WIFI_TO_LINUXCNC);
      Serial.print("]");
      Serial.print(" WIFI_TO_LINUXCNC = ");
      Serial.println(flag);
    #endif
    Serial.println("------------------- End Feature Map Decode Dump ------------------- ");
  #endif

}
#endif //#define FEATUREMAP_H_

