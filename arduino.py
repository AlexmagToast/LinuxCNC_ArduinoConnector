#!/usr/bin/python3.9
#    HAL userspace component to interface with Arduino board
#    Copyright (C) 2007 Jeff Epler <jepler@unpythonic.net>
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



import serial, time, hal
Inputs = 17
InPinmap = [32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48]


# Name, positions
AnalogKnob = [""]
OutPinmap = [32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48]


c = hal.component("arduino")

c.newpin("SpOd", hal.HAL_FLOAT, hal.HAL_IN) #8 Pos latching poti
c.newpin("FdRes", hal.HAL_FLOAT, hal.HAL_IN) #4 pos latching poti
c.newpin("AK", hal.HAL_FLOAT, hal.HAL_IN) #absolute Encoder Knob
c.newpin("SpSp", hal.HAL_FLOAT, hal.HAL_IN) #potentiometer


#c.newpin("analog-in-%02d" % port, hal.HAL_FLOAT, hal.HAL_OUT)
#c.newparam("analog-in-%02d-offset" % port, hal.HAL_FLOAT, hal.HAL_RW)
#c.newparam("analog-in-%02d-gain" % port, hal.HAL_FLOAT, hal.HAL_RW)

for port in range(Inputs):
    c.newpin("digital-in-%02d" % InPinmap[port], hal.HAL_BIT, hal.HAL_IN)
    c.newparam("digital-in-%02d-invert" % InPinmap[port], hal.HAL_BIT, hal.HAL_RW)


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