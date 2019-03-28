/*-------------------------------
Test EGI COLOR functions

Midas Zhou
-------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "egi_timer.h"
#include "egi_fbgeom.h"
#include "egi_color.h"
#include "egi_log.h"


int main(void)
{
	int k=0;
	int delt=1;
	EGI_16BIT_COLOR color,subcolor;

	/* --- init logger --- */
  	if(egi_init_log("/mmc/log_color") != 0)
	{
		printf("Fail to init logger,quit.\n");
		return -1;
	}
        /* --- start egi tick --- */
//        tm_start_egitick();
        /* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

	/* get a random color */
	color= egi_color_random(deep);
	subcolor=color;
	fbset_color(subcolor);
	draw_filled_rect(&gv_fb_dev, 100, 200, 100+80, 200+80);
	usleep(990000);


while(1)
{

	subcolor=egi_colorbrt_adjust(color,k);
	printf("---k=%d,  subcolor: 0x%02X  ||||  color: 0x%02X ---\n",k,subcolor,color);
//	if(subcolor==color);
//		k=0;

	fbset_color(subcolor);
	draw_filled_rect(&gv_fb_dev, 100, 200, 100+80, 200+80);

	k += delt; /* Y Max.255, k to be little enough */
	if(subcolor==0xFFFF)delt=-2;
	if(subcolor==0x0000)delt=2;

//	tm_delayms(50);
//	usleep(300000);
}

  	/* quit logger */
  	egi_quit_log();

        /* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);


	return 0;
}

