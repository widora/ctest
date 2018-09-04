/*-----------------------------------------------------
Manufacture ID     MF7-MF0
Winbond Serial Flash   EFh

Device ID       	ID7:0              ID15:0
--------------------------------------------------
Instruction     	ABh,90h,92h,94h    9Fh
W25Q128FV(SPI Mode)	17h		   4018h
W25Q128FV(QPI Mode)     17h		   6018h



-----------------------------------------------------*/


#include "spi.h"

int main()
{
	uint8_t TxBuf[16];
	uint8_t RxBuf[16];
	int i;

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

	/*	Write Enable --06h	*/


	//// Sector Erase 4k-bytes --20h, execute the Write Enable first.
	//// 32K Block Erase 32k-bytes --52h, execute the Write Enable first.
	//// 64K Block Erase 64k-bytes --D8h, execute the Write Enable first.
	//// Chip Erase --C7h or 60h, execute the Write Enable first.

	/*	Page Program --02h
	The Page Program instruction allows from one byte to 256 bytes(a page) of data to be programmed at
	previously erased (FFh) memory locations. A Write Enable instruction must be executed before the
	device will accept the Page Program instruction.
	*/

	/*	Write Disable --04h	*/


	//	Read Data  --03h



	//---- close spi dev
	SPI_Close();

}
