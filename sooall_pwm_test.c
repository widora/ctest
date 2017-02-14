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

int main(int argc, char *argv)
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

	cfg.no        =   0;    /* pwm0 */
	cfg.clksrc    =   PWM_CLK_100KHZ; //PWM_CLK_40MHZ;
	cfg.clkdiv    =   PWM_CLK_DIV128; //DIV2 40/2=20MHZ
	cfg.old_pwm_mode =true;    /* old mode  */
	cfg.idelval   =   0;
	cfg.guardval  =   0;
	cfg.guarddur  =   0;
	cfg.wavenum   =   0;  /* forever loop */
	cfg.datawidth =   781;//--limit 2^13-1=8191  //1000; period=40*1000/(40/2) ns
	cfg.threshold =   390; //500;
	//---period=1000/100(KHZ)*(DIV)*datawidth   (us)
        if(cfg.clksrc == PWM_CLK_100KHZ)
		{
			tmp=pow(2.0,(float)(cfg.clkdiv));
			//printf("cfg.clkdiv=%d\n",cfg.clkdiv);
			printf("tmp=%d,set PWM period=%d us\n",tmp,(int)(1000.0/100.0*tmp*(int)(cfg.datawidth))); // div by integer is dangerous!!!
		}
        else if(cfg.clksrc == PWM_CLK_40MHZ)
		{
		}

	ioctl(pwm_fd, PWM_CONFIGURE, &cfg);
	ioctl(pwm_fd, PWM_ENABLE, &cfg);
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
