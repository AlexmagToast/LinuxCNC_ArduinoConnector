#pragma once
#ifndef ETHERNETFUNCS_H_
#define ETHERNETFUNCS_H_
  #if DHCP == 1
    uint8_t do_dhcp_maint()
    {
        switch (Ethernet.maintain()) {

        case 1:

          //renewed fail
          #ifdef DEBUG
            Serial.println("Error: DCHP renewed fail");
          #endif

          return -1;

        case 2:

          #ifdef DEBUG
            Serial.println("Renewed success");
            Serial.print("My IP address: ");
            Serial.println(Ethernet.localIP());
          #endif
          return 1;

        case 3:
          #ifdef DEBUG
            Serial.println("Error: rebind fail");
          #endif
          return -2;

        case 4:
          #ifdef DEBUG
            Serial.println("Rebind success");
            Serial.print("My IP address: ");
            Serial.println(Ethernet.localIP());
          #endif
          return 1;

        default:

          //nothing happened
          return 1;
      }
    }
  #endif
#endif
