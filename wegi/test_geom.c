/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Test EGI FBGEOM functions

Midas Zhou
-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "egi_timer.h"
#include "egi_cstring.h"
#include "egi_fbgeom.h"
#include "egi_symbol.h"
#include "egi_color.h"
#include "egi_log.h"
#include "egi.h"
#include "egi_math.h"

int main(void)
{
	int i,k;

	EGI_16BIT_COLOR color[3],subcolor[3];

//   for(i=0;i<30;i++) {
//	printf("sqrt of %ld is %ld. \n", 1<<i, (mat_fp16_sqrtu32(1<<i)) >> 16 );
//  }
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

	/* --- load all symbol pages --- */
	symbol_load_allpages();



#if 0
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
	int div=4;/* 4 deg per pixel, 240*2=480deg */
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
/* END >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
#endif


#if 0
/* <<<<<<<<<<<<<<  test line Chart  <<<<<<<<<<<<<<<*/
	int num=240/10+1; /* number of data */
	int cdata[240]={0}; /* 240 data */
	EGI_POINT points[240/1]; /* points */


while(1)
{
        /* flush FB FILO */
	printf("start to flush filo...\n");
        fb_filo_flush(&gv_fb_dev);
	/* draw poly line with FB FILO */
        fb_filo_on(&gv_fb_dev);

	/* ----------Prepare data and draw plines */
	fbset_color(WEGI_COLOR_ORANGE);//GREEN);
        for(i=0; i<num; i++)
	{
		points[i].x=i*(240/24); /* assign X */
		points[i].y=100+egi_random_max(80); /* assign Y */
	}
	printf("draw ploy lines...\n");
	draw_pline(&gv_fb_dev, points, num, 3);
	/* ----------Prepare data and draw plines */
	fbset_color(WEGI_COLOR_GREEN);
        for(i=0; i<num; i++)
	{
		points[i].x=i*(240/24); /* assign X */
		points[i].y=80+egi_random_max(120); /* assign Y */
	}
	printf("draw ploy lines...\n");
	draw_pline(&gv_fb_dev, points, num, 3);


        fb_filo_off(&gv_fb_dev);

	tm_delayms(55);
}

/* END >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
#endif


#if 1
/* >>>>>>>>>>>>>>>>>>>>>  TEST STOCK MARKET DATA  DISPLAY >>>>>>>>>>>>>>>>*/
        FILE *fil;
        char data[256];
        char *pt,*pt1,*pt2;
        float point,price,volume,turnover;
	int num=61; /* 240/4+1 */

	float data_point[240/4+1]={0}; /* store stock point */
	float fmin=0;
	float fmax=0;
	EGI_POINT pxy[61]={0}; /* for drawing line */
	int offy=310; /* offset of Y axis */
	int   wh=240; /* height of display window */
	int   hlgap=40; /* grid H LINE gap */
	int   unit_h=wh; /* =wh/limit_gap, in pixel, unit height */
	int point_bench=3250;
	char strdata[64]={0};


	/*-------TEST ------------*/
	iw_http_request("hq.sinajs.cn", "/list=s_sh000001", data);
	printf("reply data:%s\n",data);
	return 0;



	/* init pxy.X and data_point[] */
	for(i=0;i<num;i++) {
		pxy[i].x=i*4;
		//data_point[i]=point_bench;
	}
	pxy[num-1].x=240-1;


	/* clear areana */
	fbset_color(WEGI_COLOR_BLACK);
	draw_filled_rect(&gv_fb_dev,0,30,240-1,320-1);

	/* draw grids */
	fbset_color(WEGI_COLOR_GRAY3);
        for(i=0;i<=wh/hlgap;i++)
	 	draw_line(&gv_fb_dev,0,offy-hlgap*i,240-1,offy-hlgap*i);
//	draw_line(&gv_fb_dev,0,320-1,240-1,320-1);
	/* make some dash line effects */
	for(i=0;i<11;i++)
	{
		fbset_color(WEGI_COLOR_BLACK);
		draw_wline(&gv_fb_dev,20+i*20,40,20+i*20,320-1,9);
	}

	/* draw mid line as open price, bench_mark */
	fbset_color(WEGI_COLOR_GRAY3);
 	draw_line(&gv_fb_dev,0,offy-hlgap*3,240-1,offy-hlgap*3);


while(1)
{
	/* open data file */
        fil=fopen("/tmp/gp.data","r");
        if(fil==NULL) {
                printf("Open data file: %s. \n",strerror(errno));
                return -1;
        }

        fseek(fil,0,SEEK_SET);
        memset(data,0,sizeof(data));
        fgets(data,sizeof(data),fil);
        fclose(fil);

	/* -----TEST: cstr_split_nstr() ------ */
	i=1;
	do {
		pt=cstr_split_nstr(data, ",", i);
		if(pt!=NULL)
			printf("data[%d]: %s\n",i,pt);
		i++;
	}while(pt != NULL);


        /* extract data: name,current_point,current_price,up_down_rate, volume, turn_over, */
        //pt1=strstr(data,",");
	pt=cstr_split_nstr(data,",",1); /* get point to the char aft 1st ',' token */
        if(pt !=NULL ) {

		/* shift data and push point value into data_point[num-1] */
		memmove(data_point,data_point+1,sizeof(data_point)-sizeof(data_point[0]));
                data_point[num-1]=atof(pt);

		/* update fmin,fmax */
		if(fmin==0)
			fmin=data_point[num-1];
		else if(data_point[num-1]<fmin)
			fmin=data_point[num-1];

		if(fmax==0)
			fmax=data_point[num-1];
		else if(data_point[num-1]>fmax)
			fmax=data_point[num-1];

		/* extract price */
//                pt2=strstr((pt1+1),",");
  //              if(pt2 !=NULL ) {
    //                    price=atof(pt2+1);
      //                  printf("point: %f, price: %f \n",data_point[num-1],price);
        //        }

        }

	/* get limit gap */
	//unit_h=wh;
	if(fmax != fmin)
		unit_h=wh/(fmax-fmin);

	/* update pxy */
	for(i=0;i<num;i++)
	{
		/* if 0, fill data_point[]  with lastest data */
		if(data_point[i]==0)
			data_point[i]=data_point[num-1];

		/* --------update pxy according to data_point[] -------*/
	 	pxy[i].y=offy-(data_point[i]-fmin)*unit_h;
	}


        /* flush FB FILO */
	printf("start to flush filo...\n");
        fb_filo_flush(&gv_fb_dev);
        fb_filo_on(&gv_fb_dev);

	/* draw point ploy line */
	fbset_color(WEGI_COLOR_ORANGE);//GREEN);
	draw_pline(&gv_fb_dev, pxy, num, 0);

	/* sym_write fmin,fmax and current point value */
	sprintf(strdata,"%0.2f",fmax);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_RED,
                                        	1, 0, 50 ,strdata); /* transpcolor, x0,y0, str */
	sprintf(strdata,"%0.2f",fmin);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_GREEN,
                                        	1, 0, 320-30, strdata); /* transpcolor, x0,y0, str */
	sprintf(strdata,"%0.2f",data_point[num-1]); /* current point */
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_WHITE,
                                        	1, 240-70, 50, strdata); /* transpcolor, x0,y0, str */



	/* turn off FILO */
	fb_filo_off(&gv_fb_dev);

        tm_delayms(1000);
}

/* <<<<<<<<<<<<<<<<<<<<<  END TEST  <<<<<<<<<<<<<<<<<<*/
#endif



  	/* quit logger */
  	egi_quit_log();

        /* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);

	return 0;
}

