/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

A program to draw stock index/price in a line chart.

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

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  TEST STOCK MARKET DATA  DISPLAY >>>>>>>>>>>>>>>>>>>>>>>>>*/
/* NOTE:
   0. Http request hq.sinajs.cn to get stock data.
   1. Initiate 'fdmax' and 'fdmin' with 'fbench' instead of first input data_point[].
   2. Initiate 'famp' with a value that is smaller than the real amplitude of fluctuation,
      so the chart will reflect the fluctuation sufficiently.
      A too big value results in small fluctuation in the chart, and may be indiscernible.
   3. Initiate data_point[] with the opening point/price.
   4. Adjust weight value to chart_time_range/sampling_time_gap to get a reasonable chart result.
*/
        char data[256];		/* for http REQUEST reply buff */
        char *pt;
        float point;//price,volume,turnover;
	int num=61; /* 240/4+1 */

	/* Float Precision: 6-7 digitals, double precisio: 15-16 digitals */

	float data_point[240/4+1]={0}; /* store stock point/price for every second,or time_gap as set*/
	float fdmin=0; 		/* Min value for all data_point[] history data */
	float fdmax=0; 		/* Max value for all data_point[] history data */
	float fbench=0;		/* Bench value, usually Yesterday's value */
	EGI_POINT pxy[61]={0};  /* for drawing line */
	EGI_16BIT_COLOR symcolor;

	int   wh=210; 		/* height of display window */
	int   ng=6;		/* number of grids/slots */
	int   hlgap=wh/ng; 	/* grid H LINE gap */

	/* updd and lower deviation from benchmark */
	float fupp_dev=0; /* fupp_dev=fdmax-fbench, if fdmax>fbench;  OR fupp_dev=0;  */
	float flow_dev=0; /* flow_dev=fbench-fdmin, if fdmin<fbench;  OR flow_dev=0;  */

	int   upp_ng=3;	/* upper amplitude grid number */
	int   low_ng=3;	/* lower amplitude grid number */
	/*    if(flow_dev<fupp_dev), low_ng=flow_dev/(fupp_dev+flow_dev)+1;
	 *    else if(flow_dev>fupp_dev), upp_ng=fupp_dev/(fupp_dev+flow_dev)+1;
	 *    else: keep defalt upp_ng and low_ng
	 */
	int   chart_x0=0;  /* char x0 as of LCD FB */
	int   chart_y0=70; /* chart y0 as of LCD FB */ 
	int   offy=chart_y0+hlgap*upp_ng;  /* bench mark line offset value, offset from LCD y=0 */
	//int   offy=190;//310-120; /* offset of axis to bench mark  */
//	int   unit_h=wh; 	/* unit_h=wh/limit_gap, in pixel, unit height. MUST give an init value. */
	/* chart upp limit and low limit value */
	float   upp_limit; 	/* upper bar of chart */
	float   low_limit; 	/* low bar of char */
	float	famp=0.05;	        /* amplitude of point/price fluctuation, init=10*/
	float   funit=wh/2/famp; /* 240/2/famp, pixles per point/price  */
//	float   unit_upp=240/2.0/10.0; /* init. 12pix/point; only if fdmax>fbench; unit_upp=(wh/2)/(fdmax-fbench) */
//	float   unit_low=240/2.0/10.0; /* init. 12pix/point; only if fdmin<fbench; unit_low=(wh/2)/(fbench-fdmin) */
	char strdata[64]={0};
	char *sname="s_sh000001"; /* Index or stock name */
//	char *sname="sh600389";/* Index or stock name */
//	char *sname="sz000931";
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
	//fbset_color(WEGI_COLOR_BLACK);
	//draw_filled_rect(&gv_fb_dev,0,30,240-1,320-1);
	clear_screen(&gv_fb_dev,WEGI_COLOR_BLACK);

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

/*  <<<<<<<<<<<   Loop reading data and drawing line chart    >>>>>>>>>>  */
while(1)
{
	/* HTTP REQEST for data */
	if( iw_http_request("hq.sinajs.cn", request, data) !=0 ) {
		tm_delayms(1000);
		continue;
	}
	printf("HTTP Reply: %s\n",data);


	/* First, get bench mark value and fdmax,fdmin.
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
				return -1;
			}
			fbench=data_point[num-1]-atof(pt);	/* yesterday's point=current-dev */
			printf("Yesterday's Index: fbench=%0.2f\n",fbench);
		}
		/* If STOCK, get yesterday's closing price as bench mark */
		else {
			pn=2;
			data_point[num-1]=atof(cstr_split_nstr(data,",",pn+1));/* get current point */
			pt=cstr_split_nstr(data,",",pn); /* get yesterday closing price pointer */
			if(pt==NULL) {
				printf("%s: reply data format error.\n",__func__);
				return -2;
			}
			fbench=atof(pt);
			printf("Yestearday's Price: fbench=%0.2f\n",fbench);
		}

		/* init fdmax,fdmin whit current point */
		fdmax=fbench; //data_point[num-1];
		fdmin=fbench; //data_point[num-1];

		/* init upper and lower limit bar */
		upp_limit=fbench+famp;
		low_limit=fbench-famp;

		/* Init data_point[]  with fbench (yesterday's points/price) */
		for(i=0;i<num;i++)
			data_point[i]=data_point[num-1];//fbench;
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
	if(strstr(sname,"s_")){  /* Get STOCK INDEX:  after 1st ',' token */
		pn=1;
	}
      /* <<<<<<<<  STOCK  >>>>>>>> */
	else {		/* Get STOCK PRICE: after 3rd ',' token */
		pn=3;
	}
	pt=cstr_split_nstr(data,",",pn);
        if(pt !=NULL ) {

#if 0
		/* METHOD 1: shift data and push point value into data_point[num-1] */
		memmove(data_point,data_point+1,sizeof(data_point)-sizeof(data_point[0]));
#endif


#if 1
		/* METHOD 2: average compression, but keep lastest data_point[num-1]  */
		for(i=0;i<num-1;i++) {
			//data_point[i]=(data_point[i]+data_point[i+1])/2.0; /* AVERAGE */
			//data_point[i]=(data_point[i]*10.0+data_point[i+1])/11.0; /* SMOOTH_BIA AVERAGE */
			data_point[i]=( data_point[i]*( (num-i)*1 )+ data_point[i+1])
							/ ( (num-i)*1+1 ); /* SMOOTH_BIA AVERAGE */
		}
#endif

		/* 1. Update the latest point value */
                data_point[num-1]=atof(pt);
		printf("latest data_point=%0.2f \n", data_point[num-1]);

		/* 2. Update fdmin, fdmax, flow_dev, fupp_dev */
		if(data_point[num-1] < fdmin) {
			fdmin=data_point[num-1];
			/* update lower deviation */
			if(fdmin < fbench)
				flow_dev=fbench-fdmin;
		}
		else if(data_point[num-1] > fdmax) {
			fdmax=data_point[num-1];
			/* updata upper deviation */
			if(fdmax > fbench)
				fupp_dev=fdmax-fbench;
		}
		/* Init value as we set before,fdmix=fdmax=data_point[num-1]
		 * Just trigger to calculate fupp_dev and flow_dev.
		 */
		else if(fdmin==fdmax) {
			if(fdmax > fbench)
			  	fupp_dev=fdmax-fbench;
			if(fdmin < fbench)
				flow_dev=fbench-fdmin;
		}

		/* 3. Update upp and low grid number
		 * NOTE: upp_ng is NOT equal to fdmax, also low_ng is NOT equal to fdmin.!!!
		 * They are the same only when fdmax>fbench and fdmin<fbench.
		 */
		if(flow_dev<fupp_dev) {
			low_ng=flow_dev/(fupp_dev+flow_dev)+1;
			upp_ng=ng-low_ng;
			/* update funit */
			funit=fmin(upp_ng*hlgap/fupp_dev, low_ng*hlgap/flow_dev);
		}
		else if(flow_dev>fupp_dev) {
			upp_ng=fupp_dev/(fupp_dev+flow_dev)+1;
			low_ng=ng-upp_ng;
			/* update funit */
			funit=fmin(upp_ng*hlgap/fupp_dev, low_ng*hlgap/flow_dev);
		}
		else  { //elseif(flow_dev==fupp_dev)
			upp_ng=ng>>1;
			low_ng=ng>>1;
		}

		/* 4. Update offy for benchmark line */
		offy=chart_y0+hlgap*upp_ng;

		/* 5. Update upp_limit and low_limit */
		upp_limit=fbench+upp_ng*hlgap/funit;
		low_limit=fbench-low_ng*hlgap/funit;

		printf("funit=%0.3f, hlgap/funit=%0.3f \n",funit,hlgap/funit);
		printf("low_ng=%d, upp_ng=%d.\n",low_ng,upp_ng);
		printf("upp_limit=%0.2f, low_limit=%0.2f \n",upp_limit,low_limit);
        }

	/* update the latest pxy according to the latest data_point*/
	for(i=0;i<num;i++)
	{
		/* update PXY according to data_point[] */
		if(data_point[i]>fbench)
		 	pxy[i].y=offy-(data_point[i]-fbench)*funit;
		else
		 	pxy[i].y=offy+(fbench-data_point[i])*funit;
	}

	/* <<<<<<<    Flush FB and Turn on FILO  >>>>>>> */
        fb_filo_flush(&gv_fb_dev); /* flush and restore old FB pixel data */
        fb_filo_on(&gv_fb_dev); /* start collecting old FB pixel data */

	/* 1. Draw bench_mark line according to low_ng and upp_ng */
	fbset_color(WEGI_COLOR_GRAY3);
 	draw_line(&gv_fb_dev,0,offy,240-1,offy);

	/* 2. Draw poy line of Pxy */
	fbset_color(WEGI_COLOR_YELLOW);//GREEN);
	draw_pline(&gv_fb_dev, pxy, num, 0);

	/* 3. ----- Draw marks and symbols ------ */
	/* 3.1 INDEX or STOCK name */
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_CYAN,
                                        	1, 70, 30 ,sname); /* transpcolor, x0,y0, str */
	/* 3.2 fbench value */
	sprintf(strdata,"%0.2f",fbench);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_GRAY3,
                                        	1, 80, chart_y0+upp_ng*hlgap-20 ,strdata); /* transpcolor, x0,y0, str */
	/* 3.3 upp_limit value */
	sprintf(strdata,"%0.2f",upp_limit);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_GRAY3,
                                        	1, 80, chart_y0-20 ,strdata); /* transpcolor, x0,y0, str */
	/* 3.3 low_limit value */
	sprintf(strdata,"%0.2f",low_limit);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_GRAY3,
                                        	1, 80, chart_y0+wh-20, strdata); /* transpcolor, x0,y0, str */
	/* 3.4 draw fdmax mark */
	if(fdmax > fbench) {
		py=offy-(fdmax-fbench)*funit;
		//symcolor=WEGI_COLOR_RED;
	}
	else {
		py=offy+(fbench-fdmax)*funit;
		//symcolor=WEGI_COLOR_GREEN;
	}
	symcolor=WEGI_COLOR_ORANGE;
	fbset_color(symcolor);
	draw_wline(&gv_fb_dev, 0,py,60,py,0);
	sprintf(strdata,"%0.2f",fdmax); //fbench+(wh/2)/unit_upp);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, symcolor,
                                        	1, 0, py-20 ,strdata); /* transpcolor, x0,y0, str */
	/* 3.5 draw fdmin mark */
	if(fdmin > fbench) {
		py=offy-(fdmin-fbench)*funit;
		//symcolor=WEGI_COLOR_RED;
	}
	else {
		py=offy+(fbench-fdmin)*funit;
		//symcolor=WEGI_COLOR_GREEN;
	}
	printf(" draw fdmin mark: fdmin=%0.2f.\n",fdmin);
	symcolor=WEGI_COLOR_BLUE;
	fbset_color(symcolor);
	draw_wline(&gv_fb_dev, 0,py,60,py,0);
	sprintf(strdata,"%0.2f",fdmin); //fbench+(wh/2)/unit_upp);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, symcolor,
                                        	1, 0, py-20 ,strdata); /* transpcolor, x0,y0, str */
	/* 3.6 write current point/price value */
	symcolor=WEGI_COLOR_YELLOW;
	sprintf(strdata,"%0.2f",data_point[num-1]);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, symcolor,
                                       	1, 240-70, pxy[num-1].y-20, strdata); /* transpcolor, x0,y0, str */
	/* 3.7 write up_down value */
	if(data_point[num-1]>fbench)symcolor=WEGI_COLOR_RED;
	else {	symcolor = WEGI_COLOR_GREEN; }	sprintf(strdata,"%+0.2f",data_point[num-1]-fbench);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, symcolor,
                                       	1, 240-70, pxy[num-1].y-20-20, strdata); /* transpcolor, x0,y0, str */


	/* <<<<<<<    Turn off FILO  >>>>>>> */
	fb_filo_off(&gv_fb_dev);

        tm_delayms(1000);
}

/* <<<<<<<<<<<<<<<<<<<<<  END TEST  <<<<<<<<<<<<<<<<<<*/

  	/* quit logger */
  	egi_quit_log();

        /* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);

	return 0;
}

