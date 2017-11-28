#!/usr/bin/env python

import os
from time import sleep

#--- make sure you already install ac-certificates 
str1="curl --retry 3 --connect-timeout 20 --request POST -F 'data=@"

#---- input your own API-KEY , DEVICE ID and INPUT ID
str2="' --header 'API-KEY: xxxxxxxxx' https://www.bigiot.net/pubapi/uploadImg/did/xxx/inputid/xxx"
strImage='/tmp/webcam.jpg'

strcmd=str1+strImage+str2

while(1):
   if(os.path.isfile("/tmp/webcam.jpg")): 
	   print strcmd
	   os.system(strcmd)
	   print "Webcam photo send!"
	   os.system("rm -f /tmp/webcam.jpg")
   else:
	print "/tmp/webcam.jpg not found! wait to retry..."
   sleep(0.5)
