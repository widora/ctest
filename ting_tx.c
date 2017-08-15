#include     <stdint.h>
#include     <string.h>
#include     "ting_uart.h"
#include     "ting.h"

//----global var.------
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
	char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,8,4\r\n";
//        char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,132,4\r\n";
	char *uart_dev ="/dev/ttyS1";

  openUART(uart_dev); // open uart and set it.
  resetTing(g_fd, STR_CFG,0x6666, 0x5555); // reset ting with spicific parameters

  nb=0;
//  tcflush(g_fd,TCIOFLUSH);

  nload=64; //128;//payload length
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
