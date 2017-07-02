/*-----------------------------------------------------
TODOs and BUGs
1. Ting-01M always response as busy after Widora rebooting.
   ----Solved! set serial port as RAW MODE.
2. ttyS1: 1 input overrun(s)!!


--------------------------------------------------------*/

#include     <string.h>
#include     "ting_uart.h"
#include     "ting.h"


#define MAX_TING_LORA_ITEM 24 //max number of received key word(value) items seperated by ',' in RX received Ting RoLa string.

//----global var.------
int fd;
int  ndelay=5000; // us delay,!!!!!--1000us delay cause Messg receive error!!


int main(int argc, char **argv)
{
  int nb,nread,nwrite;
  char tmp;
  char *pbuff;
  char *pstrTingLoraItems[MAX_TING_LORA_ITEM]; //point array to received Ting LORA itmes 
  char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,132,4\r\n";
  char *dev ="/dev/ttyS1";

  //----- init buff and arrays ------
//  memset(g_strUserRxBuff,'\0',sizeof(g_strUserRxBuff));
   ClearUserRxBuff(); // clear g_strUserRxBuff
  memset(pstrTingLoraItems,0,sizeof(pstrTingLoraItems));

  //------ open UART interface-------
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

  //----- reset Ting-----
  sendTingCMD("AT+RST\r\n",50000);
  sleep(1);//wait long enough
  tcflush(fd,TCIOFLUSH);
  //----- configure ----
  sendTingCMD(STR_CFG,50000);
  //------ get version ------
  sendTingCMD("AT+VER?\r\n",ndelay);
  //------- set ADDR -------
  sendTingCMD("AT+ADDR=5555\r\n",ndelay);
  sendTingCMD("AT+ADDR?\r\n",ndelay);
  //----set DEST address -----
  sendTingCMD("AT+DEST=6666\r\n",ndelay);

  nb=0;
  pbuff=g_strUserRxBuff;
  //----clear tty FIFO hardware buff
  tcflush(fd,TCIOFLUSH);
  //---set RX mode
  sendTingCMD("AT+RX?\r\n",ndelay);
  while(1)
  {
		nread=read(fd,pbuff,50); //--50?????
		if(nread<0)
		{
			printf("read tty error\n");
			pbuff=g_strUserRxBuff; // reset pbuff
			nread=0;
		}
		pbuff+=nread;
		nb+=nread;
   		//if(read(fd,&tmp,50)>0)
   		//{
	      	//	g_strUserRxBuff[nb]=tmp;
		//	nb++;
		if( (nread>0) && ( *(pbuff-1)=='\n' || nb>511) ) // '\n' is the end of a string,common end \r\n
		{
			//g_strUserRxBuff[nb]='\0';
			*pbuff='\0'; // add string end
 			printf("Message Received: %s",g_strUserRxBuff);

			//----reset count and buff pointer
			nb=0;
			pbuff=g_strUserRxBuff; //reset pbuff

			//--------parse recieved data -----
			sepWordsInTingLoraStr(g_strUserRxBuff,pstrTingLoraItems);//separate key words and get total length.
			parseTingLoraWordsArray(pstrTingLoraItems);//parse key words as of commands and data

			//----get RSSI
			sendTingCMD("AT+RSSI?\r\n",ndelay);
			//---reset RX mode
			sendTingCMD("AT+RX?\r\n",ndelay);

			//----clear tty FIFO hardware buff
			tcflush(fd,TCIOFLUSH);
		}
//		usleep(ndelay);

  	}
    //close(fd);
    //exit(0);
}
