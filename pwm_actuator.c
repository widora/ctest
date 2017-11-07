/*------------------------------------------------------------------
                     ----- NOTE -----
PWM driver for MT7688 Based on:
Author: qianrushizaixian
refer to:  blog.csdn.net/qianrushizaixian/article/details/46536005

                  ----- TODOs and BUGs -----
1. The SG90 actuator will sometime get stuck mechanically.
2. Low voltage of motor power supply will casue running noise.

------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h> //-- -lm
#include <stdbool.h> //bool
#include "/home/midas/ctest/kmods/soopwm/sooall_pwm.h"
#include "msg_common.h"
#define PWM_DEV "/dev/sooall_pwm"

int main(int argc, char *argv[])
{
	int ret = -1;
	int pwm_fd;
	int pwmno=1;
	int copt;
	int sg_angle=0;
	//-------cfg1 for pwm2(actuator)
	struct pwm_cfg cfg1;
	int tmp;
        //---for IPC Msg ---
        int msg_id=-1;
        int msg_key=MSG_KEY_SG; //MSG_TYPE_SG_PWM_WIDTH
        char strmsg[32]={0};
	int msg_ret;
        //---create or get SG message queue -------
        if(msg_id=createMsgQue(msg_key)<0){
                printf("create message queue fails!\n");
                exit(-1);
        }


	//---- select pwm port 0-3, default use pwm0 port ---
/*
	if(argc>1)
		pwmno = atoi(argv[1]);
*/

	//----- get options------
	while ( (copt=getopt(argc,argv, "hs:")) !=-1 ){
		switch (copt) {
			case 'h':
				printf("Usage: \n");
				printf("	-h  Help \n");
				printf("	-s [-90 90] set SG angle. Default is 0. \n");
				break;
			case 's':
				tmp=atoi(optarg);
				if( tmp >= -90 && tmp <= 90)
					sg_angle=-tmp;//!!!--reverse angle for vision !!!!!!
				else
					printf("SG angle out of range [-90 90].\n");
				break;
			default:
				printf("SG angle set to 0 as per default.\n");

		}//switch
	}//while
	//---- check invalid options
	for ( copt = optind; copt<argc; copt++){
		printf("--- No option for %s! ---\n",argv[copt]);
	}


	pwm_fd = open(PWM_DEV, O_RDWR);
	if (pwm_fd < 0) {
		printf("open pwm fd failed\n");
		return -1;
	}


//========-----------   pwm2 configuration : actuator  ----------==========
	cfg1.no        =   pwmno;
	cfg1.clksrc    =   PWM_CLK_100KHZ; //40MHZ or 100KHZ; 
	cfg1.clkdiv    =   PWM_CLK_DIV0; //  //DIV2 40/2=20MHZ
	cfg1.old_pwm_mode =true;    /* true=old mode --- false=new mode */
	//---- NEW MODE conf.-----
/*
	cfg1.lduration =   2-1; //(duration 2)for NEW MODE !!!!!! set N as N-1
	cfg1.hduration =   2-1; //(duration 2)for NEW MODE  !!!!!! set N as N-1
	cfg1.senddata0 =   0xf000f000;//1101110111011101;//0xAAAA; for NEW MODE
	cfg1.senddata1 =	  0xf0f0f0f0;//1101110111011101;//0xAAAA; FOR NEW MODE
*/
	//---end of NEW MODE
	cfg1.stop_bitpos = 63; // stop position of send data 0-63
	cfg1.idelval   =   0;
	cfg1.guardval  =   0; //
	cfg1.guarddur  =   0; //
	cfg1.wavenum   =   0;  /* forever loop */
	cfg1.datawidth =   2000;//period relevant,for 20ms period;  //--limit 2^13-1=8191 
	cfg1.threshold =   150; //duty relevant, (0-10000) actuator 0 position threshold: 1.5/20*2000=150
	//---- Actuator Angle  Limit ------
	//-90Deg -- 0.5ms -- threshold 50  ;;;;  90Deg - 2.5ms -- threshold 250

	//---period=1000/100(KHZ)*(DIV(1-128))*datawidth   (us)
	//---period=1000/40(MHz)*(DIV)*datawidth       (ns)
	//actuator PWM:  20,000us = 1000/100*DIV0(1)*datawidth2000 (us), so threshole adjustable: 0-2000


	//-----print corresponding period---
	if(cfg1.old_pwm_mode == true)
	{
           if(cfg1.clksrc == PWM_CLK_100KHZ)
		{
			tmp=pow(2.0,(float)(cfg1.clkdiv));
			printf("tmp=%d,set actuator PWM  period=%d us\n",tmp,(int)(1000.0/100.0*tmp*(int)(cfg1.datawidth))); // div by integer is dangerous!!!
		}
           else if(cfg1.clksrc == PWM_CLK_40MHZ)
		{
			tmp=pow(2.0,(float)(cfg1.clkdiv));
			printf("tmp=%d,set actuator PWM period=%d ns\n",tmp,(int)(1000.0/40.0*tmp*(int)(cfg1.datawidth))); // div by integer is dangerous!!!
		}
         }

	else if(cfg1.old_pwm_mode == false)
	{
		printf("senddata0= %#08x  senddata1= %#08x \n",cfg1.senddata0,cfg1.senddata1); 
	}


	//------ set pwm2-cfg2 for actuator -----
	ioctl(pwm_fd,PWM_CONFIGURE, &cfg1);
	ioctl(pwm_fd,PWM_ENABLE,&cfg1);

	/*---------   set SG angle as per optarg ---------*/
	//----  threshold: 50 -250, shall leave some gap for limit
	int gap_limit=0;
	//---input sg_angle: -90 ~ 90, actual SG output is (250-gap_limit) ~ (50+gap_limit)
	cfg1.threshold = ( (50+gap_limit) + (sg_angle+90)/180.0*((250-gap_limit)-(50+gap_limit)) );
	ioctl(pwm_fd,PWM_CONFIGURE,&cfg1);

	//------ receive IPC msg and set SG angle accordingly -----
	while(1){
		msg_ret=recvMsgQue(msg_id,MSG_TYPE_SG_PWM_WIDTH);
		if(msg_ret >0){
			printf("g_msg_data.text:%s \n",g_msg_data.text);
			//since msg_sender already covert data to threshold range (50+10,250-10)
			cfg1.threshold=atoi(g_msg_data.text);
		 	ioctl(pwm_fd,PWM_CONFIGURE,&cfg1);
			//--left msg deemed as obselete and discard
			while(msg_ret>0){
				msg_ret=recvMsgQue(msg_id,MSG_TYPE_SG_PWM_WIDTH);
			}

			usleep(5000);
		}
	}//while() for IPC msg 

	usleep(500000);


	/*------------  test PWM for Actuator -----------------*/
	//------------  threshold: 50 -250, shall leave some gap for limit 
/*
        while(1){

		for(tmp=60;tmp<240;tmp++){
			cfg1.threshold = tmp; 
			usleep(6000);
			ioctl(pwm_fd,PWM_CONFIGURE,&cfg1);
		}
		//------ reverse now -----
		for(tmp=240;tmp>60;tmp--){
			cfg1.threshold =tmp;
			usleep(6000); //!!! useless!!!, seems ioctl() will cost time!
			ioctl(pwm_fd,PWM_CONFIGURE,&cfg1);
		}

	}
*/


	return 0;
}
