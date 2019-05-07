/*-------------------------------------------------------------------
page creation jobs:
1. egi_create_XXXpage() function.
   1.1 creating eboxes and page.
   1.2 assign thread-runner to the page.
   1.3 assign routine to the page.
   1.4 assign button functions to corresponding eboxes in page.
2. thread-runner functions.
3. egi_XXX_routine() function if not use default egi_page_routine().
4. add button reaction functins, mainly for creating pages.
5. group buttons for sliding.

TODO:
	1. different values for button return, and page return,
	   that egi_page_routine() can distinguish.
	2. pack page activate and free action.

Midas Zhou
--------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> /* usleep */
#include "egi.h"
#include "egi_debug.h"
#include "egi_log.h"
#include "egi_timer.h"
#include "egi_color.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
#include "egi_btn.h"
#include "egi_page.h"
#include "egi_symbol.h"
#include "egi_pagetest.h"
#include "egi_pagehome.h"
#include "egi_pagemplay.h"
#include "egi_pageopenwrt.h"
#include "egi_pagebook.h"
#include "egi_iwinfo.h"
#include "egi_pageffplay.h"
#include "iot/egi_iotclient.h"
#include "egi_pagestock.h"

static void egi_display_cpuload(EGI_PAGE *page);
static void egi_display_iotload(EGI_PAGE *page);
static int egi_homebtn_mplay(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_openwrt(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_book(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_test(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_ffplay(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_stock(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int react_slider(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);


static	EGI_EBOX *home_btns[9*HOME_PAGE_MAX];
static	EGI_DATA_BTN *data_btns[9*HOME_PAGE_MAX];
static	EGI_EBOX *home_slider;
static	EGI_DATA_BTN *data_slider;

static	EGI_DATA_TXT *head_txt;
static	EGI_EBOX  *ebox_headbar;

static  int npg=(2 < HOME_PAGE_MAX ? 2 : HOME_PAGE_MAX ); /* page number */

/* NOTE: 9 buttons for each home page */
static	int nr=3; /* btn row number */
static	int nc=3; /* btn column number */

/*------------- [  PAGE ::   Home Page  ] -------------
create home page
write tag corresponding to each button.

Return
	pointer to a page	OK
	NULL			fail
------------------------------------------------------*/
EGI_PAGE *egi_create_homepage(void)
{
	int k; /*number of pages */
	int i,j;
	int nicode; /* icon code number */
	struct symbol_page *sympg=NULL;


	/* -------- 1. create button eboxes -------- */
  for(k=0; k<npg; k++) /* page number */
  {
        for(i=0;i<nr;i++) /* row of buttons*/
        {
                for(j=0;j<nc;j++) /* column of buttons */
                {
			if(k<1) {
				nicode=nc*i+j;
				sympg=&sympg_buttons;
			}
			else {
				nicode=0;   /* TBD later */
				sympg=&sympg_buttons; /* TBD later */
			}

			/* 1.1 create new data_btns */
			data_btns[k*nr*nc+i*nc+j]=egi_btndata_new(k*nr*nc+i*nc+j, /* int id */
							square, /* enum egi_btn_type shape */
							sympg, /* struct symbol_page *icon */
							nicode, /* int icon_code */
							&sympg_testfont /* for ebox->tag font */
						);
			/* if fail, try again ... */
			if(data_btns[k*nr*nc+i*nc+j]==NULL)
			{
				printf("egi_create_homepage(): fail to call egi_btndata_new() for data_btns[%d]. retry...\n", 3*i+j);
				i--;
				continue;
			}

			/* 2.2 create new btn eboxes */
			home_btns[k*nr*nc+i*nc+j]=egi_btnbox_new(NULL, /* put tag later */
							data_btns[k*nr*nc+i*nc+j], /* EGI_DATA_BTN *egi_data */
				        		1, /* bool movable */
						        15+(15+60)*j + 240*k, /* int x0 */
							50+(15+60)*i, /* int y0 */
							60,60, /* int width, int height */
				       			0, /* int frame, -1 no frame */
		       					-1 /* int prmcolor, no prmcolor */
						   );

			/* if fail, try again ... */
			if(home_btns[k*nr*nc+i*nc+j]==NULL)
			{
				printf("egi_create_homepage(): fail to call egi_btnbox_new() for home_btns[%d]. retry...\n", 3*i+j);
				free(data_btns[k*nr*nc+i*nc+j]);
				data_btns[k*nr*nc+i*nc+j]=NULL;

				i--;
				continue;
			}
		}
	}
   }

	/* 1.3 add button tags and reactions here */
	egi_ebox_settag(home_btns[0], "btn_mplayer");
	home_btns[0]->reaction=egi_homebtn_mplay;

	egi_ebox_settag(home_btns[1], "btn_test");
	home_btns[1]->reaction=egi_homebtn_test;

	egi_ebox_settag(home_btns[2], "btn_alarm");

	egi_ebox_settag(home_btns[3], "btn_linphone");
	data_btns[3]->icon_code=9;
	home_btns[3]->reaction=egi_homebtn_openwrt;

	egi_ebox_settag(home_btns[4], "btn_key");

	egi_ebox_settag(home_btns[5], "btn_book");
	home_btns[5]->reaction=egi_homebtn_book;

	egi_ebox_settag(home_btns[6], "btn_chart");
	home_btns[6]->reaction=egi_homebtn_stock;

	egi_ebox_settag(home_btns[7], "0");//btn_iot"); /* id=7; as for bulb */
	data_btns[7]->icon_code=11; /* SUB_COLOR + ICON_CODE */
	data_btns[7]->showtag=true; /* display tag */
	data_btns[7]->font=&sympg_numbfont;
	//data_btns[7]->font_color=WEGI_COLOR_ORANGE;
	home_btns[7]->tag_color=WEGI_COLOR_ORANGE;

	egi_ebox_settag(home_btns[8], "btn_ffplay");
	home_btns[8]->reaction=egi_homebtn_ffplay;

	/* create a bkimg_btn for home_btns[7], withou cutout in icon image */
	EGI_EBOX * bkimg_btn7=egi_copy_btn_ebox(home_btns[7]);
	if(bkimg_btn7 != NULL) /* reset egi_data */
	{
		EGI_DATA_BTN *bkbtn_data=(bkimg_btn7->egi_data);
		bkbtn_data->icon_code=7;
		bkbtn_data->id=7; /*change id for later sort */
		egi_ebox_settag(bkimg_btn7, "btn_bigiot_frame");
		bkbtn_data->font=NULL;
	}
	else {
		printf("%s: Fail to copy for bkimg_btn7! \n",__func__);
		for(i=0;i<nr*nc;i++)
			egi_ebox_free(home_btns[i]);
		return NULL;
	}

	printf("btns[7]->egi_data->icon_code=%d\n",((EGI_DATA_BTN *)(home_btns[7]->egi_data))->icon_code);
	printf("bkimg_btn7->egi_data->icon_code=%d\n",((EGI_DATA_BTN *)(bkimg_btn7->egi_data))->icon_code);


	/* ------------ buttons in PAGE 2 ------------ */
	egi_ebox_settag(home_btns[9], "btn_video0");
	data_btns[9]->icon_code=3;
	home_btns[9]->reaction=NULL;
	egi_ebox_settag(home_btns[10], "btn_video1");
	data_btns[10]->icon_code=3;
	home_btns[10]->reaction=NULL;
	egi_ebox_settag(home_btns[11], "btn_video2");
	data_btns[11]->icon_code=3;
	home_btns[11]->reaction=NULL;


        /* --------- 1.4 create a sliding area --------- */
	data_slider=egi_btndata_new(100, /* int id */
					square, /* enum egi_btn_type shape */
					NULL, /* struct symbol_page *icon */
					0, /* int icon_code */
					NULL /* for ebox->tag font, NULL ignore */
				);
	data_slider->touch_effect=NULL; /* disable touch effect */

	home_slider=egi_btnbox_new("home_slider", /* put tag later */
					data_slider, /* EGI_DATA_BTN *egi_data */
		        		0, /* bool movable, True if need refresh */
				        0, 320-40, /* int x0, int y0 */
					240,40, /* int width, int height */
		       			0, /* int frame */
       					0 /*int prmcolor, >=0 if to show box*/
				   );

	home_slider->reaction=react_slider;

        /* --------- 2. create home head-bar --------- */
        /* create head_txt */
        head_txt=egi_txtdata_new(
                0,0, /* offset X,Y */
                1, /*int nl, lines  */
                9, /*int llen, chars per line, 1 for end token! */
                &sympg_icons, /*struct symbol_page *font */
                -1  /* < 0, use default symbol color in img, int16_t color */
        );
        /* fpath == NULL */
        /* create head ebox */
        ebox_headbar= egi_txtbox_new(
                "home_headbar", /* tag */
                head_txt,  /* EGI_DATA_TXT pointer */
                false, /* let home wallpaper take over, bool movable */
                0,0, /* int x0, int y0 */
                240,30, /* int width, int height */
                -1, /* int frame, -1=no frame */
                -1 //egi_colorgray_random(medium)/*int prmcolor, -1 transparent*/
        );
	/* set symbols in home head bar */
	head_txt->txt[0][0]=4;
	head_txt->txt[0][1]=6;
	head_txt->txt[0][2]=10;
	head_txt->txt[0][3]=28; /* 6 is space */
	head_txt->txt[0][4]=6;//33;
	head_txt->txt[0][5]=6;//36;
	head_txt->txt[0][6]=6;
	head_txt->txt[0][7]=6;
	/* !!! the last  MUST end with /0 */

	/* ---------- 3.create home page -------- */
	/* 3.1 create page */
	EGI_PAGE *page_home=NULL;
	page_home=egi_page_new("page_home");
	while(page_home==NULL)
	{
			printf("egi_create_homepage(): fail to call egi_page_new(), try again ...\n");
			page_home=egi_page_new("page_home");
			//usleep(100000);
	}
	/* set bk color, applicable only if fpath==NULL  */
	page_home->ebox->prmcolor=WEGI_COLOR_OCEAN;

	/* 3.2 put pthread runner, remind EGI_PAGE_MAXTHREADS 5  */
	page_home->runner[0]=egi_display_cpuload;
	page_home->runner[1]=egi_display_iotload;
//	page_home->runner[2]=egi_iotclient;
//	page_home->runner[3]=display_stock;

	/* 3.3 set default routine job */
	page_home->routine=egi_page_routine;

	/* 3.4 set wallpaper */
	page_home->fpath="/tmp/home.jpg";

	/* add ebox to home page */
	/* beware of the sequence of the ebox list */
	for(i=0;i<7;i++)
		egi_page_addlist(page_home, home_btns[i]);
	egi_page_addlist(page_home, home_btns[7]);
	egi_page_addlist(page_home, home_btns[8]);

	/* add PAGE 2 buttons */
	for(i=9;i<npg*nr*nc;i++)
		egi_page_addlist(page_home, home_btns[i]);

//turn_off bulb bkimg	egi_page_addlist(page_home, bkimg_btn7);
	egi_page_addlist(page_home,ebox_headbar);
	egi_page_addlist(page_home, home_slider);

	return page_home;
}

/*----------------  RUNNER 1 ----------------------
display cpu load in home head-bar with motion icons

1. read /proc/loadavg to get the value
	loadavg 1-6,  >5 alarm
2. corresponding symmic_cpuload[] index from 0-5.
-------------------------------------------------*/
static void egi_display_cpuload(EGI_PAGE *page)
{
	int load=0;
	int fd;
	char strload[5]={0}; /* read in 4 byte */

        /* open symbol image file */
        fd=open("/proc/loadavg", O_RDONLY);
        if(fd<0)
        {
                printf("egi_display_cpuload(): fail to open /proc/loadavg!\n");
                perror("egi_display_cpuload()");
                return;
        }

	EGI_PDEBUG(DBG_PAGE,"page '%s':  runner thread egi_display_cpuload() is activated!.\n",page->ebox->tag);
	while(1)
	{
		lseek(fd,0,SEEK_SET);
		read(fd,strload,4);
		//printf("----------------- strload: %s -----------------\n",strload);
		load=atoi(strload);/* for symmic_cpuload[], index from 0 to 5 */
		if(load>5)
			load=5;
		//printf("----------------- load: %d -----------------\n",load);
		/* load cpuload motion icons
		 *	  symbol_motion_string() is with---sleep---function inside
		 * WARNING: use global FB dev may cause rase condition
		 */
  	 	symbol_motion_string(&gv_fb_dev, 155-load*15, &sympg_icons,
		 					1, 210,0, &symmic_cpuload[load][0]);
	}
}

/*-----------------  RUNNER 2 --------------------------
Display IoT motion icon.
Display RSSI icon.
-------------------------------------------------------*/
static void egi_display_iotload(EGI_PAGE *page)
{
	int rssi;
	int index; /* index for RSSI of  sympg_icons[index]  */

	EGI_PDEBUG(DBG_PAGE,"page '%s':  runner thread egi_display_iotload() is activated!.\n"
										,page->ebox->tag);
	while(1)
	{
		/* 1. load IoT motion icons
			  symbol_motion_string() is with sleep function */
  	 	symbol_motion_string(&gv_fb_dev, 120, &sympg_icons, 1, 180,0, symmic_iotload);

		/* 2. get RSSI value */
		iw_get_rssi(&rssi);
		if(rssi > -65) index=5;
		else if(rssi > -73) index=4;
		else if(rssi > -80) index=3;
		else if(rssi > -94) index=2;
		else 	index=1;
		//EGI_PDEBUG(DBG_PAGE,"egi_display_itoload(): rssi=%d; index=%d \n",rssi,index);

		/* 3. draw RSSI symbol
		 * WARNING: use global FB dev may cause rase condition
		 */
		symbol_writeFB(&gv_fb_dev, &sympg_icons, SYM_NOSUB_COLOR, 0, 0, 0, index, 0);/*bkcolor=0*/
	}
}



/*----------------------------------------------
objet defined routine jobs of home page

1. see default egi_page_routine() in egi_page.c
-----------------------------------------------*/
void egi_home_routine(void)
{
 	/* 1. check wifi signal stress */

	/* 2. check CPU load */

	/* 3. check MQTT connections */

	/* 4. update home_page head-bar icons */
}


/*--------------------------------------------------------------------------
button_mplay function:
mplayer
--------------------------------------------------------------------------*/
static int egi_homebtn_mplay(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
	/* bypass unwanted touch status */
	if(touch_data->status != pressing)
		return btnret_IDLE;

	/* create page and load the page */
        EGI_PAGE *page_mplay=egi_create_mplaypage();
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is created.\n", page_mplay->ebox->tag);

	/* activate to display it */
        egi_page_activate(page_mplay);
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is activated.\n", page_mplay->ebox->tag);

	/* get into routine loop */
	EGI_PLOG(LOGLV_INFO,"Now trap into routine of [page '%s']...\n", page_mplay->ebox->tag);
        page_mplay->routine(page_mplay);

	/* get out of routine loop */
	EGI_PLOG(LOGLV_INFO,"Exit routine of [page '%s'], start to free the page...\n", page_mplay->ebox->tag);
	egi_page_free(page_mplay);

	return pgret_OK;
}

/*-------------------------------------------------------------------------
button_openwrt function:
openwrt
---------------------------------------------------------------------------*/
static int egi_homebtn_openwrt(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
	/* bypass unwanted touch status */
	if(touch_data->status != pressing)
		return btnret_IDLE;

	/* create page and load the page */
        EGI_PAGE *page_openwrt=egi_create_openwrtpage();
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is created.\n", page_openwrt->ebox->tag);

	/* activate to display it */
        egi_page_activate(page_openwrt);
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is activated.\n", page_openwrt->ebox->tag);

	/* get into routine loop */
	EGI_PLOG(LOGLV_INFO,"Now trap into routine of [page '%s']...\n", page_openwrt->ebox->tag);
        page_openwrt->routine(page_openwrt);

	/* get out of routine loop */
	EGI_PLOG(LOGLV_INFO,"Exit routine of [page '%s'], start to free the page...\n", page_openwrt->ebox->tag);
	egi_page_free(page_openwrt);

	return pgret_OK;
}

/*--------------------------------------------------------------------------
button_openwrt function:
book
----------------------------------------------------------------------------*/
static int egi_homebtn_book(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{

	/* bypass unwanted touch status */
	if(touch_data->status != pressing)
		return btnret_IDLE;

	/* create page and load the page */
        EGI_PAGE *page_book=egi_create_bookpage();
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is created.\n", page_book->ebox->tag);

	/* activate to display it */
        egi_page_activate(page_book);
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is activated.\n", page_book->ebox->tag);

	/* get into routine loop */
	EGI_PLOG(LOGLV_INFO,"Now trap into routine of [page '%s']...\n", page_book->ebox->tag);
        page_book->routine(page_book);

	/* get out of routine loop */
	EGI_PLOG(LOGLV_INFO,"Exit routine of [page '%s'], start to free the page...\n", page_book->ebox->tag);
	egi_page_free(page_book);

	return pgret_OK;
}

/*----------------------------------------------------------------------------
button_test function:
for test functions
-----------------------------------------------------------------------------*/
static int egi_homebtn_test(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
	/* bypass unwanted touch status */
	if(touch_data->status != pressing)
		return btnret_IDLE;

	/* create page and load the page */
        EGI_PAGE *page_test=egi_create_testpage();
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is created.\n", page_test->ebox->tag);

        egi_page_activate(page_test);
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is activated.\n", page_test->ebox->tag);

	/* get into routine loop */
	EGI_PLOG(LOGLV_INFO,"Now trap into routine of [page '%s']...\n", page_test->ebox->tag);
        page_test->routine(page_test);

	/* get out of routine loop */
	EGI_PLOG(LOGLV_INFO,"Exit routine of [page '%s'], start to free the page...\n", page_test->ebox->tag);
	egi_page_free(page_test);

	return pgret_OK;
}


/*----------------------------------------------------------------------------
button_test function:
for test functions
-----------------------------------------------------------------------------*/
static int egi_homebtn_ffplay(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
	/* bypass unwanted touch status */
	if( touch_data->status != pressing )
		return btnret_IDLE;

	/* create page and load the page */
        EGI_PAGE *page_ffplay=egi_create_ffplaypage();
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is created.\n", page_ffplay->ebox->tag);

        egi_page_activate(page_ffplay);
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is activated.\n", page_ffplay->ebox->tag);

	/* get into routine loop */
	EGI_PLOG(LOGLV_INFO,"Now trap into routine of [page '%s']...\n", page_ffplay->ebox->tag);
        page_ffplay->routine(page_ffplay);

//	/* exit a button activated page, set refresh flag for the host page, before page freeing.*/
//	egi_page_needrefresh(ebox->container); pgret_OK will set refresh

	/* get out of routine loop */
	EGI_PLOG(LOGLV_INFO,"Exit routine of [page '%s'], start to free the page...\n",
									page_ffplay->ebox->tag);
	egi_page_free(page_ffplay);

	return pgret_OK; /* return 0 --- for page exit */
}

/*----------------------------------------------------------------------------
button_stock chart function:
for test functions
-----------------------------------------------------------------------------*/
static int egi_homebtn_stock(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
	/* bypass unwanted touch status */
	if(touch_data->status != pressing)
		return btnret_IDLE;

	/* create page and load the page */
        EGI_PAGE *page_stock=egi_create_pagestock();
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is created.\n", page_stock->ebox->tag);

        egi_page_activate(page_stock);
	EGI_PLOG(LOGLV_INFO,"[page '%s'] is activated.\n", page_stock->ebox->tag);

	/* get into routine loop */
	EGI_PLOG(LOGLV_INFO,"Now trap into routine of [page '%s']...\n", page_stock->ebox->tag);
        page_stock->routine(page_stock);

	/* get out of routine loop */
	EGI_PLOG(LOGLV_INFO,"Exit routine of [page '%s'], start to free the page...\n", page_stock->ebox->tag);
	egi_page_free(page_stock);

	return pgret_OK;
}



/*-------------------------------------------------------------------
Home Sliding bar reaction
return
---------------------------------------------------------------------*/
static int react_slider(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        static int mark[9*HOME_PAGE_MAX]; /* >= nc*nr */
	int ns; /* start grid: 0,1,2,... */
	int step=0;
	int ntp;
	int token=0;
	int tmp;
	enum egi_touch_status status;
        int i,j,k,m;
	EGI_TOUCH_DATA touch;

	status=touch_data->status;

        /* 1. set mark when press down, !!!! egi_touch_getdata() may miss this status !!! */
        if(status==pressing)
        {
                printf("pagehome react_slider(): pressing sliding bar....\n");
		for(i=0;i<npg*nc*nr;i++)
	                mark[i]=home_btns[i]->x0;

		return btnret_OK; /* do not refresh page, or status will be cut to release_hold */
        }
	/* 2. adjust button position and refresh */
        else if( status==pressed_hold )
	{

#if 1  /* slide by dx value */
		//tmp=touch_data->dx/20;
		//if(step != tmp ) {
			step=tmp;
			for(i=0;i<npg*nc*nr;i++) {
				home_btns[i]->x0 = mark[i] +  (touch_data->dx <<1); /* x2 accelerate sliding speed */
				//if( home_btns[i]->x0 > (0-home_btns[i]->width) && home_btns[i]->x0 < 240 )
				egi_ebox_needrefresh(home_btns[i]);
			}
			/* group refresh to avoid bkimg interference */
			egi_btngroup_refresh(home_btns, npg*nc*nr);
			tm_delayms(75);
		//}

#else  /* auto slide if dx great that a threshold value */
		if( touch_data->dx > 15 )
			token=1;
		else  if(touch_data->dx < -15 )
			token=-1;
		else
			token=0;

		if(token != 0) {
	    	    for(step=1; step < 240/40+1; step++) {
			for(i=0;i<npg*nc*nr;i++) {
				home_btns[i]->x0 = mark[i] + token*step*40; //( touch_data->dx << 1); /* x2 accelerate sliding speed */
				//if( home_btns[i]->x0 > (0-home_btns[i]->width) && home_btns[i]->x0 < 240 )
				egi_ebox_needrefresh(home_btns[i]);
			}
			/* group refresh to avoid bkimg interference */
			egi_btngroup_refresh(home_btns, npg*nc*nr);
			tm_delayms(75);
		   }

		   /* wait for release_hold to pass following pressed_hold status */
		   do {
                   	tm_delayms(55);
                   	egi_touch_getdata(&touch);
                   }while(touch.status != released_hold);

		}
#endif

		return btnret_OK;
	}

	/* 3. re_align all btns in 3X3 grids OR re_align to a PAGE  */
	else if( status==releasing )
   	{
		/* align X0 to PAGE */
		ns=( home_btns[0]->x0 - 100)/240; /* 80 threshold */

		/* motion effect: take m steps to move buttons to the position
	         */
		step=20; /* move 20 pixels per step */
		ntp= ( (ns*240+15) - home_btns[0]->x0 )/step;

		if(ntp<0) {
			ntp=-ntp;
			step=-step;
		}

		if(ntp>0){
			for(m=1; m<=ntp; m++) {
				for(i=0; i<npg*nr*nc; i++) {
					home_btns[i]->x0 += step;
					egi_ebox_needrefresh(home_btns[i]);
				}
				egi_btngroup_refresh(home_btns, npg*nc*nr);
				tm_delayms(25);
			}
		}

		/* final position */
		for(k=0; k<npg; k++) {
			for(i=0; i<nr; i++) {
				for(j=0; j<nc; j++) {
					home_btns[k*nc*nr+i*nc+j]->x0=15+(15+60)*j+(k+ns)*240;
					egi_ebox_needrefresh(home_btns[k*nc*nr+i*nc+j]);
				}
			}
		}

		/* group refresh to avoid bkimg interference */
		egi_btngroup_refresh(home_btns, npg*nc*nr);

		return btnret_OK; /* refresh page */

   	}
   	else /* bypass unwanted touch status */
              return btnret_IDLE;

}
