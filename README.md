
# LinuxCNC_ArduinoConnector

<img src="/ArduinoChip.svg" width="250" align="right">

By Alexander Richter, info@theartoftinkering.com 2022  
please consider supporting me on Patreon: https://www.patreon.com/theartoftinkering

This Projekt enables you to connect an Arduino to LinuxCNC and provides as many IO's as you could ever wish for.
This Software is used as IO Expansion for LinuxCNC.

## It is NOT intended for timing and security relevant IO's. Don't use it for Emergency Stops or Endstop switches! ##


You can create as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
It also supports Digital LEDs such as WS2812 or PL9823. This way you can have as many LEDs as you want and you can also define the color of them with just one Pin.
In LinuxCNC each LED is listed as one Output that can be set to HIGH and LOW. For both States you can define a color per LED. 
This way, you can make them turn on or shut off or have them Change color, from Green to Red for example. 


Currently the Software Supports: 
- Analog Inputs
- Digital Inputs
- Digital Outputs
- PWM Outputs
- Digital RGB LEDs like WS2812 or PL9823
- latching Potentiometers / Selector Switches 
- 1 absolute encoder Selector Switch

TODO  
- Matrix Keyboard Support

Should this be supported?  
- RC Servo  Support

# Compatiblity
This software works with LinuxCNC 2.8, 2.9 and 2.10. 
For 2.8 however you have to change #!/usr/bin/python3.9 in the first line of arduino.py to #!/usr/bin/python2.7. 

# Configuration
To Install LinuxCNC_ArduinoConnector.ino on your Arduino first work through the settings in the beginning of the file.
The Settings are commented in the file.

To test you Arduino you can connect to it after flashing with the Arduino IDE. Set your Baudrate to 115200. 
In the Beginning the ARduino will Spam ```E0:0``` to the console. This is used to establish connection. 
Just return ```E0:0``` to it. You can now communicate with the Arduino. Further info is in the Chapter [Serial Communication](#serial-communication-over-usb)

# Installation
1. configure the Firmware file to your demands and flash it to your arduino
2. connect the arduino to your LinuxCNC Computer via USB
3. install python-serial  
    ```sudo apt-get install python-serial```  
4. edit arduino.py to match your arduino settings.
5. also check if the Serial adress is correct for your Arduino. I found it easyest to run ```sudo dmesg | grep tty``` in Terminal. 
6. move arduino.py to  /usr/bin and make it executable with chmod +x  
    ```sudo chmod +x arduino.py  ```
    ```sudo cp arduino.py /usr/bin/arduino  ```

7. add to your hal file: ```loadusr arduino```

# Testing
To test your Setup, you can run ```halrun``` in Terminal.
Then you will see halcmd:

Enter ```loadusr arduino``` and then ```show pin```

All the Arduino generated Pins should now be listed and the State they are in. 
You can click buttons now and if you run show pin again the state should've changed. 

you can also set Pins that are listed in DIR as IN. 
Enter "setp arduino.DLED.1 TRUE" for example. This will set said Pin to HIGH or in this case, if you have it set up turn the 2. Digital LED on.


You can now use arduino pins in your hal file. 
Pin Names are named arduino.[Pin Type]-[Pin Number]. Example:
arduino.digital-in-32 for Pin 32 on an Arduino Mega2560

# Analog Inputs
These are used for example to connect Potentiometers. You can add as many as your Arduino has Analog Pins.
The Software has a smoothing parameter, which will remove jitter.

# Digital Inputs
Digital Inputs use internal Pullup Resistors. So to trigger them you just short the Pin to Ground. There are two Digital Input Types implemented.
Don't use them for Timing or Safety relevant Stuff like Endstops or Emergency Switches.
1. INPUTS uses the spezified Pins as Inputs. The Value is parsed to LinuxCNC dirketly. There is also a inverted Parameter per Pin.
2. Trigger INPUTS (SINPUTS) are handled like INPUTS, but simulate Latching Buttons. So when you press once, the Pin goes HIGH and stays HIGH, until you press the Button again. 
# Digital Outputs
Digital Outputs drive the spezified Arduinos IO's as Output Pins. You can use it however you want, but don't use it for Timing or Safety relevant Stuff like Stepper Motors.
# support of Digital RGB LEDs like WS2812 or PL9823
Digital LED's do skale very easily, you only need one Pin to drive an infinite amount of them.
To make implementation in LinuxCNC easy you can set predefined LED RGB colors. 
You can set a color for "on" and "off" State for each LED. 
LED colors are set with values 0-255 for Red, Green and Blue. 0 beeing off and 255 beeing full on.
Here are two examples:

1. This LED should be glowing Red when "on" and just turn off when "off". 
The Setting in Arduino is: 
  ```int DledOnColors[DLEDcount][3] = {{255,0,0}};```

  ```int DledOffColors[DLEDcount][3] = {{0,0,0}};```


2. This LED should glow Green when "on" and Red when "off". 
  ```int DledOnColors[DLEDcount][3] = {{0,255,0}};```  

  ```int DledOffColors[DLEDcount][3] = {{255,0,0}};```  
Easy right?                 
# Latching Potentiometers / Selector Switches
This is a special Feature for rotary Selector Switches. Instead of loosing one Pin per Selection you can turn your Switch in a Potentiometer by soldering 10K resistors between the Pins and connecting the Selector Pin to an Analog Input. 
The Software will divide the Measured Value and create Hal Pins from it. This way you can have Selector Switches with many positions while only needing one Pin for it.

# 1 absolute encoder input
Some rotary Selector Switches work with Binary Encoded Positions. The Software Supports Encoders with 32 Positions. (this could be more if requested)
For each Bit one Pin is needed. So for all 32 Positions 5 Pins are needed = 1,2,4,8,16 

# Status LED
The Arduino only works, if LinuxCNC is running and an USB Connection is established. 
To give optical Feedback of the State of the connection a Status LED setting is provided. 
This can be either an LED connected to an Output Pin or you can select one LED in your Digital LED Chain.
- It will flash slowly after startup, when it waits for communication setup by LinuxCNC.
- It will glow constantly when everything works.
- it Will flash short when Connection was lost.

# Serial communication over USB
The Send and receive Protocol is <Signal><PinNumber>:<Pin State>
After Bootup the Arduino will continuously print E0:0 to Serial. Once the Host Python skript runs and connects, it will answer and hence the Arduino knows, the connection is established. 

For testing you can still connect to it with your Serial terminal. Send ```E0:0```, afterwards it will listen to your commands and post Input Changes.

Data is always only send once, everytime it changes.

| Signal                  | Header        |direction     |Values        |
| -------------           | ------------- |------------- |------------- |
| Inputs & Toggle Inputs  | I             | write only   |0,1           |
| Outputs                 | O             | read only    |0,1           |
| PWM Outputs             | P             | read only    |0-255         |
| Digital LED Outputs     | D             | read only    |0,1           |
| Analog Inputs           | A             | write only   |0-1024        |
| Latching Potentiometers | L             | write only   |0-max Position|
| Absolute Encoder input  | K             | write only   |0-32          |
| Connection established  | E             | read/ write  |0:0           |


Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal. If it is not received in Time, the connection is lost and the arduino begins flashing an LED to alarm the User. It will however work the same and try to send it's Data to the Host.

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
