#!/music/optware/usr/bin/python
#-------------------------------------------
# Decode GPS msg as per  NMEA-0183 protocol
# Usage: read_gps.py  serial_port 
#-------------------------------------------

import serial
import sys,time
import os

sertty=''

#-----------------------------------
# open a serial port
#---------------------------------
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

#------------------------------------
# Log error func,
#  avoid '*' symbols in error
#-----------------------------------
def LogErr(error):
        os.environ['error']=str(error)
        os.system("date >> /home/gps_err.txt")
        os.system("echo $error >> /home/gps_err.txt")


#---------open serial port-----------
open_serial(sys.argv[1])

#---------- loop read GPS msg ----------
while(1):
  try:
	try:
		text=sertty.readline()
	except Exception,error:
                print error
		time.sleep(1)
		continue

	#----------------  parse msg ----------------
	inlist=text.split(',')
	#print inlist[0]
	#------- get UTC time ~GPGGA---------
	if inlist[0] == "$GNGGA":
		#print inlist
		print 'UTC_TIME %s:%s:%s' % ( inlist[1][0:2], inlist[1][2:4], inlist[1][4:6] )
		print 'Number of satellites tracked: %s' % inlist[7]

	#------ get GPGSA ----------- 
	# fix_value :  1= no fix, 2= 2D fix, 3= 3D fix  
	if inlist[0] == "$GPGSA":
		#print inlist
		fix_value = inlist[2]
		if fix_value != '2' and fix_value != '3' :
			print '---- Warning: fix value < 2 !! --- '
		else:
			print '---- fix value %s ----'% fix_value		

	#------ get Position ~GPRMC------------
	if inlist[0] == "$GNRMC":
		#print inlist
		#---- check status ----
		rmc_status = inlist[2]
		#---rmc_status: A-active, V-void
		if rmc_status == 'V':
			print " Warning: GPRMC status is Void! "

		strLatitude=inlist[3]
		strLongitude=inlist[5]
		print strLatitude,strLongitude
		valLatitude=float(strLatitude[0:2])+(float(strLatitude[2:]))/60.0
		valLongitude=float(strLongitude[0:3])+(float(strLongitude[3:]))/60.0
		
		print "Latitude: %f  Longitude: %f" % (valLatitude,valLongitude)		

	#----- sleep a while ---------
	time.sleep(0.2)

  except Exception,error:
	LogErr(error)
	pass
