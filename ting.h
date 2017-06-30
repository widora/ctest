/*-------------------------------------------------

Common header for Widora Ting LoRA UART test program

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
/***@brief  设置串口通信速率
*@param  fd     类型 int  打开串口的文件句柄
*@param  speed  类型 int  串口速度
*@return  void*/

int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
	    B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {115200,38400,  19200,  9600,  4800,  2400,  1200,  300,
	    38400,  19200,  9600, 4800, 2400, 1200,  300, };
void set_speed(int fd, int speed)
{
  int   i;
  int   status;
  struct termios   Opt;
  tcgetattr(fd, &Opt);
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
   {
   	if  (speed == name_arr[i])
   	{
   	    tcflush(fd, TCIOFLUSH);
    	cfsetispeed(&Opt, speed_arr[i]);
    	cfsetospeed(&Opt, speed_arr[i]);
    	status = tcsetattr(fd, TCSANOW, &Opt);
    	if  (status != 0)
            perror("tcsetattr fd1");
     	return;
     	}
   tcflush(fd,TCIOFLUSH);
   }
}
/**
*@brief   设置串口数据位，停止位和效验位
*@param  fd     类型  int  打开的串口文件句柄*
*@param  databits 类型  int 数据位   取值 为 7 或者8*
*@param  stopbits 类型  int 停止位   取值为 1 或者2*
*@param  parity  类型  int  效验类型 取值为N,E,O,,S
*/
int set_Parity(int fd,int databits,int stopbits,int parity)
{
	struct termios options;
 if  ( tcgetattr( fd,&options)  !=  0)
  {
  	perror("SetupSerial 1");
  	return(false);
  }
  options.c_cflag &= ~CSIZE;
  switch (databits) /*设置数据位数*/
  {
  	case 7:
  		options.c_cflag |= CS7;
  		break;
  	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		fprintf(stderr,"Unsupported data size\n");
		return (false);
	}
  switch (parity)
  	{
  	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);  /* 设置为奇效验*/
		options.c_iflag |= INPCK;             /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;
	case 'S':
	case 's':  /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		fprintf(stderr,"Unsupported parity\n");
		return (false);
		}
  /* 设置停止位*/
  switch (stopbits)
  	{
  	case 1:
  		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		fprintf(stderr,"Unsupported stop bits\n");
		return (false);
	}
  /* Set input parity option */
  if (parity != 'n')
  		options.c_iflag |= INPCK;
    options.c_cc[VTIME] = 150; // 15 seconds
    options.c_cc[VMIN] = 0;

  tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
  if (tcsetattr(fd,TCSANOW,&options) != 0)
  	{
  		perror("SetupSerial 3");
		return (false);
	}
  return (true);
 }
/**
*@breif 打开串口
*/
int OpenDev(char *Dev)
{
int	fd = open( Dev, O_RDWR | O_NOCTTY );         //| O_NOCTTY | O_NDELAY
	if (-1 == fd)
		{ /*设置数据位数*/
			perror("Can't Open Serial Port");
			return -1;
		}
	else
	return fd;

}

/*-----------------------------------------------------
  send command string to LORA and get feedback string

  buff[],fd ---global var, see in ting_*x.c
-----------------------------------------------------*/
extern char buff[];
extern int fd;
void sendCMD(const char* strCMD,int ndelay)
{

        int nread,len;
        char strtmp[50];
        len=strlen(strCMD);
        write(fd,strCMD,len);
        usleep(ndelay);
        nread=read(fd,buff,50); //read out ting reply
        buff[nread]='\0';
        strncpy(strtmp,strCMD,len);
        strtmp[len-2]='\0'; //--to  skip \r\n
        printf("%s: %s",strtmp,buff);
}

//========================== TING DATA PARSE ===========================

/*-----------------------------------------------------------------------------
   get point array to key word(value) items separated by ',' in origin Ting xstring.
after operation, all ',' will be replaced by '\0' as the end an string item.
  1. char* strRecv MUST be modifiable.
  2. char* pstrItems[]  will return points to each itmes.
  3. Number of items will be returned.
------------------------------------------------------------------------------*/
int sepWordsInTingStr(char* pstrRecv, char* pstrItems[])
{
	int nstr=0;
	char* const delim=",";
	char* pStrData;
	char** ppStrCur=&pstrRecv;// pp to remaining chars.

	memset(pstrItems,0,sizeof(pstrItems));// clear arrays first
	while(pStrData=strsep(ppStrCur,delim)) //--get a point to a new string
	{
		pstrItems[nstr]=pStrData;
		nstr++;
//		printf("%s\n",pStrData);
	}

	return nstr;
}


/*-------------------------------------------------------
parse Ting key word/value items stored in a string array.

-------------------------------------------------------*/
void parseTingWordsArray(char* pstrItems[])
{
	int k=0;

	while(pstrItems[k]!=NULL)
	{
		printf("pstrItems[%d]=%s\n",k,pstrItems[k]);
		k++;
	}

}

