/*-------------------------------------------------
Based on:
        www.cnblogs.com/subo_peng/p/4848260.html
        Author: lzy

-------------------------------------------------*/
#ifndef __SPI_H__
#define __SPI_H__

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

//----- turn on/off SPI DEBUG
#define SPI_DEBUG 0

extern const char *str_spi_device;
extern uint8_t spi_mode;
extern uint8_t spi_bits; // 8bits,MSB first
extern uint32_t spi_speed;// set speed
extern uint16_t delay;
extern int g_SPI_Fd; //SPI device file descriptor

//----- FUCNTION DECLARATION -----
extern void pabort(const char *s);
extern int SPI_Transfer( const uint8_t *TxBuf,  uint8_t *RxBuf, int len,int ns);
extern int SPI_Write(const uint8_t *TxBuf, int len);
extern int SPI_Write_Command_Data(const uint8_t *cmd, int ncmd, const uint8_t *dat, int ndat);
extern int SPI_Write_then_Read(const uint8_t *TxBuf, int n_tx, uint8_t *RxBuf, int n_rx);
extern int SPI_Write_then_Write(const uint8_t *TxBuf1, int n_tx1, uint8_t *TxBuf2, int n_tx2);
extern int SPI_Read(uint8_t *RxBuf, int len);
extern int SPI_Open(void);
extern int SPI_Close(void);
extern int SPI_LookBackTest(void);


#endif
