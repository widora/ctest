/*---------------------------------------------------------------
TODOs and BUGs
1. Periodically check CC1101 working status, and reset it if necessary

-----------------------------------------------------------------*/

#include <stdio.h>
#include "cc1101.h"

//------global variables -----
struct timeval t_fstart,t_start,t_end;
long cost_time=0;
struct tm *tm_local;



//======================= MAIN =============================
int main(void)
{
	int len,i,j;
	int ret;
	uint8_t Txtmp,data[32];
	unsigned char TxBuf[DATA_LENGTH];
	unsigned char RxBuf[DATA_LENGTH];

 	memset(data,0,sizeof(data));
	memset(TxBuf,0,sizeof(TxBuf));
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
	//set SPI CLOCK = 5MHZ;MAX. SPI CLOCK=6.5MHz for burst access.
	usleep(200000);//Good for init ?
	RESET_CC1100();
	usleep(200000);
        halRfWriteRfSettings(); //--default register set

        //----set symbol rate,deviatn,filter BW properly -------
	// ----to be consisten with cc1101_rx.c ....
	setModFmt(MODFMT_MSK); //MODFMT_ASK_OOK //set modulation format
        setFreqDeviatME(0,4); //deviation: 25.4KHz
        setKbitRateME(248,10); //rate: 50kbps
	setChanBWME(0,3); //filterBW: (3,3)58KHz (0,3)102KHz
        setChanSpcME(248,2);//channel spacing: (248,2)200KHz 
        setCarFreqMHz(433,0);//(Base freq,+channel_number)

        halSpiWriteBurstReg(CCxxx0_PATABLE,PaTabel,8);//---set Power

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
	sleep(2);

        //----- transmit data -----
	len=60; // max.DATA_LENGTH-4, packet-bytes format is 1(length)+1(addr)+len(data)+2(append)
	j=0;
	for(i=0;i<len;i++)
		TxBuf[i]=i;
        while(1)
	{
		j++;
		//--renew TxBuf
		for(i=0;i<len;i++)
			TxBuf[i]=j+1;
		//--print TxBuf
		printf("%dth transmit:",j);
		for(i=0;i<len;i++){
			printf("%d, ",TxBuf[i]);
		}
		printf("\n");

		//------ cal. interval time for packets receiving ---------
                gettimeofday(&t_end,NULL);
                cost_time=t_end.tv_usec-t_start.tv_usec;
                if(cost_time < 0)
                	cost_time=cost_time+1000000*(t_end.tv_sec-t_start.tv_sec);
                printf("Transmitting interval time:%dus\n",cost_time);
                gettimeofday(&t_start,NULL);
		//---send packet
	        halRfSendPacket(TxBuf,len,TxBuf[0]); //---transmit data, TxBuf[]=0~255 is addr.
		usleep(50000); //---!!!! !CRITICAL: enough time window for receiving.

	}
	//----- receive data -----
//	halRfReceivePacket(RxBuf,DATA_LENGTH);

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
