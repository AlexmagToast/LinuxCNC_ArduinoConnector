#!/usr/bin/python3.9
import serial, time, hal
#	LinuxCNC_ArduinoConnector
#	By Alexander Richter, info@theartoftinkering.com 2022

#	This Software is used as IO Expansion for LinuxCNC. Here i am using a Mega 2560.

#	It is NOT intended for timing and security relevant IO's. Don't use it for Emergency Stops or Endstop switches!

#	You can create as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
#	You can also generate "virtual Pins" by using latching Potentiometers, which are connected to one analog Pin, but are read in Hal as individual Pins.

#	Currently the Software provides: 
#	- analog Inputs
#	- latching Potentiometers
#	- 1 absolute encoder input
#	- digital Inputs
#	- digital Outputs

#	The Send and receive Protocol is <Signal><PinNumber>:<Pin State>
#	To begin Transmitting Ready is send out and expects to receive E: to establish connection. Afterwards Data is exchanged.
#	Data is only send everythime it changes once.

#	Inputs                  = 'I' -write only  -Pin State: 0,1
#	Outputs                 = 'O' -read only   -Pin State: 0,1
#	PWM Outputs             = 'P' -read only   -Pin State: 0-255
#   Digital LED Outputs     = 'D' -read only   -Pin State: 0,1
#	Analog Inputs           = 'A' -write only  -Pin State: 0-1024
#	Latching Potentiometers = 'L' -write only  -Pin State: 0-max Position
#	Absolute Encoder input  = 'K' -write only  -Pin State: 0-32


#	Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal

#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; either version 2 of the License, or
#	(at your option) any later version.
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#	See the GNU General Public License for more details.
#	You should have received a copy of the GNU General Public License
#	along with this program; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


c = hal.component("arduino") #name that we will cal pins from in hal
connection = '/dev/ttyACM0'


# Set how many Inputs you have programmed in Arduino and which pins are Inputs
Inputs = 16
InPinmap = [32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48]

# Set how many Outputs you have programmed in Arduino and which pins are Outputs
Outputs = 9
OutPinmap = [10,9,8,7,6,5,4,3,2,21]

# Set how many PWM Outputs you have programmed in Arduino and which pins are PWM Outputs
PwmOutputs = 2
PwmOutPinmap = [11,12]

# Set how many Analog Inputs you have programmed in Arduino and which pins are Analog Inputs
AInputs = 1
AInPinmap = [1]


# Set how many Latching Analog Inputs you have programmed in Arduino and how many latches there are
LPoti = 2
LPotiLatches = [[2,9],
				[3,4]]

# Set if you have an Absolute Encoder Knob and how many positions it has (only one supported, as i don't think they are very common and propably nobody uses these anyway)
AbsKnob = 1
AbsKnobPos = 32

# Set how many Digital LED's you have connected. 
DLEDcount = 8


Debug = 1
########  End of Config!  ########
olddOutStates= [0]*Outputs
oldPwmOutStates=[0]*PwmOutputs

######## SetUp of HalPins ########

# setup Input halpins
for port in range(Inputs):
    c.newpin("dIn.{}".format(InPinmap[port]), hal.HAL_BIT, hal.HAL_OUT)
    c.newparam("dIn.{}-invert".format(InPinmap[port]), hal.HAL_BIT, hal.HAL_OUT)

# setup Output halpins
for port in range(Outputs):
    c.newpin("dOut.{}".format(OutPinmap[port]), hal.HAL_BIT, hal.HAL_IN)
    olddOutStates[port] = 0

# setup Pwm Output halpins
for port in range(PwmOutputs):
    c.newpin("PwmOut.{}".format(PwmOutPinmap[port]), hal.HAL_FLOAT, hal.HAL_IN)
    oldPwmOutStates[port] = 255
# setup Analog Input halpins
for port in range(AInputs):
    c.newpin("aIn.{}".format(AInPinmap[port]), hal.HAL_FLOAT, hal.HAL_OUT)

# setup Latching Poti halpins
for Poti in range(LPoti):
	for Pin in range(LPotiLatches[Poti][1]):
		c.newpin("LPoti.{}.{}" .format(LPotiLatches[Poti][0],Pin), hal.HAL_BIT, hal.HAL_OUT)

# setup Absolute Encoder Knob halpins
if AbsKnob:
	for port in range(AbsKnobPos):
		c.newpin("AbsKnob.{}".format(port), hal.HAL_BIT, hal.HAL_OUT)

# setup Digital LED halpins
if DLEDcount > 0:
	for port in range(DLEDcount):
		c.newpin("DLED.{}".format(port), hal.HAL_BIT, hal.HAL_IN)

c.ready()

#setup Serial connection
arduino = serial.Serial(connection, 115200, timeout=1, xonxoff=False, rtscts=False, dsrdtr=True)
######## GlobalVariables ########
firstcom = 0
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
    for ele in input_str:
        if ele.isdigit():
            out_number += ele
    return int(out_number) 

def managageOutputs():
	for port in range(PwmOutputs):
		State = int(c["PwmOut.{}".format(PwmOutPinmap[port])])
		if oldPwmOutStates[port] != State: 	#check if states have changed
			Sig = 'P'
			Pin = int(PwmOutPinmap[port])
			command = "{}{}:{}\n".format(Sig,Pin,State)
			arduino.write(command.encode())
			if (Debug):print ("Sending:{}".format(command.encode()))
			oldPwmOutStates[port]= State

	for port in range(Outputs):
		State = int(c["dOut.{}".format(OutPinmap[port])])
		if olddOutStates[port] != State:	#check if states have changed
			Sig = 'O'
			Pin = int(OutPinmap[port])
			command = "{}{}:{}\n".format(Sig,Pin,State)
			arduino.write(command.encode())
			if (Debug):print ("Sending:{}".format(command.encode()))
			olddOutStates[port]= State
		
	for port in range(DLEDcount):
		State = int(c["DLED.{}".format(port)])
		Sig = 'D'
		Pin = int(port)
		command = "{}{}:{}\n".format(Sig,Pin,State)
		arduino.write(command.encode())
		if (Debug):print ("Sending:{}".format(command.encode()))


while True:
	
	try:
		data = arduino.readline().decode('utf-8')
		if (Debug):print ("I received:{}".format(data))
		data = data.split(":",1)

		try:
			cmd = data[0][0]
			if cmd == "":
				if (Debug):print ("No Command!:{}.".format(cmd))
			
			else:
				if not data[0][1]:
					io = 0
				else:
					io = extract_nbr(data[0])
				value = extract_nbr(data[1])
				if value<0: value = 0
				

				if cmd == "I":
					firstcom = 1
					if value == 1:
						c["dIn.{}".format(io)] = 1
						c["dIn.{}-invert".format(io)] = 0
						if(Debug):print("dIn{}:{}".format(io,1))
						
					if value == 0:
						c["dIn.{}".format(io)] = 0
						c["dIn.{}-invert".format(io)] = 1
						if(Debug):print("dIn{}:{}".format(io,0))
					else:pass

				elif cmd == "A":
					firstcom = 1
					c["aIn.{}".format(io)] = value
					if (Debug):print("aIn.{}:{}".format(io,value))

				elif cmd == "L":
					firstcom = 1

					for Poti in range(LPoti):
						if LPotiLatches[Poti][0] == io:
							for Pin in range(LPotiLatches[Poti][1]):
								if Pin == value:
									c["LPoti.{}.{}" .format(io,Pin)] = 1
									if(Debug):print("LPoti.{}.{} =1".format(io,Pin))
								else:
									c["LPoti.{}.{}" .format(io,Pin)] = 0
									if(Debug):print("LPoti.{}.{} =0".format(io,Pin))	

				elif cmd == "K":
					firstcom = 1
					for port in range(AbsKnobPos):
						if port == value:
							c["AbsKnob.{}".format(port)] = 1
							if(Debug):print("AbsKnob.{}:{}".format(port,1))
						else:
							c["AbsKnob.{}".format(port)] = 0
							if(Debug):print("AbsKnob.{}:{}".format(port,0))
					
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
	
	if firstcom == 1: managageOutputs()

	if keepAlive(event):
		arduino.write(b"E:\n")
		if (Debug):print("keepAlive")
		event = time.time()
	