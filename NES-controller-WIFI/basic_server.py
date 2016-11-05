#!/usr/bin/env python3

import socket, time, binascii

#because I want to define stuff clearer:
NONE    = 0b00000000
RIGHT   = 0b00000001
LEFT    = 0b00000010
DOWN    = 0b00000100
UP      = 0b00001000
START   = 0b00010000
SELECT  = 0b00100000
BUTTONB = 0b01000000
BUTTONA = 0b10000000

sockUDP = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sockUDP.bind(("0.0.0.0", 7869)) 

#sockTCP = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#sockTCP.bind(("0.0.0.0", 7869))

#We use the try structure here to just clean up when we break out with ctrl-C
try:
	while True:
		data, addr = sockUDP.recvfrom(1024)
		d = int(binascii.hexlify(data),16)
		ts = time.time()

		print(str(ts).ljust(20, " "), ": ", end="")
		print( "LEFT   : " if d & LEFT    else "       : ", end="")
		print( "RIGHT  : " if d & RIGHT   else "       : ", end="")
		print( "UP     : " if d & UP      else "       : ", end="")
		print( "DOWN   : " if d & DOWN    else "       : ", end="")
		print( "A      : " if d & BUTTONA else "       : ", end="")
		print( "B      : " if d & BUTTONB else "       : ", end="")
		print( "START  : " if d & START   else "       : ", end="")
		print( "SELECT : " if d & SELECT  else "       : ", end="")
		
		print( format(d, "#04x"), end="")

		print( " : D: ", end="")
		print( format(d, "d"))
except:
	pass
