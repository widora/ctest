#include "egi_color.h"
#include "egi_debug.h"
#include <stdlib.h>
#include <stdint.h>
//#include <time.h>
#include <stdio.h>
#include <sys/time.h> /*gettimeofday*/

/*-------------------------------------------------------------------------
get a random 16bit color from Douglas.R.Jacobs' RGB Hex Triplet Color Chart

with reference to  http://www.jakesan.com

R: 0x00-0x33-0x66-0x99-0xcc-0xff
G: 0x00-0x33-0x66-0x99-0xcc-0xff
B: 0x00-0x33-0x66-0x99-0xcc-0xff


Midas Zhou
----------------------------------------------------------------------------*/
uint16_t egi_random_color(void)
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
		color[i]= 0x33*j;  /*i=0,1,2 -> R,G,B*/
		/* to make G >= 0x66 */
		if(i==1)
			if(color[i]<0x66)
			{
				i--;
				continue;
			}

		//printf(" ----- color j=%d\n",j);
	}

	ret=COLOR_RGB_TO16BITS(color[0],color[1],color[2]);
	PDEBUG("egi random color: 0X%04X \n",ret);
	return ret;
}
