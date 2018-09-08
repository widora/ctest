/*-------------------------------------------------------------------------------
	<<<<< Nor Flash Chip  W25Q128FV  Operation Test >>>>>

Total 16Mbytes, with 256 blocks(64Kbyte), each block has 16 secotrs(4kbyte),
and each sector has 16 page(256byte).
16Mbyte = 256(Blocks)*16(Sectors)*16(Pages)*256(Bytes)

Operation Tests inclueds:
	1. Read Chip IDs.
	2. Check whether the chip is busy, then soft-reset.
	3. Enter power-down state to protect write/read, then release the state.
	4. Write data (no more than 32 bytes) to memory, then read back and print.
	5. Write one page(256Bytes) data to flash, then read back and print.
	6. Write/Read/Check whole memory for defective pages.
	   6.1  erase 64_block first
	   6.2  write all 0 to each page of the block !!!!!
	   6.3  read back each page and check the data
	7. Whole chip_erase test.

Note:
0. Use default set WPS=CMP=BP1=BP2=BP3=0, no protection for any address.
1. Flash write speed is abt. 125kBytes/s for single SPI transfer with 18MHz clock.
2. Check BUSY bit of Status Register-1 first to make sure the flash chip is ready.
-----------------------------------------------------------------------------------*/
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
	-2   other error
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
		dat[i]=0;//0xff;//i;
		buf[i]=0;//xff;//i;
	}
	//--- check comp_u8array()
	buf[22]=9;
	if(comp_u8array(dat,buf,256) != 0)
		printf("dat[] and buf[] differs!\n");

	//--- confirm address
	printf("set addr=0x%06X\n",addr);

	//----- init and open spi dev -----
        if( SPI_Open() != 0 )
                return -1;
	else
		printf("spi df opened successfuly!\n");


	/* >>>>>>>>>>>>>>>>(((  1. Read Chip IDs  )))<<<<<<<<<<<<<<<<< */
	flash_read_IDs();

	//---- read Status Registers
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	/* >>>>>>>>>>>>>>>>((( 2. Check whether the chip is busy, then soft-reset  )))<<<<<<<<<<<<<<<<< */
	if(flash_is_busy())
	{
		printf("\n!!!--- The flash chip is busy or in power-down state now ---!!! \n\nMaybe previous erase/write operation not finished yet!. \
Try it later.\n");
//		return 0;
		//------ soft reset the device -------
		printf("Don't want to wait,soft reset the chip anyway ....\n\n");
		flash_soft_reset();
	}


	/* >>>>>>>>>>>>>>>>(((  3. Enter power-down state then release  )))<<<<<<<<<<<<<<<<< */
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

	/* >>>>>>>>>>>>>>>>(((  4. Write data no more than 32 bytes  )))<<<<<<<<<<<<<<<<< */
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


	/* >>>>>>>>>>>>>>>>>>>(((  5. Flash write one page  )))<<<<<<<<<<<<<<<<<<<< */
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


	/* >>>>>>>>>(((  6. Write/Read/Check memory for defective pages  )))<<<<<<<<<< */
	nblock=256; //total 256  64k-blocks for a 16MB chip.
	printf("Write/Read/Check to find defective page if any ...\n");
	for(mk=0;mk<nblock;mk++)
	{
	    int j=0; //for error page counter
	    /* >>>>>>>>>>>>>>>>(((  write/read each 64k-block = 16*16pages  )))<<<<<<<<<<<<<<< */
		// erase 64k block before write/program
		if(flash_block_erase(false, mk*64*1024)!=0) return -1;
		//////////////
//		printf(".....start write a 64k-block ...........>>>>> \n");
//		gettimeofday(&tm_start,NULL);
		printf("Write/Read/Check test for 64k_block no.%d ... ",mk);
		for(i=0;i<16*16;i++) //64k=16(sectors)*16(pages)*256B,one page=256Byte
		{
			if(flash_write_page(dat, mk*64*1024+i*256) !=0)return -1;
			if(flash_read_data(mk*64*1024+i*256, buf, 256) !=0)return -1;
			if(comp_u8array(dat,buf,256) != 0)
			{
				printf("\n  Error data found in page no.%d of 64k_block no.%d. ",i,mk);
				j++;
			}
		}
		if(j==0)printf("...OK\n");
		else printf("\n");
//		gettimeofday(&tm_end,NULL);
//		printf(".....finish write a 64k-block ..........<<<<< ");
//		tm_used=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
//		printf(" time cost: %dms \n",tm_used);
		///////////////

	}


	/* >>>>>>>>>>>>>>>>>>>((( 7. Chip Erase Test )))<<<<<<<<<<<<<<<<<<<< */
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


	//---- read Status Registers after operation
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	//---- close spi dev
	printf("Close spi fd.\n");
	SPI_Close();

	return 0;
}
