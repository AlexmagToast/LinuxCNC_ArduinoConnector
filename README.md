
# LinuxCNC_ArduinoConnector
By Alexander Richter, info@theartoftinkering.com 2022
please consider supporting me on Patreon: https://www.patreon.com/theartoftinkering

For my CNC Machine i wanted to include more IO's than my Mesa card was offering. This Projekt enables to connect Arduino to LinuxCNC to include as many IO's as you wish.

This Software is used as IO Expansion for LinuxCNC. Here i am using a Mega 2560.
# +++this is still in early development and is untested, i don't recommend using it in an actual machine+++
It is NOT intended for timing and security relevant IO's. Don't use it for Emergency Stops or Endstop switches!

You can create as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
You can also generate "virtual Pins" by using latching Potentiometers, which are connected to one analog Pin, but are read in Hal as individual Pins.

Currently the Software provides: 
- analog Inputs
- latching Potentiometers
- 1 absolute encoder input
- digital Inputs
- digital Outputs

The Send and receive Protocol is <Signal><PinNumber>:<Pin State>
To begin Transmitting Ready is send out and expects to receive E: to establish connection. Afterwards Data is exchanged.
Data is only send everythime it changes once.

Inputs                  = 'I' -write only  -Pin State: 0,1
Outputs                 = 'O' -read only   -Pin State: 0,1
PWM Outputs             = 'P' -read only   -Pin State: 0-255
Analog Inputs           = 'A' -write only  -Pin State: 0-1024
Latching Potentiometers = 'L' -write only  -Pin State: 0-max Position
Absolute Encoder input  = 'K' -write only  -Pin State: 0-32

Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal. If connection is lost the arduino begins flashing an LED to alarm the User. 

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


# Installation
- configure the Firmware file to your demands and flash it to your arduino
- connect the arduino to your LinuxCNC Computer via USB
- install python-serial
- open arduino.py and configure it to match your arduino settings. 
- also check if the Serial adress is correct for your Arduino
- move arduino.py to  /usr/bin and make it executable with chmod +x

- add to your hal file: loadusr arduino


You can now use arduino pins in your hal file. 
Pin Names are named arduino.[Pin Type]-[Pin Number]. Example:
arduino.digital-in-32 for Pin 32 on an Arduino Mega2560
