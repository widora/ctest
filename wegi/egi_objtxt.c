/*----------------------- egi_obj.c ------------------------------

1. All txt type ebox are to be allocated/freeed dynamically.

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include "egi_color.h"
#include "egi.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
#include "egi_timer.h"
#include "egi_debug.h"
//#include "egi_timer.h"
#include "symbol.h"



/*---------------------------------------
return a random value not great than max
---------------------------------------*/
int egi_random_max(int max)
{
	int ret;
	struct timeval tmval;

	gettimeofday(&tmval,NULL);

        srand(tmval.tv_usec);
        ret = 1+(int)((float)max*rand()/(RAND_MAX+1.0));
	printf("random max ret=%d\n",ret);

	return ret;
}


/* ---------------------------  ebox memo --------------------------------*/

/*-------------------------------------------
Create an 240x320 size txt ebox
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
		type_txt, /*enum egi_ebox_type type */
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
		type_txt, /*enum egi_ebox_type type */
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
		type_txt, /*enum egi_ebox_type type */
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
	printf("start to egi_txtdata_new()...\n");
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
	PDEBUG("create_ebox_notes(): strat to egi_txtbox_new().....\n");
	EGI_EBOX  *ebox_clock= egi_txtbox_new(
		"timer txt", /* tag */
		type_txt, /*enum egi_ebox_type type */
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

	for(i=0;i<total;i++)
	{
	      PDEBUG("create ebox notes txtebox[%d].\n",i);
	      txtebox[i]=create_ebox_notes(i, egi_random_max(80), egi_random_max(320-108), egi_random_color());
	      if(txtebox[i]==NULL)
	      {
			printf("egi_txtbox_demon(): create a txtebox[%d] fails!\n",i);
			return;
	      }

	      PDEBUG("egi_txtbox_demon(): start to activate txtebox[%d]\n",i);
	      ret=txtebox[i]->activate(txtebox[i]);
	      if(ret != 0)
			printf(" egi_txtbox_demo() txtebox activate fails with ret=%d\n",ret);

	      /* apply decoration method */
	      PDEBUG("egi_txtbox_demon(): start to decorate txtebox[%d]\n",i);
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
	printf("--------- txtebox demon over END ---------\n");
}
