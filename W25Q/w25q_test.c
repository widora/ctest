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
-------------------------------------------------------------------------------------------*/
#include "spi.h"
#include "w25q.h"
#include <stdbool.h>
#include <sys/time.h>


/*--------------------------------------------------------------
			Main()
--------------------------------------------------------------*/
int main(void)
{

	int i;
	// Flash capacity 16Mbyte = 256(Blocks)*16(Sectors)*16(Pages)*256(Bytes)
	int addr=4096*2048;//24bits address, 16MB=4K*4K
	uint8_t dat[256];
	uint8_t buf[256];

	uint8_t TxBuf[4+32]; //4=1byte instruction + 3bytes addr.
	uint8_t RxBuf[36];

	struct timeval tm_start, tm_end;
	int tm_used;

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
/*
	TxBuf[0]=0xAB;
	SPI_Write_then_Read(TxBuf,4,RxBuf,1); //followed by 3 dummy bytes, get 1-byte ID
	printf("Release Power-down and get Device ID: 0x%02X\n",RxBuf[0]);
*/
	//---- read Status Registers
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	//--- erase sector before program
//	if(flash_sector_erase(addr)!=0) return -1;//24bits address, multiply of 4k
	//--- erase 32 block(true) or 64K block(false)
	if(flash_block_erase(false,addr)!=0) return -1;//24bits address, multiply of 4k



	/* >>>>>>>>>>>>>>>>(((  Write data no more than 32 bytes  )))<<<<<<<<<<<<<<<<< */
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

//	addr=0x00; //start from beginning of the flash
//	for(i=0;i<256*16*16;i++) // 256*16*16 pages totally for a 16MB flash
		if(flash_write_page(dat, addr+i*256) !=0)return -1;
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


	/* >>>>>>>>>>>>>>>>>>>(((  Chip Erase  )))<<<<<<<<<<<<<<<<<<<< */
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



	//---- read Status Registers
	for(i=1;i<4;i++)
		printf("Status Register-%d: 0x%02X \n",i,flash_read_status(i));

	//---- close spi dev
	printf("Close spi fd.\n");
	SPI_Close();

	return 0;
}
