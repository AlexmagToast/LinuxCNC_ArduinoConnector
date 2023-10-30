#ifndef ETHERNETFUNCS_H_
#define ETHERNETFUNCS_H_
  #if DHCP == 1
    #include "EthernetFuncs.h"
    //#include <Ethernet.h>
    //#include <Serial.h>
    void do_dhcp_maint()
    {
        switch (Ethernet.maintain()) {

        case 1:

          //renewed fail

          Serial.println("Error: renewed fail");

          break;

        case 2:

          //renewed success

          Serial.println("Renewed success");

          //print your local IP address:

          Serial.print("My IP address: ");

          Serial.println(Ethernet.localIP());

          break;

        case 3:

          //rebind fail

          Serial.println("Error: rebind fail");

          break;

        case 4:

          //rebind success

          Serial.println("Rebind success");

          //print your local IP address:

          Serial.print("My IP address: ");

          Serial.println(Ethernet.localIP());

          break;

        default:

          //nothing happened

          break;

      }
    }
  #endif
#endif
