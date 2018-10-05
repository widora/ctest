/*-----------------------------------------------------------------

This program expects to control mplayer running in slave mode.

Environment Setup include:
-- lirc module  
-- ALSA

-----------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>  //sockaddr_in,htons..
#include <unistd.h> //read,write
#include <stdlib.h> //--exit()
#include <errno.h>
#include <fcntl.h> //---open()
#include <string.h> //---strcpy()
#include <stdbool.h> // true,false

#define CODE_NUM_0 22
#define CODE_NUM_1 12
#define CODE_NUM_2 24
#define CODE_NUM_3 94
#define CODE_NUM_4 8
#define CODE_NUM_5 28
#define CODE_NUM_6 90
#define CODE_NUM_7 66
#define CODE_NUM_8 82
#define CODE_NUM_9 74

#define CODE_SHUTDOWN 69
#define CODE_MODE 70
#define CODE_MUTE 71
#define CODE_EQ   7
#define CODE_NEXT 67
#define CODE_PREV 64
#define CODE_PLAY_PAUSE 68
#define CODE_VOLUME_UP 9
#define CODE_VOLUME_DOWN 21
#define CODE_RELOAD 25 //---- 2 arrows 

#define MAXLINE 100
#define List_Item_Max_Num 15
#define RADIO_LIST_MAX_NUM 30
#define RADIO_ADDRS_LEN 60

#define MODE_MPLAYER 0
#define MODE_RADIO 1
#define MODE_AIRBAND 2
#define MODE_XIAMEN 3


static char str_dev[]="/dev/LIRC_dev";


int main(int argc, char** argv)
{

//----------- LIRC ------------
   int fd;
   int code_num; // temprarily store  digtal number from LIRC
   unsigned int LIRC_DATA = 0; //---raw data from LIRC module 
   unsigned int LIRC_CODE =0;
   unsigned int PAUSE_TOKEN=0;
   unsigned int volume_val=100;
   char strCMD[50];

//---------------  OPEN LIRC DEVICE   --------------
    fd = open(str_dev, O_RDWR | O_NONBLOCK);
    if (fd < 0)
     {
        printf("can't open %s\n",str_dev);
        return -1;
      }

//--------- loop for input command and receive response ----------
  while(1)
  {
    //--------------- receive LIRC data ---------
    LIRC_DATA=0;
    read(fd, &LIRC_DATA, sizeof(LIRC_DATA));
    if(LIRC_DATA!=0)
    {
    	printf("LIRC_DATA: 0x%0x\n",LIRC_DATA);  
     	LIRC_CODE=(LIRC_DATA>>16)&0x000000ff;
      	printf("LIRC_CODE: %d\n",LIRC_CODE);

      	if(LIRC_CODE==CODE_NEXT)
        {
	   printf(" play next \n");
	   system("echo pt_step 1 >/home/slave\n");
      	}
      	else if(LIRC_CODE==CODE_PREV)
      	{
	   printf(" play prev \n");
	   system("echo pt_step -1 >/home/slave\n");
      	}
      	else if(LIRC_CODE==CODE_VOLUME_UP)
      	{
	   printf(" play volume up \n");
	   system("echo volume +50 >/home/slave\n");
      	}
      	else if(LIRC_CODE==CODE_VOLUME_DOWN)
      	{
	   printf(" play volume down \n");
	   system("echo volume -50 >/home/slave\n");
      	}
      	else if(LIRC_CODE==CODE_SHUTDOWN)
      	{
	   printf(" shut down \n");
	   system(" killall -9 mplayer\n");
      	}
      	else if(LIRC_CODE==CODE_RELOAD)
      	{
	   printf(" reload mplay \n");
	   system("/home/radioxm.sh");
      	}

    }

    usleep(250000); //---sleep

  } //--end while()


  close(fd);
  return 0;

}
