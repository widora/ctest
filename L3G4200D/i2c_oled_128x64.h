/*--------------------------------------------
1.Non_ASCII char will be replaced by code 127(delta,the last symbol)

Midas
---------------------------------------------*/

#ifndef __I2C__OLED__H
#define __I2C__OLED__H

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
#include "ascii2.h"

int g_fdOled;
static char *g_pstroledDev="/dev/i2c-0";
static uint8_t g_u8oledAddr=0x78>>1;
enum oled_sig{   //---command or data
oled_SIG_CMD,
oled_SIG_DAT
};

//---- i2c ioctl data ----
struct i2c_rdwr_ioctl_data g_i2c_iodata;
//---- i2c dev lock ----
struct flock g_i2cFdReadLock;
struct flock g_i2cFdWriteLock;

//---- 16x8_size ASCII char. frame buffer for oled display -----
static struct oled_Ascii16x8_Frame_Buff {
bool refresh_on[4][16]; // 1-refresh 0-no refresh
char char_buff[4][16];//4 lines x 16 ASCII chars
} g_Oled_Ascii16x8_Frame={0};

//-------functions declaration----
void init_I2C_IOdata(void);
void free_I2C_IOdata(void);
void init_I2C_Slave(void);
void initOledTimer(void);
void sigHndlOledTimer(int signo);
void sendDatCmdoled(enum oled_sig datcmd,uint8_t val); // send data or command to I2C device
void sendCmdOled(uint8_t cmd);
void sendDatOled(uint8_t dat);
void initOledDefault(void); // open i2c device and set defaul parameters for OLED
void fillOledDat(uint8_t dat); // fill OLED GRAM with specified data
void drawOledAscii16x8(uint8_t start_row, uint8_t start_column,unsigned char c);
void push_Oled_Ascii32x18_Buff(const char* pstr, uint8_t nrow, uint8_t nstart);
void refresh_Oled_Ascii32x18_Buff(bool ref_all);
void drawOledStr16x8(uint8_t start_row, uint8_t start_column,const char* pstr);
void clearOledV(void); //clear OLED GRAM with vertical addressing mode, effective!
void setStartLine(int k);
void actOledScroll(void);
void deactOledScroll(void);
int intFcntlOp(int fd, int cmd, int type, off_t offset, int whence, off_t len);


#endif
