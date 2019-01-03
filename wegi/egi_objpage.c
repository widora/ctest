#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* usleep */
#include "egi.h"
#include "egi_color.h"
#include "egi_txt.h"
#include "egi_btn.h"
#include "egi_page.h"
#include "symbol.h"
#include "egi_objpage.h"


/*------------------------------------------------------
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

        for(i=0;i<3;i++) /* row of buttons*/
        {
                for(j=0;j<3;j++) /* column of buttons */
                {
			/* 1. create new data_btns */
			data_btns[3*i+j]=egi_btndata_new(3*i+j, /* int id */
							square, /* enum egi_btn_type shape */
							&sympg_icon, /* struct symbol_page *icon */
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

	/* add tags  here */
	egi_ebox_settag(home_btns[0], "btn_mplayer");
	egi_ebox_settag(home_btns[1], "btn_iot");
	egi_ebox_settag(home_btns[2], "btn_alarm");
	egi_ebox_settag(home_btns[3], "btn_mp1");
	egi_ebox_settag(home_btns[4], "btn_key");
	egi_ebox_settag(home_btns[5], "btn_book");
	egi_ebox_settag(home_btns[6], "btn_chart");
	egi_ebox_settag(home_btns[7], "btn_mp2");
	egi_ebox_settag(home_btns[8], "btn_radop");

	/* create home page */
	EGI_PAGE *page_home=NULL;
	page_home=egi_page_new("page_home");
	while(page_home==NULL)
	{
			printf("egi_create_homepage(): fail to call egi_page_new(), try again ...\n");
			page_home=egi_page_new("page_home");
			usleep(100000);
	}

	/* set wallpaper */
	page_home->fpath="/tmp/home.jpg";

	/* add ebox to home page */
	for(i=0;i<9;i++)
		egi_page_addlist(page_home, home_btns[i]);



	return page_home;
}



/*------------------------------------------------------
create mplayer page
write tag corresponding to each button.

Return
	pointer to a page	OK
	NULL			fail
------------------------------------------------------*/
EGI_PAGE *egi_create_mplaypage(void)
{
	int i,j;

	EGI_EBOX *mplay_btns[6];
	EGI_DATA_BTN *data_btns[6];

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
						        15+(15+60)*j, 150+(15+60)*i, /* int x0, int y0 */
							70,70, /* int width, int height */
				       			-1, /* int frame,<0 no frame */
		       					egi_random_color() /*int prmcolor */
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

	/* add tags  here */
	egi_ebox_settag(mplay_btns[0], "btn_backward");
	egi_ebox_settag(mplay_btns[1], "btn_play&pause");
	egi_ebox_settag(mplay_btns[2], "btn_forward");
	egi_ebox_settag(mplay_btns[3], "btn_close");
	egi_ebox_settag(mplay_btns[4], "btn_home");
	egi_ebox_settag(mplay_btns[5], "btn_minimize");

	/* create home page */
	EGI_PAGE *page_mplay=NULL;
	page_mplay=egi_page_new("page_mplayer");
	while(page_mplay==NULL)
	{
			printf("egi_create_mplaypage(): fail to call egi_page_new(), try again ...\n");
			page_mplay=egi_page_new("page_home");
			usleep(100000);
	}


	/* set wallpaper */
	page_mplay->fpath="/tmp/mplay.jpg";

	/* add ebox to home page */
	for(i=0;i<6;i++)
		egi_page_addlist(page_mplay, mplay_btns[i]);

	return page_mplay;
}

