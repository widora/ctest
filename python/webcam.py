#!/usr/bin/env python

from os import system
from time import sleep

str1="curl --request POST -F 'data=@"
str2="' --header 'API-KEY: d45b14473' http://www.bigiot.net/pubapi/uploadImg/did/551/inputid/546"
strImage='/tmp/webcam.jpg'

strcmd=str1+strImage+str2

while(1):
   system("fswebcam --no-banner -r 480x320 /tmp/webcam.jpg")
   sleep(15)
   print strcmd
   system (strcmd)
   print "Webcam photo send!"

