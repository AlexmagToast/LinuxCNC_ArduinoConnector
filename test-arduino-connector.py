#!/usr/bin/python3.9
from queue import Empty
import traceback
from numpy import block

import serial, time, os

from linuxcnc_arduinoconnector.ArduinoConnector import ConnectionType, SerialConnetion, UDPConnection

#connection = '/dev/ttyACM0' 	#this is the port your Arduino is connected to. You can check with ""sudo dmesg | grep tty"" in Terminal
#connection = '/dev/tty.usbmodemF412FA68D6802'
# Map of board index IDs and a human-readable alias
# This map gets used by the connection manager to track the connection state of each mapped arduino
arduinoMap = { 1:'myArduinoUno'}

# Set how many Inputs you have programmed in Arduino and which pins are Inputs, Set Inputs = 0 to disable
Inputs = 2 
InPinmap = [8,9] #Which Pins are Inputs?

# Set how many Toggled ("sticky") Inputs you have programmed in Arduino and which pins are Toggled Inputs , Set SInputs = 0 to disable
SInputs = 1
sInPinmap = [10] #Which Pins are SInputs?


# Set how many Outputs you have programmed in Arduino and which pins are Outputs, Set Outputs = 0 to disable
Outputs = 2				#9 Outputs, Set Outputs = 0 to disable
OutPinmap = [11,12]	#Which Pins are Outputs?

# Set how many PWM Outputs you have programmed in Arduino and which pins are PWM Outputs, you can set as many as your Arduino has PWM pins. List the connected pins below.
PwmOutputs = 0			#number of PwmOutputs, Set PwmOutputs = 0 to disable 
PwmOutPinmap = [11,12]	#PwmPutput connected to Pin 11 & 12

# Set how many Analog Inputs you have programmed in Arduino and which pins are Analog Inputs, you can set as many as your Arduino has Analog pins. List the connected pins below.
AInputs = 0				#number of AInputs, Set AInputs = 0 to disable 
AInPinmap = [1]			#Potentiometer connected to Pin 1 (A0)



# Set how many Latching Analog Inputs you have programmed in Arduino and how many latches there are, you can set as many as your Arduino has Analog pins. List the connected pins below.
LPoti = 0				#number of LPotis, Set LPoti = 0 to disable 

LPotiLatches = [[1,9],	#Poti is connected to Pin 1 (A1) and has 9 positions
				[2,4]]	#Poti is connected to Pin 2 (A2) and has 4 positions

SetLPotiValue = [1,2] 	#0 OFF - creates Pin for each Position
					  	#1 S32 - Whole Number between -2147483648 to 2147483647
						#2 FLOAT - 32 bit floating point value

LPotiValues = [[40, 50,60,70,80,90,100,110,120],
			   [0.001,0.01,0.1,1]]



# Set if you have an binary encoded Selector Switch and how many positions it has (only one supported, as i don't think they are very common and propably nobody uses these anyway)
# Set BinSelKnob = 0 to disable
BinSelKnob = 0 	#1 enable
BinSelKnobPos = 32

#Do you want the Binary Encoded Selector Switches to control override Settings in LinuxCNC? This function lets you define values for each Position. 
SetBinSelKnobValue = [[0]] #0 = disable 1= enable
BinSelKnobvalues = [[180,190,200,0,0,0,0,0,0,0,0,0,0,0,0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170]]

#Enable Quadrature Encoders
QuadEncs = 0
QuadEncSig = [2,2] 
#1 = send up or down signal (typical use for selecting modes in hal)
#2 = send position signal (typical use for MPG wheel)


#Enable Joystick support. 
# Intended for use as MPG. useing the Joystick will update a counter, which can be used as Jog Input. 
# Moving the Joystick will either increase or decrease the counter. Modify Jog-scale in hal to increase or decrease speed.
JoySticks = 0	#number of installed Joysticks
JoyStickPins = [0,1] #Pins the Joysticks are connected to. 
	#in this example X&Y Pins of the Joystick are connected to Pin A0& A1. 




# Set how many Digital LED's you have connected. 
DLEDcount = 0 


# Support For Matrix Keypads. This requires you to install and test "xdotool". 
# You can install it by typing "sudo apt install xdotool" in your console. After installing you can test your setup by entering: " xdotool type 'Hello World' " in Terminal. 
# It should enter Hello World. 
# If it doesn't, something is not working and this program will not work either. Please get xdotool working first. 
#
# Assign Values to each Key in the following Settings.
# These Inputs are handled differently from everything else, because thy are send to the Host instead and emulate actual Keyboard input.
# You can specify special Charakters however, which will be handled as Inputs in LinuxCNC. Define those in the LCNC Array below.


Keypad = 0  # Set to 1 to Activate
LinuxKeyboardInput = 0  # set to 1 to Activate direct Keyboard integration to Linux.


Columns = 4
Rows = 4
Chars = [      #here you must define as many characters as your Keypad has keys. calculate columns * rows . for example 4 *4 = 16. You can write it down like in the example for ease of readability.
 "1", "2", "3", "A",
 "4", "5", "6", "B",
 "7", "8", "9", "C",
 "Yay", "0", "#", "D"
] 

# These are Settings to connect Keystrokes to Linux, you can ignore them if you only use them as LinuxCNC Inputs.

Destination = [    #define, which Key should be inserted in LinuxCNC as Input or as Keystroke in Linux. 
          #you can ignore it if you want to use all Keys as LinuxCNC Inputs.
          # 0 = LinuxCNC 
          # 1 = press Key in Linux
          # 2 = write Text in Linux
  1, 1, 1, 0,
  1, 1, 1, 0,
  1, 1, 1, 0, 
  2, 1, 0, 0
]
# Background Info:
# The Key press is received as M Number of Key:HIGH/LOW. M2:1 would represent Key 2 beeing Pressed. M2:0 represents letting go of the key.
# Key Numbering is calculated in an 2D Matrix. for a 4x4 Keypad the numbering of the Keys will be like this:
#
#  0,  1,  2,  3,
#  4,  5,  6,  7,
#  8,  9,  10,  11,
#  12,  13,  14,  15
#

# this is an experimental feature, meant to support MatrixKeyboards with integrated LEDs in each Key but should work with any other LED Matrix too.
# It creates Output Halpins that can be connected to signals in LinuxCNC
MultiplexLED = 0  # Set to 1 to Activate
LedVccPins = 3 
LedGndPins = 3



Debug = 0		#only works when this script is run from halrun in Terminal. "halrun","loadusr arduino" now Debug info will be displayed.

########  End of Config!  ########


# global Variables for State Saving

olddOutStates= [0]*Outputs
oldPwmOutStates=[0]*PwmOutputs
oldDLEDStates=[0]*DLEDcount
oldMledStates = [0]*LedVccPins*LedGndPins

if LinuxKeyboardInput:
	import subprocess

# Inputs and Toggled Inputs are handled the same. 
# For DAU compatiblity we set them up seperately. 
# Here we merge the arrays.

Inputs = Inputs+ SInputs
InPinmap += sInPinmap


# Storing Variables for counter timing Stuff
counter_last_update = {}
min_update_interval = 100
######## SetUp of HalPins ########


#setup Serial connection
#arduino = serial.Serial(connection, 115200, timeout=1, xonxoff=False, rtscts=False, dsrdtr=True)
######## GlobalVariables ########

event = time.time()
timeout = 9 #send something after max 9 seconds


######## Functions ########

def keepAlive(event):
	return event + timeout < time.time()

def readinput(input_str):
	for i in range(50):

		if input_str:
			string = input_str.decode()  # convert the byte string to a unicode string
			print (string)
			num = int(string) # convert the unicode string to an int
	return num


def extract_nbr(input_str):
	if input_str is None or input_str == '':
		return 0

	out_number = ''
	for i, ele in enumerate(input_str):
		if ele.isdigit() or (ele == '-' and i+1 < len(input_str) and input_str[i+1].isdigit()):
			out_number += ele
	return int(out_number)

def managageOutputs():
	for port in range(PwmOutputs):
		State = int(c["pwmout.{}".format(PwmOutPinmap[port])])
		if oldPwmOutStates[port] != State: 	#check if states have changed
			Sig = 'P'
			Pin = int(PwmOutPinmap[port])
			command = "{}{}:{}\n".format(Sig,Pin,State)
			arduino.write(command.encode())
			if (Debug):print ("Sending:{}".format(command.encode()))
			oldPwmOutStates[port]= State
			time.sleep(0.01)

	for port in range(Outputs):
		State = int(c["dout.{}".format(OutPinmap[port])])
		if olddOutStates[port] != State:	#check if states have changed
			Sig = 'O'
			Pin = int(OutPinmap[port])
			command = "{}{}:{}\n".format(Sig,Pin,State)
			arduino.write(command.encode())
			if (Debug):print ("Sending:{}".format(command.encode()))
			olddOutStates[port]= State
			time.sleep(0.01)
		
	for dled in range(DLEDcount):
		State = int(c["dled.{}".format(dled)])
		if oldDLEDStates[dled] != State: #check if states have changed
			Sig = 'D'
			Pin = dled
			command = "{}{}:{}\n".format(Sig,Pin,State)
			arduino.write(command.encode())
			if (Debug):print ("Sending:{}".format(command.encode()))
			oldDLEDStates[dled] = State
			time.sleep(0.01)
	if MultiplexLED > 0:
		for mled in range(LedVccPins*LedGndPins):
			State = int(c["mled.{}".format(mled)])
			if oldMledStates[mled] != State: #check if states have changed
				Sig = 'M'
				Pin = mled
				command = "{}{}:{}\n".format(Sig,Pin,State)
				arduino.write(command.encode())
				if (Debug):print ("Sending:{}".format(command.encode()))
				oldMledStates[mled] = State
				time.sleep(0.01)
    
#sc = SerialConnetion(myType=ConnectionType.SERIAL, dev='/dev/tty.usbmodem11201')
sc = UDPConnection(myType=ConnectionType.UDP, listenip='', listenport=54321)
sc.startRxTask()
    
while True:
	try:
		try:
			#if sc.getState(0) == ConnectionState.CONNECTED:
			#	command = "{}{}:{}\n".format('O', '4', '0')
			#	sc.sendCommand(command)
			cmd = sc.rxQueue.get(block=False, timeout=100)

			#if cmd != None:
			#	processCommand(cmd.payload)
		except Empty:
			time.sleep(.1)
			#if sc.getState(0) == ConnectionState.CONNECTED:
			#	time.sleep(.1)
			#	command = "{}{}:{}\n".format('O', '4', '1')
			#	sc.sendCommand(command)
			#	time.sleep(.1)
	except KeyboardInterrupt:
		sc.stopRxTask()
		sc = None
		raise SystemExit
	except Exception as ex:
		just_the_string = traceback.format_exc()
		if Debug:print(just_the_string)



'''

while True:
	try:
		data = arduino.readline().decode('utf-8')					#read Data received from Arduino and decode it
		if (Debug):print ("I received:{}".format(data))
		data = data.split(":",1)

		try:
			cmd = data[0][0]
			if cmd == "":
				if (Debug):print ("No Command!:{}".format(cmd))
			
			else:
				if not data[0][1]:
					io = 0
				else:
					io = extract_nbr(data[0])
				value = extract_nbr(data[1])
				#if value<0: value = 0
				if (Debug):print ("No Command!:{}.".format(cmd))

				if cmd == "I":
					firstcom = 1
					if value == 1:
						if c["din.{}-invert".format(io)] == 0:
							c["din.{}".format(io)] = 1
							if(Debug):print("din{}:{}".format(io,1))
						else: 
							c["din.{}".format(io)] = 0
							if(Debug):print("din{}:{}".format(io,0))
						
						
					if value == 0:
						if c["din.{}-invert".format(io)] == 0:
							c["din.{}".format(io)] = 0
							if(Debug):print("din{}:{}".format(io,0))
						else: 
							c["din.{}".format(io)] = 1
							if(Debug):print("din{}:{}".format(io,1))
					else:pass


				elif cmd == "A":
					firstcom = 1
					c["ain.{}".format(io)] = value
					if (Debug):print("ain.{}:{}".format(io,value))

				elif cmd == "L":
					firstcom = 1
					for Poti in range(LPoti):
						if LPotiLatches[Poti][0] == io and SetLPotiValue[Poti] == 0:
							for Pin in range(LPotiLatches[Poti][1]):
								if Pin == value:
									c["lpoti.{}.{}" .format(io,Pin)] = 1
									if(Debug):print("lpoti.{}.{} =1".format(io,Pin))
								else:
									c["lpoti.{}.{}" .format(io,Pin)] = 0
									if(Debug):print("lpoti.{}.{} =0".format(io,Pin))
						
						if LPotiLatches[Poti][0] == io and SetLPotiValue[Poti] >= 1:
							c["lpoti.{}.{}" .format(io,"out")] = LPotiValues[Poti][value]
							if(Debug):print("lpoti.{}.{} = 0".format("out",LPotiValues[Poti][value]))

				elif cmd == "K":
					firstcom = 1
					if SetBinSelKnobValue[0] == 0:
						for port in range(BinSelKnobPos):
							if port == value:
								c["binselknob.{}".format(port)] = 1
								if(Debug):print("binselknob.{}:{}".format(port,1))
							else:
								c["binselknob.{}".format(port)] = 0
								if(Debug):print("binselknob.{}:{}".format(port,0))
					else: 
						c["binselknob.{}.{}" .format(0,"out")] = BinSelKnobvalues[0][value]

				elif cmd == "M":
						firstcom = 1
						if value == 1:
							if Destination[io] == 1 and LinuxKeyboardInput == 1:
								subprocess.call(["xdotool", "key", Chars[io]])
							if(Debug):print("Emulating Keypress{}".format(Chars[io]))
							if Destination[io] == 2 and LinuxKeyboardInput == 1:
								subprocess.call(["xdotool", "type", Chars[io]])
							if(Debug):print("Emulating Keypress{}".format(Chars[io]))
								
							else:
								c["keypad.{}".format(Chars[io])] = 1
							if(Debug):print("keypad{}:{}".format(Chars[io],1))

						if value == 0 & Destination[io] == 0:
							c["keypad.{}".format(Chars[io])] = 0
							if(Debug):print("keypad{}:{}".format(Chars[io],0))

							
				elif cmd == "R":
					firstcom = 1
					if JoySticks > 0:
						for pins in range(JoySticks*2):
							if (io == JoyStickPins[pins]):
								c["counter.{}".format(io)] = value
						if (Debug):print("counter.{}:{}".format(io,value))
					if QuadEncs > 0:
						if QuadEncSig[io]== 1:
							if value == 0:
								c["counterdown.{}".format(io)] = 1
								time.sleep(0.001)
								c["counterdown.{}".format(io)] = 0
								time.sleep(0.001)
							if value == 1:
								c["counterup.{}".format(io)] = 1
								time.sleep(0.001)
								c["counterup.{}".format(io)] = 0
								time.sleep(0.001)
						if QuadEncSig[io]== 2:
									c["counter.{}".format(io)] = value

				elif cmd == 'E':
					arduino.write(b"E0:0\n")
					if (Debug):print("Sending E0:0 to establish contact")
				else: pass
	

		except: pass
	

	except KeyboardInterrupt:
		if (Debug):print ("Keyboard Interrupted.. BYE")
		exit()
	except: 
		if (Debug):print ("I received garbage")
		arduino.flush()
	
	if firstcom == 1: managageOutputs()		#if ==1: E0:0 has been exchanged, which means Arduino knows that LinuxCNC is running and starts sending and receiving Data

	if keepAlive(event):	#keep com alive. This is send to help Arduino detect connection loss.
		arduino.write(b"E:\n")
		if (Debug):print("keepAlive")
		event = time.time()
	
	time.sleep(0.01)	
	
'''

