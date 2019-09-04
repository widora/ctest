/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example to resize an image.

Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdio.h>
#include "egi_common.h"
#include "egi_pcm.h"
#include "egi_FTsymbol.h"

int main(int argc, char** argv)
{
	int i,j,k;
	int ret;

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
EGI_IMGBUF* pimg=NULL; /* input picture image */
EGI_IMGBUF* eimg=NULL;
EGI_IMGBUF* softimg=NULL;

show_jpg("/tmp/home.jpg",&gv_fb_dev, false, 0, 0);



pimg=egi_imgbuf_alloc();


//(egi_imgbuf_loadjpg("/tmp/test.jpg", pimg);
//egi_imgbuf_loadpng("/tmp/test.png", pimg);

if( egi_imgbuf_loadjpg(argv[1],pimg)!=0 && egi_imgbuf_loadpng(argv[1],pimg)!=0 ) {
  printf(" Fail to load file %s!\n", argv[1]);
  exit(-1);
}


do {    ////////////////////////////    LOOP TEST   /////////////////////////////////

//   show_jpg("/tmp/home.jpg",&gv_fb_dev, false, 0, 0);
   for(i=24; i<=240*3; i+=12 ) {
	show_jpg("/tmp/home.jpg",&gv_fb_dev, false, 0, 0);

	/* resize */
	eimg=egi_imgbuf_resize( pimg, i, i );   /* eimg,height,width,   align center */
	//eimg=egi_imgbuf_resize( pimg, i, i ); /* eimg,height,width,   align left   */
	if(eimg==NULL)
		exit(-1);

	printf("start windisplay...\n");
	egi_imgbuf_windisplay( eimg, &gv_fb_dev, -1,    	/* img, FB, subcolor */
                               0, 0,   				/* int xp, int yp */
			       120-(i>>1), 0,			/* xw, yw , align center */
			       //0, 0,				/* xw, yw , align left */
			       eimg->width, eimg->height   /* winw,  winh */
			       //0, 0, eimg->width>240?240:eimg->width , eimg->height   /* xw, yw, winw,  winh */
			      );
	egi_imgbuf_free(eimg);
	tm_delayms(150);
    }

	tm_delayms(3000);

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




#if 0 /* <<<<<<<<<<<<<<<<<    test egi_imgbuf_avgsoft(), 2D array  >>>>>>>>>>>>>> */
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
	show_jpg("/tmp/home.jpg",&gv_fb_dev, false, 0, 0);
	gettimeofday(&tm_start,NULL);
	egi_imgbuf_windisplay( softimg, &gv_fb_dev, -1,    	/* img, FB, subcolor */
                               0, 0,   				/* int xp, int yp */
			       0, 0, softimg->width, softimg->height   /* xw, yw, winw,  winh */
			      );
	gettimeofday(&tm_end,NULL);
	printf("windisplay time cost: %dms\n",tm_signed_diffms(tm_start, tm_end));

	/* free softimg */
	egi_imgbuf_free(softimg);

	tm_delayms(500);
#endif /* ---- End test egi_imgbuf_avgsoft() ---- */
