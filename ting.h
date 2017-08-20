/*-------------------------------------------------

Common header for Widora Ting LoRA UART test program

KEY WORDS:
TS --  Time stamp


--------------------------------------------------*/
#ifndef __TING_H__
#define __TING_H__

#include     <stdio.h>      /*标准输入输出定义*/
#include     <stdlib.h>     /*标准函数库定义*/
#include     <unistd.h>     /*Unix标准函数定义*/
#include     <sys/types.h>  /**/
#include     <sys/stat.h>   /**/
#include     <fcntl.h>      /*文件控制定义*/
#include     <termios.h>    /*PPSIX终端控制定义*/
#include     <errno.h>      /*错误号定义*/
#include     <stdbool.h>
#include     <string.h>
#include     "mygpio.h"

#define WPIN_TING_MCU_RST 42 // set widora GPIO for Ting MCU reset 
#define MAX_TING_LORA_ITEM 24 //max number of received key word(value) items seperated by ',' in RX received Ting RoLa string.
#define USER_RX_BUFF_SIZE 512
#define USER_TX_BUFF_SIZE 512
#define LOOP_DEAD_COUNT 5 // max. number of trying read() uart dev.
#define MAX_AT_REPLY_SIZE 50 //30 nread=read(g_fd,pstr,MAX_READ_SIZE) !!!!- suitable size for length of reply-string from  Ting
//-especially for AT+SEND reply as "AT,SENDING\r\n" and "AT,SENDED\r\n",
#define MAX_ROLA_STR_SIZE 100 //--denpend on UART set ??? 50? suitable size for received Rola string length.

//----- AT commend reply notation as returned by sendTingCMD() -----
#define AT_CMD_EXCUTED_OK 0  //-- AT commends well received and successfuly executed by Ting
#define AT_LORA_TRANSMIT_OK 1 //--- AT reply: complete to transmit data by LoRa. 
#define AT_CMD_EXCUTED_FAIL -1  //-- AT commends NOT received OR executed by Ting
#define AT_LORA_TRANSMIT_FAIL -2 //--- AT reply:  transmit data by LoRa failed. 

//----- serial port
int g_fd=-1;
int  g_ndelay=5000; // us delay,!!!!!--1000us delay cause receiving from Ting error!!

//----- time
struct timeval g_tm;
char g_pstr_time[30]; //time stamp string

//---buffers
char g_strUserRxBuff[USER_RX_BUFF_SIZE]; //--string from Ting LoRa RX, like "LR,6666,40,xxxx,xx......"  xxxx--payload
char g_strUserTxBuff[USER_TX_BUFF_SIZE]; //--string ready to send for Ting LoRa Tx, like "xxxxxx,xxxxx,xx,,...."  xxxx- payload
char g_strAtBuff[64]; //64--string for AT cmd/replay buff, such as "OK\r\n","-063\r\nOK\r\n" etc.
int  g_intLoraRxLen;  //-- length of Ting Lora Rx string in form of  "LR,****,##,XXX,XXX,XXXX...."
//--g_intLoraRxLen to be renewed in sepWordsInTingLoraStr() only
//--- for error rate analysis ----
int g_intErrCount=0; //--Error counter
int g_intMissCount=0; //--LoRa missing occurrence counter
int g_intRcvCount=0;  //-- LoRa success of receipt counter
int g_intEscapeReadLoopCount=0; //--- times of escaping from read(UART) dead loop
unsigned char g_tmpUchar=0; //--temp store. for lora data test, may be '0'(48) to '~'(126)  

/*----- prepare GPIO operation -------*/
void setPinMmap(void)
{
 if(gpio_mmap()!=0)
 {
    printf("--- gpio_mmap failed! ---"); //-------------------- return ----------
    exit;
 }
  mt76x8_gpio_set_pin_direction(WPIN_TING_MCU_RST,1); //1 as for output

}

/*------ release PIN mmap -----------*/
void resPinMmap(void)
{
  close(gpio_mmap_fd); //gpio_mmap_fd in "mygpio.h"
 }

/*------------- reset Ting MCU ------------*/
void ResetTingMCU(void)
{
     setPinMmap();
     mt76x8_gpio_set_pin_value(WPIN_TING_MCU_RST,0);
     usleep(200000); 
     mt76x8_gpio_set_pin_value(WPIN_TING_MCU_RST,1);
     resPinMmap();
}

/*------- renew time for char *g_pstr_time -------*/
void RenewTimeStr(char *g_pstr_time)
{
     gettimeofday(&g_tm,NULL);
     sprintf(g_pstr_time,"%11.3f", g_tm.tv_sec+g_tm.tv_usec/1000000.0);
}

/*------ clear g_strUsrRxBuff[] and fill with '\0' ------*/
void ClearUserRxBuff(void)
{
   memset(g_strUserRxBuff,'\0',sizeof(g_strUserRxBuff));
}

/*------ clear g_strUsrTxBuff[] and fill with '\0' ------*/
void ClearUserTxBuff(void)
{
   memset(g_strUserTxBuff,'\0',sizeof(g_strUserTxBuff));
}

/*-----------------------------------------
 push string to g_strUserTxBuff 
-----------------------------------------*/
int intPush2UserTxBuff(char* pstr)
{
  int len=strlen(pstr);
  if( (USER_TX_BUFF_SIZE-strlen(g_strUserTxBuff)-1) >= len )
  {
  	strcat(g_strUserTxBuff,pstr);
	return len;
   }
   else
	return 0;
}

/*-----------------------------------------
 push string to g_strUserRxBuff 
-----------------------------------------*/
int intPush2UserRxBuff(char* pstr)
{
  int len=strlen(pstr);
  if( (USER_RX_BUFF_SIZE-strlen(g_strUserRxBuff)-1) >= len )
  {
  	strcat(g_strUserRxBuff,pstr);
	return len;
   }
   else
	return 0;
}


/*----------------------------------------------------------------------------------------
  send command string to LORA and get feedback string
  strCMD: AT command string for Ting-01M
  strOK: words in Ting reply string indicating that Ting executed the command successfully
	 usually is 'OK', or 'SENDED' for 'AT+SEND=xx\r\n' LoRa command. 
  return value:
	  -1 - Ting responds, but not 'OK'
           0 - Ting responds with 'OK'.
           1 - reaches LOOP_DEAD_COUNT; 
	   2 - fail to write to serial prot
	   3 - other reasons to cause failure
  buff[],fd ---global var, see in ting_*x.c
  Window: '\r'-- move cursor to the head of current line  '\n' --- start a new line
  Linux: '\r'--same above  '\n'-- start a new line and get cursor to the head of the new line.
------------------------------------------------------------------------------------------------*/
int sendTingCMD(const char* strCMD, const char* strOK, int ndelay)
{
    int ret = 3;
    int nb; //-- total bytes received from Ting.
    int len,nread;
    int nloop=0;
    char *pstr; // pointer to g_strAtBuff[];
    char strtmp[50];

    len=strlen(strCMD);

    printf("start write(g_fd,strCMD,len)....\n");
    if(write(g_fd,strCMD,len)<0)
    {
	perror("sendTingCMD():write to serial port");
	printf("write(g_fd,strCMD,len<0 \n");
	return 2;
    }
    usleep(ndelay);

    //---init
    nb=0;
    memset(g_strAtBuff,0,sizeof(g_strAtBuff));
    pstr=g_strAtBuff;

   //------- get feedback string from Ting -----
    while(1) // !!!! todo: avoid deadloop !!!!
   {
	printf("start: nread=read(g_fd,pstr,MAX_AT_REPLY_SIZE)  ---  nloop=%d\n",nloop);
	nread=read(g_fd,pstr,MAX_AT_REPLY_SIZE);//TIMEOUT=5s //--30 !!!!- suitable size for length of reply-string from  Ting
//-especially for AT+SEND reply as "AT,SENDING\r\n" and "AT,SENDED\r\n",
	if(nread<0)
	{
		printf("sendTingCMD():read Ting CMD feedback from serial port, nread<0!");
		//---reset count and pstr
		pstr=g_strAtBuff;
		nb=0;
		nread=0;
		//perror("sendTingCMD():read serial port for Ting feedback string");
		//return;
	}
	printf("nread=%d \n",nread);
	pstr+=nread;
	nb+=nread;
	//check if g_strAtBuff[] is out of capacity
	if(nb > (sizeof(g_strAtBuff)-1))
		printf("Recevied AT reply string is too long, g_strAtBuff[] is out of its capacity!\n");
	nloop++;
	//--- jump out of dead loop ---
	if(nloop > LOOP_DEAD_COUNT)
	{
	    printf("sendTingCMD: nloop > LOOP_DEAD_COUNT, end reading UART ...\n"); 
//	    (pstr-2)='\0'; // add string end before '\r\n', to get rid of '\r\n' when printf
	    sprintf(g_strAtBuff,"Error! nread() escapes deadloop!\n");
	    g_intEscapeReadLoopCount++; // count number
	    ret=1;
	    break;
	}

	//----skip first '\r\n' --
	if((nread>0) && (g_strAtBuff[0]=='\r' || g_strAtBuff[0]=='\n'))
	{
		//---reset count and pstr
		pstr=g_strAtBuff;
		nb=0;
	}

	if((nread>0) && (*(pstr-1)=='\n')) //----get end of a reply string
	{
		//--- check whether Ting responds with 'OK' or 'SENDED'  ---
		printf("g_strAtBuff:%s \n",g_strAtBuff);
	        if(strstr(g_strAtBuff,strOK)) //if strOK is found in g_strAtBuff
		{
		      *(pstr-2)='\0'; // add string end before '\r\n', to get rid of '\r\n' when printf
		      ret=0;
		      break;
		}
		else
		{
		    //ret=-1;
		    //break; 
		    continue; //--strOK not received from Ting, continue to read
		}
	}
    }

    strncpy(strtmp,strCMD,len);
    strtmp[len-2]='\0'; //--to  skip \r\n for command string.

    //------printf command to Ting and its reply string
    printf("sendTingCMD ret=%d %s:  %s\n",ret,strtmp,g_strAtBuff);

    return ret;
}


/*------------------------------------------------
Compare two string to ascertain they are identical 
-------------------------------------------------*/
bool blMatchStrWords(char* pstr, const char* pkeyword)
{
	if(strcmp(pstr,pkeyword)==0)return true;
	else
		return false;
}


/*-----------------------------------------------------------------------------
   get point array to key word(value) items separated by ',' in origin Ting xstring.
after operation, all ',' will be replaced by '\0' as the end of a string item.
  1. char* strRecv MUST be modifiable.
  2. char* pstrTingLoraItems[]  will gets points to each itmes.
  3. Total Number of items will be returned.
  4. '\r\n' is remained !!!! check tty read function,
------------------------------------------------------------------------------*/
int sepWordsInTingLoraStr(char* pstrRecv, char* pstrTingLoraItems[])
{
	int nstr=0;
	char* const delim=",";
	char* pStrData;
	char** ppStrCur=&pstrRecv;// pp to remaining chars.

	memset(pstrTingLoraItems,0,sizeof(pstrTingLoraItems));// clear arrays first

	g_intLoraRxLen=0;
	if(!strstr(pstrRecv,delim))// if no "," is found in received string, then return 0.
		return 0;
	g_intLoraRxLen=strlen(pstrRecv);//-- total length

	while(pStrData=strsep(ppStrCur,delim)) //--get a point to a new string
	{
		pstrTingLoraItems[nstr]=pStrData;
		nstr++;
//		printf("%s\n",pStrData);
	}

	return nstr;
}


/*-------------------------------------------------------------------------------
1. Clear serial buffer(TCIOFLUSH) and set Ting to Rola RX mode, keep reading
 serial port until get a complete  Rola string replied from Ting-01M. 
 Received string will be stored in g_strUserRxBuff[].
2. Return count number of received chars including '\r\n', or minus for failure.
3. '\r\n' is remained in g_strUserRxBuff[] !!!!
--------------------------------------------------------------------------------*/
int recvTingLoRa(void)
{
  int nb=0;
  int ret_cmd;
  int nread;
  int nloop=0;
  char *pstr; //--pointer to g_strUserRxBuff[]

  //----clear tty FIFO hardware buff
  // tcflush(g_fd,TCIOFLUSH);

  //----- clear g_strUserRxBuff[] -----
  memset(g_strUserRxBuff,0,sizeof(g_strUserRxBuff));
  pstr=g_strUserRxBuff;

  //--- command Ting to set RX mode ----
  printf("start sendTingCMD(AT+RX?)...\n");
  ret_cmd=sendTingCMD("AT+RX?\r\n","OK",g_ndelay);
  if(ret_cmd != 0)
	return -1;

  while(1)
  {
	printf("start: nread=read(g_fd,pstr,MAX_ROLA_STR_SIZE)...");
        nread=read(g_fd,pstr,MAX_ROLA_STR_SIZE); //--50? suitable size for Ting Rola string.
	printf("  nread=%d \n",nread);
        if(nread<0)
        {
                printf("read serial port error\n");
                pstr=g_strUserRxBuff; // reset pbuff
                nb=0;
                nread=0;// !!!!!!
        }
        pstr+=nread;
        nb+=nread;
	//check if g_strUserRxBuff[] is out of capacity
	if(nb > (sizeof(g_strUserRxBuff)-1))
		printf("Recevied Ting LoRa string is too long, g_strUserRxBuff[] is out of its capacity!\n");	nloop++;

	//--- jump out of dead loop ---
	if(nloop > LOOP_DEAD_COUNT)
	{
	    printf("recvTingRoLa: nloop > LOOP_DEAD_COUNT, end reading UART ...\n"); 
	    *(pstr-2)='\0'; // add string end before '\r\n', to get rid of '\r\n' when printf
	    g_intEscapeReadLoopCount++; // count number
	    break;
	}

        //---following unnecessary, '\r\n' will be dealt by sendTingCMD()
        if( (nb==1) && ( *(pstr-1)=='\n' || *(pstr-1)=='\r') )//get rid of '\r\n' first if applicable
        {
                pstr=g_strUserRxBuff; // reset pbuff
                nb=0;
        }

        if( (nb>2) && ( *(pstr-1)=='\n' || nb>511) ) // '\n' is the end of a string,common end \r\n
        {
                *pstr='\0'; // add string end
                printf("Message Received: %s",g_strUserRxBuff);
                break;
        }
  }
  return nb;

}

/*------------------------------------------------------------------------------------------
parse RX received Ting Lora key word/value items stored in a string array separated by ','
in form of "LR,****,##,XXX,XXX,XXXX...."  ****--source addr.    ## --payload length in hex.
-------------------------------------------------------------------------------------------*/
void parseTingLoraWordsArray(char* pstrTingLoraItems[])
{
	int k=0;
	int len_payload; 
	unsigned char new_dat;
/* //------for test---
	while(pstrTingLoraItems[k]!=NULL)
	{
		printf("pstrTingLoraItems[%d]=%s\n",k,pstrTingLoraItems[k]);
		k++;
	}
*/
	//--- assert pstrTingLoraItems[] first ---!!!!!
	if(pstrTingLoraItems[5] == NULL)
		return;

        //----- get payload length -----
	len_payload=strtoul(pstrTingLoraItems[2],NULL,16);
	printf("Total length:%d\n",g_intLoraRxLen); 
	printf("Lora palyload length:%d\n",len_payload);

	//---check length and pick err,simple way !!!!! ---
	if( (g_intLoraRxLen-len_payload) != 13 )
	{
	        g_intErrCount++;
		return;
	}

	//----check pastrTingLoraItems[] first -----
	//-- if(pstrTingLoraItems[5]==NULL)
	if(blMatchStrWords(pstrTingLoraItems[0],"LR"))
		printf("------- Parse Received LoRa data ------\n");
	printf("Lora source address:%s\n",pstrTingLoraItems[1]);

	//------- check if any Lora transmission is missed ------
	new_dat =*(unsigned char *)(pstrTingLoraItems[5]); //-- char '0'(48)-'~'(126)
	printf("new_dat=%c\n",new_dat);
	if(g_tmpUchar == 0)
		g_tmpUchar = new_dat; 
	else if(new_dat > g_tmpUchar)
		g_intMissCount+=new_dat-g_tmpUchar-1; //--cal missing lora data
	else if(new_dat < g_tmpUchar) // from '~'(126) return to '0'(48)
		g_intMissCount+=new_dat+78-g_tmpUchar;
	g_tmpUchar = new_dat; // store new_dat to .. 

	//--- count number of LoRa transmission received ---
	g_intRcvCount++;
	//----check data continuity and see if there is a RoLa transmission not recevied -----

	//---- print client and host time stamp -----
	printf("Source time stamp:%s\n",pstrTingLoraItems[4]);
 	RenewTimeStr(g_pstr_time);
        printf("Host current time: %s\n",g_pstr_time);
}

/*-----------------------------------------------
check whether Ting-01M is active or not
return:  0 -- active  others -- busy or dead
------------------------------------------------*/
int checkTingActive(void)
{
  int ret;
  //reaffirm that g_fd is effective first
  ret=sendTingCMD("AT+VER?\r\n","OK",g_ndelay);
  if(ret != 0)
  {
	printf("----WARNING: Ting seems not active now!  sendTingCMD(AT+VER) = %d \n", ret);
	return ret;
  }

  return 0;
}


/*----------------------------------------------------------------------------------
reset Ting-01M
g_fd : uart IO file handle
str_config: configuration for Ting put in a string
          Example str_config[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,132,4\r\n";
self_add: address of ME.
dest_addr: destination address of LoRa send/receive operation
all address in hex. range 0x0000-FFFF
------------------------------------------------------------------------------------*/
void resetTing(int g_fd, const char* str_config, int self_addr, int dest_addr)
{
  static char strCMD[100]={0};

  if( (self_addr>0xffff) || (self_addr<0)) 
  {
        printf("Ting address out of range[0x0000-FFFF]!\n");
        exit;
  }

  if( (dest_addr>0xffff) || (dest_addr<0)) 
  {
        printf("Ting dest address out of range[0x0000-FFFF]!\n");
        exit;
  }

  //--- reset MCU ---
  printf("start reset Ting MCU ...\n");
  ResetTingMCU();
  usleep(500000);//wait ?
  //----- reset LoRa -----
  printf("start configuring Ting and LoRa...\n");
  tcflush(g_fd,TCIOFLUSH);// flush invalid data in uart buffer
  sendTingCMD("AT+RST\r\n","OK",500000);
  sleep(1);//wait long enough
//  tcflush(g_fd,TCIOFLUSH);
  //----- configure ----
  sendTingCMD(str_config,"OK",50000);
  //------ get version ------
  sendTingCMD("AT+VER?\r\n","OK",g_ndelay);
  //------- set ADDR -------
  sprintf(strCMD,"AT+ADDR=%04X\r\n",self_addr);
  sendTingCMD(strCMD,"OK",g_ndelay);
  sendTingCMD("AT+ADDR?\r\n","OK",g_ndelay);
  //----set DEST address -----
  sprintf(strCMD,"AT+DEST=%04X\r\n",dest_addr);
  sendTingCMD(strCMD,"OK",g_ndelay);
  //----set PD0 as RX indication -----
  sendTingCMD("AT+ACK=1\r\n","OK",g_ndelay);
}


/*--------------------------------------------------
open serial device file and set interface properly.
g_fd: global file handle for the UART
--------------------------------------------------*/
void openUART(char *uart_dev)
{
  //------ open UART interface-------
  g_fd = OpenDev(uart_dev);
  if (g_fd>0)
         set_speed(g_fd,115200);
  else
        {
                printf("Can't Open Serial Port!\n");
                exit;
        }
  //----set databits,stopbits,parity for UART -----
  if (set_Parity(g_fd,8,1,'N')== false) //set_Prity(fd,databits,stopbits,parity)
  {
    printf("Set Parity Error\n");
    exit;
  }

}


#endif
