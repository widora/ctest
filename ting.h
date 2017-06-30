/*-------------------------------------------------

Common header for Widora Ting LoRA UART test program

KEY WORDS:
TS --  Time stamp


--------------------------------------------------*/

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


#define USER_RX_BUFF_SIZE 512
#define USER_TX_BUFF_SIZE 512

struct timeval g_tm;
char g_pstr_time[30]; //time stamp string

//---buffers
char g_strUserRxBuff[USER_RX_BUFF_SIZE]; //--string from Ting LoRa RX, like "LR,6666,40,xxxx,xx......"  xxxx--payload
char g_strUserTxBuff[USER_TX_BUFF_SIZE]; //--string ready to send for Ting LoRa Tx, like "xxxxxx,xxxxx,xx,,...."  xxxx- payload
char g_strAtBuff[64]; //--string for AT cmd/replay buff, such as "OK\r\n","-063\r\nOK\r\n" etc.


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

  buff[],fd ---global var, see in ting_*x.c
  Window: '\r'-- move cursor to the head of current line  '\n' --- start a new line
  Linux: '\r'--same above  '\n'-- start a new line and get cursor to the head of the new line.
------------------------------------------------------------------------------------------------*/
//extern char buff[];
extern int fd;
void sendCMD(const char* strCMD,int ndelay)
{

    int nb,len;
    char ctmp;
    char strtmp[50];
    len=strlen(strCMD);
    write(fd,strCMD,len);
    usleep(ndelay);
    nb=0;
    while(1) // !!!! todo: avoid deadloop !!!!
   {
	if(read(fd,&ctmp,1)>0)
	{
		g_strAtBuff[nb]=ctmp;
		if( ctmp=='\n' && nb>1)// end of return string
		{
			g_strAtBuff[nb]='\0'; //--get rid of '\n'
			break;
		}
		else if(ctmp=='\r')
		{
			g_strAtBuff[nb]='\0'; // get rid of '\r'
		}
		else if(ctmp=='\n' && nb<2)
		{
		 	nb=0;//only '\r\n',no data in buff; reset buff pointer
		}
		nb++;
	}
    }

    //nread=read(fd,buff,50); //read out ting reply
    //buff[nread]='\0';
    strncpy(strtmp,strCMD,len);
    strtmp[len-2]='\0'; //--to  skip \r\n
    printf("%s: %s\n",strtmp,g_strAtBuff);
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
after operation, all ',' will be replaced by '\0' as the end an string item.
  1. char* strRecv MUST be modifiable.
  2. char* pstrTingLoraItems[]  will return points to each itmes.
  3. Number of items will be returned.
  4. '\r\n' is remained in the last itme!!!!
------------------------------------------------------------------------------*/
int sepWordsInTingLoraStr(char* pstrRecv, char* pstrTingLoraItems[])
{
	int nstr=0;
	char* const delim=",";
	char* pStrData;
	char** ppStrCur=&pstrRecv;// pp to remaining chars.

	memset(pstrTingLoraItems,0,sizeof(pstrTingLoraItems));// clear arrays first
	while(pStrData=strsep(ppStrCur,delim)) //--get a point to a new string
	{
		pstrTingLoraItems[nstr]=pStrData;
		nstr++;
//		printf("%s\n",pStrData);
	}

	return nstr;
}


/*------------------------------------------------------------------------
parse RX received Ting Lora key word/value items stored in a string array.

------------------------------------------------------------------------*/
void parseTingLoraWordsArray(char* pstrTingLoraItems[])
{
	int k=0;
	
	while(pstrTingLoraItems[k]!=NULL)
	{
//		printf("pstrTingLoraItems[%d]=%s\n",k,pstrTingLoraItems[k]);
		k++;
	}

	if(blMatchStrWords(pstrTingLoraItems[0],"LR"))
		printf("------- LoRa data received! ------\n");
	printf("Lora Palyload Data:%s\n",pstrTingLoraItems[3]);


}

/*----- renew time for char *g_pstr_time----------*/
void RenewStrTime(char *g_pstr_time)
{
     gettimeofday(&g_tm,NULL);
     sprintf(g_pstr_time,"%ld.%ld", g_tm.tv_sec,g_tm.tv_usec);
}
