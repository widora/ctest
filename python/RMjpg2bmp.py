#!/usr/bin/env python

#=========================   NOTE  ====================================
# 1. This script will convert a jpg image file to a 24bit-BMP file,
# The image will be resized and rotated to fit for a 480x320 LCD 
# The max. original side-size of a jpg image is limited to abt. 3500-pixels,   
# otherwise it may crash and quit.  
# 2. In order to use python image processing, you must install python-imglib first.
# 3. Usage:   RMjpg2bmp.py  /tmp/ P   (convert jpg files in /tmp/ to a 
# 24bit BMP filev, storing with names of Pxxx, such as P1.bmp, P2.bmp.....etc.
#----------------------------- ----------------------------------------

import Image
import os
import time
import glob
import types
import sys


strpath=sys.argv[1]  #-----user input directory
strtype1='*.JPG'
strtype2='*.jpg'
strtype3='*.jpeg'

#strK='N'   #-- string add before a newfile name
strK=str(sys.argv[2])

k=1
token=1 

while(token):
   try:
      piclist1=glob.glob(strpath+strtype1)
      piclist2=glob.glob(strpath+strtype2)
      piclist3=glob.glob(strpath+strtype3)
      piclist=piclist1+piclist2+piclist3

      if piclist==False:
          print "No picture found in the directory!"
          token=0 
      print type(piclist)
      for pitem in piclist:
	  im=Image.open(pitem)
          print "Open image file:",pitem
	  print "Format of the pictur: ",im.format,'HV:',im.size,im.mode

          #----------resize the picture and/or rotate  to fit for 480x320 LCD size---------
          imH=float(im.size[0])  #--H
          imV=float(im.size[1])  #--V
          if imH > imV:  #----adjust pic and make sure V(height)>H(width)
             print "imH > imV"
             im=im.rotate(90)
             print "rotate 90deg finished!"
             imH=float(im.size[0])
             imV=float(im.size[1])

          if (imV/imH) >= (480.0/320.0):
             print "imV/imH >=480/320"
             omH=int(imH*(480/imV))
             omH=omH>>2
             omH=omH<<2
             omV=480
          if (imV/imH) < (480.0/320.0):
             print "imV/imH < 480/320"
             omV=int(imV*(320/imH))
             omV=omV>>2
             omV=omV<<2  
             omH=320

          print "omH=",omH," omV=",omV
          om=im.resize((omH,omV))  #--(width,height)
         
          strsave=strpath+strK+str(k)+'.bmp'
          print strsave
          om.save(strsave)
          print "Transfer the picture to 480x320BMP successfully!"
          k+=1

      token=0

   except KeyboardInterrupt:
	  print "Exit by User or System."
	  raise 

   except:
          print "Error!"
          raise
