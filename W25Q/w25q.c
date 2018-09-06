/*------------- W25Q128 Half-Dual SPI OPERTAION ---------------
Nor flash W25Q128
Total 16Mbytes, with 256 blocks(64Kbyte), each block has 16 secotrs(4kbyte),
and each sector has 16 page(256byte).
16Mbyte = 256(Blocks)*16(Sectors)*16(Pages)*256(Bytes)

Pending:
1. Check whether space is enough before write/program.
2. Max. load for each half-dual SPI ioctl operation is 36bytes, including Tx and Rx data.
3. Address to be aligned for flash-write operation ???
   Address is aligned for flash-erase operation.
--------------------------------------------------------------------------*/
#include <stdbool.h>
#include "w25q.h"
#include "spi.h"


/*----------------------------------
Read Status Registers
	Status Register-1(05h)
	Status Register-2(35h)
	Status Register-3(15h)

reg: 1,2 or 3 for Regiser-number

return: register status data
------------------------------------*/
uint8_t flash_read_status(int regnum)
{
	uint8_t TxBuf[2],RxBuf[2];

	switch(regnum)
	{
		case 1:
			TxBuf[0]=W25Q_READ_STATUS_REG_1;
			break;
		case 2:
			TxBuf[0]=W25Q_READ_STATUS_REG_2;
			break;
		case 3:
			TxBuf[0]=W25Q_READ_STATUS_REG_3;
			break;
		default:
			TxBuf[0]=W25Q_READ_STATUS_REG_1;
			printf("Status register number error! use Register-1.\n");
	}

	SPI_Write_then_Read(TxBuf,1,RxBuf,1);

	return RxBuf[0];
}

/*---------------------------
Write Enable Instruction
return:
	0	OK
----------------------------*/
int flash_write_enable(void)
{
	uint8_t TxBuf[2];

	/*	Write Enable --06h	*/
	TxBuf[0]=W25Q_WRITE_ENABLE;
	if(SPI_Write(TxBuf,1) < 1)
		return -1;

	/*     Global/Sector Unlock --98h    necessary -----------????????  */
	TxBuf[0]=W25Q_GLOBAL_BLOCK_UNLOCK;
	if(SPI_Write(TxBuf,1) < 1)
		return -1;

	return 0;
}

/*----------------------------------------------------------------
Note that WEL bit automatically reset after power-up and upon
completion of the Write Status Register,Page Program,Sector Erase,
Block Erase,Chip Erase,Erase/Program Security Register,and Reset
instruction.

Write disable flash
return:
	0	OK
------------------------------------------------------------------*/
int flash_write_disable(void)
{
	uint8_t TxBuf[2];

	/*	Write Disable --04h	*/
	TxBuf[0]=W25Q_WRITE_DISABLE;
	if(SPI_Write(TxBuf,1) < 1)
		return -1;

	return 0;
}

/*--------------------------------------------------------
wait for flash busy status, etc. during erasing,writing.

return ms:
	>=0	OK
---------------------------------------------------------*/
int flash_wait_busy(void)
{
	uint8_t TxBuf[2],RxBuf[2];

	// ------ check BUSY bit in status register-1 S0 ------
	// Read Status Register-1  --05h, get S7-S0
	TxBuf[0]=W25Q_READ_STATUS_REG_1;

	int wdelay=5; //status poll interval in ms.
	int i=0;

	do {
		usleep(1000*wdelay); //----set delay........
		i++;
		if(SPI_Write_then_Read(TxBuf,1,RxBuf,1) < 1)
		{
			return -1;
		}

		//--check if BUSY(S0=1)
//		printf("Return Status Register-1: 0x%02x \n",RxBuf[0]);

		if(((RxBuf[0]) & 0x01) == 0)
			break;
//		else
//			printf("BUSY for Write/Erase.\n");

	} while(1);

	return i*wdelay; //in ms
}

/*----------------------------------------------
4K Sector erase.

Erase sector by specified 24bits address
Check STATUS to confirm completion of erasing.
return:
	0	OK
------------------------------------------------*/
int flash_sector_erase(int addr)
{
	uint8_t CmdBuf[4];
	int tmp;

	//// Sector Erase 4k-bytes --20h, execute the Write Enable first.
	CmdBuf[0]=W25Q_4K_ERASE;
	//24bits address,mupltiply of 4k=2^12
	CmdBuf[1]=(addr & 0xFF0000)>>16;
	CmdBuf[2]=(addr & 0xF000)>>8;
	CmdBuf[3]=0;

	//--- enable write before sector erase
	if(flash_write_enable()!=0)
		return -1;

	//--- send sector erase command
	if(SPI_Write(CmdBuf,4)<1)
		return -1;

	// wait erasing completion
	tmp=flash_wait_busy();

	if(tmp < 0)
	{
		printf("flash_wait_busy() fails!\n");
		return -1;
	}
	else
		printf("Finish erasing 4k sector starting from 0x%06x in %dms\n",(addr&0xFFF000),tmp);

	return 0;
}



//// 32K Block Erase 32k-bytes --52h, execute the Write Enable first.
//// 64K Block Erase 64k-bytes --D8h, execute the Write Enable first.
//// Chip Erase --C7h or 60h, execute the Write Enable first.
/*----------------------------------------------
32K or 64K Block erase.
Erase sector by specified 24bits address
Check STATUS to confirm completion of erasing.
return:
	0	OK
------------------------------------------------*/
int flash_block_erase(bool bl32k, int addr)
{
	uint8_t CmdBuf[4];
	int tmp;

	if(bl32k)
	{
		//----- 32K Block Erase --52h, execute the Write Enable first.
		CmdBuf[0]=W25Q_32K_ERASE;
		//24bits address,mupltiply of 32k=2^15 bit
		CmdBuf[1]=(addr & 0xFF0000)>>16;
		CmdBuf[2]=(addr & 0x8000)>>8;
		CmdBuf[3]=0;
	}
	else
	{
		//----- 64K Block Erase --D8h, execute the Write Enable first.
		CmdBuf[0]=W25Q_64K_ERASE;
		//24bits address,mupltiply of 64k=2^16 bit
		CmdBuf[1]=(addr & 0xFF0000)>>16;
		CmdBuf[2]=0;
		CmdBuf[3]=0;
	}

	//--- enable write before sector erase
	if(flash_write_enable()!=0)
		return -1;

	//--- send erase command
	if(SPI_Write(CmdBuf,4)<1)
		return -1;

	// wait erasing completion
	tmp=flash_wait_busy();

	if(tmp < 0)
	{
		printf("flash_wait_busy() fails!\n");
		return -1;
	}
	else
	{
		if(bl32k)
			printf("Finish erasing 32K block starting from 0x%06X in %dms\n",addr&0xFF8000,tmp);
		else
			printf("Finish erasing 64K block starting from 0x%06X in %dms\n",addr&0xFF0000,tmp);
	}

	return 0;
}


/*----------------------------------------------------------------------
Flash Write (Page Program) --02h

The Page Program instruction allows from one byte to 256 bytes(a page) of
data to be programmed at previously erased (FFh) memory locations.
A Write Enable instruction must be executed before the device will accept
the Page Program instruction.

!!!!---  because of MT7688 HW limit, The Max. payload is 4+32 bytes for each
SPI transfer.  1byte for command, 3bytes for 24bits address, so only 32bytes
available for each flash write ---!!!!

dat: data pointer
addr: where to write
cnt:  number of bytes to write


!!! LIMIT cnt=max.32 !!!
return:
	0	OK
----------------------------------------------------------------------*/
int flash_write_bytes(uint8_t *dat, int addr,int cnt)
{
	uint8_t CmdBuf[4];
	int i;

	if(cnt>32)
	{
		cnt=32;
		printf("WARNING: Max. 32bytes for each flash write. other data will be discarded!\n");
	}

	//--- Page Program Command (write) --02h
	CmdBuf[0]=W25Q_PAGE_PROGRAM;
	CmdBuf[1]=(addr & 0xFF0000)>>16;
	CmdBuf[2]=(addr & 0xFF00)>>8;
	CmdBuf[3]=(addr & 0xFF);

	//--- Write Enable before Page Program !!!
	if(flash_write_enable()!=0)
		return -1;

	//--- send write command
       	if( SPI_Write_Command_Data(CmdBuf, 4, dat, cnt) < 1)
		return -1;

	// wait write/erasing completion
	flash_wait_busy();

	//--- print data
	printf("write data to 0x%06X: 0x",addr);
	for(i=0;i<cnt;i++)
	{
		printf("%02x",dat[i]);
	}
	printf("\n");

	return 0;
}


/*----------------------------------------------------------------------
Flash Write One Page (256bytes)

NOTE: !!!--- Address NOT alinged for page ---!!!

dat: pointer to data.

return:
	0	OK
----------------------------------------------------------------------*/
int flash_write_page(uint8_t *dat, int addr)
{
	int i;
	uint8_t CmdBuf[4];
	int faddr=addr;

	//--- Page Write/Program Command --02h
	CmdBuf[0]=W25Q_PAGE_PROGRAM;

	//--- send write command
	for(i=0;i<256/32;i++)
	{
		//--- shift addr ----
		faddr+=i*32;
		CmdBuf[1]=(faddr & 0xFF0000)>>16;
		CmdBuf[2]=(faddr & 0xFF00)>>8;
		CmdBuf[3]=(faddr & 0xFF);

		//--- Write Enable before Page Program !!!
		if(flash_write_enable()!=0)
			return -1;

		//--- SPI transfer
        	if( SPI_Write_Command_Data(CmdBuf, 4, dat+i*32, 32) < 1)
			return -1;

		// wait write/erasing completion
		flash_wait_busy();
/*
		printf("write page i=%d\n",i);
		for(k=0;k<32;k++)
			printf("0x%02X",*(dat+i*32+k));
		printf("\n");
*/
	}

	return 0;
}


/*----------------------------------------------------------------------
Flash Read Data

NOTE: !!!--- Address NOT alinged for page ---!!!

addr:  flash data address
dat:   pointer to received data buf.
cnt:   number of data bytes to be read

return:
	0	OK
----------------------------------------------------------------------*/
int flash_read_data(int addr, uint8_t *dat, int cnt)
{

	int i;
	int n32,nrem;
	uint8_t CmdBuf[4];
	int faddr=addr;

	//--- Flash Read Data Command  --03h
	CmdBuf[0]=W25Q_READ_DATA;

	n32=cnt>>5;// cnt/32
	nrem=cnt%32;// remainder

	//--- read 32bytes one SPI transfer
	for(i=0;i<n32;i++)
	{
		//--- shift addr ----
		faddr+=i*32;
		CmdBuf[1]=(faddr & 0xFF0000)>>16;
		CmdBuf[2]=(faddr & 0xFF00)>>8;
		CmdBuf[3]=(faddr & 0xFF);

		if(SPI_Write_then_Read(CmdBuf,4,dat+32*i,32)<1)
			return -1;

		// wait write/erasing completion
		flash_wait_busy();

	}

	//--- read remainder
	//--- shift addr ----
	faddr+=n32*32;
	CmdBuf[1]=(faddr & 0xFF0000)>>16;
	CmdBuf[2]=(faddr & 0xFF00)>>8;
	CmdBuf[3]=(faddr & 0xFF);

	if(SPI_Write_then_Read(CmdBuf,4,dat+32*n32,nrem)<1)
		return -1;

	// wait write/erasing completion
	flash_wait_busy();

	return 0;
}

