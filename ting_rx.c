#include     <string.h>
#include      "ting.h"

//----global var.------
int fd;
char buff[512];
int  ndelay=2000; // us delay,!!!!!--1000us delay cause Messg receive error!!

int main(int argc, char **argv)
{
	int nb,nread,nwrite;
	char tmp;
	char *pbuff;
	char  STR_CFG[]="AT+CFG=434000000,10,6,7,1,1,0,0,0,0,3000,132,4\r\n";
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
  sendCMD("AT+RST\r\n",50000);
  sleep(1);//wait long enough
  tcflush(fd,TCIOFLUSH);

  //----- configure ----
  sendCMD(STR_CFG,50000);
  //------ get version ------
  sendCMD("AT+VER?\r\n",ndelay);
  //------- set ADDR -------
  sendCMD("AT+ADDR=5555?\r\n",ndelay);
  sendCMD("AT+ADDR?\r\n",ndelay);
  //----set DEST address -----
  sendCMD("AT+DEST=6666\r\n",ndelay);

  nb=0;
  tcflush(fd,TCIOFLUSH);

  sendCMD("AT+RX?\r\n",ndelay);
  while(1)
  {
   		while((nread = read(fd,&tmp,1))>0)
   		{
			//sprintf(pbuff,"%s",tmp);
	      		buff[nb]=tmp;
			nb++;
			if( tmp=='\n' || nb>511) // '\n' is the end of a string,common end \r\n
			{
				buff[nb]='\0';
      				printf("Message Received: %s",buff);
				nb=0;
				//----
				sendCMD("AT+RSSI?\r\n",ndelay);
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
