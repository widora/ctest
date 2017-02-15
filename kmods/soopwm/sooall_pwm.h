/*------------------------------------------------------------------
Author: qianrushizaixian
refer to:  blog.csdn.net/qianrushizaixian/article/details/46536005
------------------------------------------------------------------*/

#ifndef _SOOALL_PWM_H_
#define _SOOALL_PWM_H_

/* the source of the clock */
typedef enum {
	PWM_CLK_100KHZ,
	PWM_CLK_40MHZ
}PWM_CLK_SRC;

/* clock div */
typedef enum {
	PWM_CLI_DIV0 = 0,
	PWM_CLK_DIV2,
	PWM_CLK_DIV4,
	PWM_CLK_DIV8,
	PWM_CLK_DIV16,
	PWM_CLK_DIV32,
	PWM_CLK_DIV64,
	PWM_CLK_DIV128,
}PWM_CLK_DIV;

struct pwm_cfg {
	int no;
	PWM_CLK_SRC    clksrc;
	PWM_CLK_DIV    clkdiv;
	bool 	       old_pwm_mode; //--
	unsigned char  idelval;
	unsigned char  guardval;
	unsigned short guarddur;
	unsigned short wavenum;
	unsigned short datawidth; //biggest 2^13-1 
	unsigned short threshold;
};

/* ioctl */
#define PWM_ENABLE      0
#define PWM_DISABLE     1
#define PWM_CONFIGURE   2
#define PWM_GETSNDNUM   3
#endif
