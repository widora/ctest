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

#if 1
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


EGI_IMGBUF* eimg=NULL;
char**	fpaths=NULL;	/* File paths */
int	ftotal=0; 	/* File counts */

unsigned long int off;

const wchar_t *wstr1=L"  大觉金仙没垢姿，\n	\
  西方妙相祖菩提。\n	\
  不生不灭三三行，\n	\
  全气全神万万慈。\n	\
  空寂自然随变化，\n	\
  真如本性任为之。\n	\
  与天同寿庄严体，\n	\
  历劫明心大法师。\n	\
  显密圆通真妙诀，\n	\
  惜修生命无他说。\n	\
  都来总是精气神，\n	\
";

const wchar_t *wstr2=L"  谨固牢藏休漏泄。\n	\
  休漏泄，体中藏，\n	\
  汝受吾传道自昌。\n	\
  口诀记来多有益，\n	\
  屏除邪欲得清凉。\n	\
  得清凉，光皎洁，\n	\
  好向丹台赏明月。\n	\
  月藏玉兔日藏乌，\n	\
  自有龟蛇相盘结。\n	\
  相盘结，性命坚，\n	\
  却能火里种金莲。\n	\
  攒簇五行颠倒用，\n	\
  功完随作佛和仙。\n	\
  口诀记来多有益，\n	\
  屏除邪欲得清凉。\n	\
  得清凉，光皎洁，\n	\
  好向丹台赏明月。\n	\
  月藏玉兔日藏乌，\n	\
  自有龟蛇相盘结。\n	\
  相盘结，性命坚，\n	\
  却能火里种金莲。\n	\
  攒簇五行颠倒用，\n	\
  功完随作佛和仙。\
";

#if 0
        if(argc<2) {
                printf("Usage: %s file\n",argv[0]);
                exit(-1);
        }

        /* 1. Load pic to imgbuf */
	eimg=egi_imgbuf_readfile(argv[1]);
	if(eimg==NULL) {
        	EGI_PLOG(LOGLV_ERROR, "%s: Fail to read and load file '%s'!", __func__, argv[1]);
		return -1;
	}
#endif

do {    ////////////////////////////   1.  LOOP TEST   /////////////////////////////////

	fb_shift_buffPage(&gv_fb_dev,0);
	fb_clear_backBuff(&gv_fb_dev, WEGI_COLOR_GRAY3);
	fb_shift_buffPage(&gv_fb_dev,1);
	fb_clear_backBuff(&gv_fb_dev, WEGI_COLOR_BLACK);

        /* words */
	fb_shift_buffPage(&gv_fb_dev,0);
        FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.bold,         /* FBdev, fontface */
                                          24, 24, wstr1,  		    /* fw,fh, pstr */
                                          240, 320/(24+6), 6,             /* pixpl, lines, gap */
                                          0, 10,                      	    /* x0,y0, */
                                          WEGI_COLOR_BLACK, -1, -1 );   /* fontcolor, transcolor,opaque */

	fb_shift_buffPage(&gv_fb_dev,1);
        FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.bold,         /* FBdev, fontface */
                                          24, 24, wstr2,  		    /* fw,fh, pstr */
                                          240, 320/(24+6), 6,             /* pixpl, lines, gap */
                                          0, 10,                      	    /* x0,y0, */
                                          WEGI_COLOR_WHITE, -1, -1 );   /* fontcolor, transcolor,opaque */



      while(1) {
	for(i=0; i<320; i++) {
		if( i>320 && i<320*2) {
			off=i%320;
		}
		else if (i>320*2-1) {
			i=0;
		}
		memcpy(gv_fb_dev.map_fb, gv_fb_dev.map_buff+240*2*i, gv_fb_dev.screensize);
		usleep(5000);
		i+=1;
	}
      }


	/* Refresh FB by memcpying back buffer to FB */
	fb_shift_buffPage(&gv_fb_dev,0);
	fb_refresh(&gv_fb_dev);
	sleep(2);
	usleep(55000);
	//tm_delayms(100);

	fb_shift_buffPage(&gv_fb_dev,1);
	fb_refresh(&gv_fb_dev);
	sleep(2);
	usleep(55000);
	//tm_delayms(100);



} while(1); ///////////////////////////   END LOOP TEST   ///////////////////////////////


#if 0	/* Free eimg */
	egi_imgbuf_free(eimg);
	eimg=NULL;
#endif

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


