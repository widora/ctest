#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "cc1101.h"

//------global variables -----
int nTotal=0;
int nDATA_rec=0; // all data number received include nDATA_err, excetp CRC error.
int nDATA_err=0;
int nCRC_err=0;
int nFAIL=0;

void ExitProcess()
{
	float CRC_err_rate;
	printf("-------Exit Process------\n");
	printf("nTotal=%d\n",nTotal);
	printf("nDATA_rec=%d\n",nDATA_rec);
	printf("nDATA_err=%d\n",nDATA_err);
	printf("nCRC_err=%d\n",nCRC_err);
	printf("nFAIL=%d\n",nFAIL);
	CRC_err_rate=(float)nCRC_err/nTotal*100.0;
	printf("CRC error rate: %5.2f %%\n",CRC_err_rate);
//	exit(0);
}

//======================= MAIN =============================
int main(void)
{
	int len,i,j;
	int ret,ccret;
	int dbmRSSI;
	uint8_t Txtmp,data[32];
	unsigned char TxBuf[DATA_LENGTH];
	unsigned char RxBuf[DATA_LENGTH];

 	memset(data,0,sizeof(data));
	//memset(TxBuf,0,sizeof(TxBuf));
	memset(RxBuf,0,sizeof(RxBuf));

	//TxBuf[0]=0xac;
	//TxBuf[1]=0xab;

        //-------- Exit Signal Process ---------
        signal(SIGINT,ExitProcess);


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

	printf("----------  CC1101 Configuration --------\n");
	//---- get Modulation Format  ---
	printf("Set modulation format to    %s\n",getModFmtStr());
	//----- get symbol Kbit rate ----
        printf("Set symbol rate to       %8.1f Kbit/s\n",getKbitRate());
        //------ get carrier frequency ---
        printf("Set carrier frequency to   %8.3f MHz\n",getCarFreqMHz());
	//------ get  IF frequency ----
	printf("Set IF to                  %8.3f KHz\n",getIfFreqKHz());
	//------ get channel Bandwith ----
	printf("Set channel BW to          %8.3f KHz\n", getChanBWKHz());
	//------ get channel spacing  ----
	printf("Set channel spacing to     %8.3f KHz\n",getChanSpcKHz());

	printf("----------------------------------------\n");
	sleep(3);
        //----- transmit data -----
//        halRfSendPacket(TxBuf,DATA_LENGTH); //DATA_LENGTH);

	//----- receive data -----
	len=12;
	j=0;
  	while(1) //--- !!! Wait a little time just before setting up for next  TX_MODE !!!
	{
	   j++;
  	   ccret=halRfReceivePacket(RxBuf,DATA_LENGTH);
  	   if(ccret==1) //receive success
	   {
		nDATA_rec++;
		printf("------ nDATA_rec=%d ------\n",nDATA_rec);
		if(1)//j%10 == 0)
		{
 			printf("%dth received data:",j);
			for(i=0;i<len;i++)
				printf("%d, ",RxBuf[i]);
			printf("\n");
			printf("--------- RSSI: %ddBm --------\n",getRSSIdbm());
		}
		//---pick error data
		if(RxBuf[0]!=RxBuf[1])
		{
 			nDATA_err++;
			printf("------ nDATA_rec=%d ------\n",nDATA_err);
 			printf("%dth received data ERROR! :",j);
			for(i=0;i<len;i++)
				printf("%d, ",RxBuf[i]);
			printf("\n");
		}
	    }

  	   else if(ccret==2) // receive CRC error
	   {
 		nCRC_err++;
		printf("------ nCRC_err=%d ------\n",nCRC_err);
	   }
	   else if(ccret==0)//fail
	   {
		nFAIL++;
		printf("------ nFAIL=%d ------\n",nFAIL);
	   }

	   nTotal++;
        }//while
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
