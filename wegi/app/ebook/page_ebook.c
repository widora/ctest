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
[Y0-Y29]
{0,0},{240-1, 29}               ---  Head title bar

[Y30-Y260]
{0,30}, {240-1, 260}            --- Image/subtitle Displaying Zone
[Y150-Y260] Sub_displaying

[Y150-Y265]
{0,150}, {240-1, 260}           --- box area for subtitle display

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
#include "egi_FTsymbol.h"
#include "page_ebook.h"

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

static uint16_t btn_symcolor;

static int ebook_prev(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int ebook_playpause(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int ebook_next(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int ebook_playmode(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int ebook_exit(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data);
static int ebook_decorate(EGI_EBOX *ebox);


/*---------- [  PAGE ::  EBOOK ] ---------
1. create eboxes for 4 buttons and 1 title bar

Return
	pointer to a page	OK
	NULL			fail
------------------------------------------------------*/
EGI_PAGE *create_ebook_page(void)
{
	int i;
	int btnum=5;
	EGI_EBOX *ebook_btns[5];
	EGI_DATA_BTN *data_btns[5];

	/* --------- 1. create buttons --------- */
        for(i=0;i<btnum;i++) /* row of buttons*/
        {
		/* 1. create new data_btns */
		data_btns[i]=egi_btndata_new(	i, /* int id */
						square, /* enum egi_btn_type shape */
						&sympg_sbuttons, /* struct symbol_page *icon. If NULL, use geometry. */
						0, /* int icon_code, assign later.. */
						&sympg_testfont /* for ebox->tag font */
					);
		/* if fail, try again ... */
		if(data_btns[i]==NULL)
		{
			EGI_PLOG(LOGLV_ERROR,"egi_create_ffplaypage(): fail to call egi_btndata_new() for data_btns[%d]. retry...\n", i);
			i--;
			continue;
		}

		/* Do not show tag on the button */
		data_btns[i]->showtag=false;

		/* 2. create new btn eboxes */
		ebook_btns[i]=egi_btnbox_new(  NULL, /* put tag later */
						data_btns[i], /* EGI_DATA_BTN *egi_data */
				        	1, /* bool movable */
					        48*i, 320-(60-5), /* int x0, int y0 */
						48, 60, /* int width, int height */
				       		0, /* int frame,<0 no frame */
		       				egi_color_random(color_medium) /*int prmcolor, for geom button only. */
					   );
		/* if fail, try again ... */
		if(ebook_btns[i]==NULL)
		{
			printf("egi_create_ffplaypage(): fail to call egi_btnbox_new() for ebook_btns[%d]. retry...\n", i);
			free(data_btns[i]);
			data_btns[i]=NULL;
			i--;
			continue;
		}
	}

	/* get a random color for the icon */
//	btn_symcolor=egi_color_random(medium);
//	EGI_PLOG(LOGLV_INFO,"%s: set 24bits btn_symcolor as 0x%06X \n",	__FUNCTION__, COLOR_16TO24BITS(btn_symcolor) );
	btn_symcolor=WEGI_COLOR_BLACK;//ORANGE;

	/* add tags, set icon_code and reaction function here */
	egi_ebox_settag(ebook_btns[0], "Prev");
	data_btns[0]->icon_code=(btn_symcolor<<16)+ICON_CODE_PREV; /* SUB_COLOR+CODE */
	ebook_btns[0]->reaction=ebook_prev;

	egi_ebox_settag(ebook_btns[1], "Play&Pause");
	data_btns[1]->icon_code=(btn_symcolor<<16)+ICON_CODE_PAUSE; /* default status is playing*/
	ebook_btns[1]->reaction=ebook_playpause;

	egi_ebox_settag(ebook_btns[2], "Next");
	data_btns[2]->icon_code=(btn_symcolor<<16)+ICON_CODE_NEXT;
	ebook_btns[2]->reaction=ebook_next;

	egi_ebox_settag(ebook_btns[3], "Exit");
	data_btns[3]->icon_code=(btn_symcolor<<16)+ICON_CODE_EXIT;
	ebook_btns[3]->reaction=ebook_exit;

	egi_ebox_settag(ebook_btns[4], "Playmode");
	data_btns[4]->icon_code=(btn_symcolor<<16)+ICON_CODE_LOOPALL;
	ebook_btns[4]->reaction=ebook_playmode;


	/* --------- 2. create title bar --------- */
#if 0
	EGI_EBOX *title_bar= create_ebox_titlebar(
	        0, 0, /* int x0, int y0 */
        	0, 2,  /* int offx, int offy, offset for txt */
		WEGI_COLOR_GRAY, //egi_colorgray_random(medium), //light),  /* int16_t bkcolor */
    		NULL	/* char *title */
	);
	egi_txtbox_settitle(title_bar, "	eFFplay V0.0 ");
#endif

	/* --------- 3. create ffplay page ------- */
	/* 3.1 create ffplay page */
	EGI_PAGE *page_ebook=egi_page_new("page_ebook");
	while(page_ebook==NULL)
	{
		printf("%s: fail to call egi_page_new(), try again ...\n", __func__);
		page_ebook=egi_page_new("page_ebook");
		tm_delayms(10);
	}
	/* of BK color, if NO wallpaper defined */
	page_ebook->ebox->prmcolor=WEGI_COLOR_BLACK;
	/* decoration */
	page_ebook->ebox->method.decorate=ebook_decorate;

        /* 3.2 put pthread runner */
//        page_ebook->runner[0]= ;

        /* 3.3 set default routine job */
        page_ebook->routine=egi_page_routine; /* use default routine function */

        /* 3.4 set wallpaper */
        page_ebook->fpath="/mmc/bkpng/bk3.png";

	/* 3.5 add ebox to home page */
	for(i=0;i<btnum;i++) /* add buttons */
		egi_page_addlist(page_ebook, ebook_btns[i]);

//	egi_page_addlist(page_ebook, title_bar); /* add title bar */


	return page_ebook;
}


/*-----------------  RUNNER 1 --------------------------

-------------------------------------------------------*/
static void ebook_runner(EGI_PAGE *page)
{

}

/*--------------------------------------------------------------------
ebook PREV
return
----------------------------------------------------------------------*/
static int ebook_prev(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;

	/* only react to status 'pressing' */
	return btnret_OK;
}

/*--------------------------------------------------------------------
ebook palypause
return
-------------------------------------------------------------------*/
static int ebook_playpause(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;


	return btnret_OK;
}

/*--------------------------------------------------------------------
ebook NEXT
return
----------------------------------------------------------------------*/
static int ebook_next(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;

	return btnret_OK;
}

/*--------------------------------------------------------------------
ebook Display Mode
return
----------------------------------------------------------------------*/
static int ebook_playmode(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
	static int count=0;

        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;


	return btnret_OK;
}

/*--------------------------------------------------------------------
EBOOK exit
return
----------------------------------------------------------------------*/
static int ebook_exit(EGI_EBOX * ebox, EGI_TOUCH_DATA * touch_data)
{
        /* bypass unwanted touch status */
        if(touch_data->status != pressing)
                return btnret_IDLE;

#if 1
	/*TEST: send HUP signal to iteself */
	if(raise(SIGUSR1) !=0 ) {
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to send SIGUSR1 to itself.\n",__func__);
	}
	EGI_PLOG(LOGLV_ERROR,"%s: Finish rasie(SIGSUR1), return to page routine...\n",__func__);

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

	return pgret_OK; /* need refresh page */

#else
        egi_msgbox_create("Message:\n   Click! Start to exit page!", 300, WEGI_COLOR_ORANGE);
        return btnret_REQUEST_EXIT_PAGE;  /* end process */
#endif
}


/*-----------------------------------------
	Decoration for the page
-----------------------------------------*/
static int ebook_decorate(EGI_EBOX *ebox)
{
	/* bkcolor for bottom buttons */
//	fbset_color(WEGI_COLOR_GRAY3);
//        draw_filled_rect(&gv_fb_dev,0,266,239,319);

	return 0;
}
