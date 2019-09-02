/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example to analyze MIC captured audio and display its spectrum.

Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdio.h>
#include "egi_common.h"
#include "egi_pcm.h"
#include "egi_FTsymbol.h"

int main(void)
{
	int i,j,k;
	int ret;
	int blur_size;

	struct timeval tm_start;
	struct timeval tm_end;

        /* <<<<<  EGI general init  >>>>>> */
#if 1
        printf("tm_start_egitick()...\n");
        tm_start_egitick();		   	/* start sys tick */
        printf("egi_init_log()...\n");
        if(egi_init_log("/mmc/log_fb") != 0) {	/* start logger */
                printf("Fail to init logger,quit.\n");
                return -1;
        }
#endif
        printf("symbol_load_allpages()...\n");
        if(symbol_load_allpages() !=0 ) {   	/* load sys fonts */
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
        if(FTsymbol_load_appfonts() !=0 ) {  	/* load FT fonts LIBS */
                printf("Fail to load FT appfonts, quit.\n");
                return -2;
        }
        printf("init_fbdev()...\n");
        if( init_fbdev(&gv_fb_dev) )		/* init sys FB */
                return -1;


int rad=200;
EGI_IMGBUF* pimg=NULL;
EGI_IMGBUF* eimg=NULL;
EGI_IMGBUF* softimg=NULL;

show_jpg("/tmp/home.jpg",&gv_fb_dev, false, 0, 0);

pimg=egi_imgbuf_alloc();
//egi_imgbuf_loadjpg("/tmp/fish.jpg", pimg);
egi_imgbuf_loadpng("/tmp/test.png", pimg);


blur_size=74;


do {    ////////////////////////////    LOOP TEST   /////////////////////////////////


#if 0	/* --- change blur_size --- */
	if(blur_size > 35 )
		blur_size -=15;
	else
		blur_size -=3;
	if(blur_size <= 0) {
		tm_delayms(3000);
		blur_size=75;
	}

#else   /* --- keep blur_size --- */
	blur_size=1;
#endif
	printf("blur_size=%d\n", blur_size);

/* ------------------------------------------------------------------------------------
NOTE:
1. egi_imgbuf_avgsoft(), operating 2D array data of color/alpha, is faster than
   egi_imgbuf_avgsoft2() with 1D array data.
   This is because 2D array is much faster for sorting/picking data.!!!?

2. When blur_size==1, they are nearly the same speed.


-------------------------------------------------------------------------------- */
	show_jpg("/tmp/home.jpg",&gv_fb_dev, false, 0, 0);

#if 1	/* ------- test egi_imgbuf_avgsoft(), use 2D array ------- */
	gettimeofday(&tm_start,NULL);
	softimg=egi_imgbuf_avgsoft(pimg, blur_size, true, false); /* eimg, size, alpha_on, hold_on */
	gettimeofday(&tm_end,NULL);
	printf("egi_imgbuf_avgsoft() 2D time cost: %dms\n",tm_signed_diffms(tm_start, tm_end));
	/* set frame */
	#if 0
	egi_imgbuf_setframe( softimg, frame_round_rect,	/* EGI_IMGBUF, enum imgframe_type */
	                     -1, 1, &rad );		/*  init alpha, int pn, const int *param */
	#endif
	/* display */
	printf("start windisplay...\n");
	gettimeofday(&tm_start,NULL);
	egi_imgbuf_windisplay( softimg, &gv_fb_dev, -1,    	/* img, FB, subcolor */
                               0, 0,   				/* int xp, int yp */
			       0, 0, softimg->width, softimg->height   /* xw, yw, winw,  winh */
			      );
	gettimeofday(&tm_end,NULL);
	printf("windisplay time cost: %dms\n",tm_signed_diffms(tm_start, tm_end));

	/* free softimg */
	egi_imgbuf_free(softimg);

	tm_delayms(1000);
#endif /* ---- End test egi_imgbuf_avgsoft() ---- */


#if 0	/* ------- test egi_imgbuf_avgsoft2(), 1D  ------- */
	gettimeofday(&tm_start,NULL);
	softimg=egi_imgbuf_avgsoft2(pimg, blur_size, true);
	gettimeofday(&tm_end,NULL);
	printf("egi_imgbuf_avgsoft2(): 1D time cost: %dms\n",tm_signed_diffms(tm_start, tm_end));
	/* set frame */
	#if 0
	egi_imgbuf_setframe( softimg, frame_round_rect,	/* EGI_IMGBUF, enum imgframe_type */
 	                     -1, 1, &rad );		/*  init alpha, int pn, const int *param */
	#endif
	/* display */
	printf("start windisplay...\n");
	gettimeofday(&tm_start,NULL);
	egi_imgbuf_windisplay( softimg, &gv_fb_dev, -1,    	/* img, FB, subcolor */
                               0, 0,   				/* int xp, int yp */
			       0, 0, softimg->width, softimg->height   /* xw, yw, winw,  winh */
			      );
	gettimeofday(&tm_end,NULL);
	printf("windisplay time cost: %dms\n",tm_signed_diffms(tm_start, tm_end));

	/* free softimg */
	egi_imgbuf_free(softimg);

	tm_delayms(1000);
#endif /* ---- End test egi_imgbuf_avgsoft2() ---- */

//	tm_delayms(250/blur_size); //2000);


#if 0
        eimg=egi_imgbuf_newFrameImg( 80, 180,		  /* int height, int width */
                          	  255, egi_color_random(color_medium), /* alpha, color */
				  frame_round_rect,	  /* enum imgframe_type */
                               	  1, &rad );		  /* int pn, int *param */

	egi_imgbuf_windisplay( eimg, &gv_fb_dev, -1,    	/* img, FB, subcolor */
                               0, 0,   				/* int xp, int yp */
			       30,30, eimg->width, eimg->height   /* xw, yw, winw,  winh */
			      );

	egi_imgbuf_free(eimg);
#endif



}while(1); ////////////////////////////    LOOP TEST   /////////////////////////////////

	egi_imgbuf_free(pimg);


        /* <<<<<  EGI general release >>>>> */
        printf("FTsymbol_release_allfonts()...\n");
        FTsymbol_release_allfonts();
        printf("symbol_release_allpages()...\n");
        symbol_release_allpages();
	printf("release_fbdev()...\n");
        fb_filo_flush(&gv_fb_dev);
        release_fbdev(&gv_fb_dev);
        printf("egi_quit_log()...\n");
        egi_quit_log();
        printf("<-------  END  ------>\n");

return 0;
}


/* ------------------------- TEST RESULT ------------------

	 <<<  -----   Typical data for processing a PNG image data, size 1024 x 901. -----   >>>

blur_size=59
avgsoft 2D time cost: 12468ms
avgsoft 1D time cost: 28766ms
start windisplay...
windisplay time cost: 48ms
blur_size=44
avgsoft 2D time cost: 9631ms
avgsoft 1D time cost: 20279ms
start windisplay...
windisplay time cost: 48ms
blur_size=29
avgsoft 2D time cost: 6785ms
avgsoft 1D time cost: 13483ms
start windisplay...
windisplay time cost: 48ms
blur_size=26
avgsoft 2D time cost: 6238ms
avgsoft 1D time cost: 12683ms
start windisplay...
windisplay time cost: 42ms
blur_size=23
avgsoft 2D time cost: 5604ms
avgsoft 1D time cost: 10852ms
start windisplay...
windisplay time cost: 49ms
blur_size=20
avgsoft 2D time cost: 4978ms
avgsoft 1D time cost: 9701ms
start windisplay...
windisplay time cost: 48ms
blur_size=17
avgsoft 2D time cost: 4372ms
avgsoft 1D time cost: 5617ms
start windisplay...
windisplay time cost: 49ms
blur_size=14
avgsoft 2D time cost: 3800ms
avgsoft 1D time cost: 4025ms
start windisplay...
windisplay time cost: 49ms
blur_size=11
avgsoft 2D time cost: 3250ms
avgsoft 1D time cost: 3343ms
start windisplay...
windisplay time cost: 49ms
blur_size=8
avgsoft 2D time cost: 2684ms
avgsoft 1D time cost: 2782ms
start windisplay...
windisplay time cost: 48ms
blur_size=5
avgsoft 2D time cost: 2096ms
avgsoft 1D time cost: 2192ms
start windisplay...
windisplay time cost: 48ms
blur_size=2
avgsoft 2D time cost: 1491ms
avgsoft 1D time cost: 1554ms
start windisplay...
windisplay time cost: 48ms

		<<< ---- blur_size ==1 ---- >>>
blur_size=1
avgsoft 2D time cost: 1173ms
avgsoft 1D time cost: 1161ms
start windisplay...
windisplay time cost: 42ms
blur_size=1
avgsoft 2D time cost: 1158ms
avgsoft 1D time cost: 1163ms
start windisplay...
windisplay time cost: 42ms
blur_size=1
avgsoft 2D time cost: 1153ms
avgsoft 1D time cost: 1163ms
start windisplay...
windisplay time cost: 42ms
blur_size=1
avgsoft 2D time cost: 1160ms
avgsoft 1D time cost: 1161ms
start windisplay...
windisplay time cost: 43ms
blur_size=1
avgsoft 2D time cost: 1153ms
avgsoft 1D time cost: 1158ms


	 <<<  -----   Typical data for processing a PNG image data, size 532 x 709. -----   >>>

blur_size=59
avgsoft 2D time cost: 6423ms
avgsoft 1D time cost: 7012ms
start windisplay...
windisplay time cost: 48ms
blur_size=44
avgsoft 2D time cost: 5859ms
avgsoft 1D time cost: 5511ms
start windisplay...
windisplay time cost: 42ms
blur_size=29
avgsoft 2D time cost: 3518ms
avgsoft 1D time cost: 4033ms
start windisplay...
windisplay time cost: 37ms
blur_size=26
avgsoft 2D time cost: 3203ms
avgsoft 1D time cost: 3739ms
start windisplay...
windisplay time cost: 37ms
blur_size=23
avgsoft 2D time cost: 2923ms
avgsoft 1D time cost: 3422ms
start windisplay...
windisplay time cost: 34ms
blur_size=20
avgsoft 2D time cost: 2687ms
avgsoft 1D time cost: 3118ms
start windisplay...
windisplay time cost: 33ms
blur_size=17
avgsoft 2D time cost: 2383ms
avgsoft 1D time cost: 2338ms
start windisplay...
windisplay time cost: 32ms
blur_size=14
avgsoft 2D time cost: 2091ms
avgsoft 1D time cost: 1991ms
start windisplay...
windisplay time cost: 30ms
blur_size=11
avgsoft 2D time cost: 1808ms
avgsoft 1D time cost: 1680ms
start windisplay...
windisplay time cost: 28ms
blur_size=8
avgsoft 2D time cost: 1503ms
avgsoft 1D time cost: 1400ms
start windisplay...
windisplay time cost: 26ms
blur_size=5
avgsoft 2D time cost: 1159ms
avgsoft 1D time cost: 1056ms
start windisplay...
windisplay time cost: 23ms
blur_size=2
avgsoft 2D time cost: 896ms
avgsoft 1D time cost: 763ms
start windisplay...
windisplay time cost: 20ms

		<<< ---- blur_size ==1 ---- >>>

blur_size=1
avgsoft 2D time cost: 722ms
avgsoft 1D time cost: 550ms
start windisplay...
windisplay time cost: 20ms
blur_size=1
avgsoft 2D time cost: 720ms
avgsoft 1D time cost: 546ms
start windisplay...
windisplay time cost: 19ms
blur_size=1
avgsoft 2D time cost: 722ms
avgsoft 1D time cost: 549ms

----------------------------------------------------------*/
