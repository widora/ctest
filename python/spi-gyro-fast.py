#!/usr/bin/env python
import os
import time
import subprocess
#mport math

nn=0


fin=open('/home/spiin','rb+',buffering=0)  #---binary write & read mode
fout=open('/home/spiout','rb+',buffering=0)

#----------set continous measuring model------------
fq=subprocess.Popen("mosquitto_pub -h widora.org -t midasGYRO -l",bufsize=-1,shell=True,stdin=subprocess.PIPE)


#------------------- configure  SPI  ---------------------
os.system('spi-config -d /dev/spidev32766.1 -m 0 -s 500000')

#---------- initilize gyro and setup registers ------------
os.system("echo -e '\x200f' | spi-pipe -d /dev/spidev32766.1 -b 2 -n 1 | hexdump") #--------- write REG1 ,power up the device 

#--- other REGx seems don't work with SPI ------------
#-- so keep all default 

fm=0.00875  #--- when FS=250dps 


#-------------------  re-configure  SPI  ---------------------
os.system('spi-config -d /dev/spidev32766.1 -m 0 -s 10000000')

#-----------------------------    read data  but discard  ---------------------
os.system("echo -e '\xe800' | spi-pipe -d /dev/spidev32766.1 -b 2 -n 1 | hexdump")  #--return XLXH ---
os.system("echo -e '\xea00' | spi-pipe -d /dev/spidev32766.1 -b 2 -n 1 | hexdump")  #--return YLYH ---
os.system("echo -e '\xec00' | spi-pipe -d /dev/spidev32766.1 -b 2 -n 1 | hexdump")  #--return ZLZH ---

#-----------------  establish spi pipe --------------
os.system("spi-pipe -d /dev/spidev32766.1 -b 1 -n -1 </home/spiin >/home/spiout &")

strFin='\xa8\xa9\xaa\xab\xac\xad' #--address of XLXH YLYH ZLZH 

while(1):
  try:    
     strMQTT=''
     
     fin.flush()
     fout.flush()

     #---------------------  write read-address command to spi-pipe -------------- 
     fin.write(strFin) 
 
     #-----  $  return of XLXH YLYH ZLZH  -------- 
     
     XL=ord(fout.read(1))
     XH=ord(fout.read(1))
     #angRX=XH*256+XL
     angRX=(XH<<8)+XL
     YL=ord(fout.read(1))
     YH=ord(fout.read(1))
     #angRY=YH*256+YL
     angRY=(YH<<8)+YL
     ZL=ord(fout.read(1))
     ZH=ord(fout.read(1))
     #angRZ=ZH*256+ZL
     angRZ=(ZH<<8)+ZL

     #print ' XL=',str(hex(XL)),' XH=',str(hex(XH))
     #print ' YL=',str(hex(YL)),' YH=',str(hex(YH))
     #print ' ZL=',str(hex(ZL)),' ZH=',str(hex(ZH))

     #print ' XL=',XL,' XH=',XH
     #print ' YL=',YL,' YH=',YH
     #print ' ZL=',ZL,' ZH=',ZH

     #strX='0x'+strXH+strXL
     #strY='0x'+strYH+strYL
     #strZ='0x'+strZH+strZL
   
     #print ' strX=',strX,  
     #print ' strY=',strY,  
     #print ' strZ=',strZ  

     #----------------------- RX ----------
     #angRX=int(strX,16)
     if angRX > 32768:
        angRX=angRX-65536
     angRX=fm*angRX  #----degree per second
     #print ' angRX=',angRX,
     
     #----------------------- RY ---------
     #angRY=int(strY,16)
     if angRY > 32768:
        angRY=angRY-65536
     angRY=fm*angRY  #----degree per second
     #print ' angRY=',angRY,
       
     #----------------------- RZ ----------
     #angRZ=int(strZ,16)
     if angRZ > 32768:
        angRZ=angRZ-65536
     angRZ=fm*angRZ  #----degree per second
     #print ' angRZ=',angRZ
      
     nn+=1

     strangRX='%07.2f'%(angRX)
     strangRY='%07.2f'%(angRY)
     strangRZ='%07.2f'%(angRZ)
     
     strMQTT='NN='+str(nn)+'   angRX='+strangRX+' :  '+'angRY='+strangRY+' :  '+'angRZ='+strangRZ
     print strMQTT
     
     fq.stdin.write(strMQTT+'\n')
     fq.stdin.flush()     
     

     #---------------------------------------------------------
     #time.sleep(0.01)

  except KeyboardInterrupt:
      print 'User interrupt to stop the program.' 
      fq.stdin.close()
      fq.terminate()
      fin.close()
      fout.close()
      os.system('killall spi-pipe')
      raise



