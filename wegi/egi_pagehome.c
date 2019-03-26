/*-------------------------------------------------------------------------
page creation jobs:
1. egi_create_XXXpage() function.
   1.1 creating eboxes and page.
   1.2 assign thread-runner to the page.
   1.3 assign routine to the page.
   1.4 assign button functions to corresponding eboxes in page.
2. thread-runner functions.
3. egi_XXX_routine() function if not use default egi_page_routine().
4. add button reaction functins, mainly for creating pages.

TODO:
	1. different values for button return, and page return,
	   that egi_page_routine() can distinguish.
	2. pack page activate and free action.

Midas Zhou
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> /* usleep */
#include "egi.h"
#include "egi_debug.h"
#include "egi_log.h"
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

//static uint16_t btn_symcolor;

static void egi_display_cpuload(EGI_PAGE *page);
static void egi_display_iotload(EGI_PAGE *page);
static int egi_homebtn_mplay(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_openwrt(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_book(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_test(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int egi_homebtn_ffplay(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);


/*------------- [  PAGE ::   Home Page  ] -------------
create home page
write tag corresponding to each button.

Return
	pointer to a page	OK
	NULL			fail
------------------------------------------------------*/
EGI_PAGE *egi_create_homepage(void)
{
	int i,j;

	EGI_EBOX *home_btns[9];
	EGI_DATA_BTN *data_btns[9];
	EGI_DATA_TXT *head_txt;
	EGI_EBOX  *ebox_headbar;

	/* -------- 1. create button eboxes -------- */
        for(i=0;i<3;i++) /* row of buttons*/
        {
                for(j=0;j<3;j++) /* column of buttons */
                {
			/* 1.1 create new data_btns */
			data_btns[3*i+j]=egi_btndata_new(3*i+j, /* int id */
							square, /* enum egi_btn_type shape */
							&sympg_buttons, /* struct symbol_page *icon */
							3*i+j, /* int icon_code */
							&sympg_testfont /* for ebox->tag font */
						);
			/* if fail, try again ... */
			if(data_btns[3*i+j]==NULL)
			{
				printf("egi_create_homepage(): fail to call egi_btndata_new() for data_btns[%d]. retry...\n", 3*i+j);
				i--;
				continue;
			}

			/* 2.2 create new btn eboxes */
			home_btns[3*i+j]=egi_btnbox_new(NULL, /* put tag later */
							data_btns[3*i+j], /* EGI_DATA_BTN *egi_data */
				        		0, /* bool movable */
						        15+(15+60)*j, 85+(15+60)*i, /* int x0, int y0 */
							60,60, /* int width, int height */
				       			1, /* int frame */
		       					-1 /*int prmcolor */
						   );

			/* if fail, try again ... */
			if(data_btns[3*i+j]==NULL)
			{
				printf("egi_create_homepage(): fail to call egi_btnbox_new() for home_btns[%d]. retry...\n", 3*i+j);
				free(data_btns[3*i+j]);
				data_btns[3*i+j]=NULL;

				i--;
				continue;
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

	egi_ebox_settag(home_btns[7], "0");//btn_iot"); /* id=7; as for bulb */
	data_btns[7]->icon_code=11; /* SUB_COLOR + ICON_CODE */
	data_btns[7]->showtag=true; /* display tag */
	data_btns[7]->font=&sympg_numbfont;
	data_btns[7]->font_color=WEGI_COLOR_ORANGE;

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

	printf("btns[7]->egi_data->icon_code=%d\n",((EGI_DATA_BTN *)(home_btns[7]->egi_data))->icon_code);
	printf("bkimg_btn7->egi_data->icon_code=%d\n",((EGI_DATA_BTN *)(bkimg_btn7->egi_data))->icon_code);

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
			usleep(100000);
	}
	/* set bk color, applicable only if fpath==NULL  */
	page_home->ebox->prmcolor=WEGI_COLOR_OCEAN;

	/* 3.2 put pthread runner, remind EGI_PAGE_MAXTHREADS 5  */
	page_home->runner[0]=egi_display_cpuload;
	page_home->runner[1]=egi_display_iotload;
	page_home->runner[2]=NULL; //egi_iotclient;

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
	egi_page_addlist(page_home, bkimg_btn7);
	egi_page_addlist(page_home,ebox_headbar);

	return page_home;
}

/*-----------------  RUNNER 1 --------------------------
display cpu load in home head-bar with motion icons

1. read /proc/loadavg to get the value
	loadavg 1-6,  >5 alarm

2. corresponding symmic_cpuload[] index from 0-5.

-------------------------------------------------------*/
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
			  symbol_motion_string() is with---sleep---function inside */
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

		/* 3. draw RSSI symbol */
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
