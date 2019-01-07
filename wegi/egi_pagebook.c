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
#include <unistd.h> /* usleep */
#include "egi.h"
#include "egi_debug.h"
#include "egi_color.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
#include "egi_btn.h"
#include "egi_page.h"
#include "egi_symbol.h"
#include "egi_pagehome.h"
#include "egi_pagemplay.h"
#include "egi_pagebook.h"




static void egi_pagebook_runner(EGI_PAGE *page);
static int egi_pagebook_exit(EGI_EBOX * ebox, enum egi_btn_status btn_status);




/*------------- [  PAGE ::   Home Book  ] -------------
create book page
Create an 240x320 size txt ebox

Return
        pointer to a page       OK
        NULL                    fail
------------------------------------------------------*/
EGI_PAGE *egi_create_bookpage(void)
{
        int i,j;

        EGI_PAGE 	*page_book;
        EGI_DATA_TXT	*book_txt;
	EGI_EBOX	*ebox_book;
        EGI_EBOX  	*title_bar;

	/* 1. create book_txt */
	book_txt=egi_txtdata_new(
		5,5, /* offset X,Y */
      	  	12, /*int nl, lines  */
       	 	24, /*int llen, chars per line */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* uint16_t color */
	);

	/* 2. set book txt fpath */
	book_txt->fpath="/home/memo.txt";

	/* 3. create memo ebox */
	ebox_book= egi_txtbox_new(
		"book", /* tag */
        	book_txt,  /* EGI_DATA_TXT pointer */
        	true, /* bool movable */
       	 	12,0, /* int x0, int y0 */
        	240,320, /* int width, int height */
        	-1, /* int frame, -1=no frame */
        	WEGI_COLOR_ORANGE /*int prmcolor*/
	);

        /* 4. create title bar */
        title_bar= create_ebox_titlebar(
                0, 0, /* int x0, int y0 */
                0, 2,  /* int offx, int offy */
                egi_colorgray_random(light),  /* int16_t bkcolor */
                NULL    /* char *title */
        );
        egi_txtbox_settitle(title_bar, "   Today's News ");

        /* 5. create book page  */
        /* 5.1 create book page */
        page_book=egi_page_new("page_book");
        while(page_book==NULL)
        {
                        printf("egi_create_bookpage(): fail to call egi_page_new(), try again ...\n");
                        page_book=egi_page_new("page_book");
                        usleep(100000);
        }
        /* 5.2 put pthread runner */
        //page_mplay->runner[0]= ;

        /* 5.3 set default routine job */
        page_book->routine=egi_page_routine; /* use default */

        /* 5.4 set wallpaper */
        page_book->fpath="/tmp/mplay.jpg";


	/* 5.5 add to page ebox list */
	egi_page_addlist(page_book,ebox_book); /* book */
 	egi_page_addlist(page_book, title_bar); /* title bar */



	return page_book;
}



/*-----------------  RUNNER 1 --------------------------

-------------------------------------------------------*/
static void egi_pagebook_runner(EGI_PAGE *page)
{

}

/*-----------------------------------
btn_close function:
return
-----------------------------------*/
static int egi_pagebook_exit(EGI_EBOX * ebox, enum egi_btn_status btn_status)
{
        return -1;
}



