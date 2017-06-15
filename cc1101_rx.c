#include <stdio.h>
#include <math.h>
#include "cc1101.h"

//======================= MAIN =============================
int main(void)
{
	int len,i,j;
	int ret;
	uint8_t Txtmp,data[32];
	unsigned char TxBuf[DATA_LENGTH];
	unsigned char RxBuf[DATA_LENGTH];

 	memset(data,0,sizeof(data));
	//memset(TxBuf,0,sizeof(TxBuf));
	memset(RxBuf,0,sizeof(RxBuf));

	//TxBuf[0]=0xac;
	//TxBuf[1]=0xab;



	SPI_Open();

        //Txtmp=0x3b;
        //SPI_Write(&Txtmp,1); //clear RxBuf in cc1101
	///Txtmp=0x3b|READ_SINGLE;
        //ret=SPI_Transfer(&Txtmp,RxBuf,1,1);
	//printf("RXBYTES: ret=%d, =x%02x\n",ret,RxBuf[0]);

	//Txtmp=0x3b|READ_BURST;
	//ret=SPI_Write_then_Read(&Txtmp,1,RxBuf,2); //RXBYTES
	//printf("RXBYTES: ret=%d, =x%02x %02x\n",ret,RxBuf[0],RxBuf[1]);

/*	//--------- register operation fucntion test  -----
        //halSpiWriteReg(0x25,0x7f);
	data[0]=data[1]=data[2]=data[3]=data[4]=0xff;
	halSpiWriteBurstReg(0x25,data,5);
	halSpiReadBurstReg(0x25,RxBuf,5);
	printf("halSpiReadBurstReg(0x25,RxBuf,5) : ");
	for(i=0;i<5;i++)
	    printf("%02x",RxBuf[i]);
	printf("\n");

	printf("halSpiReadReg(0x25)=x%02x\n",halSpiReadReg(0x25));

	printf("halSpiReadReg(0x31)=x%02x\n",halSpiReadReg(0x31));
	printf("halSpiReadStatus(0x31) Chip ID: x%02x\n",halSpiReadStatus(0x31));
*/

	//-----init CC1101-----
	//set SPI CLOCK = 5MHZ
        usleep(200000);//Good for init?
        RESET_CC1100();
        usleep(200000);
	halRfWriteRfSettings();
	halSpiWriteBurstReg(CCxxx0_PATABLE,PaTabel,8);
	
	//----- get symbol Kbit rate --
        printf("Set symbol rate to %8.1f Kbit/s\n",getKbitRate());
        //------ get carrier frequency ---
        printf("Set carrier frequency to %8.3f MHz\n",getCarFreqMHz());

        //----- transmit data -----
//        halRfSendPacket(TxBuf,DATA_LENGTH); //DATA_LENGTH);

	//----- receive data -----
	len=15;
	j=0;
  	while(1)
	{
	   j++; 
  	   if(halRfReceivePacket(RxBuf,DATA_LENGTH))
	   {
		//---print once for every 20 packet.
		//printf("data received! ------\n");
		if(j%10 == 0)
		{
 			printf("%dth received data:",j);
			for(i=0;i<len;i++)
				printf("%d, ",RxBuf[i]);
			printf("\n");
		}
		//---pick error data
		if(RxBuf[0]!=0)
		{
 			printf("%dth received data ERROR! :",j);
			for(i=0;i<len;i++)
				printf("%d, ",RxBuf[i]);
			printf("\n");
		}
	  }
        }

/*
	//---- check status ---- 
	for(i=0;i<10;i++)
	{
	 	usleep(100);
		printf("Status Byte:0x%02x\n",halSpiGetStatus());
	}
*/

	SPI_Close();
}
