#!/usr/bin/python3.9
from queue import Empty
import traceback
from numpy import block
import yaml

import serial, time, os

from linuxcnc_arduinoconnector.ArduinoConnector import ConnectionType, SerialConnetion, UDPConnection


with open('example_config.yaml', 'r') as file:
    docs = yaml.safe_load_all(file)

    for doc in docs:
        print(doc)
        
sc = SerialConnetion(myType=ConnectionType.SERIAL, dev='/dev/cu.usbserial-0001')
#sc = UDPConnection(myType=ConnectionType.UDP, listenip='', listenport=54321)
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

