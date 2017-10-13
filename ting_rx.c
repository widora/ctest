/*-----------------------------------------------------
TODOs and BUGs
1. Ting-01M always response as busy after Widora rebooting.
   ----Solved! set serial port as RAW MODE.
2. ttyS1: 1 input overrun(s)!!  --solved.
3. 'AT+ACK=1\r\n' return with 'ERR:CMD' --ting firmware
4. Rules for ting LoRa string.
5. parse LoRa string,get data and commands, execut commands then. 

--------------------------------------------------------*/
#include   <string.h>
#include   "msg_common.h"
#include   "ting_uart.h"
#include   "ting.h"

/*-----------------------      TING LoRa CONFIGURATION STRING EXAMPLE      -------------------------
Example:  char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,132,4\r\n";
434000000          RF_Freq(Hz),
       10          Power(dBm 10 or 20 )
        6          BW(0-9 for 0-7.8KHz; 1-10.4KHz, 2-15.6HKz, 3-20.8KHz, 4-31.2KHz,
                              5-41.6KHz, 6-62.5KHz, 7-125KHz, 8-250KHz,9-500KHz)
                   (* increase signal BW can shorten Tx time,however at cost of sensitivity performance. )
        7          Spreading_Factor (6-12 for 2^6-2^12)
        1          Cyclic_Error_Coding_Rate (1-4 for 1-4/5, 2-4/6, 3-4/7, 4-4/8)
        1          CRC (0-OFF,1-ON)
        0          Header Type (0-Explict 1-Implict) !!! Implict_Header MUST be used for SF=6 !!! 
        0          Rx_Mode ( 0- continous RX, 1- single RX)
        0          Frequency_Hop ( 0- OFF, 1- ON)
     3000          RX_Packet_Timeout (1-65535ms)
      132          4+User_DATA_Length (Valid for Implict_Header mode only, 5-255)
        4          Preamble_Length (4-65535),long enough to span one Idle cycle,then be detectable when circuit wakes
---------------------------------------------------------------------------------------------------*/
static  char  STR_CFG[]="AT+CFG=434000000,10,4,10,1,1,0,0,0,0,3000,8,64\r\n";


//=================== MAIN FUNCTIONS ================
int main(int argc, char **argv)
{
  int nb,nread,nwrite;
  int intRSSI;
  char tmp;
  int  nRecLoRa=0; //bytes of received LoRa data
  int nitem; // numbers of separated items in received Ting-LoRa data 
  char *pbuff;
  char *pstrTingLoraItems[MAX_TING_LORA_ITEM]; //point array to received Ting LORA itmes 
  char *uart_dev ="/dev/ttyS1";
  //--- for IPC message------
  int msg_id=-1;
  int msg_key=MSG_KEY_OLED_TEST;
  char strmsg[32]={0};  

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

  openUART(uart_dev); // open uart and set it.
  resetTing(g_fd, STR_CFG,0x5555, 0x6666); // reset ting with spicific parameters

  while(1)
  {
        //---- to confirm that Ting is active 
	if(checkTingActive() != 0)
	{
		 printf("checkTingActive() fails! reset Ting ...\n");
		 resetTing(g_fd, STR_CFG,0x5555, 0x6666); // (selfaddr,destaddr) reset ting with spicific parameters
	}
	//------ to confirm self addr.
	printf("------------------confirme addr.--------------------\n");
	sendTingCMD("AT+ADDR?\r\n","OK",g_ndelay);

	//---- set RX and get LORA message
	printf("start recvTingLoRa()...\n");
	nRecLoRa=recvTingLoRa(); // total bytes include '\r\n'
	printf("Totally %d bytes of LoRa data received from Ting-01M.\n",nRecLoRa);

	//--------parse recieved data -----
	nitem=sepWordsInTingLoraStr(g_strUserRxBuff,pstrTingLoraItems);//separate key words and get total length.
	printf("sepWordsInTingLoraStr()...get %d items in LoRa data.\n",nitem);
	printf("start parseTingLoraWordsArray()...\n");
	parseTingLoraWordsArray(pstrTingLoraItems);//parse key words as of commands and data

	//----get RSSI, only if received string is valid and complete.
	if(nitem == 7)
	{
/*
		printf("start sendTingCMD(AT+RSSI?)...\n");
		sendTingCMD("AT+RSSI?\r\n","OK",g_ndelay); // RSSI in returned g_strAtBuff[]
		//----- send data to IPC Message Queue--------
		printf("start sendMsgQue()...\n");
        	if(sendMsgQue(msg_id,MSG_TYPE_TING,g_strAtBuff)!=0)
			printf("Send message queue failed!\n");
*/
		if(sendTingCMD("AT+RSSI?\r\n","OK",g_ndelay)==0) // RSSI in returned g_strAtBuff[]
		{
		   intRSSI=atoi(g_strAtBuff+3);
		   sprintf(strmsg,"%d  %ddBm",g_intRcvCount,intRSSI);
		   printf("----- %s -----\n",strmsg);
		}
		else
		{
		   sprintf(strmsg,"%d",g_intRcvCount);
		}

        	if(sendMsgQue(msg_id,MSG_TYPE_TING,strmsg)!=0)
			printf("Send message queue failed!\n");
	}
        else if(nitem>0)
	{
		sprintf(strmsg,"%s","Err");
        	if(sendMsgQue(msg_id,MSG_TYPE_TING,strmsg)!=0)
			printf("Send message queue failed!\n");
	}

	//---- summary ---
        printf("-----< g_intRcvCount=%d, g_intErrCount=%d  g_intMissCount=%d g_intEscapeReadLoopCount=%d >-----\n" \
	,g_intRcvCount,g_intErrCount,g_intMissCount,g_intEscapeReadLoopCount);

   }
    //delete IPC message queue
    //close(g_fd);
    //exit(0);
}
