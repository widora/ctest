/*--------------------------------------------------------------------------
Nor flash W25Q128
Total 16Mbytes, with 256 blocks(64Kbyte), each block has 16 secotrs(4kbyte),
and each sector has 16 page(256byte).


Manufacture ID         MF7:0
-----------------------------
Winbond Serial Flash   EFh


Device ID       	ID7:0              ID15:0
--------------------------------------------------
Instruction     	ABh,90h,92h,94h    9Fh
W25Q128FV(SPI Mode)	17h		   4018h
W25Q128FV(QPI Mode)     17h		   6018h


Pending:
1. Check whether left space is enough before write/program.


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

	/*     Global/Sector Unlock --98h    */
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

/*----------------------------------------------
wait for flash busy status, etc. during erasing.

return ms:
	>=0	OK
------------------------------------------------*/
int flash_wait_busy(void)
{
	uint8_t TxBuf[2],RxBuf[2];

	// ------ check BUSY bit in status register-1 S0 ------
	// Read Status Register-1  --05h, get S7-S0
	TxBuf[0]=0x05;

	int wdelay=10; //status poll interval in ms.
	int i=0;

	do {
		usleep(1000*wdelay); //----set delay........
		i++;
		if(SPI_Write_then_Read(TxBuf,1,RxBuf,1) < 1)
		{
			return -1;
		}
		//--check if BUSY(S0=1)
		printf("Return Status Register-1: 0x%02x \n",RxBuf[0]);

		if(((RxBuf[0]) & 0x01) == 1)
			printf("BUSY for Write/Erase.\n");
		else
			break;

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

//	usleep(10*1000);

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
			printf("Finish erasing 32K block starting from 0x%06x in %dms\n",addr&0xFF8000,tmp);
		else
			printf("Finish erasing 64K block starting from 0x%06x in %dms\n",addr&0xFF0000,tmp);
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

return:
	0	OK
----------------------------------------------------------------------*/
int flash_write_bytes(uint8_t *dat, int addr,int cnt)
{
	int i;

	if(cnt>32)
	{
		cnt=32;
		printf("WARNING: Max. 32bytes for each flash write. other data will be discarded!\n");
	}

	//--- Page Program --02h
	TxBuf[0]=0x02;
	TxBuf[1]=(addr & 0xFF0000)>>16;
	TxBuf[2]=(addr & 0xFF00)>>8;
	TxBuf[3]=(addr & 0xFF);

	//--- copy data to TxBuf[]
	for(i=0;i<cnt;i++)
	{
		TxBuf[4+i]=dat[i];
	}

	//--- Write Enable before Page Program !!!
	if(flash_write_enable()!=0)
		return -1;

	//--- send write command
	if(SPI_Write(TxBuf,4+cnt)<1)
		return -1;

        // wait write/erase completion
        flash_wait_busy();


	//--- print data
	printf("write data to 0x%06X: 0x",addr);
	for(i=0;i<cnt;i++)
	{
		printf("%02x",TxBuf[i+4]);
	}
	printf("\n");

	return 0;
}


/*--------------------------------------------------------------
			Main()
--------------------------------------------------------------*/
int main(void)
{

	int i;
	int addr=(55<<12)+5;//24bit addr.
	uint8_t dat[32];

	printf("set addr=0x%06x\n",addr);

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
	flash_write_enable();
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	//--- erase sector before program
	if(flash_sector_erase(addr)!=0) return -1;//24bits address, multiply of 4k
	//--- erase 64K block
//	if(flash_block_erase(false,addr)!=0) return -1;//24bits address, multiply of 4k

	for(i=0;i<32;i++)
	{
		dat[i]=i<<1;
	}
	//---- write data to flash
        flash_write_bytes(dat,addr,32);

	//	Read Data  --03h
	TxBuf[0]=0x03;
	TxBuf[1]=(addr & 0xFF0000)>>16;
	TxBuf[2]=(addr & 0xFF00)>>8;
	TxBuf[3]=(addr & 0xFF);
	SPI_Write_then_Read(TxBuf,4,RxBuf,32);
//	usleep(300000);
	printf("read data from 0x%X: 0x",addr);
	for(i=0;i<32;i++)
	{
		printf("%02x",RxBuf[i]);
	}
	printf("\n");


	//---- read Status Registers
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));


	//---  Write disable flash after use !!! 
	if(flash_write_disable()!=0)
		printf("flash_write_disable() fails!\n");

	//---- close spi dev
	printf("Close spi fd.\n");
	SPI_Close();

	return 0;
}
