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
#include "egi_objpage.h"


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

	/* ----- 1. create button eboxes ----- */
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
                egi_colorgray_random(medium)/*int prmcolor, -1 transparent*/
        );
	/* set symbols in home head bar */
	head_txt->txt[0][0]=4;
	head_txt->txt[0][1]=6;
	head_txt->txt[0][2]=11;
	head_txt->txt[0][3]=6; /* 6 is space */
	head_txt->txt[0][4]=33;
	head_txt->txt[0][5]=6;//36;
	head_txt->txt[0][6]=6;
	head_txt->txt[0][7]=28;
	/* !!! the last  MUST end with /0 */

	/* ----- 3.create home page ----- */
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
	/* beware of the sequence of the ebox list */
	for(i=0;i<9;i++)
		egi_page_addlist(page_home, home_btns[i]);
	egi_page_addlist(page_home,ebox_headbar);


	return page_home;
}



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

	/* add tags  here */
	egi_ebox_settag(mplay_btns[0], "btn_backward");
	egi_ebox_settag(mplay_btns[1], "btn_play&pause");
	egi_ebox_settag(mplay_btns[2], "btn_forward");
	egi_ebox_settag(mplay_btns[3], "btn_close");
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


	/* --------- 3. create home page ------- */
	EGI_PAGE *page_mplay=NULL;
	page_mplay=egi_page_new("page_mplayer");
	while(page_mplay==NULL)
	{
			printf("egi_create_mplaypage(): fail to call egi_page_new(), try again ...\n");
			page_mplay=egi_page_new("page_mplay");
			usleep(100000);
	}


	/* set wallpaper */
	page_mplay->fpath="/tmp/mplay.jpg";

	/* add ebox to home page */
	for(i=0;i<6;i++) /* buttons */
		egi_page_addlist(page_mplay, mplay_btns[i]);
	egi_page_addlist(page_mplay, title_bar); /* title bar */


	return page_mplay;
}


/*------ [  PAGE  ::  OpenWRT System Information ] ------
1. create eboxes for 6 buttons and 1 title bar
2. create OpenWRT system page

Return
	pointer to a page	OK
	NULL			fail
--------------------------------------------------------*/
EGI_PAGE *egi_create_openwrtpage(void)
{
	int i,j;

	EGI_EBOX *open_btns[6];
	EGI_DATA_BTN *data_btns[6];

	/* --------- 1. create buttons --------- */
        for(i=0;i<2;i++) /* row of buttons*/
        {
                for(j=0;j<3;j++) /* column of buttons */
                {
			/* 1. create new data_btns */
			data_btns[3*i+j]=egi_btndata_new(3*i+j, /* int id */
							square, /* enum egi_btn_type shape */
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
			open_btns[3*i+j]=egi_btnbox_new(NULL, /* put tag later */
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
				printf("egi_create_openwrtpage(): fail to call egi_btnbox_new() for open_btns[%d]. retry...\n", 3*i+j);
				free(data_btns[3*i+j]);
				data_btns[3*i+j]=NULL;

				i--;
				continue;
			}
		}
	}

	/* add tags  here */
	egi_ebox_settag(open_btns[0], "btn_openwrt 0");
	egi_ebox_settag(open_btns[1], "btn_openwrt 1");
	egi_ebox_settag(open_btns[2], "btn_openwrt 2");
	egi_ebox_settag(open_btns[3], "btn_openwrt 3");
	egi_ebox_settag(open_btns[4], "btn_openwrt 4");
	egi_ebox_settag(open_btns[5], "btn_openwrt 5");


	/* --------- 2. create title bar --------- */
	EGI_EBOX *title_bar= create_ebox_titlebar(
	        0, 0, /* int x0, int y0 */
        	0, 2,  /* int offx, int offy */
		//egi_color_random(light),  /* uint16_t bkcolor */
		egi_colorgray_random(light),  /* uint16_t bkcolor */
    		NULL	/* char *title */
	);
	egi_txtbox_settitle(title_bar, "    OpenWRT System Info.");


	/* --------- 3. create home page ------- */
	EGI_PAGE *page_openwrt=NULL;
	page_openwrt=egi_page_new("page_openwrt");
	while(page_openwrt==NULL)
	{
			printf("egi_create_openwrtpage(): fail to call egi_page_new(), try again ...\n");
			page_openwrt=egi_page_new("page_openwrt");
			usleep(100000);
	}
	/* set prmcolor */
	page_openwrt->ebox->prmcolor=egi_colorgray_random(deep);

	/* set wallpaper */
	//page_openwrt->fpath="/tmp/mplay.jpg";

	/* add ebox to home page */
	for(i=0;i<6;i++) /* buttons */
		egi_page_addlist(page_openwrt, open_btns[i]);
	egi_page_addlist(page_openwrt, title_bar); /* title bar */

	return page_openwrt;
}

