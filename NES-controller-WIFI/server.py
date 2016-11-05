#!/usr/bin/env python3

import socket, threading, time, binascii, logging

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

def threadUDP():
	#listen for UDP connections and act on them, only need one, source IP is used to differentiate

	logging.debug("Starting UDP listener on port: 7869")
	sockUDP = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sockUDP.bind(("0.0.0.0", 7869))
	
	while True:
		data, addr = sockUDP.recvfrom(1024)
		d = int(binascii.hexlify(data),16)
		ts = time.time()

		out = str(ts).ljust(20, " ")
		out += ": "
		out += "LEFT   : " if d & LEFT    else "       : "
		out += "RIGHT  : " if d & RIGHT   else "       : "
		out += "UP     : " if d & UP      else "       : "
		out += "DOWN   : " if d & DOWN    else "       : "
		out += "A      : " if d & BUTTONA else "       : "
		out += "B      : " if d & BUTTONB else "       : "
		out += "START  : " if d & START   else "       : "
		out += "SELECT : " if d & SELECT  else "       : "

		out += format(d, "#04x")

		out += " : D: "
		out += format(d, "d")
		
		logging.debug(out)


def threadTCP():
	#listen for TCP connections and create a new thread to handle each new one
	logging.debug("Starting UDP listener on port: 7869")
	sockTCP = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sockTCP.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	sockTCP.bind(('0.0.0.0', 7869))
	sockTCP.listen(5)

	while True:
		client, addr = sockTCP.accept()
		client.settimeout(60)	#TODO: higher? will be using keepalives
		threading.Thread(target=threadTCPClient, args=(client, addr)).start()
		

def threadTCPClient(client, address):
	#for each client maintain a connection
	logging.debug("Starting TCP Client Connection")
	ip, port = client.getpeername()
	logging.debug("Connection from " + str(ip) + ":" + str(port))

	maxSize = 1024
	buffer = ''
	data = True
	while True:
		try:
			data = client.recv(maxSize)
			if data:
				#TODO return config data needed on the other end
				#TODO parse data from client
				if(b'\x00' in data):
					#process the data
					logging.debug("Process: " + buffer)

					#wipe the buffer
					#TODO: handle there being more then null in this read
					buffer = ''
				else:
					buffer += data.decode("utf-8")
			else:
				raise Exception('Client Disconnected')
		except Exception as e:
			logging.debug(str(e))
			client.close()
			logging.debug("Client Connection Closed")
			return False

if __name__ == "__main__":
	logging.basicConfig(level=logging.DEBUG, format='(%(threadName)-10s) %(message)s')
	threading.Thread(target=threadUDP, name="UDP Server").start()
	threading.Thread(target=threadTCP, name="TCP Master").start()
