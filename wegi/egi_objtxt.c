/*----------------------- egi_obj.c ------------------------------

1. All txt type ebox are to be allocated/freeed dynamically.

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include "bmpjpg.h"
#include "egi_color.h"
#include "egi.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
#include "egi_timer.h"
#include "egi_debug.h"
//#include "egi_timer.h"
#include "egi_symbol.h"


static int egi_txtbox_decorate(EGI_EBOX *ebox);


/* ---------------------------  ebox memo --------------------------------*/

/*-------------------------------------------
Create an 240x30 size txt ebox
return:
	txt ebox pointer 	OK
	NULL			fai.
--------------------------------------------*/
EGI_EBOX *create_ebox_memo(void)
{
	/* 1. create memo_txt */
	EGI_DATA_TXT *memo_txt=egi_txtdata_new(
		5,5, /* offset X,Y */
      	  	12, /*int nl, lines  */
       	 	24, /*int llen, chars per line */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* uint16_t color */
	);

	/* 2. fpath */
	memo_txt->fpath="/home/memo.txt";

	/* 3. create memo ebox */
	EGI_EBOX  *ebox_memo= egi_txtbox_new(
		"memo stick", /* tag */
        	memo_txt,  /* EGI_DATA_TXT pointer */
        	true, /* bool movable */
       	 	12,0, /* int x0, int y0 */
        	240,320, /* int width, int height */
        	-1, /* int frame, -1=no frame */
        	WEGI_COLOR_ORANGE /*int prmcolor*/
	);

	return ebox_memo;
}


/* ---------------------------  ebox clock --------------------------------*/

/*-------------------------------------------
Create an  txt ebox digital clock
return:
	txt ebox pointer 	OK
	NULL			fai.
--------------------------------------------*/
EGI_EBOX *create_ebox_clock(void)
{

	/* 1. create a data_txt */
	EGI_DATA_TXT *clock_txt=egi_txtdata_new(
		20,0, /* offset X,Y */
      	  	3, /*int nl, lines  */
       	 	64, /*int llen, chars per line */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* uint16_t color */
	);

	strncpy(clock_txt->txt[1],"abcdefg",5);

	/* 2. create memo ebox */
	EGI_EBOX  *ebox_clock= egi_txtbox_new(
		"timer txt", /* tag */
        	clock_txt,  /* EGI_DATA_TXT pointer */
        	true, /* bool movable */
       	 	60,5, /* int x0, int y0 */
        	120,20, /* int width, int height */
        	0, /* int frame,0=simple frmae, -1=no frame */
        	WEGI_COLOR_BROWN /*int prmcolor*/
	);

	return ebox_clock;
}


/* ---------------------------  ebox note --------------------------------*/
EGI_EBOX *create_ebox_note(void)
{

	/* 1. create a data_txt */
	EGI_DATA_TXT *note_txt=egi_txtdata_new(
		5,5, /* offset X,Y */
      	  	2, /*int nl, lines  */
       	 	32, /*int llen, chars per line */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* uint16_t color */
	);

	/* 2. create memo ebox */
	EGI_EBOX  *ebox_clock= egi_txtbox_new(
		"note pad", /* tag */
        	note_txt,  /* EGI_DATA_TXT pointer */
        	true, /* bool movable */
       	 	5,80, /* int x0, int y0 */
        	230,60, /* int width, int height */
        	-1, /* int frame, -1=no frame */
        	WEGI_COLOR_GRAY /*int prmcolor*/
	);

	return ebox_clock;
}


/*-------------------------------------------------
Create an  txt note ebox with parameters

num: 		id number for txt
x0,y0:		left top cooordinate
bkcolor:	ebox color

return:
	txt ebox pointer 	OK
	NULL			fai.
------------------------------------------------*/
EGI_EBOX *create_ebox_notes(int num, int x0, int y0, uint16_t bkcolor)
{
	/* 1. create a data_txt */
	egi_pdebug(DBG_OBJTXT,"start to egi_txtdata_new()...\n");
	EGI_DATA_TXT *clock_txt=egi_txtdata_new(
		10,30, /* offset X,Y */
      	  	3, /*int nl, lines  */
       	 	64, /*int llen, chars per line */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* uint16_t color */
	);

	if(clock_txt == NULL)
	{
		printf("create_ebox_notes(): clock_txt==NULL, fails!\n");
		return NULL;
	}
	else if (clock_txt->txt[0]==NULL)
	{
		printf("create_ebox_notes(): clock_txt->[0]==NULL, fails!\n");
		return NULL;
	}

        sprintf(clock_txt->txt[0],"        2019 ");
        sprintf(clock_txt->txt[1],"Happy New Year!");
        sprintf(clock_txt->txt[2],"Note NO. %d", num);

	/* 2. create memo ebox */
	egi_pdebug(DBG_OBJTXT,"create_ebox_notes(): strat to egi_txtbox_new().....\n");
	EGI_EBOX  *ebox_clock= egi_txtbox_new(
		NULL, /* tag, put later */
        	clock_txt,  /* EGI_DATA_TXT pointer */
        	true, /* bool movable */
       	 	x0,y0, /* int x0, int y0 */
        	160,66, /* int width, int height */
        	0, /* int frame,0=simple frmae, -1=no frame */
        	bkcolor /*int prmcolor*/
	);

	return ebox_clock;
}


/*----------------------------------------------
	A simple demo for txt type ebox
----------------------------------------------*/
void egi_txtbox_demo(void)
{
	int total=56;
	int i;
	EGI_EBOX *txtebox[56];
	int ret;

	printf("--------- txtebox demon --- START ---------\n");

	for(i=0;i<total;i++)
	{
	      egi_pdebug(DBG_OBJTXT,"create ebox notes txtebox[%d].\n",i);
	      txtebox[i]=create_ebox_notes(i, egi_random_max(80), egi_random_max(320-108), egi_color_random(light));
	      if(txtebox[i]==NULL)
	      {
			printf("egi_txtbox_demon(): create a txtebox[%d] fails!\n",i);
			return;
	      }
	      /* put decorate */
	      txtebox[i]->decorate=egi_txtbox_decorate;

	      egi_pdebug(DBG_OBJTXT,"egi_txtbox_demon(): start to activate txtebox[%d]\n",i);
	      ret=txtebox[i]->activate(txtebox[i]);
	      if(ret != 0)
			printf(" egi_txtbox_demo() txtebox activate fails with ret=%d\n",ret);

	      /* apply decoration method */
	      egi_pdebug(DBG_OBJTXT,"egi_txtbox_demon(): start to decorate txtebox[%d]\n",i);
	      ret=txtebox[i]->decorate(txtebox[i]);
	      if(ret != 0)
			printf(" egi_txtbox_demo() txtebox decorate fails with ret=%d\n",ret);


	      tm_delayms(5);
//	      usleep(200000);
	}
	tm_delayms(3000); /* hold on SHOW */

	for(i=total-1;i>=0;i--)
	{
		      txtebox[i]->sleep(txtebox[i]); /* dispare from LCD */
		      txtebox[i]->free(txtebox[i]);
		      tm_delayms(5);
	      // --- free ---
	}
//	tm_delayms(2000); /* hold on CLEAR */
	//getchar();
	printf("--------- txtebox demon --- END ---------\n");
}




/////////////////////// <<<<<    EGI PATTERNS    >>>>> ////////////////////////

/*-----------   EGI_PATTERN :  TITLE BAR 240x30 -----------------
Create an txt_ebox  for a title bar.
standard tag "title_bar"

x0,y0:		left top cooordinate
offx,offy:	offset of txt
bkcolor:	ebox color, bkcolor

return:
	txt ebox pointer 	OK
	NULL			fai.
---------------------------------------------------------*/
EGI_EBOX *create_ebox_titlebar(
	int x0, int y0,
	int offx, int offy,
	uint16_t bkcolor,
	char *title
)
{
	/* 1. create a data_txt */
	egi_pdebug(DBG_OBJTXT,"start to egi_txtdata_new()...\n");
	EGI_DATA_TXT *title_txt=egi_txtdata_new(
		offx,offy, /* offset X,Y */
      	  	1, /*int nl, lines  */
       	 	64, /*int llen, chars per line, however also limited by width */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* int16_t color */
	);

	if(title_txt == NULL)
	{
		printf("create_ebox_titlebar(): title_txt=egi_txtdata_new()=NULL, fails!\n");
		return NULL;
	}
	else if (title_txt->txt[0]==NULL)
	{
		printf("create_ebox_titlebar(): title_txt->[0]==NULL, fails!\n");
		return NULL;
	}

	/* 2. put title string */
	if(title==NULL)
	{
		egi_pdebug(DBG_OBJTXT,"create_ebox_titlebar(): title==NULL, use default title\n");
	        strncpy(title_txt->txt[0], "--- title bar ---", title_txt->llen-1); /* default */
	}
	else
	{
	        strncpy(title_txt->txt[0], title, title_txt->llen-1); /* default */
	}


	/* 3. create memo ebox */
	PDEBUG("create_ebox_titlebar(): start egi_txtbox_new().....\n");
	EGI_EBOX  *ebox_title= egi_txtbox_new(
		"title_bar", /* tag, or put later */
        	title_txt,  /* EGI_DATA_TXT pointer */
        	true, /* bool movable */
       	 	x0,y0, /* int x0, int y0 */
        	240,30, /* int width;  int height,which also related with symheight and offy */
        	0, /* int frame, 0=simple frmae, -1=no frame */
        	bkcolor /*int prmcolor*/
	);

	return ebox_title;
}


/*--------------------------------------------------
	txtbox decorationg function
---------------------------------------------------*/
static int egi_txtbox_decorate(EGI_EBOX *ebox)
{
        int ret=0;

        egi_pdebug(DBG_TXT,"egi_txtbox_decorate(): start to show_jpg()...\n");
        ret=show_jpg("/tmp/openwrt.jpg", &gv_fb_dev, SHOW_BLACK_NOTRANSP, ebox->x0+2, ebox->y0+2);

        return ret;
}


/*------------------------------------------------------------
Create Message Box, Size:240x50
display for a while, then release.

long ms: msg displaying time, in ms.
------------------------------------------------------------*/
void egi_display_msgbox (char *msg, long ms, uint16_t bkcolor)
{
	printf("---------- len(msg)=%d ----------\n",strlen(msg));

	/* 1. create a data_txt */
	egi_pdebug(DBG_OBJTXT,"egi_display_msgbox(): start to egi_txtdata_new()...\n");
	EGI_DATA_TXT *msg_txt=egi_txtdata_new(
		30,12, /* offset X,Y */
      	  	1, /*int nl, lines  */
       	 	64, /*int llen, chars per line, however also limited by width */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* int16_t color */
	);

	if(msg_txt == NULL)
	{
		printf("egi_display_msgbox(): msg_txt=egi_txtdata_new()=NULL, fails!\n");
		return;
	}
	else if (msg_txt->txt[0]==NULL)
	{
		printf("egi_display_msgbox(): msg_txt->[0]==NULL, fails!\n");
		return;
	}

	/* 2. put msg string */
	if(msg==NULL)
	{
		egi_pdebug(DBG_OBJTXT,"egi_display_msgbox(): msg==NULL, use default MSG\n");
	        strncpy(msg_txt->txt[0], "--- Message Box ---", msg_txt->llen-1); /* default */
	}
	else
	{
	        strncpy(msg_txt->txt[0], msg, msg_txt->llen-1); /* default */
	}


	/* 3. create msg ebox */
	egi_pdebug(DBG_OBJTXT,"egi_display_msgbox(): start egi_txtbox_new().....\n");
	EGI_EBOX  *msgbox= egi_txtbox_new(
		"msg_box", /* tag, or put later */
        	msg_txt,  /* EGI_DATA_TXT pointer */
        	true, /* bool movable */
       	 	0,50, /* int x0, int y0 */
        	240,50, /* int width;  int height,which also related with symheight and offy */
        	2, /* int frame, 0=simple frmae, -1=no frame */
        	bkcolor /*int prmcolor*/
	);

	/* 4. display msg box */
	msgbox->activate(msgbox);/* activate to display */
	tm_delayms(ms);
	msgbox->sleep(msgbox); /* erase the image */
	msgbox->free(msgbox); /* release */

}
