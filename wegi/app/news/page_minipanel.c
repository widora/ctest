/*-------------------------------------------------------------------
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


                        (((  --------  PAGE DIVISION  --------  )))
[Y266-Y319]
{0,266}, {240-1, 320-1}         --- Buttons


TODO:


Midas Zhou
-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "egi_common.h"
//#include "sound/egi_pcm.h"
#include "egi_FTsymbol.h"
#include "page_minipanel.h"

/* icon code for button symbols */
#define ICON_CODE_PREV 		0
#define ICON_CODE_PAUSE 	1
#define ICON_CODE_STOP		10
#define ICON_CODE_PLAY 		3
#define ICON_CODE_NEXT 		2
#define ICON_CODE_EXIT 		5
#define ICON_CODE_SHUFFLE	6	/* pick next file randomly */
#define ICON_CODE_REPEATONE	7	/* repeat current file */
#define ICON_CODE_LOOPALL	8	/* loop all files in the list */
#define ICON_CODE_GOHOME	9

/* Control button IDs */
#define BTN_ID_PREV		0
#define BTN_ID_PLAYPAUSE	1
#define BTN_ID_NEXT		2
#define BTN_ID_EXIT		3	/* SLEEP */
#define BTN_ID_PLAYMODE		4	/* PLAYMODE or EXIT  */

#define SLIDER_ID_VOLUME        100

static EGI_PAGE* page_miniPanel;


static EGI_BOX slide_zone={ {0,0}, {239,260} };
static uint16_t btn_symcolor;

/* buttons */
static int btnum=5;
static EGI_DATA_BTN 	*data_btns[5];
static EGI_EBOX 	*panel_btns[5];

/* volume slider */
static EGI_DATA_BTN 	*data_slider;
static EGI_EBOX     	*volume_slider;

int create_miniPanel(void);
int free_miniPanel(void);

static int react_play(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int react_playpause(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int react_next(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int react_playmode(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int react_exit(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int page_decorate(EGI_EBOX *ebox);
static int sliding_volume(EGI_PAGE* page, EGI_TOUCH_DATA * touch_data);


/*-----------------------------------------
1. Create EGI_EBOX elements in the page.

Return
	pointer to a page	OK
	NULL			fail
-----------------------------------------*/
EGI_PAGE *create_panelPage(void)
{
	int i;

	/* --------- 1. create buttons --------- */
        for(i=0;i<btnum;i++) /* row of buttons*/
        {
		/* create new data_btns */
		data_btns[i]=egi_btndata_new(	i, 			/* int id */
						btnType_square, 	/* enum egi_btn_type shape */
						&sympg_sbuttons, 	/* struct symbol_page *icon. If NULL, use geometry. */
						0, 			/* int icon_code, assign later.. */
						&sympg_testfont 	/* for ebox->tag font */
					);
		/* if fail, try again ... */
		if(data_btns[i]==NULL) {
			EGI_PLOG(LOGLV_ERROR,"egi_create_ffplaypage(): fail to call egi_btndata_new() for data_btns[%d]. retry...", i);
			i--;
			continue;
		}

		/* Do not show tag on the button */
//default	data_btns[i]->showtag=false;

		/* create new btn eboxes */
		panel_btns[i]=egi_btnbox_new(   NULL, 		/* put tag later */
						data_btns[i], 	/* EGI_DATA_BTN *egi_data */
				        	1, 		/* bool movable */
					        (320-240)/2+48*i, 5, 	/* int x0, int y0 */
						48, 48, 	/* int width, int height */
				       		-1, 		/* int frame,<0 no frame */
		       				WEGI_COLOR_GRAY /*int prmcolor, for geom button only. */
					   );
		/* if fail, try again ... */
		if(panel_btns[i]==NULL) {
			printf("egi_create_ffplaypage(): fail to call egi_btnbox_new() for panel_btns[%d]. retry...\n", i);
			free(data_btns[i]);
			data_btns[i]=NULL;
			i--;
			continue;
		}
	}

	/* get a random color for the icon */
//	btn_symcolor=egi_color_random(color_medium);
//	EGI_PLOG(LOGLV_INFO,"%s: set 24bits btn_symcolor as 0x%06X \n",	__FUNCTION__, COLOR_16TO24BITS(btn_symcolor) );
	btn_symcolor=WEGI_COLOR_BLACK+1;

	/* add tags, set icon_code and reaction function here */
	egi_ebox_settag(panel_btns[0], "Prev");
	data_btns[0]->icon_code=(btn_symcolor<<16)+ICON_CODE_PREV; /* SUB_COLOR+CODE */
	panel_btns[0]->reaction=react_play;

	egi_ebox_settag(panel_btns[1], "Play&Pause");
	data_btns[1]->icon_code=(btn_symcolor<<16)+ICON_CODE_PAUSE; /* default status is playing*/
	panel_btns[1]->reaction=react_playpause;

	egi_ebox_settag(panel_btns[2], "Next");
	data_btns[2]->icon_code=(btn_symcolor<<16)+ICON_CODE_NEXT;
	panel_btns[2]->reaction=react_next;

	egi_ebox_settag(panel_btns[3], "Exit");
	data_btns[3]->icon_code=(btn_symcolor<<16)+ICON_CODE_EXIT;
	panel_btns[3]->reaction=react_exit;

	egi_ebox_settag(panel_btns[4], "Playmode");
	data_btns[4]->icon_code=(btn_symcolor<<16)+ICON_CODE_LOOPALL;
	panel_btns[4]->reaction=react_playmode;


        /* --------- 2. create a horizontal sliding bar --------- */
        int sb_len=200; 	/* slot length */
        int sb_pv=0; 		/* initial  percent value */
	EGI_POINT sbx0y0={ (320-240)/2, 50+25 }; /* starting point */

        data_slider=egi_sliderdata_new(   /* slider data is a EGI_DATA_BTN + privdata(egi_data_slider) */
                                        /* ---for btnbox-- */
                                        1, btnType_square,      /* data id, enum egi_btn_type shape */
                                        NULL,           	/* struct symbol_page *icon */
                                        0,              	/* int icon_code, */
                                        &sympg_testfont,	/* struct symbol_page *font */
                                        /* ---for slider--- */
					slidType_horiz,		/*  enum egi_slid_type */
                                        sbx0y0,			/* slider starting EGI_POINT pxy */
                                        5,sb_len,           	/* slot width, slot length */
                                        sb_pv*sb_len/100,      	/* init val, usually to be 0 */
                                        WEGI_COLOR_GRAY,  	/* EGI_16BIT_COLOR val_color */
                                        WEGI_COLOR_GRAY5,   	/* EGI_16BIT_COLOR void_color */
                                        WEGI_COLOR_WHITE   	/* EGI_16BIT_COLOR slider_color */
                            );

        volume_slider=egi_slider_new(
                                "volume slider",  	/* char *tag, or NULL to ignore */
                                data_slider, 		/* EGI_DATA_BTN *egi_data */
                                15,15,       		/* slider block: ebox->width/height, to be Min. if possible */
                                50,50,       		/* touchbox size, twidth/theight */
                                -1,          		/* int frame, <0 no frame */
                                -1      /* prmcolor
					 * 1. Let <0, it will draw default slider, instead of applying gemo or icon.
                                         * 2. prmcolor geom applys only if prmcolor>=0 and egi_data->icon!=NULL
					 */
                           );
	/* set EBOX id for volume_slider */
	volume_slider->id=SLIDER_ID_VOLUME;
        /* set reaction function */
        volume_slider->reaction=NULL;

	/* --------- 3. create panel page ------- */
	/* 3.1 create panel page */
	EGI_PAGE *page_panel=egi_page_new("page_panel");
	while(page_panel==NULL)
	{
		printf("egi_create_ffplaypage(): fail to call egi_page_new(), try again ...\n");
		page_panel=egi_page_new("page_panel");
		tm_delayms(10);
	}
//	page_panel->ebox->prmcolor=WEGI_COLOR_BLACK;
	/* NOTE: PAGE refresh call egi_imgbuf_windisplay2(), which has NO pos_rotation maping!! */
	page_panel->ebox->frame_img=egi_imgbuf_create( 320, 100, 250, WEGI_COLOR_WHITE); /* (H, W, alpha,color) */

	/* decoration */
//	page_panel->ebox->method.decorate=page_decorate; /* draw lower buttons canvas */

        /* 3.2 put pthread runner */
//        page_panel->runner[0]= thread_runner;

        /* 3.3 set default routine job */
//	page_panel->routine=egi_homepage_routine;  /* for sliding operation */

        /* 3.4 set wallpaper */
        //page_panel->fpath="/home/musicback.jpg";

	/* 3.5 add ebox to home page */
	for(i=0; i<btnum; i++) 	/* Add buttons */
		egi_page_addlist(page_panel, panel_btns[i]);
	egi_page_addlist(page_panel, volume_slider); /* add volume_slider ebox */

	return page_panel;
}


/*-----------------  RUNNER 1 --------------------------

-------------------------------------------------------*/
static void thread_runner(EGI_PAGE *page)
{

}

/*---------------------------------------------------------------
			 react PREV
----------------------------------------------------------------*/
static int react_play(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;

	/* ffmusic.c is forced to play the next song,
         * so change 'play&pause' button icon from PLAY to PAUSE
	 */
	if((data_btns[BTN_ID_PLAYPAUSE]->icon_code&0xffff)==ICON_CODE_PLAY)
		data_btns[BTN_ID_PLAYPAUSE]->icon_code=(btn_symcolor<<16)+ICON_CODE_PAUSE; /* toggle icon */

	/* set refresh flag for this ebox */
	egi_ebox_needrefresh(ebox);

	/* set FFmuz_Ctx->ffcmd, FFplay will reset it. */
	//FFmuz_Ctx->ffcmd=cmd_prev;

	/* only react to status 'pressing' */
	return btnret_OK;
}

/*----------------------------------------------------------------
			react NEXT
----------------------------------------------------------------*/
static int react_next(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;

	/* ffmusic.c is forced to play the next song,
         * so change 'play&pause' button icon from PLAY to PAUSE
	 */
	if( (data_btns[BTN_ID_PLAYPAUSE]->icon_code&0xffff)==ICON_CODE_PLAY )
		data_btns[BTN_ID_PLAYPAUSE]->icon_code=(btn_symcolor<<16)+ICON_CODE_PAUSE; /* toggle icon */

	/* set refresh flag for this ebox */
	egi_ebox_needrefresh(ebox);

	/* set FFmuz_Ctx->ffcmd, FFplay will reset it. */
	//FFmuz_Ctx->ffcmd=cmd_next;

	return btnret_OK;
}


/*--------------------------------------------------------------------
 				react PLAYPAUSE
----------------------------------------------------------------------*/
static int react_playpause(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;

	/* toggle the icon between play and pause */
	if( (data_btns[BTN_ID_PLAYPAUSE]->icon_code & 0x0ffff ) == ICON_CODE_PLAY ) {

		/* set FFmuz_Ctx->ffcmd, FFplay will reset it. */
		//FFmuz_Ctx->ffcmd=cmd_play;
		data_btns[BTN_ID_PLAYPAUSE]->icon_code=(btn_symcolor<<16)+ICON_CODE_PAUSE; /* toggle icon */
	}
	else {

		/* set FFmuz_Ctx->ffcmd, FFplay will reset it. */
		//FFmuz_Ctx->ffcmd=cmd_pause;
		data_btns[BTN_ID_PLAYPAUSE]->icon_code=(btn_symcolor<<16)+ICON_CODE_PLAY;  /* toggle icon */
	}

	/* set refresh flag for this ebox */
	egi_ebox_needrefresh(ebox);

	return btnret_OK;
}


/*--------------------------------------------------------------------
			react PLAY MODE
---------------------------------------------------------------------*/
static int react_playmode(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
//	static int count=0;

        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;


#if 0 /*---------  AS PLAYMODE LOOP SELECTION BUTTON  ---------*/

	/* only react to status 'pressing' */
	struct egi_data_btn *data_btn=(struct egi_data_btn *)(ebox->egi_data);

	count++;
	if(count>2)
		count=0;

	/* Default LOOPALL, rotate code:  LOOPALL -> REPEATONE -> SHUFFLE */
	data_btn->icon_code=(btn_symcolor<<16)+(ICON_CODE_LOOPALL-count%3);

	/* command for ffplay, ffplay will reset it */
	if(count==0) {					/* LOOP ALL */
		FFmuz_Ctx->ffmode=mode_loop_all;
		FFmuz_Ctx->ffcmd=cmd_mode;		/* set cmd_mode at last!!! as for LOCK. */
	}
	else if(count==1) { 				/* REPEAT ONE */
		FFmuz_Ctx->ffmode=mode_repeat_one;
		FFmuz_Ctx->ffcmd=cmd_mode;
	}
	else if(count==2) { 				/* SHUFFLE */
		FFmuz_Ctx->ffmode=mode_shuffle;
		FFmuz_Ctx->ffcmd=cmd_mode;
	}

	/* set refresh flag for this ebox */
	egi_ebox_needrefresh(ebox);

	return btnret_OK;

#else /* ---------  AS EXIT BUTTON  --------- */


	return btnret_REQUEST_EXIT_PAGE;
#endif

}

/*-----------------------------------------------------------------
			 react  REACT EXIT
-----------------------------------------------------------------*/
static int react_exit(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;

#if 1
	/*TEST: send HUP signal to iteself */
	if(raise(SIGUSR1) !=0 ) {
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to send SIGUSR1 to itself.",__func__);
	}
	EGI_PLOG(LOGLV_ASSERT,"%s: Finish rasie(SIGSUR1), return to page routine...",__func__);

  /* 1. When the page returns from SIGSTOP by signal SIGCONT, status 'pressed_hold' and 'releasing'
   *    will be received one after another,(In rare circumstances, it may receive 2 'preesed_hold')
   *	and the signal handler will call egi_page_needrefresh().
   *    But 'releasing' will trigger btn refresh by egi_btn_touch_effect() just before page refresh.
   *    After refreshed, need_refresh will be reset for this btn, so when egi_page_refresh() is called
   *    later to refresh other elements, this btn will erased by page bkcolor/wallpaper.
   * 2. The 'releasing' status here is NOT a real pen_releasing action, but a status changing signal.
   *    Example: when status transfers from 'pressed_hold' to PEN_UP.
   * 3. To be handled by page routine.
   */

	return pgret_OK; /* need refresh page, a trick here to activate the page after CONT signal */

#else
        egi_msgbox_create("Message:\n   Click! Start to exit page!", 300, WEGI_COLOR_ORANGE);
        return btnret_REQUEST_EXIT_PAGE;  /* end process */
#endif
}


/*-----------------------------------------
	Decoration for the page
-----------------------------------------*/
static int page_decorate(EGI_EBOX *ebox)
{
	/* bkcolor for bottom buttons */
	fbset_color(WEGI_COLOR_GRAY3);
        draw_filled_rect(&gv_fb_dev,0,266,239,319);

	return 0;
}


/*-----------------------------------------------------------------
                   Sliding Operation handler
Slide up/down to adjust ALSA volume.
------------------------------------------------------------------*/
static int sliding_volume(EGI_PAGE* page, EGI_TOUCH_DATA * touch_data)
{
	return 0;	

}


/*-----------------------------
Release and free all resources
-----------------------------*/
void egi_free_panelPage(void)
{

   /* all EBOXs in the page will be release by calling egi_page_free()
    *  just after page routine returns.
    */


}

/*-------------------------------------
Create miniPanel page and acitivate it.
------------------------------------*/
int create_miniPanel(void)
{
	/* create page */
	page_miniPanel=create_panelPage();
	if(page_miniPanel==NULL)
		return -1;

	/* activate */
	egi_page_activate(page_miniPanel);

	/* routine */
	// egi_page_routine(page_miniPanel);

	return 0;
}

/*-------------------------------------
Create miniPanel page and acitivate it.
------------------------------------*/
int free_miniPanel(void)
{

	egi_page_free(page_miniPanel);
	page_miniPanel=NULL;

	/* refresh screen  */

	return 0;
}


int miniPanel_routine(void)
{
	EGI_TOUCH_DATA touch_data;
	EGI_EBOX *ebox=NULL;
	//enum egi_retval ret;
	int ret;

	/* catch a hit */
        while ( egi_touch_timeWait_press(-1, &touch_data)==0 )
        {
	    ebox=egi_hit_pagebox(touch_data.coord.x, touch_data.coord.y, page_miniPanel, type_btn|type_slider);
		if(ebox != NULL) {
			ret= ebox->reaction(ebox, &touch_data);
			if(btnret_REQUEST_EXIT_PAGE)
				return 0;
		}
	}

	return 0;
}
