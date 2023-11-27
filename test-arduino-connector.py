#!/usr/bin/python3.9
from queue import Empty
import traceback
#from numpy import block
import yaml

import serial, time, os

from linuxcnc_arduinoconnector.ArduinoConnector import ArduinoYamlParser, ConnectionType, SerialConnetion, UDPConnection



#with open('example_config.yaml', 'r') as file:
#	docs = yaml.safe_load_all(file)

#	for doc in docs:
#		a = ArduinoYamlParser(doc=doc)
a = ArduinoYamlParser(path='new_config.yaml')
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
		#if Debug:print(just_the_string)



