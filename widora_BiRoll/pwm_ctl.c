/*------------------------------------------------------------------
PWM driver Based on:
Author: qianrushizaixian
refer to:  blog.csdn.net/qianrushizaixian/article/details/46536005

--- TODOs & BUGs ---
1. the motor will get stuck when starting pwm_threshold is too small. 
  --- use ACTIVATE_EMERG_STOP to prevent it.
2.

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
#include "pinctl.h"

#include "/home/midas/ctest/kmods/soopwm/sooall_pwm.h"
#define PWM_DEV "/dev/sooall_pwm"

static int g_pwm_fd; // pwm device descriptor
static struct pwm_cfg  g_pwm_cfg; //pwm configuration strut


/*-------------  init_Motor_Control  ----------------------
Prepare motor direction control pins and PWM configuration
for motor speed control.

Return:
	0  -- OK
	<0 -- Fails
------------------------------------------------------*/
int init_Motor_Control(void)
{
	int ndiv;

	//---- prepare pins for MOTOR direction and Bake control ----
	if( Prepare_CtlPins() != 0)
	{
		fprintf(stderr,"prepare control pins failed!\n");
		return -1;
	}
	//---- brake motors
	SET_MOTOR_BRAKE;
	//--- open pwm dev ----
	g_pwm_fd = open(PWM_DEV, O_RDWR);
	if (g_pwm_fd < 0)
	{
		fprintf(stderr,"open %s failed!\n",PWM_DEV);
		return -1;
	}
	//---- pwm configuration -------
	g_pwm_cfg.no        =   0;    /* pwm0 */
	g_pwm_cfg.clksrc    =   PWM_CLK_40MHZ; //40MHZ or 100KHZ; 20-30KHZ for NIDEC24H PWM control
	g_pwm_cfg.clkdiv    =   PWM_CLK_DIV4; //DIV4 for 25KHz,40us  //DIV2 40/2=20MHZ
	g_pwm_cfg.old_pwm_mode =true;    /* true=old mode --- false=new mode */
	g_pwm_cfg.stop_bitpos = 63; // stop position of send data 0-63
	g_pwm_cfg.idelval   =   0;
	g_pwm_cfg.guardval  =   0; //
	g_pwm_cfg.guarddur  =   0; //
	g_pwm_cfg.wavenum   =   0;  /* forever loop */
	g_pwm_cfg.datawidth =   400;// for 25KHZ,40us;  //--limit 2^13-1=8191 
	g_pwm_cfg.threshold =   0; //(0-400)for PWM adjust
	//---period=1000/100(KHZ)*(DIV(1-128))*datawidth   (us)
	//---period=1000/40(MHz)*(DIV)*datawidth       (ns)
	//MOTOR PWM 25KHz:  40,000ns = 1000/40*DIV4*datawidth400 (ns), so threshole adjustable: 0-400

	//---print corresponding period---
	if(g_pwm_cfg.old_pwm_mode == true)
	{
           if(g_pwm_cfg.clksrc == PWM_CLK_100KHZ)
		{
			ndiv=pow(2.0,(float)(g_pwm_cfg.clkdiv));
			printf("tmp=%d,set PWM period=%d us\n",ndiv,(int)(1000.0/100.0*ndiv*(int)(g_pwm_cfg.datawidth))); // div by integer is dangerous!!!
		}
           else if(g_pwm_cfg.clksrc == PWM_CLK_40MHZ)
		{
			ndiv=pow(2.0,(float)(g_pwm_cfg.clkdiv));
			printf("tmp=%d,set PWM period=%d ns\n",ndiv,(int)(1000.0/40.0*ndiv*(int)(g_pwm_cfg.datawidth))); // div by integer is dangerous!!!
		}
         }
	else if(g_pwm_cfg.old_pwm_mode == false)
	{
		printf("senddata0= %#08x  senddata1= %#08x \n",g_pwm_cfg.senddata0,g_pwm_cfg.senddata1); 
	}

	//----first set configuration, then enable it -----
	g_pwm_cfg.no        =   0;    /* pwm0 */
	ioctl(g_pwm_fd, PWM_CONFIGURE, &g_pwm_cfg);
	ioctl(g_pwm_fd, PWM_ENABLE, &g_pwm_cfg);
	g_pwm_cfg.no        =   1;    /* pwm1 */
	ioctl(g_pwm_fd, PWM_CONFIGURE, &g_pwm_cfg);
	ioctl(g_pwm_fd, PWM_ENABLE, &g_pwm_cfg);

	return 0;
}

/*----------------------------------------
Release motor contorl
colse pwm device and release GPIO pin mmap
-----------------------------------------*/
void release_Motor_Control(void)
{
	//----- close pwm dev fd -----
	close(g_pwm_fd);
	//----- release pin mmap ----
	Release_CtlPints();
}


/*----------------------------------------
colse pwm device and release GPIO pin mmap
both PWM0 and PWM1 will be configured
PWM0 -- right wheel motor
PWM1 -- left wheel motro
pwmval: pwm threshold value
	Limit: 400(fastest) - 250(slowest)
dirval:
	> 0 forward
	= 0 brake
	< 0 revers
-----------------------------------------*/
void set_Motor_Speed(int pwmval,  int dirval)
{
	//----- set running direction ------
	if (dirval == 0)
		SET_MOTOR_BRAKE;
	else if(dirval >0)
		SET_MOTOR_FORWARD;
	else
		SET_MOTOR_REVERSE;

	//----- set pwm shreshold to change running speed -----
	if(pwmval > 400 || pwmval <250)
	{
		printf(" PWM threshold out of range(250~400) !\n");
		return;
	}
	g_pwm_cfg.threshold=pwmval;
	g_pwm_cfg.no        =   0;    /* pwm0 */
	ioctl(g_pwm_fd, PWM_CONFIGURE, &g_pwm_cfg);
	ioctl(g_pwm_fd, PWM_ENABLE, &g_pwm_cfg);
	g_pwm_cfg.no        =   1;    /* pwm1 */
	ioctl(g_pwm_fd, PWM_CONFIGURE, &g_pwm_cfg);
	ioctl(g_pwm_fd, PWM_ENABLE, &g_pwm_cfg);

}

//========================== Main  ==========
int main(int argc, char *argv[])
{
	int ret = -1;
	int k=0;
	int maxpwm=400;
	int minpwm=255;

	init_Motor_Control();

  while(1)
  {
	printf("test accelerating ...\n");
	set_Motor_Speed(350, 0); //brake
	for(k=minpwm; k<maxpwm; k++)
	{
		set_Motor_Speed(k, 1);//forward
		usleep(20000);
	}
	sleep(5);

	printf("test braking ...\n");
	set_Motor_Speed(350, 0); //brake
	sleep(3);

	printf("test decelerating ...\n");
	for(k=maxpwm; k>minpwm; k--)
	{
		set_Motor_Speed(k, -1);//forward
		usleep(20000);
	}
	sleep(5);
	set_Motor_Speed(350, 0); //brake
  }//end of while()

	release_Motor_Control();

	ret=0;
	return ret;
}
