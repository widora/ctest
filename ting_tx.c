#include     <stdint.h>
#include      "ting.h"

int main(int argc, char **argv)
{
	int fd;
	int nb,nread,nwrite;
	char tmp;
	char buff[512];
	char sendBuf[65];
	uint8_t k;
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

  //----set DEST address -----
  write(fd,"AT+DEST=5678?\r\n",30);
  usleep(ndelay);
  nread=read(fd,buff,30);
  buff[nread]='\0';
  printf("AT+DEST=5678?: %s",buff);

  nb=0;
  tcflush(fd,TCIOFLUSH);
  
  sendBuf[64]='\0';//end of string
  k='0';
  while(1)
  {
	  if(k=='~')
	      k='0';
	  else
	      k++; 
	  memset(sendBuf,k,64);//sizeof(sendBuf));
	  printf("Sending %s\n",sendBuf);
	  write(fd,"AT+SEND=64\r\n",20);
	  usleep(20000);
          write(fd,sendBuf,64);
	  usleep(500000); //sf=7// Spreading fact=6 sleep(3); //!!! critial !!!!
    }
    //close(fd);
    //exit(0);
}
