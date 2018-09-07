/*--------------------------------------------------------------------------------------
Nor flash W25Q128
Total 16Mbytes, with 256 blocks(64Kbyte), each block has 16 secotrs(4kbyte),
and each sector has 16 page(256byte).
16Mbyte = 256(Blocks)*16(Sectors)*16(Pages)*256(Bytes)

Manufacture ID         MF7:0
-----------------------------
Winbond Serial Flash   EFh


Device ID       	ID7:0              ID15:0
--------------------------------------------------
Instruction     	ABh,90h,92h,94h    9Fh
W25Q128FV(SPI Mode)	17h		   4018h
W25Q128FV(QPI Mode)     17h		   6018h


Note:
0. Use default set WPS=CMP=BP1=BP2=BP3=0, no protection for any address.
1. Flash write speed is abt. 125kBytes/s for single SPI transfer with 18MHz clock.
2. Check BUSY bit of Status Register-1 first to make sure the flash chip is ready.
-------------------------------------------------------------------------------------------*/
#include "spi.h"
#include "w25q.h"
#include <stdbool.h>
#include <sys/time.h>
#include <stdio.h>

/*------------------------------------------
       compare two uint8_t array
return:
	0    same
	-1   different
------------------------------------------*/
int comp_u8array(uint8_t *src, uint8_t *dest, int n)
{
	int i;

	for(i=0;i<n;i++)
	{
		if( *(src+i) != *(dest+i) ) return -1;
	}

	return 0;
}

/*-------------------------------------------------------
			Main()
-------------------------------------------------------*/
int main(void)
{
	int i,mk;
	int nblock;
	// Flash capacity 16Mbyte = 256(Blocks)*16(Sectors)*16(Pages)*256(Bytes)
	int addr=0;//24bits address, 16MB=4K*4K
	uint8_t dat[256];
	uint8_t buf[256];

	struct timeval tm_start, tm_end;
	int tm_used;

	//--- prepare test data
	for(i=0;i<256;i++)
	{
		dat[i]=0;//i;
		buf[i]=0;//i;
	}
	//--- check comp_u8array()
	buf[222]=9;
	if(comp_u8array(dat,buf,256) != 0)
		printf("dat[] and buf[] differs!\n");


	//--- confirm address
	printf("set addr=0x%06X\n",addr);

	//----- init and open spi dev -----
        if( SPI_Open() != 0 )
                return -1;
	else
		printf("spi df opened successfuly!\n");


	/* >>>>>>>>>>>>>>>>(((  Read Chip IDs  )))<<<<<<<<<<<<<<<<< */
	flash_read_IDs();

	/*	Release Power-down/Device ID(ABh)	 */
/*
	TxBuf[0]=0xAB;
	SPI_Write_then_Read(TxBuf,4,RxBuf,1); //followed by 3 dummy bytes, get 1-byte ID
	printf("Release Power-down and get Device ID: 0x%02X\n",RxBuf[0]);
*/

	//---- read Status Registers
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	/* >>>>>>>>>>>>>>>>((( check whether the chip is busy, then soft-reset  )))<<<<<<<<<<<<<<<<< */
	if(flash_is_busy())
	{
		printf("\n!!!--- The flash chip is busy or in power-down state now ---!!! \n\nMaybe previous erase/write operation not finished yet!. \
Try it later.\n");
//		return 0;
		//------ soft reset the device -------
		printf("Don't want to wait,soft reset the chip anyway ....\n\n");
		flash_soft_reset();
	}


	/* >>>>>>>>>>>>>>>>(((  enter power-down then release  )))<<<<<<<<<<<<<<<<< */
	printf("\nNow enter Power-down state,the chip will accpet no command unitl you release it.\n");
	flash_power_down();
	printf("Put Y/y to release power-down state, or others to retain power-down state. : ");
	char cin=getchar();
	printf("\n");
	if( cin=='Y' || cin=='y' )
	{
		printf("Release Power-down now...\n\n");
		flash_release_power_down();
	}

	/* >>>>>>>>>>>>>>>>(((  Write data no more than 32 bytes  )))<<<<<<<<<<<<<<<<< */
	//--- erase sector before program
//	if(flash_sector_erase(addr)!=0) return -1;//24bits address, multiply of 4k
	//--- erase 32 block(true) or 64K block(false)
	if(flash_block_erase(false,addr)!=0) return -1;//24bits address, multiply of 4k

        flash_write_bytes(dat,addr,32);
	//---- read back to confirm
	flash_read_data(addr, buf, 32);
	printf("read data from 0x%06X: 0x",addr);
	for(i=0;i<32;i++)
	{
		printf("%02x",buf[i]);
	}
	printf("\n");


	/* >>>>>>>>>>>>>>>>>>>(((  flash write one page  )))<<<<<<<<<<<<<<<<<<<< */
	// abt. 2ms for each page, so write speed is abt. 125k/s.
	// erase sector before write/program
	if(flash_sector_erase(addr)!=0) return -1;//24bits address, multiply of 4k

	printf(".....start flash_write_page()...........>>>>> \n");
	gettimeofday(&tm_start,NULL);

	if(flash_write_page(dat, addr) !=0)return -1;

	gettimeofday(&tm_end,NULL);
	printf(".....finish flash_write_page()..........<<<<< ");
	tm_used=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
	printf(" time cost: %dms \n",tm_used);

	//---- read one page data back to confirm
	flash_read_data(addr, buf, 256);
	printf("read data from 0x%06X: 0x",addr);
	for(i=0;i<256;i++)
	{
		printf("%02x",buf[i]);
	}
	printf("\n");


	/* >>>>>>>>>(((  Scan whole chip to check defective pages  )))<<<<<<<<<< */
	nblock=256; //total 256  64k-blocks for a 16MB chip.
	printf("Write and read data to check defective page if any ...\n");
	for(mk=0;mk<nblock;mk++)
	{
	    /* >>>>>>>>>>>>>>>>(((  write/read each 64k-block = 16*16pages  )))<<<<<<<<<<<<<<< */
		// erase 64k block before write/program
		if(flash_block_erase(false, mk*64*1024)!=0) return -1;

		//////////////
//		printf(".....start write a 64k-block ...........>>>>> \n");
//		gettimeofday(&tm_start,NULL);

		for(i=0;i<16*16;i++) // 64k=16*16*page(256B)
			if(flash_write_page(dat, mk*64*1024+i*256) !=0)return -1;

//		gettimeofday(&tm_end,NULL);
//		printf(".....finish write a 64k-block ..........<<<<< ");
//		tm_used=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
//		printf(" time cost: %dms \n",tm_used);
		///////////////

	    /* >>>>>>>>>>>>>(((  check data for a 64k-block  )))<<<<<<<<<<<<<<<<< */
//		printf("read back and check integrity of data...\n");
		for(i=0;i<16*16;i++) //64k=16*16*page(256B)
		{
			flash_read_data(mk*64*1024+i*256, buf, 256);
			if(comp_u8array(dat,buf,256) != 0)
				printf("Error data found in page(0-255): %d of 64k-block(0-255): %d \n ",i,mk);
		}
		//---- read one page data to confirm
/*
		flash_read_data(addr+9*256, buf, 256);
		printf("read data from 0x%06X: 0x",addr+9*256);
		for(i=0;i<256;i++)
		{
			printf("%02x",buf[i]);
		}
		printf("\n");
*/
	}


	/* >>>>>>>>>>>>>>>>>>>(((  Chip Erase  )))<<<<<<<<<<<<<<<<<<<< */
/*
        printf(".....start flash_chip_erase()...........>>>>> \n");
        gettimeofday(&tm_start,NULL);
	flash_chip_erase();
        gettimeofday(&tm_end,NULL);
        printf(".....finish flash_chip_erase()..........<<<<< ");
        tm_used=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
        printf(" time cost: %dms \n",tm_used);

        //---- read written page data back to confirm
        flash_read_data(addr, buf, 256);
        printf("After chip erasing, read data from 0x%06X: 0x",addr);
        for(i=0;i<256;i++)
        {
                printf("%02x",buf[i]);
        }
        printf("\n");
*/


	//---- read Status Registers
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	//---- close spi dev
	printf("Close spi fd.\n");
	SPI_Close();

	return 0;
}
