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
#include <math.h>
#include "egi_timer.h"
#include "egi_iwinfo.h"
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
/*
   0. Http request hq.sinajs.cn to get stock data.
   1. Scale value for upper. and lower part of the chart are different.
      Initiative scale value of 'unit_upp' and 'unit_low' to be set as 12pixels per_point.

   2. Initiative fmax and fmin:
       INDEX: fmax=fbench+unit_upp*wh/2(/10 for STOCK price)
              fmin=fbench-unit_low*wh/2(/10 for STOCK price)

*/
        char data[256];		/* for http REQUEST reply buff */
        char *pt;
        float point;//price,volume,turnover;
	int num=61; /* 240/4+1 */

	/* float precision: 6-7 digitals, double precisio: 15-16 digitals */
	float data_point[240/4+1]={0}; /* store stock point for every second*/
	float fmin=0; 		/* Min value for all data_point[] history data */
	float fmax=0; 		/* Max value for all data_point[] history data */
	float fbench=0;		/* Bench value, usually Yesterday's value */
	EGI_POINT pxy[61]={0};  /* for drawing line */
	EGI_16BIT_COLOR symcolor;

	int   wh=240; 		/* height of display window */
	int   ng=6;		/* number of grids/slots */
	int   hlgap=wh/ng; 	/* grid H LINE gap */

	float fupp_dev; /* fupp_dev=fmax-fbench, if fmax>fbench;  OR fupp_dev=0;  */
	float flow_dev; /* flow_dev=fbench-fmin, if fmin<fbench;  OR flow_dev=0;  */
	int   upp_ng=3;	/* = fupp_dev/(fupp_dev+flow_dev)*6,  */
	int   low_ng=3;	/* = ng-upp_ng; */
	int   offy=70+hlgap*upp_nag;
	//int   offy=190;//310-120; /* offset of axis to bench mark  */
//	int   unit_h=wh; 	/* unit_h=wh/limit_gap, in pixel, unit height. MUST give an init value. */
	float   upp_limit; 	/* =fbench+unit_upp*wh/w, upper bar of chart */
	float   low_limit; 	/* =fbench-unit_low*wh/w, low bar of char */
	float	famp=1;	        /* amplitude of point/price fluctuation, init=10*/
	float   funit=wh/2/famp; /* 240/2/famp, pixles per point/price  */
//	float   unit_upp=240/2.0/10.0; /* init. 12pix/point; only if fmax>fbench; unit_upp=(wh/2)/(fmax-fbench) */
//	float   unit_low=240/2.0/10.0; /* init. 12pix/point; only if fmin<fbench; unit_low=(wh/2)/(fbench-fmin) */
	char strdata[64]={0};
//	char *sname="s_sh000001"; /* Index or stock name */
//	char *sname="sh600389";/* Index or stock name */
	char *sname="sz000931";
	char request[32];
	int px,py;
	int pn;

	/* generate request string */
	memset(request,0,sizeof(request));
	strcat(request,"/list=");
	strcat(request,sname);

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
	 	draw_line(&gv_fb_dev,0,offy+wh/2-hlgap*i,240-1,offy+wh/2-hlgap*i);

	/* make some dash line effects */
	for(i=0;i<11;i++)
	{
		fbset_color(WEGI_COLOR_BLACK);
		draw_wline(&gv_fb_dev,20+i*20,40,20+i*20,320-1,9);
	}

	/* draw mid line as open price, bench_mark */
	fbset_color(WEGI_COLOR_GRAY3);
 	draw_line(&gv_fb_dev,0,offy+wh/2-hlgap*3,240-1,offy+wh/2-hlgap*3);

while(1)
{
	/* HTTP REQEST for data */
	if( iw_http_request("hq.sinajs.cn", request, data) !=0 ) {
		tm_delayms(500);
		continue;
	}
	printf("HTTP Reply: %s\n",data);


	/* First, get bench mark value and fmax,fmin.
	 * Usually bench mark value is yesterday's price or point value
	 * Run once only.
	 */
	if(fbench==0)
	{
		/* If INDEX, get current point */
		if(strstr(sname,"s_")) {
			pn=1;
			data_point[num-1]=atof(cstr_split_nstr(data,",",pn));/* get current point */
			pt=cstr_split_nstr(data,",",pn+1); /* get deviation pointer */
			if(pt==NULL) {
				printf("%s: reply data format error.\n",__func__);
				return;
			}
			fbench=data_point[num-1]-atof(pt);	/* yesterday's point=current-dev */
			printf("Yesterday's Index: fbench=%0.2f\n",fbench);
		}
		/* If STOCK, get yesterday's closing price as bench mark */
		else {
			pn=2;
			pt=cstr_split_nstr(data,",",pn); /* get yesterday closing price pointer */
			if(pt==NULL) {
				printf("%s: reply data format error.\n",__func__);
				return;
			}
			fbench=atof(pt);
			printf("Yestearday's Price: fbench=%0.2f\n",fbench);
		}

		/* init fmax,fmin whit current point */
		fmax=data_point[num-1];
		fmin=data_point[num-1];

		/* init upper and lower limit bar */
		upp_limit=fbench+famp;
		low_limit=fbench-famp;

		/* Init data_point[]  with fbench (yesterday's points/price) */
		for(i=0;i<num;i++)
			data_point[i]=fbench;
	}


        /* extract data:
	 *
	 * 1. INDEX data format:
	 *  	index name, current_point,up_down value,up_down_rate, volume, turn_over,
	 *
	 * 2. STOCK data format:
	 *  	stock name, Today's opening_price, Yestearday 's closing_price, current_price,
	 *	Highest price, Lowest price, Buy 1 price, Sell 1 price, number of traded shares/100
	 *	traded value/10000, Buy1 Volume, Buy1 Price,......Sell1 Volume, Sell1 Price....
	 */

      /* <<<<<<<<  INDEX  >>>>>>>> */
	if(strstr(sname,"s_")){  /* STOCK INDEX:  after 1st ',' token */
		pn=1;
	}
	else {		/* STOCK PRICE: after 3rd ',' token */
		pn=3;
	}
	pt=cstr_split_nstr(data,",",pn);
        if(pt !=NULL ) {
		/* shift data and push point value into data_point[num-1] */
		memmove(data_point,data_point+1,sizeof(data_point)-sizeof(data_point[0]));
		/* update the latest point value */
                data_point[num-1]=atof(pt);
		printf("latest data_point=%0.2f \n", data_point[num-1]);

		/* update funit */
		if( fabs(data_point[num-1]-fbench) > famp )
		{
			famp=fabs(data_point[num-1]-fbench);
			upp_limit=fbench+famp;
			low_limit=fbench-famp;
			funit=wh/2/famp;
		}

		/* update fmin,fmax */
		if(data_point[num-1]<fmin) {
			fmin=data_point[num-1];
		}
		if(data_point[num-1]>fmax) {
			fmax=data_point[num-1];
		}


        }

      /* <<<<<<<<  STOCK  >>>>>>>> */

	/* update the latest pxy according to the latest data_point*/
	for(i=0;i<num;i++)
	{
		/* update PXY according to data_point[] */
		if(data_point[i]>fbench)
		 	pxy[i].y=offy-(data_point[i]-fbench)*funit;
		else
		 	pxy[i].y=offy+(fbench-data_point[i])*funit;
//		printf("unit_upp: %0.2f,  unit_low: %0.2f, data_point[%d]=%0.2f, pxy[%d].y=%d \n",
//						unit_upp,unit_low,i,data_point[i],i,pxy[i].y);

	}

        /*  Flush FB FILO */
        fb_filo_flush(&gv_fb_dev);
        fb_filo_on(&gv_fb_dev);

	/* draw  ploy line */
	fbset_color(WEGI_COLOR_YELLOW);//GREEN);
	draw_pline(&gv_fb_dev, pxy, num, 0);

	/* ----- Draw marks and symbols ------ */
	/* INDEX or STOCK name */
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_CYAN,
                                        	1, 70, 30 ,sname); /* transpcolor, x0,y0, str */
	/* fbench value */
	sprintf(strdata,"%0.2f",fbench);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_GRAY3,
                                        	1, 80, 165 ,strdata); /* transpcolor, x0,y0, str */
	/* upp_limit value */
	sprintf(strdata,"%0.2f",upp_limit); //fbench+(wh/2)/funit);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_GRAY3,
                                        	1, 80, 50 ,strdata); /* transpcolor, x0,y0, str */
	/* low_limit value */
	sprintf(strdata,"%0.2f",low_limit); //fbench-(wh/2)/funit);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_GRAY3,
                                        	1, 80, 320-30, strdata); /* transpcolor, x0,y0, str */
	/* ---- draw fmax mark---- */
	if(fmax > fbench) {
		py=offy-(fmax-fbench)*funit;
		//symcolor=WEGI_COLOR_RED;
	}
	else {
		py=offy+(fbench-fmax)*funit;
		//symcolor=WEGI_COLOR_GREEN;
	}
	symcolor=WEGI_COLOR_ORANGE;
	fbset_color(symcolor);
	draw_wline(&gv_fb_dev, 0,py,60,py,0);
	sprintf(strdata,"%0.2f",fmax); //fbench+(wh/2)/unit_upp);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, symcolor,
                                        	1, 0, py-20 ,strdata); /* transpcolor, x0,y0, str */

	/* ---- draw fmin mark---- */
	if(fmin > fbench) {
		py=offy-(fmin-fbench)*funit;
		//symcolor=WEGI_COLOR_RED;
	}
	else {
		py=offy+(fbench-fmin)*funit;
		//symcolor=WEGI_COLOR_GREEN;
	}
	symcolor=WEGI_COLOR_BLUE;
	fbset_color(symcolor);
	draw_wline(&gv_fb_dev, 0,py,60,py,0);
	sprintf(strdata,"%0.2f",fmin); //fbench+(wh/2)/unit_upp);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, symcolor,
                                        	1, 0, py-20 ,strdata); /* transpcolor, x0,y0, str */

	/* current point/price value */
	if(data_point[num-1]>fbench)symcolor=WEGI_COLOR_RED;
	else {	symcolor = WEGI_COLOR_GREEN; }
	sprintf(strdata,"%0.2f",data_point[num-1]);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, symcolor,
                                        	1, 240-70, pxy[num-1].y-20, strdata); /* transpcolor, x0,y0, str */

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

