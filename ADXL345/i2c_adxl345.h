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

#if 0
#define ADXL_ODR_1600HZ  0xE
#define ADXL_ODR_800HZ  0xD
#define ADXL_ODR_400HZ  0xC
#define ADXL_ODR_200HZ  0xB
#define ADXL_ODR_100HZ  0xA
#define ADXL_ODR_50HZ  0x9  // D3-D0 in Register 0x2C
#define ADXL_ODR_25HZ  0x8
#endif

enum ADXL_ODRBW_setval
{
 ADXL_DR3200_BW1600  =0xE,
 ADXL_DR1600_BW800   =0xE,
 ADXL_DR800_BW400    =0xD,
 ADXL_DR400_BW200    =0xC,
 ADXL_DR200_BW100    =0xB,
 ADXL_DR100_BW50     =0xA,
 ADXL_DR50_BW25      =0x9,  // D3-D0 in Register 0x2C
 ADXL_DR25_BW12_5    =0x8,
};

#define ADXL_NORMAL_POWER 0x00 //D4 in Register 0X2C
#define ADXL_LOWER_POWER 0x10

#define ADXL_FULL_MODE  0x8 // D3 in Register 0x32 --- full resolution -----
#define ADXL_10BIT_MODE  0x0 // D3 in Register 0x32
#define ADXL_RANGE_2G 0x0   //-2g~+2g D0 D1 in Register 0x31
#define ADXL_RANGE_4G 0x1   //-4g~+4g
#define ADXL_RANGE_8G 0x2   //-8g~+8g
#define ADXL_RANGE_16G 0x3  //-16g~+16g


//-------------- var. -------------
#define ADXL_DATAREADY_WAITUS 1000 //wait time(us) for XYZ data 
#define ADXL_BIAS_SAMPLE_NUM 1024

static float fsmg_full=3.9; //3.9mg/LSB scale factor for full resolution.
static char *g_I2Cfdev="/dev/i2c-0"; //i2c device file
static int g_I2Cfd; //file descriptor
static uint8_t  I2C_Addr_ADXL345=0x1d; //0x1d--SDO high  //0x53--SDO grounding

//---- i2c ioctl data ----
static struct i2c_rdwr_ioctl_data g_i2c_ioctlmsg;
static struct i2c_msg g_msgs[2];//0-for write, 1-for read

//---------  functions declaration  ---------
void init_I2C_IOmsg(void);
static int init_ADXL_I2C_Slave(void);
static int I2C_Single_Write(uint8_t reg_addr, uint8_t reg_dat);
static int I2C_Single_Read(uint8_t reg_addr, uint8_t *reg_dat);
static int I2C_Multi_Read(uint8_t nbytes, uint8_t reg_addr, uint8_t *reg_dat);
bool DataReady_ADXL345(void);
int Init_ADXL345(enum ADXL_ODRBW_setval ODRBW_setval, uint8_t g_range);
void adxl_read_int16AXYZ(int16_t  *accXYZ);
inline void adxl_get_int16BiasXYZ(int16_t* bias_xyz);


#endif
