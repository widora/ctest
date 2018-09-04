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
	uint8_t TxBuf[4]={0x90,0,0,0};
	uint8_t RxBuf[4];


	//----- init and open spi dev -----
        if( SPI_Open() != 0 )
                return -1;
	else
		printf("spi df opened successfuly!\n");

	//----- send Instruction 0x90000000 to W25Q then get MID/DID
	SPI_Write_then_Read(TxBuf,4,RxBuf,2);
	printf("Manufacturer ID: 0x%02X,  Device ID: 0x%02X \n",RxBuf[0],RxBuf[1]);

	//----- send Instruction 0x9F to W25Q then get JEDEC ID in "MF7:0,ID15:8,ID7:0"
	TxBuf[0]=0x9F;
	SPI_Write_then_Read(TxBuf,1,RxBuf,3);
	printf("Manufacturer ID: 0x%02X, Memory Type: 0x%02X, Capacity: 0x%02X \n",RxBuf[0],RxBuf[1],RxBuf[2]);


	//---- close spi dev
	SPI_Close();

}
