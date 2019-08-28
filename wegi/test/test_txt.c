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
	int i,j;
	int ret;
	EGI_POINT pt;


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


	/* ------ prepare TXT type ebox ------- */

unsigned char utext[200]="千山鸟飞绝,\n	\
万径人踪灭。\n	\
孤舟蓑笠翁, \n	\
独钓寒江雪。";

int width=115;

/* For FTsymbols */
EGI_DATA_TXT *memo_txt=egi_utxtdata_new( 5, 5,     		    /* offset from ebox left top */
                       		         4, width,                    /* lines, pixels per line */
		                         egi_appfonts.regular,      /* font face type */
                                	 18, 18,         	    /* font width and height, in pixels */
                                	 5,			    /* adjust gap */
					 WEGI_COLOR_BLACK           /* font color  */
                        	       );
		memo_txt->utxt=utext;

EGI_EBOX  *ebox_txt= 	egi_txtbox_new(	"memo stick",   /* tag */
			              	memo_txt,  	/* EGI_DATA_TXT pointer */
                		 	true, 		/* bool movable */
			                0,0, 		/* int x0, int y0 */
			/* TODO: if ebox width < txt width */
			                width+5, 60, 	/* width, height(adjusted as per nl and fw) */
			                -1, 		/* int frame, -1=no frame */
			                WEGI_COLOR_ORANGE   /* prmcolor*/
        			     );

	/* activate and display */
	ebox_txt->activate(ebox_txt);


do {    ////////////////////////   LOOP TEST 	////////////////////////////

	tm_delayms(500);

	/* move ebox txt */
	egi_randp_inbox(&pt, &gv_fb_box);
	ebox_txt->x0=pt.x;
	ebox_txt->y0=pt.y;

	/* refresh ebox txt */
	ebox_txt->need_refresh=true;
	ebox_txt->refresh(ebox_txt);

} while(1);  ////////////////////////////   END   //////////////////////////////


	/* erase ebox txt image and free */
	ebox_txt->sleep(ebox_txt);
	ebox_txt->free(ebox_txt);

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
