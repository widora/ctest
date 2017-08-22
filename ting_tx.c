#include     <stdint.h>
#include     <string.h>
#include     "ting_uart.h"
#include     "ting.h"

/*-----------------------      TING LoRa CONFIGURATION STRING EXAMPLE      -------------------------
Example:  char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,132,4\r\n";
434000000          RF_Freq(Hz),
       10	   Power(dBm 10 or 20 )
	6	   BW(0-9 for 0-7.8KHz; 1-10.4KHz, 2-15.6HKz, 3-20.8KHz, 4-31.2KHz,
		              5-41.6KHz, 6-62.5KHz, 7-125KHz, 8-250KHz,9-500KHz)
		   (* increase signal BW can shorten Tx time,however at cost of sensitivity performance. )
        7	   Spreading_Factor (6-12 for 2^6-2^12)
	1	   Cyclic_Error_Coding_Rate (1-4 for 1-4/5, 2-4/6, 3-4/7, 4-4/8)
	1	   CRC (0-OFF,1-ON)
	0	   Header Type (0-Explict 1-Implict) !!! Implict_Header MUST be used for SF=6 !!! 
	0	   Rx_Mode ( 0- continous RX, 1- single RX)
	0	   Frequency_Hop ( 0- OFF, 1- ON)
     3000	   RX_Packet_Timeout (1-65535ms)
      132	   4+User_DATA_Length (Valid for Implict_Header mode only, 5-255)
	4	   Preamble_Length (4-65535), long enough to span one Idle cycle,then be detectable when circuit wakes 
---------------------------------------------------------------------------------------------------*/
static  char  STR_CFG[]="AT+CFG=434000000,20,4,10,1,1,0,0,0,0,3000,8,64\r\n"; // 0x23=35 bytes playload ~5s 'LR,6666,23,TS,1503203973.725,BBBBBBBBBBBBBBBB,\r\n'
//static  char  STR_CFG[]="AT+CFG=434000000,20,2,10,1,1,0,0,0,0,3000,8,64\r\n"; // 0x23=35 bytes playload ~10s 'LR,6666,23,TS,1503203973.725,BBBBBBBBBBBBBBBB,\r\n'
//static  char  STR_CFG[]="AT+CFG=434000000,20,2,9,1,1,0,0,0,0,3000,8,128\r\n"; // 35bytes playload ~8s
//static char STR_CFG[]="AT+CFG=434000000,20,3,7,1,1,0,0,0,0,3000,8,128\r\n"; // 35bytes playload ~2.5s
//static char STR_CFG[]="AT+CFG=434000000,10,3,7,1,1,0,0,0,0,3000,8,32\r\n"; // 83bytes playload ~1.7s
//static  char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,132,4\r\n"; // 83bytes playload ~0.6s

char buff[512];
int  ndelay=2000; // us delay,!!!!!--1000us delay cause Messg receive error!!

int main(int argc, char **argv)
{
	int nb,nread,nwrite;
	int txlen;
	int intWriteErrCount=0;
	char tmp;
	char strcmd[64];
	char sendBuf[512];
	int nload;//payload length,bytes.
	uint8_t k;
	char *pbuff;

	char *uart_dev ="/dev/ttyS1";

  openUART(uart_dev); // open uart and set it.
  resetTing(g_fd, STR_CFG,0x6666, 0x5555); // reset ting with spicific parameters

  nb=0;
//  tcflush(g_fd,TCIOFLUSH);

  nload=16;//64; //128;//payload length
//  sendBuf[nload]='\0';//give and end for the string
  k='0';
  while(1)
  {
	  if(k=='~')
	      k='0';
	  else
	      k++; 
	  // load asiic char to sendBuf[]
	  memset(sendBuf,k,nload);//sizeof(sendBuf)); 

	  //----- push data to g_strUserTxBuff[] for Lora TX
	  RenewTimeStr(g_pstr_time);
	  ClearUserTxBuff();//clear g_strUserTxBuff[]
	  intPush2UserTxBuff("TS,");
	  intPush2UserTxBuff(g_pstr_time);
	  intPush2UserTxBuff(",");//sep. token
	  intPush2UserTxBuff(sendBuf);
	  intPush2UserTxBuff(",");// sep. token

	  printf("Ting-01M Sending %s\n",g_strUserTxBuff);//sendBuf);

	  txlen=strlen(g_strUserTxBuff);
	  //----send data to Ting for Lora to transmit
	  sprintf(strcmd,"AT+SEND=%d\r\n",txlen);
	  sendTingCMD(strcmd,"OK",ndelay);
	  printf("writing g_strUserTxBuff to Ting-01M...\n");
          nwrite=write(g_fd,g_strUserTxBuff,txlen);
	  if(nwrite != txlen)
		intWriteErrCount++;
	  printf("---< intWriteErrCount=%d >---\n",intWriteErrCount);

	  //---wait for transmission to finish
	  // sleep(1);//usleep(8000); send 64bytes //sleep(1) send 128bytes;//send 64bytes sf=7 usleep(500000); //send 64bytes Sf=6 sleep(3); //!!! critial !!!!
	  sendTingCMD("AT?\r\n","SENDED",ndelay); //readout all Ting-01M replies as "AT,SENDING\r\n", "AT,SENDED\r\n" and "AT,OK\r\n"
          usleep(500000); //delay after Ting finish sending data...

    }
    //close(g_fd);
    //exit(0);
}
