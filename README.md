
# LinuxCNC_ArduinoConnector
By Alexander Richter, info@theartoftinkering.com 2022
please consider supporting me on Patreon: https://www.patreon.com/theartoftinkering

For my CNC Machine i wanted to include more IO's than my Mesa card was offering. This Projekt enables to connect Arduino to LinuxCNC to include as many IO's as you wish.

This Software is used as IO Expansion for LinuxCNC. Here i am using a Mega 2560.

+++It is NOT intended for timing and security relevant IO's. Don't use it for Emergency Stops or Endstop switches!+++

You can create as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
You can also generate "virtual Pins" by using latching Potentiometers, which are connected to one analog Pin, but are read in Hal as individual Pins.
It also supports Digital LEDs such as WS2812 or PL9823. This way you can have as many LEDs as you want and you can also define the color of them with just one Pin.
In LinuxCNC each LED is listed as one Output that can be set to HIGH and LOW. For both States you can define a color per LED. 
This way, you can make them turn on or shut off or have them Change color, from Green to Red for example. 


Currently the Software provides: 
- analog Inputs
- digital Inputs
- digital Outputs
- support of Digital RGB LEDs like WS2812 or PL9823
- latching Potentiometers
- 1 absolute encoder input


# compatiblity
This software works with LinuxCNC 2.8, 2.9 and 2.10. 
For 2.8 however you have to change #!/usr/bin/python3.9 in the first line of arduino.py to #!/usr/bin/python2.7. 


# Installation
- configure the Firmware file to your demands and flash it to your arduino
- connect the arduino to your LinuxCNC Computer via USB
- install python-serial
    sudo apt-get install python-serial
- edit arduino.py to match your arduino settings.
- also check if the Serial adress is correct for your Arduino. I found it easyest to run "sudo dmesg | grep tty" in Terminal. 
- move arduino.py to  /usr/bin and make it executable with chmod +x
    sudo chmod +x arduino.py
    sudo cp arduino.py /usr/bin/arduino

- add to your hal file: loadusr arduino

# Testing
To test your Setup, you can run "halrun" in Terminal.
Then you will see halcmd:

Enter "loadusr arduino" and then "show pin"

All the Arduino generated Pins should now be listed and the State they are in. 
You can click buttons now and if you run show pin again the state should've changed. 

you can also set Pins that are listed in DIR as IN. 
Enter "setp arduino.DLED.1 TRUE" for example. This will set said Pin to HIGH or in this case, if you have it set up turn the 2. Digital LED on.


You can now use arduino pins in your hal file. 
Pin Names are named arduino.[Pin Type]-[Pin Number]. Example:
arduino.digital-in-32 for Pin 32 on an Arduino Mega2560



# Serial communication over USB
The Send and receive Protocol is <Signal><PinNumber>:<Pin State>
To begin Transmitting Ready is send out and expects to receive E: to establish connection. Afterwards Data is exchanged.
Data is only send everythime it changes once.

  Inputs & Toggle Inputs  = 'I' -write only  -Pin State: 0,1
  Outputs                 = 'O' -read only   -Pin State: 0,1
  PWM Outputs             = 'P' -read only   -Pin State: 0-255
  Digital LED Outputs     = 'D' -read only   -Pin State: 0,1
  Analog Inputs           = 'A' -write only  -Pin State: 0-1024
  Latching Potentiometers = 'L' -write only  -Pin State: 0-max Position
  Absolute Encoder input  = 'K' -write only  -Pin State: 0-32

Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal. If connection is lost the arduino begins flashing an LED to alarm the User. 

# License
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA