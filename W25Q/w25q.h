/*-------- W25Q128 Half-Dual SPI OPERTAION --------*/

#ifndef __W25Q__H
#define __W25Q__H

#include <stdbool.h>
#include <stdint.h>

//----W25Q128FV Instruction Set Table ----
#define W25Q_READ_DATA 0x03
#define W25Q_WRITE_ENABLE 0x06
#define W25Q_WRITE_DISABLE 0x04
#define W25Q_READ_STATUS_REG_1 0x05
#define W25Q_READ_STATUS_REG_2 0x35
#define W25Q_READ_STATUS_REG_3 0x15
#define W25Q_CHIP_ERASE 0xc7 //or 0x60
#define W25Q_POWER_DOWN 0xB9
#define W25Q_RELEASE_POWER_DOWN 0xAB
#define W25Q_ENABLE_RESET 0x66
#define W25Q_RESET_DEVICE 0x99
#define W25Q_PAGE_PROGRAM 0x02 //write
#define W25Q_4K_ERASE 0x20
#define W25Q_32K_ERASE 0x52
#define W25Q_64K_ERASE 0xD8
#define W25Q_GLOBAL_BLOCK_LOCK 0x7E
#define W25Q_GLOBAL_BLOCK_UNLOCK 0x98

extern uint8_t flash_read_status(int regnum);
extern int flash_write_enable(void);
extern int flash_write_disable(void);
extern int flash_wait_busy(int interval);
extern int flash_sector_erase(int addr);
extern int flash_block_erase(bool bl32k, int addr);
extern int flash_chip_erase(void);
extern int flash_write_bytes(uint8_t *dat, int addr,int cnt);
extern int flash_write_page(uint8_t *dat, int addr);
extern int flash_read_data(int addr, uint8_t *dat, int cnt);


#endif
