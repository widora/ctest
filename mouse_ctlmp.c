/*-------------------------------------------------------------------
with reference to:
https://item.congci.com/-/content/linux-shubiao-shuju-duqu-caozuo
-------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#define  LEFT_KEY 9
#define  RIGHT_KEY 10
#define  MID_KEY 12
#define  WH_UP 255
#define  WH_DOWN 1

//---- double click check -----
#define DOUBLE_CLICK_INTERVAL_MAX 250 //ms
#define DOUBLE_CLICK_INTERVAL_MIN 100 //ms
struct timeval Lprev_time,Lnow_time; //left button previous click time and current click time
struct timeval Rprev_time,Rnow_time;//right button previous click time and current click time

#define  MAX_FREQ_ITEMS 10
static float FM_FREQ[MAX_FREQ_ITEMS]={
87.9,
89.9,
91.4,
93.4,
97.7,
99.0,
101.7,
103.7,
105.7,
107.7,
};

#define MAX_CMD_ITEMS 5

const char CMD_LIST[MAX_CMD_ITEMS][50]={
"screen -dmS PLAY_B /mplayer/playB.sh",
"screen -dmS PLAY_LIST /mplayer/usb_playlist",
"screen -dmS PLAY_XM /mplayer/usb_playxmlist",
"screen -dmS PLAY_F /mplayer/playF.sh",
"screen -dmS PLAY_MP3 /mplayer/playMP3.sh",
};

//------ check for double click -------
bool is_dbclick(struct timeval prev_time, struct timeval now_time)
{
	int interval; // time interval
	interval = (now_time.tv_sec-prev_time.tv_sec)*1000+(now_time.tv_usec-prev_time.tv_usec)/1000;
	printf("interval=%d\n",interval);
	return ( (interval>DOUBLE_CLICK_INTERVAL_MIN && interval<DOUBLE_CLICK_INTERVAL_MAX)  ? true : false );

}


int main(int argc,char **argv) {
   int fd,retval;
   int nread,nwrite;
   int i,k=0,m=0;//k--counter for CMD_LIST[], m--counter for FM_FREQ[]
   unsigned char buf[4]={0};
   unsigned char setbuf[6]={0xf3,200,0xf3,100,0xf3,80};
   fd_set readfds;
   struct timeval tv;
   //------
//   unsigned int volume_val=50; //50% for USB_Speaker
   unsigned int volume_val=80; //80% for Headphone
   char strCMD[50];
   bool fm_radio_on=false;

   if(argc<2)
   {
	printf(" Usage: %s  dev_path \n",argv[0]);
	exit(-1);
   }
   if( (fd=open(argv[1],O_RDWR))<0)
   {
	printf(" Fail to open %s !\n",argv[1]);
	exit(-1);
   }
   else
   {
	printf("Open %s successfuly!\n",argv[1]);
   }

   //-- set mouse type to Miscrosft Intellimouse
   nwrite=write(fd,setbuf,6);
   printf("%d bytes written to mousedev\n",nwrite);

   //------ set volume
//   sprintf(strCMD,"amixer -c 1 set Speaker %d%%",volume_val);
   sprintf(strCMD,"amixer -c 0 set Headphone %d%%",volume_val);
   system(strCMD);

   //-----start mplayer slave first  -----
   system("killall -9 fm.sh");
   system(CMD_LIST[1]);

   //-----init timeval
   gettimeofday(&Lprev_time,NULL);
   gettimeofday(&Rprev_time,NULL);

   //------------- loop read mouse ----------
   while(1)
   {
	tv.tv_sec=5;
	tv.tv_usec=0;

	FD_ZERO(&readfds);
	FD_SET(fd,&readfds);

	retval = select(fd+1, &readfds,NULL,NULL,&tv);
	if(retval==0)
		printf("Time out!\n");
	if(FD_ISSET(fd,&readfds))
	{
		nread=read(fd,buf,4);
		if(nread<0)
		{
		    continue;
		}
/*
		//printf("%d bytes data: 0x%02x, X:%d, Y:%d, Z:%d  buf[4]=%d  buf[7]=%d \n",nread,buf[0],buf[1],buf[2],buf[3],buf[4],buf[7]);
		printf("%d bytes data: ",nread);
		for(i=0;i<4;i++)
			printf("buf[%d]=%d, ",i,buf[i]);
		printf("\n");
*/
//------------------ control mplayer ---------------
                 if( buf[0] == LEFT_KEY )//left key press
                 {
                         printf("--left key--\n");
			 //----double click check -------
			 gettimeofday(&Lnow_time,NULL);
			 if(is_dbclick(Lprev_time,Lnow_time)){
				printf("Left button double click detected!\n");
				// shift command list and exectue
				if(k < (MAX_CMD_ITEMS-1))
					k++;
				else k=0;
				printf("command: %s \n",CMD_LIST[k]);
				//reassure to switch off radio
				fm_radio_on=false;
				system("killall -9 fm.sh");
				system(CMD_LIST[k]);
			 }
			 else if(!fm_radio_on)   //only if  mplayer_on
                         	system("mprev");

			 Lprev_time=Lnow_time;//renew Lprev_time
                 }
                 else if( buf[0] == RIGHT_KEY )//right key press
		 {
                         printf("--right key--\n");
			 //----double click check -------
			 gettimeofday(&Rnow_time,NULL);
			 if(is_dbclick(Rprev_time,Rnow_time)){
				printf("Right button double click detected!\n");
				fm_radio_on = !fm_radio_on; //switch radio status
				if(!fm_radio_on){ //if switch off radio, then turn on mplayer
				  	system("killall -9 fm.sh");
					system(CMD_LIST[k]);
				}
			 }
			 Rprev_time=Rnow_time;//renew Lprev_time

			//-------- Tune radio or mplayer -----------
			if(fm_radio_on){  //if radio is on
				m++;
				if(m > MAX_FREQ_ITEMS-1) m=0; 
				sprintf(strCMD,"screen -dmS FM%-6.2f /mplayer/fm.sh %6.2f\n",FM_FREQ[m],FM_FREQ[m]);
				printf("%s",strCMD);
				system(strCMD);
			}
			else //else control mplayer
                        	system("mnext");
		 }
                 else if( buf[0] == MID_KEY )//middle key press
		 {
			 printf("--mid key--\n");
			 system("mpause");
//			 usleep(100000);
		 }
		 else if ( buf[3] == WH_UP ){
//                       system("vup");
                         if(volume_val<125){
                                 volume_val+=4;
                                 //sprintf(strCMD,"amixer -c 1 set Speaker %d%%",volume_val);
                                 sprintf(strCMD,"amixer -c 0 set Headphone %d%%",volume_val);
                                 system(strCMD);
			}
                  }
		 else if ( buf[3] == WH_DOWN ){
//                       system("vdown");
                         if(volume_val>5){
                                 volume_val-=4;
                                 //sprintf(strCMD,"amixer -c 1 set Speaker %d%%",volume_val);
                                 sprintf(strCMD,"amixer -c 0 set Headphone %d%%",volume_val);
                                 system(strCMD);
			}
                  }


	}
   }
   close(fd);

   return 0;
}
