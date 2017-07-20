import sys

#Setup serial
import serial
try:
	ser = serial.Serial('/dev/tty.usbserial-A9EX93JR', 115200, timeout=10)
	print("connected to: " + ser.portstr)
	count=0
except:
	print "couldn't open serial port"
	sys.exit()

#Main
while True:
	line = ser.readline()
	print line
ser.close()
