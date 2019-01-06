#include "egi_color.h"
#include "egi_debug.h"
#include <stdlib.h>
#include <stdint.h>
//#include <time.h>
#include <stdio.h>
#include <sys/time.h> /*gettimeofday*/



/*-------------------------------------------------------------------------
get a random 16bit color from Douglas.R.Jacobs' RGB Hex Triplet Color Chart

rang: color range:
	0--all range
	1--light color
	2--mid range
	3--deep color

with reference to  http://www.jakesan.com

R: 0x00-0x33-0x66-0x99-0xcc-0xff
G: 0x00-0x33-0x66-0x99-0xcc-0xff
B: 0x00-0x33-0x66-0x99-0xcc-0xff

Midas Zhou
------------------------------------------------------------------------*/
uint16_t egi_color_random(enum egi_color_range range)
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
		                printf(" ----------- color G =0X%02X\n",color[i]);
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
        egi_pdebug(DBG_COLOR,"egi random color: 0X%04X \n",ret);
        return ret;
}


/*---------------------------------------------------
get a random 16bit gray_color from Douglas.R.Jacobs' 
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

Midas Zhou
---------------------------------------------------*/
uint16_t egi_colorgray_random(enum egi_color_range range)
{
        int i,j;
        uint8_t color; /*R=G=B*/
        struct timeval tmval;
        uint16_t ret;

        gettimeofday(&tmval,NULL);
        srand(tmval.tv_usec);

        /* random number 0-5 */
        for(;;)
        {
                j=(int)(15.0*rand()/(RAND_MAX+1.0));
                color= 0x11*j;  /*i=0,1,2 -> R,G,B, j=0-14(0-E)*/

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

        ret=COLOR_RGB_TO16BITS(color,color,color);

        egi_pdebug(DBG_COLOR,"egi random color GRAY: 0X%02X%02X%02X \n",color,color,color);
        return ret;
}
