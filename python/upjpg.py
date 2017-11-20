#!/usr/bin/env python

import os
from time import sleep

str1="curl --request POST -F 'data=@"
#---- use your own KEY and IDs
str2="' --header 'API-KEY: xxxxxxxxx' https://www.bigiot.net/pubapi/uploadImg/did/xxx/inputid/xxx"
strImage='/tmp/webcam.jpg'

strcmd=str1+strImage+str2

while(1):
   if(os.path.isfile("/tmp/webcam.jpg")): 
	   print strcmd
	   os.system(strcmd)
	   print "Webcam photo send!"
	   os.system("rm -f /tmp/webcam.jpg")
   sleep(0.5)
