/*-------------------------------------------------------------------
Based on PWM driver from:
https://item.congci.com/-/content/linux-shubiao-shuju-duqu-caozuo

Use mouse to control motor
	- Midas
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
#include <pthread.h>
#include "ipcsock_common.h"
#define LIMIT_LOW_GAP 10 //LOW SPEED LIMIT GAP for motor pwm threshold
#define LIMIT_HIGH_GAP 10 //HIGHT SPEED LIMIT GAP  for motor pwm threshold

#define  LEFT_KEY 9
#define  RIGHT_KEY 10
#define  MID_KEY 12
#define  WH_UP 255
#define  WH_DOWN 1

int main(int argc,char **argv)
{
   int fd,retval;
   int nread,nwrite;
   int i;
   unsigned char buf[4]={0};
   unsigned char setbuf[6]={0xf3,200,0xf3,100,0xf3,80};
   fd_set readfds;
   struct timeval tv;
   bool statusEmergStop=false;
   int pwm_threshold=0; //--init value!

   //-- for IPC Msg Sock ---
   pthread_t thread_IPCSockClient;
   int pret;

   //----------- parse argv opt ---------------
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


   //----- clear msg_dat before IPCSockClient session ----
   memset(&msg_dat,0,sizeof(struct struct_msg_dat));//set msg_dat.msg_id=0, to prevent IPSockClient from sending it.

   //----- create IPC Sock Client thread -----
   pret=pthread_create(&thread_IPCSockClient,NULL,(void *)&create_IPCSock_Client,&msg_dat);
   if(pret != 0){
            printf("Fail to create thread for IPC Sock Client!\n");
             exit -1;
   }


   //----------------- loop read mouse -------------
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
		//printf("%d bytes data: 0x%02x, X:%d, Y:%d, Z:%d  buf[4]=%d  buf[7]=%d \n",nread,buf[0],buf[1],buf[2],buf[3],buf[4],buf[7]);
/*
		printf("%d bytes data: ",nread);
		for(i=0;i<4;i++)
			printf("buf[%d]=%d, ",i,buf[i]);
		printf("\n");
*/
		//---------------------- mouse control ---------------
                if( buf[0] == LEFT_KEY )//left key press
                {
                }
                else if( buf[0] == RIGHT_KEY )//right key press
		{
		}
                else if( buf[0] == MID_KEY )//middle key press
		{
			//------ change motor run direction ----
			if(statusEmergStop==true){
				msg_dat.dat=IPCDAT_MOTOR_NORMAL;
	                	msg_dat.msg_id=IPCMSG_MOTOR_STATUS; //enable this msg!
				statusEmergStop=false;
			}
			else{
				msg_dat.dat=IPCDAT_MOTOR_EMERGSTOP;
	                	msg_dat.msg_id=IPCMSG_MOTOR_STATUS; //enable this msg!
				statusEmergStop=true;
			}
		}
		else if ( buf[3] == WH_DOWN ){
		//----- speed up motor -----
	                //-- pwm threshold range [0 high_speed - 400 low_speed]
        	        //--- pthread_mute_lock here msg_dat here.....
			//if(msg_dat.dat < 400-LIMIT_LOW_GAP){
			if(pwm_threshold < 400-LIMIT_LOW_GAP){
				pwm_threshold+=20;
				if(pwm_threshold > 400-LIMIT_LOW_GAP)
					pwm_threshold=400-LIMIT_LOW_GAP;
				msg_dat.dat=pwm_threshold;
	                	msg_dat.msg_id=IPCMSG_PWM_THRESHOLD; //enable this msg!
	        	        printf("mouse_ctlmotor: set msg_dat.dat = %d \n",msg_dat.dat);
			}

                 }
		 else if ( buf[3] == WH_UP ){
		  //----- slow down motor ------
	                //-- pwm threshold range [0 high_speed - 400 low_speed]
        	        //--- pthread_mute_lock here msg_dat here.....
			//if(msg_dat.dat > -400 +LIMIT_HIGH_GAP){
			if(pwm_threshold > -400 +LIMIT_HIGH_GAP){
				pwm_threshold-=20;
				if(pwm_threshold < -400+LIMIT_HIGH_GAP)
					pwm_threshold=-400+LIMIT_HIGH_GAP;
				msg_dat.dat=pwm_threshold;
	                	msg_dat.msg_id=IPCMSG_PWM_THRESHOLD; //enable this msg!
	        	        printf("mouse_ctlmotor: set msg_dat.dat = %d \n",msg_dat.dat);
			}

                 }

	}
   }
   close(fd);

   return 0;
}
