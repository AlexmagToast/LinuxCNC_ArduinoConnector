
# You are in the Alpha Branch of V2 of Arduino-connector, it is still in early development and not working! 

## LinuxCNC_ArduinoConnector V2

<img src="/ArduinoChip.svg" width="250" align="right">

By Alexander Richter,  
please consider supporting me on Patreon:  
https://www.patreon.com/theartoftinkering  

Website: https://theartoftinkering.com  
Youtube: https://youtube.com/@theartoftinkering

and  
Ken Thompson (not THAT Ken Thompson)  
https://github.com/KennethThompson

Copyright (c) 2023 Alexander Richter & Ken Thompson



# What's new? 
With this new Version the configuration and communcation between Arduino and the receiving Python script is completely reworked.

For the User these are the Main new Features: 
- all of the configuration is done in a single yaml script. 
- support for multiple Microcontrollers simultaneously
- support for Ethernet and Wifi connections (in the future)
- more versatile communication protokol

# How does it work? 
V2 is inspired by ESPHome project and Klipper Firmware.
The Microcontroller (MCU) is flashed with a standard firmware and is confectioned by the python script at startup.
The main goal is to improve on the weaknesses of V1, where People often got confused by the neccessary configurations both in Arduino and Python file. 
Also it will be much easier to share and compare configurations between different setups.

# When can I use it? 
Currently we work on the base functionality and only most basic IO's work, like reading and writing Digital and Analog Pins. 
We are however greatly motivated and spend (to the chagrin of our wives) most of our freetime on the development of this project. 

# Can I help? 
Of course! Even if you are not a Programmer we invite you to discuss with us new feature Ideas and improvements. We try to think of every possible way of how people could want to use things, but we are always happy for new Input!
You can do so by creating an Issue here on Github or writing Alex an Email: info@theartoftinkering.com. 

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
