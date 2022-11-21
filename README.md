# LinuxCNC_ArduinoConnector

+++this is still in early development and is untested, i don't recommend using it in an actual machine+++
For my CNC Machine i wanted to include more IO's than my Mesa card was offering. This Projekt enables to connect Arduino to LinuxCNC to include as many IO's as you wish.


Arduino is handling all the IO's and sends all changes over Serial. It is decoded and integrated in hal with Python-serial.

This protocol is slow compared to other solutions, but easily adaptable and expandable through the Arduino platform.

# Features i have included: 
(as many Pins as your ARduino provides)
- analog Inputs
- latching Potentiometers
- 1 absolute encoder input
- digital Inputs
- digital Outputs
Right now i am also working on
- virtual Pins (multiplexed LED's or WS2812)


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
arduino.digital-in-32
