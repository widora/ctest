#include     <string.h>
#include      "ting.h"

//----global var.------
int fd;
char buff[512];

void sendCMD(char* strCMD)
{
	int nread,len;
	char *strtmp;
	len=strlen(strCMD);
	write(fd,strCMD,len);
	usleep(2000);
	nread=read(fd,buff,50); //read out ting reply
	buff[nread]='\0';
	strncpy(strtmp,strCMD,len-3);
//	*(strtmp+len-3)='\0';
	printf("%s: %s",strtmp,buff);
}




int main(int argc, char **argv)
{
	int nb,nread,nwrite;
	char tmp;
	char *pbuff;
	char  STR_CFG[]="AT+CFG=434000000,20,6,7,1,1,0,0,0,0,3000,8,4\r\n\0";
	char *dev ="/dev/ttyS1";
	int  ndelay=2000; // us delay,!!!!!--1000us delay cause Messg receive error!!

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

 //-----reste LORA------
/*
  write(fd,"AT+RST?\r\n",13);
  usleep(500000);
  read(fd,buff,30);
  printf("AT+RST: %s",buff);
*/
 
  nwrite=write(fd,STR_CFG,strlen(STR_CFG)); 
//  printf("nwrite=%d\n",nwrite);
  usleep(50000);//!!!! enough time for configuration
  read(fd,buff,50);
  printf("AT+CFG: %s",buff);

  write(fd,"AT+VER?\r\n",12);
  usleep(ndelay);
  read(fd,buff,20);
  printf("AT+VER: %s",buff);

  write(fd,"AT+ADDR?\r\n",13);
  usleep(ndelay);
  read(fd,buff,30);
  printf("AT+ADDR?: %s",buff);

  write(fd,"AT+RX?\r\n",20);
  usleep(ndelay);
  nread=read(fd,buff,30);
  buff[nread]='\0';
  printf("AT+RX?: %s",buff);

  nb=0;
  tcflush(fd,TCIOFLUSH);
  while(1)
  	{
   		while((nread = read(fd,&tmp,1))>0)
   		{
			//sprintf(pbuff,"%s",tmp);
	      		buff[nb]=tmp;
			nb++;
			if( tmp=='\n' || nb>511) // '\n' is the end of a string,common end \r\n
			{
				
      				printf("Message Received: %s",buff);
				nb=0;
				//----
				sendCMD("AT+RSSI?\r\n");
				//---reset RX mode
				write(fd,"AT+RX?\r\n",15);
				usleep(ndelay);
				nread=read(fd,buff,50); //read out ting reply
				buff[nread]='\0';
				//usleep(ndelay);
				printf("reset to RX Mode: %s",buff);
			}
		}
//		usleep(ndelay);

  	}
    //close(fd);
    //exit(0);
}
