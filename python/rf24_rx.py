#!/music/optware/usr/bin/python
#-----------------------------
# Usage: rf24_rx serial_portal 
# 
#------------------------------

import serial
import sys,time
import binascii

sertty=''

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


#---------open serial port-----------
open_serial(sys.argv[1])
time.sleep(0.5)

try:
     while(1):
	#----- read msg from UART
	text=sertty.readline()[: -1]
	if len(text)>50:
		continue	
	#print text
	msg_list = text.split(',')

	#----- get crc in rec. text 
	rec_crc = msg_list[0]
	#print "rec_crc: %s" % rec_crc

	#----- get msg text
	crclen=len(rec_crc)
	msg=text[crclen+1:]
	#print msg

	#----- cal. crc32 for rec. msg list 1st item
#        if len(msg_list)>1:
#		#print len(msg_list)
#		msg = msg_list[1]
#	else:
#		continue
#
#	#msg.replace(' ','') # rid blanks 
#	recal_crc32 = "%x" % (binascii.crc32(msg) & 0xffffffff)
#	#print recal_crc32


	#----- cal. crc16 for rec. msg list 1st item
        if len(msg_list)>1:
                msg = msg_list[1]
        else:
                continue
	#--- blanks in msg is OK for cal_crc16
	recal_crc16 = cal_crc16(msg,True)

	#----- check crc 
	if recal_crc16 == rec_crc:
		print text
		print "recal_crc16: %s" % recal_crc16

	#sertty.write("AT?")
	#time.sleep(1)
	#sertty.write("AT+RATE?=1")
	#time.sleep(1)

except Exception,error:
	LogErr(error)
