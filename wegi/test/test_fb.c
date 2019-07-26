/*-----------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Test FBDEV functions


Midas Zhou
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "utils/egi_utils.h"

int main(int argc, char **argv)
{

EGI_IMGBUF *eimg=NULL;
FBDEV 	   vfb;

wchar_t *prophesy=L"人心生一念，天地尽皆知。善恶若无报，乾坤必有私。";

do {
        /* EGI general init */
	printf("tm_start_egitick()...\n");
        tm_start_egitick();
	printf("egi_init_log()...\n");
        if(egi_init_log("/mmc/log_fb") != 0) {
                printf("Fail to init logger,quit.\n");
                return -1;
        }
	printf("symbol_load_allpages()...\n");
        if(symbol_load_allpages() !=0 ) {
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
        if(FTsymbol_load_appfonts() !=0 ) {  /* and FT fonts LIBS */
                printf("Fail to load FT appfonts, quit.\n");
                return -2;
        }

	/* init a FB */
	printf("init_fbdev()...\n");
        if( init_fbdev(&gv_fb_dev) )
		return -1;

	/* init a virtal FB */
	printf("load PNG to imgbuf...\n");
        eimg=egi_imgbuf_new();
        if( egi_imgbuf_loadpng("/mmc/icerock.png", eimg) ) //icerock.png
                return -1;

	printf("init_virt_fbdev()...\n");
	if(init_virt_fbdev(&vfb, eimg))
		return -1;


#if 1 /* <<<<<<<<<<< TEST: virtual FB functions  <<<<<<<<<<<<< */
	printf("draw pcircle on virt FB...\n");
	fbset_color(egi_color_random(color_medium));
	/* draw on VIRT FB */
	draw_pcircle(&vfb, 120, 120, 80, 20);
	/* write on VIRT FB */
	printf("write on virt FB...\n");
        FTsymbol_unicstrings_writeFB(&vfb, egi_appfonts.bold,   /* FBdev, fontface */
                                           36, 36, prophesy,    /* fw,fh, pstr */
                                           240, 5, 5,           /* pixpl, lines, gap */
                                           0, 80,                      /* x0,y0, */
                                           WEGI_COLOR_BLACK, -1, -1);   /* fontcolor, stranscolor,opaque */

	printf("windisplay...\n");
	egi_imgbuf_windisplay(eimg, &gv_fb_dev, -1, 0, 0, 0, 0, 240, 320 );
	//egi_imgbuf_windisplay2(eimg, &gv_fb_dev, 0, 0, 0, 0, 240,320);



	tm_delayms(500);

#endif /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */



#if 0 /* <<<<<<<<<<< TEST: fb_buffer_FBimg() and fb_restore_FBimg() <<<<<<<<<<<<< */

	printf("fb_buffer_FBimg(NB=0)...\n");
	fb_buffer_FBimg(&gv_fb_dev,0);

	tm_delayms(500);

	printf("fb_restore_FBimg(NB=0, clear=TRUE)...\n");
	fb_restore_FBimg(&gv_fb_dev,0,true);


#endif /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

	printf("egi_imgbuf_free()...\n");
	egi_imgbuf_free(eimg);

	printf("release_virt_fbdev()...\n");
	release_virt_fbdev(&vfb);

	printf("release_fbdev()...\n");
	release_fbdev(&gv_fb_dev);

	printf("FTsymbol_release_allfonts()...\n");
	FTsymbol_release_allfonts();

	printf("symbol_release_allpages()...\n");
	symbol_release_allpages();

	printf("egi_quit_log()...\n");
	egi_quit_log();
	printf("---------- END -----------\n");

}while(1);


	return 0;
}
