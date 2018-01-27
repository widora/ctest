/*---------------     ----------------
TODOs and BUGs:
---------------------------------------------------------------------------*/
#ifndef  __GYRO_L3G4200D__
#define __GYRO_L3G4200D__

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include "i2c_oled_128x64.h"
#include "gyro_spi.h"

//---- SPI Read and Write BITs set -----
#define WRITE_SINGLE 0x00
#define WRITE_BURST 0x40
#define READ_SINGLE 0x80
#define READ_BURST 0xc0


//----- define registers -----
#define L3G_WHO_AM_I 0x0f
#define L3G_CTRL_REG1 0x20
#define L3G_CTRL_REG2 0x21
#define L3G_CTRL_REG3 0x22
#define L3G_CTRL_REG4 0x23
#define L3G_CTRL_REG5 0x24
#define L3G_REFERENCE 0x25
#define L3G_OUT_TEMP 0x26
#define L3G_STATUS_REG 0x27
#define L3G_OUT_X_L 0x28
#define L3G_OUT_X_H 0x29
#define L3G_OUT_Y_L 0x2a
#define L3G_OUT_Y_H 0x2b
#define L3G_OUT_Z_L 0x2c
#define L3G_OUT_Z_H 0x2d
#define L3G_FIFO_CTRL_REG 0x2e
#define L3G_FIFO_SRC_REG 0x2f
#define L3G_INT1_CFG 0x30
#define L3G_INT1_SRC 0x31
#define L3G_INT1_TSH_XH 0x32
#define L3G_INT1_TSH_XL 0x33
#define L3G_INT1_TSH_YH 0x34
#define L3G_INT1_TSH_YL 0x35
#define L3G_INT1_TSH_ZH 0x36
#define L3G_INT1_TSH_ZL 0x37
#define L3G_INT1_DURATION 0x38

#define L3G_READ_WAITUS 500  //for ODR=800Hz;  poll wait time in us for read RX RY RZ
#define L3G_BIAS_SAMPLE_NUM 1024 //total number of samples needed for RXRYRZ bias calcualation

//----- global variables -----
extern double sf_dpus;//init in Init_L3G4200D()  //70/1000.0-dps, 70/1000000.0-dpms //sensitivity factor for FS=2000 dps.
extern bool gtok_QuitGyro;//=false //=true to inform a  thread to quit if true
extern double g_fangXYZ[3]; //float value of XYZ angle
extern int16_t g_bias_int16RXYZ[3];

//----- Function Declaration  ------
void halSpiWriteReg(const uint8_t addr, const uint8_t value);
void halSpiWriteBurstReg(const uint8_t addr,  uint8_t *buffer, const uint8_t count);
uint8_t halSpiReadReg(uint8_t addr);
void halSpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count);
uint8_t halSpiReadStatus(uint8_t addr);
int Init_L3G4200D(void);
bool status_XYZ_available(void);
inline void gyro_read_int16RXYZ(int16_t *angRXYZ);
inline void gyro_get_int16BiasXYZ(int16_t* bias_xyz);
void  thread_gyroWriteOled(void);


#endif
