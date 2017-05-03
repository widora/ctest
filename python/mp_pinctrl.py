#!/usr/bin/env python

#---------------------------------------
# Button Control for Mplayer
# Use python MRAA to control mplayer
#			Midas-Zhou
#---------------------------------------
import mraa
import time
import os
import datetime

num=0
rnum=0
nvol=100
tstart=0
tend=0
nlist=1

def callPAUSEEND(data):
  global tstart
  global nlist
  pinPAUSE.isrExit()
  print "GPIO- PAUSE_END  interrupt triggered with userdata=",data
  tend=datetime.datetime.now()
  tpause=(tend-tstart).seconds
  print "time pause: %s " %(tpause)
  if tpause < 2:
     print "tpause < 2" 
     os.system("echo 'pause'>/home/slave")
  else:
     nlist+=1
     if nlist>5:
        nlist=1
        os.system("echo loadlist /home/"+str(nlist)+".list >/home/slave") 
     else:
        os.system("echo loadlist /home/"+str(nlist)+".list >/home/slave") 
  
  tstart=0
  pinPAUSE.isr(mraa.EDGE_RISING,callPAUSE,None)

def callPAUSE(data):
  global tstart
  pinPAUSE.isrExit()
  tstart=datetime.datetime.now()
  print tstart
  pinPAUSE.isr(mraa.EDGE_FALLING,callPAUSEEND,None)



def callVOLDOWN(data):
  global pinVOLDOWN
  global nvol

  print "GPIO- VOLDOWN interrupt triggered with userdata=",data

  pinVOLDOWN.isrExit()
  time.sleep(0.3)
  pinVOLDOWN.isr(mraa.EDGE_RISING,callVOLDOWN,None)
  
  nvol-=2
  if nvol < 0:  
     nvol=0
  os.system("amixer set Speaker "+str(nvol)+" >/dev/null")
  

def callVOLUP(data):
  global nvol
  global pinVOLUP

  print "GPIO- VOLUP interrupt triggered with userdata=",data
  pinVOLUP.isrExit()
  time.sleep(0.3)
  pinVOLUP.isr(mraa.EDGE_RISING,callVOLUP,None)

  nvol+=2
  if nvol > 127:  
     nvol=127
  os.system("amixer set Speaker "+str(nvol)+" >/dev/null")



def callNEXT(data1):
  global pinNEXT

  pinNEXT.isrExit()
  time.sleep(0.2)
  pinNEXT.isr(mraa.EDGE_RISING,callNEXT,None)
    
  print "GPIO- NEXT interrupt triggered with userdata=",data1

  cmdstr="echo 'pt_step 1'>/home/slave"
  os.system(cmdstr)

def callPREV(data2):
  global pinPREV

  pinPREV.isrExit()
  time.sleep(0.2)
  pinPREV.isr(mraa.EDGE_RISING,callPREV,None)
  print "GPIO- PREV interrupt triggered with userdata=",data2
  
  cmdstr="echo 'pt_step -1'>/home/slave"
  os.system(cmdstr)
   


pinNEXT=mraa.Gpio(17,owner=False,raw=True)
pinNEXT.dir(mraa.DIR_IN)
pinNEXT.isr(mraa.EDGE_RISING,callNEXT,None)

pinPREV=mraa.Gpio(15,owner=False,raw=True)
pinPREV.dir(mraa.DIR_IN)
pinPREV.isr(mraa.EDGE_RISING,callPREV,None)

pinVOLUP=mraa.Gpio(40,owner=False,raw=True)
pinVOLUP.dir(mraa.DIR_IN)
pinVOLUP.isr(mraa.EDGE_RISING,callVOLUP,None)

pinVOLDOWN=mraa.Gpio(42,owner=False,raw=True)
pinVOLDOWN.dir(mraa.DIR_IN)
pinVOLDOWN.isr(mraa.EDGE_RISING,callVOLDOWN,None)

pinPAUSE=mraa.Gpio(16,owner=False,raw=True)
pinPAUSE.dir(mraa.DIR_IN)
pinPAUSE.isr(mraa.EDGE_RISING,callPAUSE,None)


print "Satrt GPIO monitoring...."
print "Setting Speaker volume..."
os.system("amixer set Speaker 105")

while(True):
  time.sleep(0.2) #use pass will waste CPU time
  

