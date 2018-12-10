/*-----------------------  touch_home.c ------------------------------
XPT2046 touch pad

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include "spi.h"
#include "xpt2046.h"



/*--------------   8BITS CONTROL COMMAND FOR XPT2046   -------------------
[7] 	S 		-- 1:  new control bits,
	     		   0:  ignore data on pins

[6-4]   A2-A0 		-- 101: X-position
		   	   001: Y-position

[3]	MODE 		-- 1:  8bits resolution
		 	   0:  12bits resolution

[2]	SER/(-)DFR	-- 1:  normal mode
			   0:  differential mode

[1-0]	PD1-PD0		-- 11: normal power
			   00: power saving mode
---------------------------------------------------------------*/
#define XPT_CMD_READXP	0xD0 //0xD0 //1101,0000  /* read X position data */
#define XPT_CMD_READYP	0x90 //0x90 //1001,0000 /* read Y position data */

/* ----- XPT bias and limit value ----- */
#define XPT_XP_MIN 7
#define XPT_XP_MAX 116 //actual 116
#define XPT_YP_MIN 17
#define XPT_YP_MAX 116  //actual 116

/* ------ touch read sample number ------*/
#define XPT_SAMPLE_EXPNUM 4  /* 2^4=2*2*2*2 */
#define XPT_SAMPLE_NUMBER 1<<XPT_SAMPLE_EXPNUM  /* sample for each touch-read session */

/* ----- LCD parameters ----- */
#define LCD_SIZE_X 240
#define LCD_SIZE_Y 320



/*---------------------------------------------------------------
read XPT touching coordinates, and normalize it.
*x --- 2bytes value
*y --- 2bytes value
return:
	0 	Ok
	<0 	pad untouched or invalid value

Note:
	1. Only first byte read from XPT is meaningful !!!??
---------------------------------------------------------------*/
int xpt_read_xy(uint8_t *xp, uint8_t *yp)
{
	uint8_t cmd;

	/* poll to get XPT touching coordinate */
	cmd=XPT_CMD_READXP;
	SPI_Write_then_Read(&cmd, 1, xp, 2); /* return 2bytes valid X value */
	cmd=XPT_CMD_READYP;
	SPI_Write_then_Read(&cmd, 1, yp, 2); /* return 2byte valid Y value */

	/*  valify data,
		when untouched: Xp[0]=0, Xp[1]=0
		when untouched: Yp[0]=127=[2b0111,1111] ,Yp[1]=248=[2b1111,1100]
	*/
	if(xp[0]>0 && yp[0]<127)
	{
   		//printf("xpt_read_xy(): Xp[0]=%d, Yp[0]=%d\n",xp[0],yp[0]);

		/* normalize TOUCH pad x,y data */
		if(xp[0]<XPT_XP_MIN)xp[0]=XPT_XP_MIN;
		if(xp[0]>XPT_XP_MAX)xp[0]=XPT_XP_MAX;
		if(yp[0]<XPT_YP_MIN)yp[0]=XPT_YP_MIN;
		if(yp[0]>XPT_YP_MAX)yp[0]=XPT_YP_MAX;

		return 0;
    	}
	else
	{  /* meanless xp, or pen_up */
		*xp=0;*(xp+1)=0;
		*yp=0;*(yp+1)=0;

		return -1;
	}
}

/*------------------------------------------------------------------------------
convert XPT coordinates to LCD coodrinates
xp,yp:   XPT touch pad coordinates (uint8_t)
xs,ys:   LCD coodrinates (uint16_t)

NOTE:
1. Because of different resolustion(value range), mapping XPT point to LCD point is
actually not one to one, but one to several points. however, we still keep one to
one mapping here.
--------------------------------------------------------------------------------*/
void xpt_maplcd_xy(const uint8_t *xp, const uint8_t *yp, uint16_t *xs, uint16_t *ys)
{
	*xs=LCD_SIZE_X*(xp[0]-XPT_XP_MIN)/(XPT_XP_MAX-XPT_XP_MIN+1);
	*ys=LCD_SIZE_Y*(yp[0]-XPT_YP_MIN)/(XPT_YP_MAX-XPT_YP_MIN+1);
}

