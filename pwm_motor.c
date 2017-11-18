/*------------------------------------------------------------------
PWM driver Based on:
Author: qianrushizaixian
refer to:  blog.csdn.net/qianrushizaixian/article/details/46536005

--- TODOs & BUGs ---
1. the motor will get stuck when starting pwm_threshold is too small. 
  --- use ACTIVATE_EMERG_STOP to prevent it.


Midas
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h> //-- -lm
#include <stdbool.h> //bool
#include <pthread.h>

#include "/home/midas/ctest/kmods/soopwm/sooall_pwm.h"
#include "msg_common.h"
#include "ipcsock_common.h"
#include "pinctl_motor.h"

#define PWM_DEV "/dev/sooall_pwm"

int main(int argc, char *argv[])
{
	int ret = -1;
	int pwm_fd;
	int pwmno;
	int pwm_width; // +-400fastest ~ 0 standstill 
	bool bl_stepup = true; // during step-up adjusting pwm threshold
	bool bl_emerg_stop = false; // in emergency stop statu
	struct pwm_cfg  cfg;
	int gap_limit;
	int tmp;
	//---for IPC Msg Queue ---
	int msg_id=-1;
	int msg_key=MSG_KEY_SG; //MSG_TYPE_SG_PWM_WIDTH
	char strmsg[32]={0};
	//-- for IPC Msg Sock ---
	pthread_t thread_IPCSockServer;
	pthread_t thread_Read_IPCSockClients;
	int pret;


	//---- prepare pins for MOTOR direction and emergency stop control --------
	Prepare_CtlPins();
	SET_RUNDIR_FORWARD;
	DEACTIVATE_EMERG_STOP;

	//---create or get SG message queue -------
	if(msg_id=createMsgQue(msg_key)<0){
		printf("create message queue fails!\n");
		exit(-1);
	}

	//----- create IPC Sock Server thread -----
	pret=pthread_create(&thread_IPCSockServer,NULL,(void *)&create_IPCSock_Server,&msg_dat);
        if(pret != 0){
                printf("Fail to create thread for IPC Sock Server!\n");
                exit -1;
        }
	//----- create Read IPCSock Clients thread -----
	pret=pthread_create(&thread_Read_IPCSockClients,NULL,(void *)&read_IPCSock_Clients,&msg_dat);
        if(pret != 0){
                printf("Fail to create thread for Read_IPCSockClients!\n");
                exit -1;
        }

	//--- open pwm dev ----
	pwm_fd = open(PWM_DEV, O_RDWR);
	if (pwm_fd < 0) {
		printf("open pwm fd failed\n");
		return -1;
	}

	//---- pwm configuration -------
	cfg.no        =   0;    /* pwm0 */
	cfg.clksrc    =   PWM_CLK_40MHZ; //40MHZ or 100KHZ; 20-30KHZ for NIDEC24H PWM control
	cfg.clkdiv    =   PWM_CLK_DIV4; //DIV4 for 25KHz,40us  //DIV2 40/2=20MHZ
	cfg.old_pwm_mode =false;    /* true=old mode --- false=new mode */
/*
	//---- NEW MODE conf.-----
	cfg.lduration =   2-1; //(duration 2)for NEW MODE !!!!!! set N as N-1
	cfg.hduration =   2-1; //(duration 2)for NEW MODE  !!!!!! set N as N-1
	cfg.senddata0 =   0xf000f000;//1101110111011101;//0xAAAA; for NEW MODE
	cfg.senddata1 =	  0xf0f0f0f0;//1101110111011101;//0xAAAA; FOR NEW MODE
 	//----- end NEW MODE conf.
*/
	cfg.stop_bitpos = 63; // stop position of send data 0-63
	cfg.idelval   =   0;
	cfg.guardval  =   0; //
	cfg.guarddur  =   0; //
	cfg.wavenum   =   0;  /* forever loop */
	cfg.datawidth =   400;//for 25KHZ,40us;  //--limit 2^13-1=8191 
	cfg.threshold =   200; //(0-400)for PWM adjust
	//---period=1000/100(KHZ)*(DIV(1-128))*datawidth   (us)
	//---period=1000/40(MHz)*(DIV)*datawidth       (ns)
	//MOTOR PWM 25KHz:  40,000ns = 1000/40*DIV4*datawidth400 (ns), so threshole adjustable: 0-400

	//------ set PWM_MODE -----
	cfg.old_pwm_mode=true;

	//---print corresponding period---
	if(cfg.old_pwm_mode == true)
	{
           if(cfg.clksrc == PWM_CLK_100KHZ)
		{
			tmp=pow(2.0,(float)(cfg.clkdiv));
			printf("tmp=%d,set PWM period=%d us\n",tmp,(int)(1000.0/100.0*tmp*(int)(cfg.datawidth))); // div by integer is dangerous!!!
		}
           else if(cfg.clksrc == PWM_CLK_40MHZ)
		{
			tmp=pow(2.0,(float)(cfg.clkdiv));
			printf("tmp=%d,set PWM period=%d ns\n",tmp,(int)(1000.0/40.0*tmp*(int)(cfg.datawidth))); // div by integer is dangerous!!!
		}
         } 

	else if(cfg.old_pwm_mode == false)
	{
		printf("senddata0= %#08x  senddata1= %#08x \n",cfg.senddata0,cfg.senddata1); 
	}

	//----first set configure, then enable pwm -----
	ioctl(pwm_fd, PWM_CONFIGURE, &cfg);
	ioctl(pwm_fd, PWM_ENABLE, &cfg);


/*------------  test PWM for MOTOR -----------------*/
	pwm_width=50;  //start speed
        while(1){
		//------ if msg_dat is invalid -----
		if(msg_dat.msg_id==IPCMSG_NONE){
			usleep(50000);
			}

		//--------- parse msg_dat to control motor  --------
		switch(msg_dat.msg_id){
			case IPCMSG_PWM_THRESHOLD: //--motor speed control
				pwm_width = msg_dat.dat;
				//---- set pwm conf. for motor control -------
				//---- first activate stop, just to prevent motor from stagnation when init threshold value is too small.
				 ACTIVATE_EMERG_STOP;
				//--- then configure with real value --
				cfg.threshold=400-abs(pwm_width); //!!!! change direction here !!!!
				ioctl(pwm_fd,PWM_CONFIGURE,&cfg);
				//--- if NOT during emergency stop, then deactivate stop now
				if(!bl_emerg_stop)
					DEACTIVATE_EMERG_STOP;
				break;

			//--IPCMSG_MOTOR_DIRECTION  seems useless,since pwm_width +/- value indicating running direction
			case IPCMSG_MOTOR_DIRECTION:  //--motor direction control
				if(msg_dat.dat==IPCDAT_MOTOR_FORWARD){
					SET_RUNDIR_FORWARD;
				}
				else if(msg_dat.dat==IPCDAT_MOTOR_REVERSE){
					SET_RUNDIR_REVERSE;
				}
				break;
			case IPCMSG_MOTOR_STATUS: //--motor emerg. stop
				if(msg_dat.dat==IPCDAT_MOTOR_EMERGSTOP){
					ACTIVATE_EMERG_STOP; //-- emergency stop
					bl_emerg_stop=true;
				}
				else if(msg_dat.dat==IPCDAT_MOTOR_NORMAL){
					DEACTIVATE_EMERG_STOP; //--normal running
					bl_emerg_stop=false;
				}
				break;

			default:
				break;
		}//end of switch

		//---- set msg_id as IPCMSG_NONE to invalidate msg_dat
		msg_dat.msg_id=IPCMSG_NONE;

		//---- set running direction ----
		if(pwm_width<0)
			SET_RUNDIR_REVERSE;
		else
			SET_RUNDIR_FORWARD;

		//---------- Send IPC MSG to SG90 actuator ---------
		//XXXXX transfer MOTOR pwm_shreshold 0-400 (0 high speed - 400 low speed)  to  SG pwm_shreshold: 60-240 (50+10, 250-10)
		//---input sg_angle: -90 ~ 90, actual SG output is (250-gap_limit) ~ (50+gap_limit)
		//tmp=pwm_width/400.0*200+40; // convert range400 -> range200, so every value may get twice.
		gap_limit = 18; //gap_limit for SG90
		// ----transfer value -400(reverse) ~ +400(forward) to 50+gap_limit ~ 250-gap_limit
		tmp=150+(100-gap_limit)*pwm_width/400;
		sprintf(strmsg,"%d",tmp);
		if(sendMsgQue(msg_id,(long)MSG_TYPE_SG_PWM_WIDTH,strmsg)!=0)
			printf("Send message queue to SG failed!\n");

	} //while


	//----- close pwm dev fd -----
	close(pwm_fd);

	//----- end pthread -----
	//--- close all sock fds.......
	pthread_join(thread_IPCSockServer,NULL);
	pthread_join(thread_Read_IPCSockClients,NULL);
	//----- remove MSG Queue -----
	//----- release pin mmap ----
	Release_CtlPints();

	return 0;
}
