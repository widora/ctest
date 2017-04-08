#!/usr/bin/env python
import os
import time
import subprocess
import math

#----------set continous measuring model------------
os.system('i2cset -y 0 0x1e 0x00 0x78') #----75HZ

#os.system('i2cset -y 0 0x1e 0x01 0xD0') #-- mG/Lsb =4.35
os.system('i2cset -y 0 0x1e 0x01 0xA0') #-- mG/Lsb =2.56
#os.system('i2cset -y 0 0x1e 0x01 0x60') #-- mG/Lsb =1.52
#os.system('i2cset -y 0 0x1e 0x01 0x20') #-- mG/Lsb =0.92
fmg=2.56  #--- count*fmg =mGuass
lg=390.0  #--- count/lg = Gauss
os.system('i2cset -y 0 0x1e 0x02 0x00')

#------- read data register 03-08, but discard the data ----
os.system('i2cget -y 0 0x1e 0x03 w')
os.system('i2cget -y 0 0x1e 0x05 w')
os.system('i2cget -y 0 0x1e 0x07 w')


fm=subprocess.Popen("mosquitto_pub -h widora.org -t COMPASS -l",bufsize=-1,shell=True,stdin=subprocess.PIPE)
 

while(1):
  try:    
     strMQTT=''
     #-------------------------  magX data --------------- 
     fos=os.popen('i2cget -y 0 0x1e 0x03')
     strX=fos.readlines()
     fos.close()
     strX=strX[0].strip()
     strH=strX
     print 'strH=',strH 
    
     fos=os.popen('i2cget -y 0 0x1e 0x04')
     strX=fos.readlines()
     fos.close()
     strX=strX[0].strip()
     strL=strX[2:4]
     print 'strL=',strL    
  
     strmagX=strH+strL
     #print strmagX,
     magX=int(strmagX,16)
     if magX > 32768:
        magX=magX-65536
     magX=fmg*magX  #----mGuass
     #magX=magX/lg  #---Guass
     #print ' magX=',magX,
     strMQTT=strMQTT+' magX='+str(magX) 

      
     #-------------------------  magZ data --------------- 
     fos=os.popen('i2cget -y 0 0x1e 0x05')
     strZ=fos.readlines()
     fos.close()
     strZ=strZ[0].strip()
     strH=strZ

     fos=os.popen('i2cget -y 0 0x1e 0x06')
     strZ=fos.readlines()
     fos.close()
     strZ=strZ[0].strip()
     strL=strZ[2:4]

     strmagZ=strH+strL
     #print strmagZ,
     magZ=int(strmagZ,16)
     if magZ > 32768:
        magZ=magZ-65536
     magZ=fmg*magZ #---mGuass
     #magZ=magZ/lg  #---Guass
     #print ' magZ=',magZ,
     strMQTT=strMQTT+' magZ='+str(magZ) 

     #-------------------------  magY data --------------- 
     fos=os.popen('i2cget -y 0 0x1e 0x07')
     strY=fos.readlines()
     fos.close()
     strY=strY[0].strip()
     strH=strY

     fos=os.popen('i2cget -y 0 0x1e 0x08')
     strY=fos.readlines()
     fos.close()
     strY=strY[0].strip()
     strL=strY[2:4]

     strmagY=strH+strL
     #print strmagY,
     magY=int(strmagY,16)
     if magY > 32768:
        magY=magY-65536
     magY=fmg*magY #---mGuass
     #magY=magY/lg   #---Guass
     #print ' magY=',magY
     strMQTT=strMQTT+' magY='+str(magY) 

     print strMQTT
     Nangle=(math.atan2(magY,magX))/3.1416*180
     strAngle= str(round(Nangle,1))+'Deg. to North'
     print strAngle
 
     fm.stdin.write(strMQTT+'\n')
     fm.stdin.flush()
     fm.stdin.write(strAngle+'\n')
     fm.stdin.flush()

     #time.sleep(0.5)  #----the program will crash with very little time sleep

  except KeyboardInterrupt:
     print 'User interrupt to stop the program.' 
     fm.stdin.close()
     fm.terminate()




