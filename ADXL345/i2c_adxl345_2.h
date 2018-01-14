/*--------------------------------------------
Operate I2C slave with  read() write()

Midas
---------------------------------------------*/

#ifndef __I2C_ADXL345_2_H__
#define __I2C_ADXL345_2_H__

#include <stdio.h>
#include <stdint.h>
#include <unistd.h> // sleep
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


#define ADXL345REG_DEVID 0x00
#define ADXL345REG_OFSX 0x1e
#define ADXL345REG_OFSY 0x1f
#define ADXL345REG_OFSZ 0x20

#define ADXL345REG_BW_RATE 0x2C
#define ADXL345REG_POWER_CTL 0x2D
#define ADXL345REG_INT_ENABLE 0x2E
#define ADXL345REG_INT_SOURCE 0x30
#define ADXL345REG_DATA_FORMAT 0x31

#define ADXL345REG_DATAX0 0x32
#define ADXL345REG_DATAX1 0x33
#define ADXL345REG_DATAY0 0x34
#define ADXL345REG_DATAY1 0x35
#define ADXL345REG_DATAZ0 0x36
#define ADXL345REG_DATAZ1 0x37


char *g_I2Cfdev="/dev/i2c-0"; //i2c device file
int g_I2Cfd; //file descriptor
uint8_t  I2C_Addr_ADXL345=0x1d; //0x1d--SDO high  //0x53--SDO grounding

//---- i2c ioctl data ----
struct i2c_rdwr_ioctl_data g_i2c_ioctlmsg;
struct i2c_msg g_msgs[2];//0-for write, 1-for read


//-------functions declaration----
void init_I2C_IOmsg(void);
void init_I2C_Slave(void);




/*-----------------------------------------
         initiate i2c ioctl msg data
-----------------------------------------*/
void init_I2C_IOmsg(void)
{
  printf("hello");
}


/*----- open i2c slave and init ioctl IOsmg -----*/
void init_I2C_Slave(void)
{
  int fret;
  struct flock lock;

  if((g_I2Cfd=open(g_I2Cfdev,O_RDWR))<0)
  {
	perror("fail to open i2c device!");
        exit(-1);
  }
  else
   	printf("Open %s successfully!\n",g_I2Cfdev);

  if(ioctl(g_I2Cfd, I2C_SLAVE_FORCE,I2C_Addr_ADXL345)<0)
  {
       perror("fail to set slave address!");
       exit(-1);
  }
  else
	printf("set i2c slave address successfully!\n");

  //----- set g_I2Cfd
  ioctl(g_I2Cfd,I2C_TIMEOUT,3);
  ioctl(g_I2Cfd,I2C_RETRIES,1); // !!!!!!!!

  //---- init i2c ioctl msg data -----
  init_I2C_IOmsg();

  //-----  set I2C speed ------
/*
  if(ioctl(g_fdOled,I2C_SPEED,200000)<0)
	printf("Set I2C speed fails!\n");
  else
	printf("Set I2C speed to 200KHz successfully!\n");
*/

}


/*------------------------------------------
Write Register data

Return
     <0  fails
     >0  OK
-------------------------------------------*/
int I2C_Single_Write(uint8_t reg_addr, uint8_t reg_dat)
{
    int ret;
    uint8_t buff[2];

    buff[0]=reg_addr; buff[1]=reg_dat;
    ret=write(g_I2Cfd,buff,2);
    if(ret !=2 )
    {
	printf(" I2C Single Read failed\n");
	return -1;
    }

    return ret;
}


/*------------------------------------------
Read single Register data

Return
     <0  fails
     >0  OK
-------------------------------------------*/
int I2C_Single_Read(uint8_t reg_addr, uint8_t *reg_dat)
{
    int ret;
    uint8_t buff;

    ret=write(g_I2Cfd,&reg_addr,1);
    if(ret !=1 )
    {
	printf(" I2C Single Read(write addr) failed\n");
	return -1;
    }
    ret=read(g_I2Cfd,&buff,1);
    if(ret !=1 )
    {
	printf(" I2C Single Read(read data) failed\n");
	return -1;
    }

    *reg_dat = buff;

    return ret;
}

/*------------------------------------------
Read multiple Register data

nbytes: bytes to be read
reg_addr:  starting register address
reg_dat[]: data returned

Return
     <0  fails
     >0  OK
-------------------------------------------*/
int I2C_Multi_Read(uint8_t nbytes, uint8_t reg_addr, uint8_t *reg_dat)
{
    int ret;
    uint8_t buff;

    ret=write(g_I2Cfd,&reg_addr,1);
    if(ret !=1 )
    {
	printf(" I2C Mutli Read():  write addr failed\n");
	return -1;
    }
    ret=read(g_I2Cfd,reg_dat,nbytes);
    if(ret !=nbytes )
    {
	printf(" I2C Multi Read():   read data failed\n");
	return -1;
    }

    return ret;
}


/*----------------------------------------------------
check ADXL345 register 0x30 to see if DATA_READY is 1
----------------------------------------------------*/
bool DataReady_ADXL345(void)
{
   uint8_t dat;
   I2C_Single_Read(ADXL345REG_INT_SOURCE, &dat);
//   printf("INT_SOURCE: 0x%02x \n",dat);
   return (dat>>7);
}



 /*---------------------------------------------------------------
              init ADXL345 
 ---------------------------------------------------------------*/
void init_ADXL345(void)
{
   usleep(500000);
   I2C_Single_Write(ADXL345REG_DATA_FORMAT, 0x0B);//Full resolutioin,+-16g,right-justified mode
   I2C_Single_Write(ADXL345REG_BW_RATE, 0x0d);//operation,0x1 reduced power mode, 0x08 ODR 25Hz, 0x1c ODR=400Hz,0x1d 800Hz,  0x1A ODR=100HZ; 0x0 normaol operation,0x1 reduced power mode
   I2C_Single_Write(ADXL345REG_POWER_CTL, 0x08);//measurement mode, no sleep
   I2C_Single_Write(ADXL345REG_INT_ENABLE, 0x00);//no interrupt
   I2C_Single_Write(ADXL345REG_OFSX, 0x00);//user-set offset adjustments in twos complement format with a scale factor of 15.6mg/LSB
   I2C_Single_Write(ADXL345REG_OFSY, 0x00);
   I2C_Single_Write(ADXL345REG_OFSZ, 0x0F);

}

/*--------------------------------------------
 Read DATAX0-DATAZ1 
 Retrun int16_t [3]
--------------------------------------------*/
void get_int16XYZ_ADXL345(int16_t  *accXYZ)
{
    uint8_t *xyz = (uint8_t *)accXYZ;

    //----- read XYZ data one byte by one byte,!!!!!!!!!!!!!!!
    //------  continous reading will crash Openwrt !!!!

    //----- wait for data to be read
    while(!DataReady_ADXL345())
          usleep(5000);
    I2C_Single_Read(0x32,xyz);
    I2C_Single_Read(0x33,xyz+1);
    I2C_Single_Read(0x34,xyz+2);
    I2C_Single_Read(0x35,xyz+3);
    I2C_Single_Read(0x36,xyz+4);
    I2C_Single_Read(0x37,xyz+5);

}

#endif
