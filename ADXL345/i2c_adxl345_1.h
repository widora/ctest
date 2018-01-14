/*--------------------------------------------

Midas
---------------------------------------------*/

#ifndef __I2C_ADXL345_1_H__
#define __I2C_ADXL345_1_H__

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


//#define I2C_MSGBUFF_SIZE 16 // max bytes of i2c read or write

char *g_I2Cdev="/dev/i2c-0";
int g_I2Cfd; //file descriptor
uint8_t  I2C_Addr_ADXL345=0x1d;//0x1d--SDO high  //0x53--SDO grounding

//uint8_t g_i2cmsg_buff[I2C_MSGBUFF_SIZE]; // for i2c msg data buff

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


/*------------------------------------------
Write data to a register of the I2C slave
reg_add: address of the register
reg_dat: data write to the reg.
Return:
	>0 bytes of data written
	<0 if fails
-------------------------------------------*/
int Single_Write_ADXL345(uint8_t reg_addr, uint8_t reg_dat)
{
    int ret;
    uint8_t buff[2];

    buff[0]=reg_addr;
    buff[1]=reg_dat;

    //---- write reg_addr to i2c
    g_msgs[0].addr=I2C_Addr_ADXL345;
    //g_msgs[0].flags, default is for write
    g_msgs[0].len=2; //data bytes
    g_msgs[0].buf=buff;
    g_i2c_ioctlmsg.nmsgs=1;
    g_i2c_ioctlmsg.msgs=&g_msgs[0];
    ret=ioctl(g_I2Cfd, I2C_RDWR, &g_i2c_ioctlmsg);

    return ret;
}



/*--------------------------------------------------------
Read data from a register of the I2C slave
reg_addr: register address
reg_dat:  register data
Return:
	>0 bytes of data returned
	<0 if fails
---------------------------------------------------------*/
int Single_Read_ADXL345(uint8_t reg_addr, uint8_t *reg_dat)
{
    int ret;

    g_msgs[0].addr=I2C_Addr_ADXL345;
    g_msgs[0].flags=0;// default is for write
    g_msgs[0].len=1; //data bytes
    g_msgs[0].buf=&reg_addr;

    g_msgs[1].addr=I2C_Addr_ADXL345;
    g_msgs[1].flags|=I2C_M_RD;
    g_msgs[1].len=1;
    g_msgs[1].buf=reg_dat;

    g_i2c_ioctlmsg.nmsgs=2;
    g_i2c_ioctlmsg.msgs=g_msgs;
    ret=ioctl(g_I2Cfd, I2C_RDWR, &g_i2c_ioctlmsg);
    if(ret<0)
	return -1;

    return ret;
}

/*----------------------------------------------------
check ADXL345 register 0x30 to see if DATA_READY is 1
----------------------------------------------------*/
bool IS_DataReady_ADXL345(void)
{
   uint8_t dat;
   Single_Read_ADXL345(ADXL345REG_INT_SOURCE, &dat);

   return (bool)(dat>>7);
}


/*--------------------------------------------------------
Read N bytes of data from registers of the I2C slave
reg_addr: tarting register address
*reg_dat:  register data returned
Return:
	>0 bytes of data returned
	<0 if fails
---------------------------------------------------------*/
int Multi_Read_ADXL345(uint8_t nbytes, uint8_t reg_addr, uint8_t *reg_dat)
{
    int ret;

    g_msgs[0].addr=I2C_Addr_ADXL345;
    g_msgs[0].flags=0;// default is for write
    g_msgs[0].len=1; //data bytes
    g_msgs[0].buf=&reg_addr;

    g_msgs[1].addr=I2C_Addr_ADXL345;
    g_msgs[1].flags|=I2C_M_RD | I2C_M_NO_RD_ACK;
    g_msgs[1].len=nbytes;
    g_msgs[1].buf=reg_dat; //reg_dat;

    g_i2c_ioctlmsg.nmsgs=2;
    g_i2c_ioctlmsg.msgs=g_msgs;
    ret=ioctl(g_I2Cfd, I2C_RDWR, &g_i2c_ioctlmsg);
    if(ret<0)
	return -1;


    return ret;
}


/*----- open i2c slave and init ioctl IOsmg -----*/
void init_I2C_Slave(void)
{
  int fret;
  struct flock lock;

  if((g_I2Cfd=open(g_I2Cdev,O_RDWR))<0)
  {
	perror("fail to open i2c bus");
        exit(1);
  }
  else
   	printf("Open i2c bus successfully!\n");

  //----- set g_I2Cfd
  ioctl(g_I2Cfd,I2C_TIMEOUT,5);
  ioctl(g_I2Cfd,I2C_RETRIES,100);

  ioctl(g_I2Cfd,I2C_SLAVE,I2C_Addr_ADXL345);

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

 /*---------------------------------------------------------------
              init ADXL345 
 ---------------------------------------------------------------*/
void init_ADXL345(void)
{
   usleep(500000);
   Single_Write_ADXL345(ADXL345REG_DATA_FORMAT, 0x0B);//Full resolutioin,+-16g,right-justified mode
   Single_Write_ADXL345(ADXL345REG_BW_RATE, 0x1d);//0x1D ODR=800Hz,0x19 ODR=50Hz, 0x1c ODR=400Hz,  0x1A-Reduced power ODR=100HZ   0x0A-normaol operation,ODR=100Hz.
   Single_Write_ADXL345(ADXL345REG_POWER_CTL, 0x08);//measurement mode, no sleep
   Single_Write_ADXL345(ADXL345REG_INT_ENABLE, 0x00);//no interrupt,  DATA_READY function always enabled, not interrupt output;
   Single_Write_ADXL345(ADXL345REG_OFSX, 0x00);//user-set offset adjustments in twos complement format with a scale factor of 15.6mg/LSB
   Single_Write_ADXL345(ADXL345REG_OFSY, 0x00);
   Single_Write_ADXL345(ADXL345REG_OFSZ, 0x05);

}



#endif
