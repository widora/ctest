/*-------------------------------------------------
Base on:
        www.cnblogs.com/subo_peng/p/4848260.html
        Author: lzy

-------------------------------------------------*/
#ifndef _GYRO_SPI_H_
#define _GYRO_SPI_H_

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

/* #include "Debug.h" */
#define pr_debug printf
#define pr_err printf

#define SPI_DEBUG 0 

static const char *spi_device = "/dev/spidev32766.1";
static uint8_t mode = 0;
static uint8_t bits = 8; /* ８ｂiｔｓ读写，MSB first。*/
static uint32_t speed =10*1000*1000;/* min.1M 设置传输速度 */
static uint16_t delay = 0;
static int g_SPI_Fd = 0;


//----- FUCNTION DECLARATION -----
void pabort(const char *s);
int SPI_Transfer( const uint8_t *TxBuf,  uint8_t *RxBuf, int len,int ns);
int SPI_Write_then_Read(const uint8_t *TxBuf, int n_tx, uint8_t *RxBuf, int n_rx);
int SPI_Write_then_Write(const uint8_t *TxBuf1, int n_tx1, uint8_t *TxBuf2, int n_tx2);
int SPI_Write(uint8_t *TxBuf, int len);
int SPI_Read(uint8_t *RxBuf, int len);
int SPI_Open(void);
int SPI_Close(void);
int SPI_LookBackTest(void);


#endif
