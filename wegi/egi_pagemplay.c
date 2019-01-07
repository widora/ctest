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
#include "egi_color.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
#include "egi_btn.h"
#include "egi_page.h"
#include "egi_symbol.h"


static int egi_pagemplay_exit(EGI_EBOX * ebox, enum egi_btn_status btn_status);

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
							0 /* int icon_code */
						);
			/* if fail, try again ... */
			if(data_btns[3*i+j]==NULL)
			{
				printf("egi_create_mplaypage(): fail to call egi_btndata_new() for data_btns[%d]. retry...\n", 3*i+j);
				i--;
				continue;
			}

			/* 2. create new btn eboxes */
			mplay_btns[3*i+j]=egi_btnbox_new(NULL, /* put tag later */
							data_btns[3*i+j], /* EGI_DATA_BTN *egi_data */
				        		0, /* bool movable */
						        10+(15+60)*j, 150+(15+60)*i, /* int x0, int y0 */
							70,70, /* int width, int height */
				       			-1, /* int frame,<0 no frame */
		       					egi_color_random(medium) /*int prmcolor */
						   );
			/* if fail, try again ... */
			if(data_btns[3*i+j]==NULL)
			{
				printf("egi_create_mplaypage(): fail to call egi_btnbox_new() for home_btns[%d]. retry...\n", 3*i+j);
				free(data_btns[3*i+j]);
				data_btns[3*i+j]=NULL;

				i--;
				continue;
			}
		}
	}

	/* add tags and reaction function here */
	egi_ebox_settag(mplay_btns[0], "btn_backward");
	egi_ebox_settag(mplay_btns[1], "btn_play&pause");
	egi_ebox_settag(mplay_btns[2], "btn_forward");

	egi_ebox_settag(mplay_btns[3], "btn_close");
	mplay_btns[3]->reaction=egi_pagemplay_exit;

	egi_ebox_settag(mplay_btns[4], "btn_home");
	egi_ebox_settag(mplay_btns[5], "btn_minimize");


	/* --------- 2. create title bar --------- */
	EGI_EBOX *title_bar= create_ebox_titlebar(
	        0, 0, /* int x0, int y0 */
        	0, 2,  /* int offx, int offy */
		egi_colorgray_random(light),  /* int16_t bkcolor */
    		NULL	/* char *title */
	);
	egi_txtbox_settitle(title_bar, "   MPlayer 1.0rc2-4.8.3 --------");


	/* --------- 3. create mplay page ------- */
	/* 3.1 create mplay page */
	EGI_PAGE *page_mplay=NULL;
	page_mplay=egi_page_new("page_mplayer");
	while(page_mplay==NULL)
	{
			printf("egi_create_mplaypage(): fail to call egi_page_new(), try again ...\n");
			page_mplay=egi_page_new("page_mplay");
			usleep(100000);
	}
        /* 3.2 put pthread runner */
        //page_mplay->runner[0]= ;

        /* 3.3 set default routine job */
        page_mplay->routine=egi_page_routine; /* use default */

        /* 3.4 set wallpaper */
        page_mplay->fpath="/tmp/mplay.jpg";


	/* add ebox to home page */
	for(i=0;i<6;i++) /* buttons */
		egi_page_addlist(page_mplay, mplay_btns[i]);
	egi_page_addlist(page_mplay, title_bar); /* title bar */


	return page_mplay;
}


/*-----------------  RUNNER 1 --------------------------

-------------------------------------------------------*/
static void egi_pagemplay_runner(EGI_PAGE *page)
{

}

/*-----------------------------------
btn_close function:
return
-----------------------------------*/
static int egi_pagemplay_exit(EGI_EBOX * ebox, enum egi_btn_status btn_status)
{
	return -1;
}

