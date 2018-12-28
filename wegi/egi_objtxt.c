/*----------------------- egi_obj.c ------------------------------

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "color.h"
#include "egi.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
//#include "egi_timer.h"
#include "symbol.h"


/* ----- common -----  txt ebox self_defined methods */
static struct egi_ebox_method txtbox_method=
{
	.activate=egi_txtbox_activate,
	.refresh=egi_txtbox_refresh,
	.sleep=egi_txtbox_sleep,
	.free=NULL,
};






/* ---------------------------  ebox memo --------------------------------*/

/*-------------------------------------------
Create an 240x320 size txt ebox
return:
	txt ebox pointer 	OK
	NULL			fai.
--------------------------------------------*/
struct egi_element_box *create_ebox_memo(void)
{

	/* 1. create memo_txt */
	struct egi_data_txt *memo_txt=egi_txtdata_new(
		5,5, /* offset X,Y */
      	  	12, /*int nl, lines  */
       	 	24, /*int llen, chars per line */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* uint16_t color */
	);

	/* 2. fpath */
	memo_txt->fpath="/home/memo.txt";

	/* 3. create memo ebox */
	struct egi_element_box  *ebox_memo= egi_txtbox_new(
		"memo stick", /* tag */
		type_txt, /*enum egi_ebox_type type */
        	memo_txt,  /* struct egi_data_txt pointer */
        	txtbox_method, /*struct egi_ebox_method method */
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
struct egi_element_box *create_ebox_clock(void)
{

	/* 1. create a data_txt */
	struct egi_data_txt *clock_txt=egi_txtdata_new(
		0,0, /* offset X,Y */
      	  	1, /*int nl, lines  */
       	 	32, /*int llen, chars per line */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BROWN /* uint16_t color */
	);

	/* 2. create memo ebox */
	struct egi_element_box  *ebox_clock= egi_txtbox_new(
		"timer txt", /* tag */
		type_txt, /*enum egi_ebox_type type */
        	clock_txt,  /* struct egi_data_txt pointer */
        	txtbox_method, /*struct egi_ebox_method method */
        	true, /* bool movable */
       	 	60,5, /* int x0, int y0 */
        	120,20, /* int width, int height */
        	-1, /* int frame, -1=no frame */
        	WEGI_COLOR_ORANGE /*int prmcolor*/
	);

	return ebox_clock;
}


/* ---------------------------  ebox note --------------------------------*/
struct egi_element_box *create_ebox_note(void)
{

	/* 1. create a data_txt */
	struct egi_data_txt *note_txt=egi_txtdata_new(
		5,5, /* offset X,Y */
      	  	2, /*int nl, lines  */
       	 	32, /*int llen, chars per line */
        	&sympg_testfont, /*struct symbol_page *font */
        	WEGI_COLOR_BLACK /* uint16_t color */
	);

	/* 2. create memo ebox */
	struct egi_element_box  *ebox_clock= egi_txtbox_new(
		"note pad", /* tag */
		type_txt, /*enum egi_ebox_type type */
        	note_txt,  /* struct egi_data_txt pointer */
        	txtbox_method, /*struct egi_ebox_method method */
        	true, /* bool movable */
       	 	5,80, /* int x0, int y0 */
        	230,60, /* int width, int height */
        	-1, /* int frame, -1=no frame */
        	WEGI_COLOR_GRAY /*int prmcolor*/
	);

	return ebox_clock;
}
