/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Test image rotation functions.

Usage:
	./test_img3  file

Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdio.h>
#include "egi_common.h"
#include "egi_pcm.h"
#include "egi_FTsymbol.h"
#include "egi_utils.h"

int main(int argc, char** argv)
{
	int i,j,k;
	int ret;

	struct timeval tm_start;
	struct timeval tm_end;

        /* <<<<<  EGI general init  >>>>>> */
#if 0
        printf("tm_start_egitick()...\n");
        tm_start_egitick();		   	/* start sys tick */
        printf("egi_init_log()...\n");
        if(egi_init_log("/mmc/log_test") != 0) {	/* start logger */
                printf("Fail to init logger,quit.\n");
                return -1;
        }
#endif

#if 0
        printf("symbol_load_allpages()...\n");
        if(symbol_load_allpages() !=0 ) {   	/* load sys fonts */
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
        if(FTsymbol_load_appfonts() !=0 ) {  	/* load FT fonts LIBS */
                printf("Fail to load FT appfonts, quit.\n");
                return -2;
        }
#endif

        printf("init_fbdev()...\n");
        if( init_fbdev(&gv_fb_dev) )		/* init sys FB */
                return -1;

	/* <<<<<  End EGI Init  >>>>> */


#if 0 ///////////////////////////////////////////////////////
EGI_IMGBUF *test_img=egi_imgbuf_create(50, 180, 0, WEGI_COLOR_RED);/* H,W, alpha ,color */
printf("(-30)%%360=%d;  (-390)%%360=%d;  30%%(-360)=%d;  390%%(-360)=%d \n",
					(-30)%360, (-390)%360, 30%(-360), 390%(-360) );
for(i=0; i<361; i+=10)
{
	egi_imgbuf_rotate(test_img, i);
}
exit(0);
#endif ///////////////////////////////////////////////////////


EGI_IMGBUF* eimg=NULL;
EGI_IMGBUF* rotimg=NULL;
int x0,y0;

char**	fpaths=NULL;	/* File paths */
int	ftotal=0; 	/* File counts */
int	num=0;		/* File index */


        if(argc<2) {
                printf("Usage: %s file\n",argv[0]);
                exit(-1);
        }

	/* Buffer FB data */
	fb_buffer_FBimg(&gv_fb_dev, 0);

do {    ////////////////////////////   1.  LOOP TEST   /////////////////////////////////

	i+=-2;

	/* restore FB data */
	fb_restore_FBimg(&gv_fb_dev, 0, false);

        /* 1. Load pic to imgbuf */
	eimg=egi_imgbuf_readfile(argv[1]);
	if(eimg==NULL) {
        	EGI_PLOG(LOGLV_ERROR, "%s: Fail to read and load file '%s'!", __func__, argv[1]);
		return -1;
	}

        /* 2. Create rotated imgbuf */
	rotimg=egi_imgbuf_rotate(eimg, i);

	/* 3. Display the image */
	x0=(240-rotimg->width)/2;
	y0=(320-rotimg->height)/2;
        egi_imgbuf_windisplay( rotimg, &gv_fb_dev, -1,		 		/* img, fb, subcolor */
                               0, 0, x0, y0,					/* xp,yp  xw,yw */
                               rotimg->width, rotimg->height);	 		/* winw, winh */

	/* 4. Free imgs */
	egi_imgbuf_free(rotimg);
	rotimg=NULL;
	egi_imgbuf_free(eimg);
	eimg=NULL;

	/* 6. Clear screen and increase num */
	usleep(200000);
	//tm_delayms(200);
	//clear_screen(&gv_fb_dev, WEGI_COLOR_BLACK);


} while(1); ///////////////////////////   END LOOP TEST   ///////////////////////////////


	#if 0
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
	#endif

return 0;
}


