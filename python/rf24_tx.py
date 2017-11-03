#!/usr/bin/python
#-----------------------------
# Usage: rf24_tx serial_portal 
# 
# ------ TODOs & BUGs  ------
# 
# 1. occasionally:  write failed: [Errno 5] Input/output error
#
#------------------------------
import serial
import binascii
import sys,time
import os

sertty=None


#-------------------------------
# CRC16 sum check
# return as cstring
#-------------------------------
def cal_crc16(x,invert):
    a = 0xFFFF
    b = 0xA001
    for byte in x:
        a ^= ord(byte)
        for i in range(8):
            last = a % 2
            a >>= 1
            if last == 1:
                a ^= b
    s = hex(a).lower()
    return s[4:6]+s[2:4] if invert == True else s[2:4]+s[4:6]

#------------------------------------
# Log error func,
#  avoid '*' symbols in error
#-----------------------------------
def LogErr(error):
        os.environ['error']=str(error)
        os.system("date >> /home/rf24_err.txt")
        os.system("echo $error >> /home/rf24_err.txt")

#------------------------------------
# open serial interface
#------------------------------------
def open_serial(serial_port):
	global sertty
	sertty=serial.Serial(
		port=serial_port,
		baudrate=9600,
		parity=serial.PARITY_ODD,
	        stopbits=serial.STOPBITS_ONE,
		bytesize=serial.SEVENBITS,
		timeout=5		
	)

	if (sertty.isOpen()):
		print "Open serial port successfully!"
	else:
		print "Fail to open serial port!"
		exit -1

#---------open serial port-----------
open_serial(sys.argv[1])

count=0

while (1):
  try:
	#----  prepare msg -------------	
	ctime=str(time.time())
	#--try insert blank for crc16 check
	str_msg = ctime + " " + "," + "------|||||\n"

	#----- cal. crc32 for 1st msg list item
#	str_msg.replace(' ','') #--strip blanks
#	crc32=binascii.crc32(str_msg.split(',')[0])
#	str_crc32 = "%08x" % (crc32 & 0xffffffff)

	#----- cal. crc16 for 1st msg list item 
	str_crc16 = cal_crc16(str_msg.split(',')[0],True)	

	#----- write to uart interf.
	str_msg=str_crc16+","+str_msg
	sertty.write(str_msg)
	print "send: %s" % str_msg

	#----- count and repeat
	count+=1
	#sleep is not a good idea
	
  except Exception,error:
	print error
	LogErr(error)
	#----try to reopen serial port
	sertty.close()
	time.sleep(1)
	LogErr("ser.close() and retry open_serial()...")
	open_serial(sys.argv[1])
	continue	
	
