/*------------------------------------------------------------------
Based on:
Author: qianrushizaixian
refer to:  blog.csdn.net/qianrushizaixian/article/details/46536005
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

#define PWM_DEV "/dev/sooall_pwm"

int main(int argc, char *argv[])
{
	int ret = -1;
	int pwm_fd;
	int pwmno;
	struct pwm_cfg  cfg;
	int tmp;

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
	//---- NEW MODE conf.-----
	cfg.lduration =   2-1; //(duration 2)for NEW MODE !!!!!! set N as N-1
	cfg.hduration =   2-1; //(duration 2)for NEW MODE  !!!!!! set N as N-1
	cfg.senddata0 =   0xf000f000;//1101110111011101;//0xAAAA; for NEW MODE
	cfg.senddata1 =	  0xf0f0f0f0;//1101110111011101;//0xAAAA; FOR NEW MODE
 	//----- end NEW MODE conf.
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

        while(1){
		for(tmp=0;tmp<400;tmp++){
			cfg.threshold=tmp;
			usleep(20000);
			ioctl(pwm_fd,PWM_CONFIGURE,&cfg);}
		usleep(900000);
		for(tmp=400;tmp>5;tmp--){
			cfg.threshold=tmp;
			usleep(20000);
			ioctl(pwm_fd,PWM_CONFIGURE,&cfg);}
		}

/*
	while (1) {
		static int cnt = 0;
		sleep(5);
		ioctl(pwm_fd, PWM_GETSNDNUM, &cfg);
		printf("send wave num = %d\n", cfg.wavenum);
		cnt++;
		if (cnt == 10) {
			ioctl(pwm_fd, PWM_DISABLE, &cfg);
			break;
		}
	}
*/
	return 0;
}