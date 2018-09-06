/*--------------------------------------------------------------------------
Nor flash W25Q128
Total 16Mbytes, with 256 blocks(64Kbyte), each block has 16 secotrs(4kbyte),
and each sector has 16 page(256byte).
16Mbyte = 256*16*16*256 byte

Manufacture ID         MF7:0
-----------------------------
Winbond Serial Flash   EFh


Device ID       	ID7:0              ID15:0
--------------------------------------------------
Instruction     	ABh,90h,92h,94h    9Fh
W25Q128FV(SPI Mode)	17h		   4018h
W25Q128FV(QPI Mode)     17h		   6018h


Pending:
1. Check whether space is enough before write/program.
2. Max. load is 36bytes for each half-dual SPI transfer.
3. TxBuf[] and RxBuf[] shall be independent in functions.
--------------------------------------------------------------------------*/

#include <stdbool.h>
#include "spi.h"

uint8_t TxBuf[4+32]; //4=1byte instruction + 3bytes addr.
uint8_t RxBuf[36];

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
			TxBuf[0]=0x05;
			break;
		case 2:
			TxBuf[0]=0x35;
			break;
		case 3:
			TxBuf[0]=0x15;
			break;
		default:
			TxBuf[0]=0x05;
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
	TxBuf[0]=0x06;
	if(SPI_Write(TxBuf,1) < 1)
		return -1;

	/*     Global/Sector Unlock --98h    necessary -----------????????  */
	TxBuf[0]=0x98;
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
	TxBuf[0]=0x04;
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
	TxBuf[0]=0x05;

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
4K sector erase.
Erase sector by specified 24bits address
Check STATUS to confirm completion of erasing.
return:
	0	OK
------------------------------------------------*/
int flash_sector_erase(int addr)
{
	uint8_t TxBuf[2],RxBuf[2];
	int tmp;

	//// Sector Erase 4k-bytes --20h, execute the Write Enable first.
	TxBuf[0]=0x20;
	//24bits address,mupltiply of 4k=2^12
	TxBuf[1]=(addr & 0xFF0000)>>16;
	TxBuf[2]=(addr & 0xF000)>>8;
	TxBuf[3]=0;

	//--- enable write before sector erase
	if(flash_write_enable()!=0)
		return -1;

	//--- send sector erase command
	if(SPI_Write(TxBuf,4)<1)
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
	uint8_t TxBuf[2],RxBuf[2];
	int tmp;

	if(bl32k)
	{
		//----- 32K Block Erase --52h, execute the Write Enable first.
		TxBuf[0]=0x52;
		//24bits address,mupltiply of 32k=2^15 bit
		TxBuf[1]=(addr & 0xFF0000)>>16;
		TxBuf[2]=(addr & 0x8000)>>8;
		TxBuf[3]=0;
	}
	else
	{
		//----- 64K Block Erase --D8h, execute the Write Enable first.
		TxBuf[0]=0xD8;
		//24bits address,mupltiply of 64k=2^16 bit
		TxBuf[1]=(addr & 0xFF0000)>>16;
		TxBuf[2]=0;
		TxBuf[3]=0;
	}

	//--- enable write before sector erase
	if(flash_write_enable()!=0)
		return -1;

	//--- send erase command
	if(SPI_Write(TxBuf,4)<1)
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

	//--- Page Program Command --02h
	CmdBuf[0]=0x02;
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
	int i,k;
	uint8_t CmdBuf[4];
	int faddr=addr;

	//--- Page Write/Program Command --02h
	CmdBuf[0]=0x02;

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
	CmdBuf[0]=0x03;

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


/*--------------------------------------------------------------
			Main()
--------------------------------------------------------------*/
int main(void)
{

	int i;
	int addr=(1024<<12);//24bit addr.
	uint8_t dat[256];
	uint8_t buf[256];

	//--- prepare test data
	for(i=0;i<256;i++)
	{
		dat[i]=i;
	}
	//--- confirm address
	printf("set addr=0x%06X\n",addr);


	//----- init and open spi dev -----
        if( SPI_Open() != 0 )
                return -1;
	else
		printf("spi df opened successfuly!\n");

	//----- send Instruction 0x90000000 to W25Q then get MID/DID
	TxBuf[0]=0x90;
	SPI_Write_then_Read(TxBuf,4,RxBuf,2);
	printf("Manufacturer ID: 0x%02X,  Device ID: 0x%02X \n",RxBuf[0],RxBuf[1]);

	//----- send Instruction 0x9F to W25Q then get JEDEC ID in "MF7:0,ID15:8,ID7:0"
	TxBuf[0]=0x9F;
	SPI_Write_then_Read(TxBuf,1,RxBuf,3);
	printf("Manufacturer ID: 0x%02X, Memory Type: 0x%02X, Capacity: 0x%02X \n",RxBuf[0],RxBuf[1],RxBuf[2]);

	//-----	Read Unique ID --4Bh
	TxBuf[0]=0x4B;
	SPI_Write_then_Read(TxBuf,5,RxBuf,8); //4-dummy bytes,then 64-bits UID
	printf("64-bits Unique ID: 0x");
	for(i=0;i<8;i++)
		printf("%02X",RxBuf[i]);
	printf("\n");


	/*	Release Power-down/Device ID(ABh)	 */
	TxBuf[0]=0xAB;
	SPI_Write_then_Read(TxBuf,4,RxBuf,1); //followed by 3 dummy bytes, get 1-byte ID
	printf("Release Power-down and get Device ID: 0x%02X\n",RxBuf[0]);

	//---- read Status Registers
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	//--- erase sector before program
//	if(flash_sector_erase(addr)!=0) return -1;//24bits address, multiply of 4k
	//--- erase 32 block(true) or 64K block(false)
	if(flash_block_erase(false,addr)!=0) return -1;//24bits address, multiply of 4k



	//================= write data(max.32) to flash ==================
        flash_write_bytes(dat,addr,32);
	//---- read back to confirm
	flash_read_data(addr, buf, 32);
	printf("read data from 0x%06X: 0x",addr);
	for(i=0;i<32;i++)
	{
		printf("%02x",buf[i]);
	}
	printf("\n");


	//================= flash write one page ===============
	//erase sector before write/program
	if(flash_sector_erase(addr)!=0) return -1;//24bits address, multiply of 4k
	printf("start flash_write_page()...\n");
	flash_write_page(dat, addr);
	printf("finish flash_write_page().......................\n");

	//---- read one page data back to confirm
	flash_read_data(addr, buf, 256);
	printf("read data from 0x%06X: 0x",addr);
	for(i=0;i<256;i++)
	{
		printf("%02x",buf[i]);
	}
	printf("\n");



	//---- read Status Registers
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	//---- close spi dev
	printf("Close spi fd.\n");
	SPI_Close();

	return 0;
}
