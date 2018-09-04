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
	int addr;//24bit addr.

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
	TxBuf[0]=0x06;
	SPI_Write(TxBuf,1);

	//// Sector Erase 4k-bytes --20h, execute the Write Enable first.
	TxBuf[0]=0x20;
	addr=25<<12;//24bits address,mupltiply of 4k.
	TxBuf[1]=(addr&0xFF0000)>>16;
	TxBuf[2]=(addr&0xFF00)>>8;
	TxBuf[3]=addr&0xFF;
	SPI_Write(TxBuf,4);

	//// ------ check BUSY bit in status register-1 S0 ------
	// Read Status Register-1  --05h, get S7-S0
	TxBuf[0]=0x05;
	i=0;
	do {
		SPI_Write_then_Read(TxBuf,1,RxBuf,1);
		//--check if BUSY(S0=1)
		if(( (RxBuf[0]) & 0x01 ) == 1)
			printf("Write/Erase is BUSY.\n");
		else
			break;
		i++;
		usleep(10000);
	} while(1);

	printf("Finish erasing 4k sector starting from 0x%06x in %dms\n",addr,i*10);

	//// 32K Block Erase 32k-bytes --52h, execute the Write Enable first.
	//// 64K Block Erase 64k-bytes --D8h, execute the Write Enable first.
	//// Chip Erase --C7h or 60h, execute the Write Enable first.



	/*	Write Enable --06h	*/
	TxBuf[0]=0x06;
	SPI_Write(TxBuf,1);

	/*	Page Program --02h
	The Page Program instruction allows from one byte to 256 bytes(a page) of data to be programmed at
	previously erased (FFh) memory locations. A Write Enable instruction must be executed before the
	device will accept the Page Program instruction.
	*/
	TxBuf[0]=0x02;
	TxBuf[1]=(addr&0xFF0000)>>16;
	TxBuf[2]=(addr&0xFF00)>>8;
	TxBuf[3]=addr&0xFF;
	TxBuf[4]=0x99;
	TxBuf[5]=0x66;
	SPI_Write(TxBuf,6);
	usleep(100000);
	printf("write data: 0x%x,0x%x\n",TxBuf[4],TxBuf[5]);
	/*	Write Enable --06h	*/
//	TxBuf[0]=0x06;
//	SPI_Write(TxBuf,1);



	/*	Write Disable --04h	*/


	//	Read Data  --03h
	TxBuf[0]=0x03;
	TxBuf[1]=(addr&0xFF0000)>>16;
	TxBuf[2]=(addr&0xFF00)>>8;
	TxBuf[3]=addr&0xFF;
	SPI_Write_then_Read(TxBuf,4,RxBuf,2);
	usleep(100000);
	printf("read data: 0x%x,0x%x\n",RxBuf[0],RxBuf[1]);



	//---- close spi dev
	SPI_Close();

	return 0;
}
