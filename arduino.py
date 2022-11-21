#!/usr/bin/python3.9
import serial, time, hal

#    LinuxCNC_ArduinoConnector
#    This Software is used to use an Arduino as IO Expansion. 
#	 Note, these IO's are not run in the servo-thread. Therefor the IO's shouldn't be used for timing critical applications.
# 	 Currently the Software provides: 
#
#		- analog Inputs
#		- latching Potentiometers
#		- 1 absolute encoder input
#		- digital Inputs
#		- digital Outputs
#		Right now i am also working on
#		- virtual Pins (multiplexed LED's or WS2812)
#
#	 By Alexander Richter, info@theartoftinkering.com
#    inspired by Jeff Epler, jepler@unpythonic.net
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

c = hal.component("arduino") #name that we will cal pins from in hal

# Set how many Inputs you have programmed in Arduino and which pins are Inputs
Inputs = 17
InPinmap = [32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48]

# Set how many Outputs you have programmed in Arduino and which pins are Outputs
Outputs = 9
OutPinmap = [10,9,8,7,6,5,4,3,2]

# Set how many PWM Outputs you have programmed in Arduino and which pins are PWM Outputs
PwmOutputs = 2
PwmOutPinmap = [12,11]

# Set how many Analog Inputs you have programmed in Arduino and which pins are Analog Inputs
AInputs = 1
AInPinmap = [79]


# Set how many Latching Analog Inputs you have programmed in Arduino and how many latches there are
LPotiKnobs = 2
LPotiKnobLatches = [8,4]

# Set if you have an Absolute Encoder Knob and how many positions it has (only one supported, as i don't think they are very common and propably nobody uses these anyway)
AbsKnob = 1
AbsKnobPos = 30


########  End of Config!  ########
######## SetUp of HalPins ########

# setup Input halpins
for port in range(Inputs):
    c.newpin("dIn-%02d" % InPinmap[port], hal.HAL_BIT, hal.HAL_IN)
    c.newparam("dIn-%02d-invert" % InPinmap[port], hal.HAL_BIT, hal.HAL_RW)

# setup Output halpins
for port in range(Outputs):
    c.newpin("dOut-%02d" % OutPinmap[port], hal.HAL_BIT, hal.HAL_IN)

# setup Pwm Output halpins
for port in range(PwmOutputs):
    c.newpin("PwmOut-%02d" % PwmOutPinmap[port], hal.HAL_FLOAT, hal.HAL_IN)

# setup Analog Input halpins
for port in range(AInputs):
    c.newpin("aIn-%02d" % AInPinmap[port], hal.HAL_FLOAT, hal.HAL_IN)

# setup Latching Poti halpins
for latches in range(LPotiKnobs):
	for port in range(LPotiKnobLatches[latches]):
		c.newpin("LPotiKnob-%02d" % [port], hal.HAL_BIT, hal.HAL_IN)

# setup Absolute Encoder Knob halpins
if AbsKnob:
	for port in range(AbsKnobPos):
		c.newpin("LPotiKnob-%02d" % [port], hal.HAL_BIT, hal.HAL_IN)





#c.newpin("analog-in-%02d" % port, hal.HAL_FLOAT, hal.HAL_OUT)
#c.newparam("analog-in-%02d-offset" % port, hal.HAL_FLOAT, hal.HAL_RW)
#c.newparam("analog-in-%02d-gain" % port, hal.HAL_FLOAT, hal.HAL_RW)




c.ready()



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




#try:

arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1, xonxoff=False, rtscts=False, dsrdtr=True)


while True:
	try:	
		data = arduino.readline().decode('utf-8')
		data = data.split(":",1)
		
		
		if len(data) == 2:
			data[1] = extract_nbr(data[1])
			if data[1]<0: data[1] = 0

			if data[0] == "Pt57":
				c.SpSp = data[1]
			elif data[0] == "LP55":	
				c.SpOd = data[1]
			elif data[0] == "LP56":
				c.FdRes = data[1]
			elif data[0] == "AK":
				c.AK = data[1]
			else: pass



	finally:
		pass
		
"""except :
	print("Lost Connection")
finally :
#	serial.close	
	print("Program closed")

"""