/*-----------------------------------------------
EGI Color Functions

Midas ZHou
-----------------------------------------------*/

#include "egi_color.h"
#include "egi_debug.h"
#include <stdlib.h>
#include <stdint.h>
//#include <time.h>
#include <stdio.h>
#include <sys/time.h> /*gettimeofday*/



/*-------------------------------------------------------------------------
Get a random 16bit color from Douglas.R.Jacobs' RGB Hex Triplet Color Chart

rang: color range:
	0--all range
	1--light color
	2--mid range
	3--deep color

with reference to  http://www.jakesan.com

R: 0x00-0x33-0x66-0x99-0xcc-0xff
G: 0x00-0x33-0x66-0x99-0xcc-0xff
B: 0x00-0x33-0x66-0x99-0xcc-0xff

------------------------------------------------------------------------*/
EGI_16BIT_COLOR egi_color_random(enum egi_color_range range)
{
        int i,j;
        uint8_t color[3]; /*RGB*/
        struct timeval tmval;
        uint16_t ret;

        gettimeofday(&tmval,NULL);
        srand(tmval.tv_usec);

        /* random number 0-5 */
        for(i=0;i<3;i++)
        {
                j=(int)(6.0*rand()/(RAND_MAX+1.0));
                color[i]= 0x33*j;  /*i=0,1,2 -> R,G,B, j=0-5*/

                if( range > 0 && i==1) /* if not all color range */
		{
			/* to select G range, so as to select color range */
                        if( color[i]==0x33*(2*range-2) || color[i]==0x33*(2*range-1) )
                        {
		                EGI_PDEBUG(DBG_COLOR," ----------- color G =0X%02X\n",color[i]);
				continue;
			}
			else /* retry */
			{
                                i--;
                                continue;
                        }
		}
        }

        ret=COLOR_RGB_TO16BITS(color[0],color[1],color[2]);
        EGI_PDEBUG(DBG_COLOR,"egi random color GRAY: 0x%04X(16bits) / 0x%02X%02X%02X(24bits) \n",
										ret,color[0],color[1],color[2]);
        return ret;
}


/*---------------------------------------------------
Get a random 16bit gray_color from Douglas.R.Jacobs'
RGB Hex Triplet Color Chart

rang: color range:
	0--all range
	1--light color
	2--mid range
	3--deep color

with reference to  http://www.jakesan.com
R: 0x00-0x33-0x66-0x99-0xcc-0xff
G: 0x00-0x33-0x66-0x99-0xcc-0xff
B: 0x00-0x33-0x66-0x99-0xcc-0xff
---------------------------------------------------*/
EGI_16BIT_COLOR egi_colorgray_random(enum egi_color_range range)
{
        int i;
        uint8_t color; /*R=G=B*/
        struct timeval tmval;
        uint16_t ret;

        gettimeofday(&tmval,NULL);
        srand(tmval.tv_usec);

        /* random number 0-5 */
        for(;;)
        {
                i=(int)(15.0*rand()/(RAND_MAX+1.0));
                color= 0x11*i;

                if( range > 0 ) /* if not all color range */
		{
			/* to select R/G/B SAME range, so as to select color-GRAY range */
                        if( color>=0x11*5*(range-1) && color <= 0x11*(5*range-1) )
				break;
			else /* retry */
                                continue;
		}

		break;
        }

        ret=(EGI_16BIT_COLOR)COLOR_RGB_TO16BITS(color,color,color);

        EGI_PDEBUG(DBG_COLOR,"egi random color GRAY: 0x%04X(16bits) / 0x%02X%02X%02X(24bits) \n",
											ret,color,color,color);
        return ret;
}

/*---------------------------------------------------------------------
Color brightness control, brightness to be increase/decreased according
to k.

Note:
1. Permit Y<0, otherwise R,G,or B MAY never get  to 0, when you need
   to obtain a totally BLACK RGB(000) color in conversion, as of a check
   point, say.


	--- RGB to YUV ---
Y=0.30R+0.59G+0.11B=(307R+604G+113G)>>10
U=0.493(B-Y)+128=( (505(B-Y))>>10 )+128
V=0.877(R-Y)+128=( (898(R-Y))>>10 )+128

	--- YUV to RGB ---
R=Y+1.4075*(V-128)=Y+1.4075*V-1.4075*128
	=(Y*4096 + 5765*V -737935)>>12
G=Y-0.3455*(U-128)-0.7169*(V-128)=Y+136-0.3455U-0.7169V
	=(4096*Y+(136<<12)-1415*U-2936*V)>>12
B=Y+1.779*(U-128)=Y+1.779*U-1.779*128
	=(Y*4096+7287*U-932708)>>12
---------------------------------------------------------------------*/
EGI_16BIT_COLOR egi_colorbrt_adjust(EGI_16BIT_COLOR color, int k)
{
	int32_t R,G,B; /* !!! to be signed int32, same as YUV */
	int32_t Y,U,V;

	/* get 3*8bit R/G/B */
	R = (color>>11)<<3;
	G = (((color>>5)&(0b111111)))<<2;
	B = (color&0b11111)<<3;
//	printf(" color: 0x%02x, ---  R:0x%02x -- G:0x%02x -- B:0x%02x  \n",color,R,G,B);
//	return (EGI_16BIT_COLOR)COLOR_RGB_TO16BITS(R,G,B);

	/* convert RBG to YUV */
	Y=(307*R+604*G+113*B)>>10;
	U=((505*(B-Y))>>10)+128;
	V=((898*(R-Y))>>10)+128;
	//printf("R=%d, G=%d, B=%d  |||  Y=%d, U=%d, V=%d \n",R,G,B,Y,U,V);
	/* adjust Y, k>0 or k<0 */
	Y += k; /* (k<<12); */
	if(Y<0) {
		printf("------ Y=%d <0 -------\n",Y);
		/* !! Let Y<0,  otherwise R,G,or B MAY never get back to 0 when you need a totally BLACK */
		// Y=0; /* DO NOT set to 0 when you need totally BLACK RBG */
	}
	/* convert YUV back to RBG */
	R=(Y*4096 + 5765*V -737935)>>12;
	//printf("R'=0x%03x\n",R);
	if(R<0)R=0;
	if(R>255)R=255;

	G=((4096*Y-1415*U-2936*V)>>12)+136;
	//printf("G'=0x%03x\n",G);
	if(G<0)G=0;
	if(G>255)G=255;

	B=(Y*4096+7287*U-932708)>>12;
	//printf("B'=0x%03x\n",B);
	if(B<0)B=0;
	if(B>255)B=255;
	printf(" Input color: 0x%02x, aft YUV adjust: R':0x%03x -- G':0x%03x -- B':0x%03x \n",color,R,G,B);

	return (EGI_16BIT_COLOR)COLOR_RGB_TO16BITS(R,G,B);
}

/*--------------------------------------------------
 Get Y(brightness) value from a 16BIT RGB color
 as of YUV

 Y=0.30R+0.59G+0.11B=(307R+604G+113G)>>10
---------------------------------------------------*/
int egi_color_getY(EGI_16BIT_COLOR color)
{
        uint16_t R,G,B;

        /* get 3*8bit R/G/B */
        R = (color>>11)<<3;
        G = (((color>>5)&(0b111111)))<<2;
        B = (color&0b11111)<<3;
        //printf(" color: 0x%02x, ---  R:0x%02x -- G:0x%02x -- B:0x%02x  \n",color,R,G,B);

        /* convert to */
        return (307*R+604*G+113*B)>>10;
}
