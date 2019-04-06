/*-------------------------------
Test EGI FBGEOM functions

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
#include "egi.h"
#include "egi_math.h"

int main(void)
{
	int i,k;

	EGI_16BIT_COLOR color[3],subcolor[3];

   for(i=0;i<30;i++) {
	printf("sqrt of %d is %ld. \n", 1<<i, (mat_fp16_sqrtu32(1<<i)) >> 16 );
  }
	/* --- init logger --- */
  	if(egi_init_log("/mmc/log_color") != 0)
	{
		printf("Fail to init logger,quit.\n");
		return -1;
	}

        /* --- start egi tick --- */
        tm_start_egitick();

        /* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

	/* <<<<<<<<<<<<<<  test draw_wline & draw_pline  <<<<<<<<<<<<<<<*/
/*
	EGI_POINT p1,p2;
	EGI_BOX box={{0,0},{240-1,320-1,}};
   while(1)
  {
	egi_randp_inbox(&p1, &box);
	egi_randp_inbox(&p2, &box);
	fbset_color(egi_color_random(medium));
	draw_wline(&gv_fb_dev,p1.x,p1.y,p2.x,p2.y,egi_random_max(11));

	usleep(200000);
  }
*/

	int rad=50; /* radius */
	int div=4;/* 2 deg per pixel, 240*2=480deg */
	int num=240/1; /* number of points */
	EGI_POINT points[240/1]; /* points */
	int delt=0;

	mat_create_fptrigontab();/* create lookup table for trigonometric funcs */

	/* clear areana */
	fbset_color(WEGI_COLOR_BLACK);
	draw_filled_rect(&gv_fb_dev,0,30,240-1,320-100);

	/* draw a square area of grids */
	fbset_color(WEGI_COLOR_GREEN);
	for(i=0;i<6;i++)
	 	draw_line(&gv_fb_dev,0,30+5+i*40,240-1,30+5+i*40);
 	draw_line(&gv_fb_dev,0,30+5+6*40-1,240-1,30+5+6*40-1); //when i=6

	for(i=0;i<6;i++)
	 	draw_line(&gv_fb_dev,0+i*40,30+5,0+i*40,30+5+240-1);
 	draw_line(&gv_fb_dev,0+6*40-1,30+5,0+6*40-1,30+5+240-1); //when i=6

	/* draw wlines */
	fbset_color(WEGI_COLOR_ORANGE);//GREEN);
while(1)
{
        /* flush FB FILO */
	printf("start to flush filo...\n");
        fb_filo_flush(&gv_fb_dev);
	/* draw poly line with FB FILO */
        fb_filo_on(&gv_fb_dev);

	/* 1. generate sin() points  */
	for(i=0;i<num;i++) {
		points[i].x=i;
		points[i].y=(rad*2*fp16_sin[(i*div+delt)%360]>>16)+40*3+35; /* 4*rad to shift image to Y+ */
	}
	fbset_color(WEGI_COLOR_BLUE);//GREEN);
	draw_pline(&gv_fb_dev, points, num, 5);

	/* 2. generate AN OTHER sin() points  */
	for(i=0;i<num;i++) {
		points[i].x=i;
		points[i].y=(rad*fp16_sin[( i*div + (delt<<1) )%360]>>16)+40*3+35; /* 4*rad to shift image to Y+ */
	}
	fbset_color(WEGI_COLOR_ORANGE);//GREEN);
	draw_pline(&gv_fb_dev, points, num, 5);

	/* alway shut off filo after use */
	fb_filo_off(&gv_fb_dev);

	tm_delayms(80);
//	egi_sleep(0,0,150);

	delt+=16;
}
	/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

  	/* quit logger */
  	egi_quit_log();

        /* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);


	return 0;
}

