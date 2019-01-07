/*-------------------------------------------------------------------------
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
---------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* usleep */
#include "egi.h"
#include "egi_debug.h"
#include "egi_color.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
#include "egi_btn.h"
#include "egi_page.h"
#include "egi_symbol.h"
#include "egi_objpage.h"
#include "egi_pagehome.h"
#include "egi_pagemplay.h"

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
			/* 1. create new data_btns */
			data_btns[3*i+j]=egi_btndata_new(3*i+j, /* int id */
							square, /* enum egi_btn_type shape */
							&sympg_buttons, /* struct symbol_page *icon */
							3*i+j /* int icon_code */
						);
			/* if fail, try again ... */
			if(data_btns[3*i+j]==NULL)
			{
				printf("egi_create_homepage(): fail to call egi_btndata_new() for data_btns[%d]. retry...\n", 3*i+j);
				i--;
				continue;
			}

			/* 2. create new btn eboxes */
			home_btns[3*i+j]=egi_btnbox_new(NULL, /* put tag later */
							data_btns[3*i+j], /* EGI_DATA_BTN *egi_data */
				        		0, /* bool movable */
						        15+(15+60)*j, 105+(15+60)*i, /* int x0, int y0 */
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

	/* add tags and reactions here */
	egi_ebox_settag(home_btns[0], "btn_mplayer");
	home_btns[0]->reaction=egi_homepage_mplay;

	egi_ebox_settag(home_btns[1], "btn_iot");
	egi_ebox_settag(home_btns[2], "btn_alarm");
	egi_ebox_settag(home_btns[3], "btn_mp1");
	egi_ebox_settag(home_btns[4], "btn_key");
	egi_ebox_settag(home_btns[5], "btn_book");
	egi_ebox_settag(home_btns[6], "btn_chart");
	egi_ebox_settag(home_btns[7], "btn_mp2");
	egi_ebox_settag(home_btns[8], "btn_radio");





	/* --------- 2. create home-head bar --------- */
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
                true, /* bool movable */
                0,0, /* int x0, int y0 */
                240,30, /* int width, int height */
                -1, /* int frame, -1=no frame */
                -1 //egi_colorgray_random(medium)/*int prmcolor, -1 transparent*/
        );
	/* set symbols in home head bar */
	head_txt->txt[0][0]=4;
	head_txt->txt[0][1]=6;
	head_txt->txt[0][2]=10;
	head_txt->txt[0][3]=6; /* 6 is space */
	head_txt->txt[0][4]=6;//33;
	head_txt->txt[0][5]=6;//36;
	head_txt->txt[0][6]=6;
	head_txt->txt[0][7]=28;
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
	/* 3.2 put pthread runner */
	page_home->runner[0]=egi_display_cpuload;
	page_home->runner[1]=egi_display_iotload;

	/* 3.3 set default routine job */
	page_home->routine=egi_page_routine;

	/* 3.4 set wallpaper */
	page_home->fpath="/tmp/home.jpg";



	/* add ebox to home page */
	/* beware of the sequence of the ebox list */
	for(i=0;i<9;i++)
		egi_page_addlist(page_home, home_btns[i]);
	egi_page_addlist(page_home,ebox_headbar);


	return page_home;
}

/*-----------------  RUNNER 1 --------------------------
display cpu load in home head-bar with motion icons
-------------------------------------------------------*/
void egi_display_cpuload(EGI_PAGE *page)
{
	int load=3; /* 0-3 */

	egi_pdebug(DBG_PAGE,"page '%s':  runner thread egi_display_cpuload() is activated!.\n",page->ebox->tag);

	while(1)
	{
		/* load cpuload motion icons
			  symbol_motion_string() is with sleep function */
  	 	symbol_motion_string(&gv_fb_dev, 155-load*15, &sympg_icons,
		 					1, 150,0, &symmic_cpuload[load][0]);
	}
}

/*-----------------  RUNNER 2 --------------------------
display IoT load in home head-bar with motion icons
-------------------------------------------------------*/
void egi_display_iotload(EGI_PAGE *page)
{
	egi_pdebug(DBG_PAGE,"page '%s':  runner thread egi_display_iotload() is activated!.\n"
										,page->ebox->tag);
	while(1)
	{
		/* load IoT motion icons
			  symbol_motion_string() is with sleep function */
  	 	symbol_motion_string(&gv_fb_dev, 120, &sympg_icons, 1, 120,0, symmic_iotload);
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


/*-----------------------------------
button_mplay function:
mplayer
-----------------------------------*/
int egi_homepage_mplay(EGI_EBOX * ebox, enum egi_btn_status btn_status)
{
        EGI_PAGE *page_mplay=egi_create_mplaypage();

        egi_page_activate(page_mplay);
	/* get into routine loop */
        page_mplay->routine(page_mplay);
	/* get out of routine loop */
	egi_page_free(page_mplay);

	return 0;
}
