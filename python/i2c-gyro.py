#!/usr/bin/env python
import os
import time
#import subprocess
#import math

#---------- initilize gyro and setup registers ------------
os.system('i2cset -y 0 0x69 0x20 0x0f') #---- PowerOn mode, ODR(output-data-rate)=100Hz, LPF1=32Hz,LPF2=12.5Hz
os.system('i2cset -y 0 0x69 0x21 0x00') #---- ODR=100Hz,   HPCF3=8Hz, High Pass Filter Mode: Normal mode  
os.system('i2cset -y 0 0x69 0x22 0x08') #---- Date Ready on DRDY/INT2  Enabled 
os.system('i2cset -y 0 0x69 0x23 0x30') #---- +-2000dps
os.system('i2cset -y 0 0x69 0x24 0x00') #---- FIFO disabled,HPF disabled

fm=0.07  #--- when FS=2000dps 

#------- read data register 03-08, but discard the data ----
os.system('i2cget -y 0 0x69 0x28 w')
os.system('i2cget -y 0 0x69 0x2a w')
os.system('i2cget -y 0 0x69 0x2c w')


while(1):
  try:    
     strMQTT=''

    


     #-------------------------  angular Rate X --------------- 
     fos=os.popen('i2cget -y 0 0x69 0x28')
     strX=fos.readlines()
     fos.close()
     strX=strX[0].strip()
     strXL=strX[2:4]
     #print 'strXL=',strXL 
    
     fos=os.popen('i2cget -y 0 0x69 0x29')
     strX=fos.readlines()
     fos.close()
     strX=strX[0].strip()
     strXH=strX
     #print 'strXH=',strXH    
  
     strRX=strXH+strXL
     #print strRX
     angRX=int(strRX,16)
     if angRX > 32768:
        angRX=angRX-65536
     angRX=fm*angRX  #----degree per second
     print ' angRX=',angRX,

     
     #-------------------------  angular Rate Y --------------- 
     fos=os.popen('i2cget -y 0 0x69 0x2a')
     strY=fos.readlines()
     fos.close()
     strY=strY[0].strip()
     strYL=strY[2:4]
     #print 'strYL=',strYL 
    
     fos=os.popen('i2cget -y 0 0x69 0x2b')
     strY=fos.readlines()
     fos.close()
     strY=strY[0].strip()
     strYH=strY
     #print 'strYH=',strYH    
  
     strRY=strYH+strYL
     #print strRY
     angRY=int(strRY,16)
     if angRY > 32768:
        angRY=angRY-65536
     angRY=fm*angRY  #----degree per second
     print ' angRY=',angRY,


     #-------------------------  angular Rate Z --------------- 
     fos=os.popen('i2cget -y 0 0x69 0x2c')
     strZ=fos.readlines()
     fos.close()
     strZ=strZ[0].strip()
     strZL=strZ[2:4]
     #print 'strZL=',strZL 
    
     fos=os.popen('i2cget -y 0 0x69 0x2d')
     strZ=fos.readlines()
     fos.close()
     strZ=strZ[0].strip()
     strZH=strZ
     #print 'strZH=',strZH    
  
     strRZ=strZH+strZL
     #print strRZ
     angRZ=int(strRZ,16)
     if angRZ > 32768:
        angRZ=angRZ-65536
     angRZ=fm*angRZ  #----degree per second
     print ' angRZ=',angRZ
     
     

     #---------------------------------------------------------
 
     #time.sleep(0.25)

  except KeyboardInterrupt:
      print 'User interrupt to stop the program.' 
      raise



