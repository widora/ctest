/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Test image operation for icon file.

Journal:
2021-06-07:
	1. Test: egi_imgbuf_savejpg().


Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <egi_common.h>
#include <unistd.h>
#include <egi_FTsymbol.h>

int main(int argc, char **argv)
{
 /* <<<<<<  EGI general init  >>>>>> */

 /* Start sys tick */
 printf("tm_start_egitick()...\n");
 tm_start_egitick();
#if 0
 /* Start egi log */
 printf("egi_init_log()...\n");
 if(egi_init_log("/mmc/log_scrollinput")!=0) {
        printf("Fail to init egi logger, quit.\n");
        return -1;
 }
 /* Load symbol pages */
 printf("FTsymbol_load_allpages()...\n");
 if(FTsymbol_load_allpages() !=0) /* FT derived sympg_ascii */
        printf("Fail to load FTsym pages! go on anyway...\n");
 printf("symbol_load_allpages()...\n");
 if(symbol_load_allpages() !=0) {
        printf("Fail to load sym pages, quit.\n");
        return -1;
  }
#endif
  /* Load freetype fonts */
  printf("FTsymbol_load_sysfonts()...\n");
  if(FTsymbol_load_sysfonts() !=0) {
        printf("Fail to load FT sysfonts, quit.\n");
        return -1;
  }
#if 0
  printf("FTsymbol_load_appfonts()...\n");
  if(FTsymbol_load_appfonts() !=0) {
        printf("Fail to load FT appfonts, quit.\n");
        return -1;
  }
#endif
  /* Initilize sys FBDEV */
  printf("init_fbdev()...\n");
  if(init_fbdev(&gv_fb_dev))
        return -1;

  /* Start touch read thread */
  printf("Start touchread thread...\n");
  if(egi_start_touchread() !=0)
        return -1;

   /* Set sys FB mode */
   fb_set_directFB(&gv_fb_dev,true);
   fb_position_rotate(&gv_fb_dev, 0);

 /* <<<<<  End of EGI general init  >>>>>> */

#if 1 /* ------------------ TEST: egi_imgbuf_savejpg()  ---------------------*/
	int i;
	EGI_IMGBUF *eimg=egi_imgbuf_readfile(argv[1]);
	EGI_IMGBUF *tmpimg;
	char strtmp[128];

while(1) {
   for(i=0; i<120; i+=5) {
	printf("Compress quality: %d\n", i);
	sprintf(strtmp,"JPEG quality=%d", i);

#if 1	/* Display original JPG */
       // fb_clear_workBuff(&gv_fb_dev, WEGI_COLOR_GRAY3);
        egi_imgbuf_windisplay2( eimg, &gv_fb_dev,             /* imgbuf, fb_dev */
                                0, 0, 0, 0,                   /* xp, yp, xw, yw */
                                eimg->width, eimg->height); /* winw, winh */

        FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_sysfonts.bold, 	  /* FBdev, fontface */
					20, 20, (const UFT8_PCHAR)"Original", /* fw,fh, pstr */
                                        320, 1, 0,                        /* pixpl, lines, gap */
                                        10, 10,                          /* x0,y0, */
                                        WEGI_COLOR_RED, -1, 255, 	/* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      	/* int *cnt, int *lnleft, int* penx, int* peny */

        fb_render(&gv_fb_dev);
	tm_delayms(500);
#endif

	/* Save to JPG */
	egi_imgbuf_savejpg("/tmp/test_savejpg.jpg", eimg, i);

	/* Read JPG and show */
	tmpimg=egi_imgbuf_readfile("/tmp/test_savejpg.jpg");

        fb_clear_workBuff(&gv_fb_dev, WEGI_COLOR_GRAY3);
        egi_imgbuf_windisplay2( tmpimg, &gv_fb_dev,             /* imgbuf, fb_dev */
                                0, 0, 0, 0,                   /* xp, yp, xw, yw */
                                tmpimg->width, tmpimg->height); /* winw, winh */

        FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_sysfonts.bold, 	  /* FBdev, fontface */
					20, 20, (const UFT8_PCHAR)strtmp, /* fw,fh, pstr */
                                        320, 1, 0,                        /* pixpl, lines, gap */
                                        10, 10,                          /* x0,y0, */
                                        WEGI_COLOR_RED, -1, 255, 	/* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      	/* int *cnt, int *lnleft, int* penx, int* peny */

        fb_render(&gv_fb_dev);

        egi_imgbuf_free2(&tmpimg);

	tm_delayms(500);
   }

} /* END while() */

#endif


#if 0 /* ------------------ TEST: ICON file operation  ---------------------*/
	int i,j;
	EGI_IMGBUF *icons_normal=NULL;
	FBDEV vfbdev={0};


        /* Load Noraml Icons */
        //icons_normal=egi_imgbuf_readfile("/mmc/gray_icons_normal.png");
        icons_normal=egi_imgbuf_readfile("/mmc/gray_icons_effect.png");
        if( egi_imgbuf_setSubImgs(icons_normal, 12*5)!=0 ) {
                printf("Fail to setSubImgs for icons_normal!\n");
                exit(EXIT_FAILURE);
        }
	if(icons_normal->alpha)
		printf("PNG file contains alpha data!\n");
	else
		printf("PNG file has NO alpha data!\n");

        /* Set subimgs */
        for( i=0; i<5; i++ )
           for(j=0; j<12; j++) {
                icons_normal->subimgs[i*12+j].x0= 25+j*75.5;            /* 25 margin, 75.5 Hdis */
                icons_normal->subimgs[i*12+j].y0= 145+i*73.5;           /* 73.5 Vdis */
                icons_normal->subimgs[i*12+j].w= 50;
                icons_normal->subimgs[i*12+j].h= 50;
        }

	/* Init virtual FBDEV with icons_normal */
	if( init_virt_fbdev(&vfbdev, icons_normal, NULL) != 0 ) {
		printf("Fail to init_virt_fbdev!\n");
		exit(EXIT_FAILURE);
	}

	/* Draw division blocks on icons_normal image */
	fbset_color2(&vfbdev, WEGI_COLOR_DARKGRAY);
        for( i=0; i<5; i++ )
           for(j=0; j<12; j++) {
		draw_rect(&vfbdev, 25+j*75.5, 145+i*73.5,  25+j*75.5+50-1, 145+i*73.5+50-1);
	}

	/* Save new icon image to file */
	egi_imgbuf_savepng("/mmc/test_img5_icons.png", icons_normal);

#endif


 /* <<<<<  EGI general release 	 >>>>>> */
 printf("FTsymbol_release_allfonts()...\n");
 FTsymbol_release_allfonts(); /* release sysfonts and appfonts */
 printf("symbol_release_allpages()...\n");
 symbol_release_allpages(); /* release all symbol pages*/
 printf("FTsymbol_release_allpage()...\n");
 FTsymbol_release_allpages(); /* release FT derived symbol pages: sympg_ascii */

 printf("fb_filo_flush() and release_fbdev()...\n");
 fb_filo_flush(&gv_fb_dev);
 release_fbdev(&gv_fb_dev);

 //printf("release virtual fbdev...\n");
 //release_virt_fbdev(&vfb);

 printf("egi_end_touchread()...\n");
 egi_end_touchread();
 #if 0
 printf("egi_quit_log()...\n");
 egi_quit_log();
 #endif
 printf("<-------  END  ------>\n");


return 0;
}

