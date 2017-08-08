/*-----------------------------------------------------
TODOs and BUGs
1. Ting-01M always response as busy after Widora rebooting.
   ----Solved! set serial port as RAW MODE.
2. ttyS1: 1 input overrun(s)!!  --solved.
3. Rules for ting LoRa string.
4. parse LoRa string,get data and commands, execut commands then. 


--------------------------------------------------------*/
#include   <string.h>
#include   "msg_common.h"
#include   "ting_uart.h"
#include   "ting.h"


//=================== MAIN FUNCTIONS ================

int main(int argc, char **argv)
{
  int nb,nread,nwrite;
  char tmp;

  char *pbuff;
  char *pstrTingLoraItems[MAX_TING_LORA_ITEM]; //point array to received Ting LORA itmes 
  char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,132,4\r\n";
  char *uart_dev ="/dev/ttyS1";
  //--- for IPC message------
  int msg_id=-1;
  int msg_key=MSG_KEY_OLED_TEST;

  //---- create message queue ------
  if((msg_id=createMsgQue(msg_key))<0)
  {
        printf("create message queue fails!\n");
        exit(-1);
  }


  //----- init buff and arrays ------
  //  memset(g_strUserRxBuff,'\0',sizeof(g_strUserRxBuff));
   ClearUserRxBuff(); // clear g_strUserRxBuff
   memset(pstrTingLoraItems,0,sizeof(pstrTingLoraItems));

  //------ open UART interface-------
  g_fd = OpenDev(uart_dev);
  if (g_fd>0)
         set_speed(g_fd,115200);
  else
 	{
	 	printf("Can't Open Serial Port!\n");
 		exit(0);
 	}

  //----set databits,stopbits,parity for UART -----
  if (set_Parity(g_fd,8,1,'N')== false) //set_Prity(fd,databits,stopbits,parity)
  {
    printf("Set Parity Error\n");
    exit(1);
  }

  //----- reset ting -----
  sendTingCMD("AT+RST\r\n",50000);
  sleep(1);//wait long enough
  tcflush(g_fd,TCIOFLUSH);
  //----- configure ----
  sendTingCMD(STR_CFG,50000);
  //------ get version ------
  sendTingCMD("AT+VER?\r\n",g_ndelay);
  //------- set ADDR -------
  sendTingCMD("AT+ADDR=5555\r\n",g_ndelay);
  sendTingCMD("AT+ADDR?\r\n",g_ndelay);
  //----set DEST address -----
  sendTingCMD("AT+DEST=6666\r\n",g_ndelay);
  //----set PD0 as RX indication -----
  sendTingCMD("AT+ACK=1\r\n",g_ndelay);

  while(1)
  {
	//---- set RX and get LORA message
	printf("start recvTingLoRa()...\n");
	recvTingLoRa();

	//--------parse recieved data -----
	printf("start sepWordsInTingLoraStr()...\n");
	sepWordsInTingLoraStr(g_strUserRxBuff,pstrTingLoraItems);//separate key words and get total length.
	printf("start parseTingLoraWordsArray()...\n");
	parseTingLoraWordsArray(pstrTingLoraItems);//parse key words as of commands and data

	//----get RSSI
	printf("start sendTingCMD(AT+RSSI?)...\n");
	sendTingCMD("AT+RSSI?\r\n",g_ndelay);

	//---- summary ---
        printf("-----< g_intRcvCount=%d, g_intErrCount=%d  g_intMissCount=%d g_intEscapeReadLoopCount=%d >-----\n" \
	,g_intRcvCount,g_intErrCount,g_intMissCount,g_intEscapeReadLoopCount);

	//----- send data to IPC Message Queue--------
	printf("start sendMsgQue()...\n");
        if(sendMsgQue(msg_id,MSG_TYPE_TING,g_strAtBuff)!=0)
		printf("Send message queue failed!\n");

   }
    //delete IPC message queue
    //close(g_fd);
    //exit(0);
}
