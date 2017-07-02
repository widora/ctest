#include     <stdint.h>
#include     <string.h>
#include     "ting_uart.h"
#include     "ting.h"

//----global var.------
int fd;
char buff[512];
int  ndelay=2000; // us delay,!!!!!--1000us delay cause Messg receive error!!


int main(int argc, char **argv)
{
	int nb,nread,nwrite;
	char tmp;
	char strcmd[64];
	char sendBuf[512];
	int nload;//payload length,bytes.
	uint8_t k;
	char *pbuff;
	char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,8,4\r\n";
	char *dev ="/dev/ttyS1";

	fd = OpenDev(dev);
	if (fd>0)
    set_speed(fd,115200);
	else
		{
		printf("Can't Open Serial Port!\n");
		exit(0);
		}
  //----set databits,stopbits,parity for UART -----
  if (set_Parity(fd,8,1,'N')== false) //set_Prity(fd,databits,stopbits,parity)
  {
    printf("Set Parity Error\n");
    exit(1);
  }

  memset(buff,'\0',sizeof(buff));

 //----- reset -----
  sendTingCMD("AT+RST\r\n",50000);
  sleep(1);//wait long enough
  tcflush(fd,TCIOFLUSH);

  //----- configure ----
  sendTingCMD(STR_CFG,50000);
  //------ get version ------
  sendTingCMD("AT+VER?\r\n",ndelay);
  //------- set ADDR -------
  sendTingCMD("AT+ADDR=6666\r\n",ndelay);
  sendTingCMD("AT+ADDR?\r\n",ndelay);
  //----set DEST address -----
  sendTingCMD("AT+DEST=5555\r\n",ndelay);

  nb=0;
  tcflush(fd,TCIOFLUSH);
   
  nload=64; //128;//payload length
  sendBuf[nload]='\0';//give and end for the string
  k='0';
  while(1)
  {
	  if(k=='~')
	      k='0';
	  else
	      k++; 
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
	  sprintf(strcmd,"AT+SEND=%d\r\n",strlen(g_strUserTxBuff));

	  sendTingCMD(strcmd,ndelay);
	  printf("writing g_strUserTxBuff to Ting-01M...\n");
          write(fd,g_strUserTxBuff,strlen(g_strUserTxBuff));
	  usleep(600000);// send 64bytes //sleep(1) send 128bytes;//send 64bytes sf=7 usleep(500000); //send 64bytes Sf=6 sleep(3); //!!! critial !!!!
	  sendTingCMD("AT?\r\n",ndelay); //readout send result
    }
    //close(fd);
    //exit(0);
}
