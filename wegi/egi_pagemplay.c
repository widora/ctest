/*--------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


page creation jobs:
1. egi_create_XXXpage() function.
   1.1 creating eboxes and page.
   1.2 assign thread-runner to the page.
   1.3 assign routine to the page.
   1.4 assign button functions to corresponding eboxes in page.
2. thread-runner functions.
3. egi_XXX_routine() function if not use default egi_page_routine().
4. button reaction functins

Midas Zhou
--------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* usleep */
#include "egi.h"
#include "egi_color.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
#include "egi_btn.h"
#include "egi_page.h"
#include "egi_symbol.h"
#include "egi_slider.h"
#include "egi_timer.h"
#include "ff_pcm.h"

static int egi_pagemplay_exit(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int slider_react(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static void check_volume_runner(EGI_PAGE *page);

#define  SLIDER_ID 100 /* use a big value */


/*---------- [  PAGE ::  Mplayer Operation ] ---------
1. create eboxes for 6 buttons and 1 title bar
2. create MPlayer Operation page

Return
	pointer to a page	OK
	NULL			fail
------------------------------------------------------*/
EGI_PAGE *egi_create_mplaypage(void)
{
	int i,j;

	EGI_EBOX *mplay_btns[6];
	EGI_DATA_BTN *data_btns[6];

	/* --------- 1. create buttons --------- */
        for(i=0;i<2;i++) /* row of buttons*/
        {
                for(j=0;j<3;j++) /* column of buttons */
                {
			/* 1. create new data_btns */
			data_btns[3*i+j]=egi_btndata_new(3*i+j, /* int id */
							circle, /* enum egi_btn_type shape */
							NULL, /* struct symbol_page *icon */
							0, /* int icon_code */
							&sympg_testfont /* for ebox->tag font */
						);
			/* if fail, try again ... */
			if(data_btns[3*i+j]==NULL)
			{
				printf("egi_create_mplaypage(): fail to call egi_btndata_new() for data_btns[%d]. retry...\n", 3*i+j);
				i--;
				continue;
			}

			/* to show tag on the button */
			data_btns[3*i+j]->showtag=true;

			/* 2. create new btn eboxes */
			mplay_btns[3*i+j]=egi_btnbox_new(NULL, /* put tag later */
							data_btns[3*i+j], /* EGI_DATA_BTN *egi_data */
				        		0, /* bool movable */
						        10+(15+60)*j, 150+(15+60)*i, /* int x0, int y0 */
							70,70, /* int width, int height */
				       			0,//1, /* int frame,<0 no frame */
		       					egi_color_random(medium) /*int prmcolor */
						   );
			/* if fail, try again ... */
			if(mplay_btns[3*i+j]==NULL)
			{
				printf("egi_create_mplaypage(): fail to call egi_btnbox_new() for mplay_btns[%d]. retry...\n", 3*i+j);
				free(data_btns[3*i+j]);
				data_btns[3*i+j]=NULL;

				i--;
				continue;
			}
		}
	}

	/* add tags and reaction function here */
	egi_ebox_settag(mplay_btns[0], "Back");
	egi_ebox_settag(mplay_btns[1], "Play");

	egi_ebox_settag(mplay_btns[2], "TEST!");
	mplay_btns[2]->reaction=egi_txtbox_demo; /* txtbox demo */

	egi_ebox_settag(mplay_btns[3], "Close");
	mplay_btns[3]->reaction=egi_pagemplay_exit;

	egi_ebox_settag(mplay_btns[4], "HOME");
	egi_ebox_settag(mplay_btns[5], "Mini.");


	/* --------- 2. create a horizontal sliding bar --------- */

	int sl=180; /* slot length */

	/* set playback volume percert to data_slider->val */
	int pvol; /* percent value */
	ffpcm_getset_volume(&pvol,NULL);
	printf("-------pvol=%%%d-----\n",pvol);
	EGI_DATA_BTN *data_slider=egi_sliderdata_new(
	                                /* for btnbox */
        	                        SLIDER_ID, square,  	/* int id, enum egi_btn_type shape */
                	                NULL,		/* struct symbol_page *icon */
					0,		/* int icon_code, */
                        	        &sympg_testfont,/* struct symbol_page *font */
	                                /* for slider */
        	                        (EGI_POINT){30,100},//pxy,	/* EGI_POINT pxy */
                	                8,sl,	 	/* slot width, slot length */
	                       	        pvol*sl/100,      /* init val, usually to be 0 */
                               	 	WEGI_COLOR_GRAY,  /* EGI_16BIT_COLOR val_color */
					WEGI_COLOR_GRAY5,   /* EGI_16BIT_COLOR void_color */
	                                WEGI_COLOR_GREEN //WHITE   /* EGI_16BIT_COLOR slider_color */
        	                    );

	EGI_EBOX *sliding_bar=egi_slider_new(
  	 			"volume slider",  /* char *tag, or NULL to ignore */
			        data_slider, /* EGI_DATA_BTN *egi_data */
			        50,50,	     /* slider block: int width, int height */
			        -1,	     /* int frame,<0 no frame */
			        -1          /* 1. Let <0, it will draw slider, instead of applying gemo or icon.
			                       2. prmcolor geom applys only if prmcolor>=0 and egi_data->icon != NULL */
			     );

	/* set reaction function */
	sliding_bar->reaction=slider_react;

	/* --------- 3. create title bar --------- */
	EGI_EBOX *title_bar= create_ebox_titlebar(
	        0, 0, /* int x0, int y0 */
        	0, 2,  /* int offx, int offy */
		WEGI_COLOR_GRAY, //egi_colorgray_random(medium), //light),  /* int16_t bkcolor */
    		NULL	/* char *title */
	);
	egi_txtbox_settitle(title_bar, "   	MPlayer 1.0rc");


	/* --------- 4. create mplay page ------- */
	/* 4.1 create mplay page */
	EGI_PAGE *page_mplay=NULL;
	page_mplay=egi_page_new("page_mplayer");
	while(page_mplay==NULL)
	{
			printf("egi_create_mplaypage(): fail to call egi_page_new(), try again ...\n");
			page_mplay=egi_page_new("page_mplay");
			usleep(100000);
	}
	page_mplay->ebox->prmcolor=egi_colorgray_random(light);

	/* 4.2 put pthread runner, remind EGI_PAGE_MAXTHREADS 5  */
        page_mplay->runner[0]=check_volume_runner;

        /* 4.3 set default routine job */
        page_mplay->routine=egi_page_routine; /* use default */

        /* 4.4 set wallpaper */
        page_mplay->fpath="/tmp/mplay.jpg";


	/* add ebox to home page */
	for(i=0;i<6;i++) /* buttons */
		egi_page_addlist(page_mplay, mplay_btns[i]);
	egi_page_addlist(page_mplay, title_bar); /* title bar */
	egi_page_addlist(page_mplay, sliding_bar); /* sliding bar */

	return page_mplay;
}


/*-------------------------    RUNNER 1   --------------------------
Check volume and refresh slider.
-------------------------------------------------------------------*/
static void check_volume_runner(EGI_PAGE *page)
{
     int pvol;
     int sval;
     EGI_EBOX *slider=egi_page_pickebox(page, type_slider, SLIDER_ID);
     EGI_DATA_BTN *data_btn=(EGI_DATA_BTN *)(slider->egi_data);
     EGI_DATA_SLIDER *data_slider=(EGI_DATA_SLIDER *)(data_btn->prvdata);

     while(1) {
	   /* get palyback volume */
	   ffpcm_getset_volume(&pvol,NULL);
	   sval=pvol*data_slider->sl/100;

	   /* slider value is drivered by ebox->x0 for H slider, so set x0 not val */
	   slider->x0=data_slider->sxy.x+sval-(slider->width>>1);

	   /* refresh it */
           slider->need_refresh=true;
           slider->refresh(slider);

	   /* check page status */
	   if(page->ebox->status==status_page_exiting)
		return;

	   egi_sleep(0,0,300); /* 300ms */
     }

}


/*----------------------------------------------------------------------
btn_close function:
return
------------------------------------------------------------------------*/
static int egi_pagemplay_exit(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
       /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;

        return btnret_REQUEST_EXIT_PAGE; /* >=00 return to routine; <0 exit this routine */
}

/*-------------------------------------------------------------------
Horizontal Sliding bar reaction
return
---------------------------------------------------------------------*/
static int slider_react(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
	static int mark;
	EGI_DATA_SLIDER *data_slider;
	int minx,maxx;
	int vol;
	static char strcmd[50];

        /* bypass unwanted touch status */
        if( touch_data->status != pressed_hold && touch_data->status != pressing )
                return btnret_IDLE;

        /* set mark when press down, !!!! egi_touch_getdata() may miss this status !!! */
        if(touch_data->status==pressing)
        {
                printf("slider_react(): pressing sliding bar....\n");
                mark=ebox->x0;
        }
	else  //(touch_data->status==pressed_hold)
	{
		data_slider=(EGI_DATA_SLIDER *)(((EGI_DATA_BTN *)(ebox->egi_data))->prvdata);
		minx=data_slider->sxy.x-(ebox->width>>1);
		maxx=minx+data_slider->sl;

		/* update slider ebox position */
		printf("touch_data->dx = %d\n",touch_data->dx);
		ebox->x0 = mark+(touch_data->dx);
		if(ebox->x0 < minx) ebox->x0=minx;
		else if(ebox->x0 > maxx) ebox->x0=maxx;

		/* adjust volume */
		vol=100*data_slider->val/data_slider->sl;
		memset(strcmd,0,sizeof(strcmd));
		sprintf(strcmd,"amixer set PCM %d%%",vol);
		system(strcmd);

	        ebox->need_refresh=true;
	        ebox->refresh(ebox);
	}

	return btnret_IDLE; /* OK, page need not refresh */
}
