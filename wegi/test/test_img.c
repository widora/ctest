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


int rad=10;
EGI_IMGBUF* eimg;
show_jpg("/tmp/home.jpg",&gv_fb_dev, false, 0, 0);

do {    ////////////////////////////    LOOP TEST   /////////////////////////////////

        eimg=egi_imgbuf_newframe( 80, 180,		  /* int height, int width */
                          	  255, egi_color_random(color_medium), /* alpha, color */
				  frame_round_rect,	  /* enum imgframe_type */
                               	  1, &rad );		  /* int pn, int *param */

	egi_imgbuf_windisplay( eimg, &gv_fb_dev, -1,    	/* img, FB, subcolor */
                               0, 0,   				/* int xp, int yp */
			       30,30, eimg->width, eimg->height   /* xw, yw, winw,  winh */
			      );
	tm_delayms(100);

	egi_imgbuf_free(eimg);

}while(1); ////////////////////////////    LOOP TEST   /////////////////////////////////

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
