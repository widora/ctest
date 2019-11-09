/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Display wbook through a FB with multiple buffer_pages.

Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdio.h>
#include "egi_common.h"
#include "egi_pcm.h"
#include "egi_FTsymbol.h"
#include "egi_utils.h"


static unsigned char *fp;		/* mmap to txt */

static unsigned long page_offset[100];  /* offset table for pages  */
static void * create_pgoffset_table(void *arg);



int main(int argc, char** argv)
{
	int i,j,k;
	int ret;

	struct timeval tm_start;
	struct timeval tm_end;

EGI_IMGBUF* eimg=NULL;

int 		fd;
int 		fsize;
struct stat 	sb;


int nret=0;
int mark;
unsigned long int 	off=0; 		/* in bytes, offset */
unsigned long int 	xres, yres;	/* X,Y pixles of the FB/Screen */
unsigned long int 	line_length; 	/* in bytes */
static EGI_TOUCH_DATA	touch_data;

pthread_t	thread_create_table;

const wchar_t *wbook=L"人心生一念，天地尽皆知。善恶若无报，乾坤必有私。\
那菩萨闻得此言，满心欢喜，对大圣道：“圣经云：‘出其言善。\
则千里之外应之；出其言不善，则千里之外适之。’你既有此心，待我到了东土大唐国寻一个取经的人来，教他救你。你可跟他做个徒弟，秉教伽持，入我佛门。再修正果，如何？”大圣声声道：“愿去！愿去！”菩萨道：“既有善果，我与你起个法名。”大圣道：“我已有名了，叫做孙悟空。”菩萨又喜道：“我前面也有二人归降，正是‘悟’字排行。你今也是‘悟’字，却与他相合，甚好，甚好。这等也不消叮嘱，我去也。”那大圣见性明心归佛教，这菩萨留情在意访神谱。\
他与木吒离了此处，一直东来，不一日就到了长安大唐国。敛雾收云，师徒们变作两个疥癫游憎，入长安城里，竟不觉天晚。行至大市街旁，见一座土地庙祠，二人径进，唬得那土地心慌，鬼兵胆战。知是菩萨，叩头接入。那土地又急跑报与城隍社令及满长安城各庙神抵，都来参见，告道：“菩萨，恕众神接迟之罪。”菩萨道：“汝等不可走漏消息。我奉佛旨，特来此处寻访取经人。借你庙宇，权住几日，待访着真僧即回。”众神各归本处，把个土地赶到城隍庙里暂住，他师徒们隐遁真形。\
毕竟不知寻出那个取经来，且听下回分解。";


        /* <<<<<  EGI general init  >>>>>> */
        printf("tm_start_egitick()...\n");
        tm_start_egitick();		   	/* start sys tick */
#if 0
        printf("egi_init_log()...\n");
        if(egi_init_log("/mmc/log_test") != 0) {	/* start logger */
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

        printf("start touchread thread...\n");
	egi_start_touchread();			/* start touch read thread */

	/* <<<<<  End EGI Init  >>>>> */


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

        /* open wchar book file */
        fd=open("/mmc/xyj_uft8.txt",O_RDONLY);
        if(fd<0) {
                perror("open file");
                return -1;
        }

        /* obtain file stat */
        if( fstat(fd,&sb)<0 ) {
                perror("fstat");
                return -2;
        }
        fsize=sb.st_size;

        /* mmap txt file */
        fp=mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
        if(fp==MAP_FAILED) {
                perror("mmap");
                return -3;
        }

	/* start pthread to create page offset table */
	if(pthread_create(&thread_create_table,NULL, &create_pgoffset_table, NULL) != 0) {
		printf("Fail to create pthread_create_table!\n");
	}
	sleep(10);

   #if 1 ////////////// TEST OFFSET TABLE /////////
	for( i=0; i<100; (i==100-1)?0:++i )
	{
	        fb_shift_buffPage(&gv_fb_dev,0); /* switch to buffer page i */
		fb_clear_backBuff(&gv_fb_dev, WEGI_COLOR_LTYELLOW);
		nret=FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.bold,      /* FBdev, fontface */
                	                          18, 18, fp+page_offset[i],         /* fw,fh, pstr */
                        	                  240-5*2, (320-10*2)/(18+4), 4,        /* pixpl, lines, gap */
                                	          5, 10,                             /* x0,y0, */
                                       	  WEGI_COLOR_BLACK, -1, -1 );  /* fontcolor, transcolor,opaque */
		/* draw a line at bottom of each page */
		fbset_color(WEGI_COLOR_BLACK);
		draw_line(&gv_fb_dev,5,yres-6, xres-5, yres-6);
		fb_page_refresh(&gv_fb_dev);
		sleep(2);
	}
   #endif

	/* Get FB params */
	xres=gv_fb_dev.vinfo.xres;
	yres=gv_fb_dev.vinfo.yres;
	line_length=gv_fb_dev.finfo.line_length;

do {    ////////////////////////////    LOOP TEST   /////////////////////////////////

	off=0;

#if 0 /* --- Write wbook to back buffers --- */
	for(i=0; i<FBDEV_MAX_BUFFER; i++)
	{
	        fb_shift_buffPage(&gv_fb_dev,i); /* switch to buffer page i */
		fb_clear_backBuff(&gv_fb_dev, WEGI_COLOR_LTYELLOW);
		nret=FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.bold,      /* FBdev, fontface */
        	//nret=FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.bold,    /* FBdev, fontface */
                	                          18, 18, fp+off, //wbook+off         /* fw,fh, pstr */
                        	                  240-5*2, (320-10*2)/(18+4), 4,        /* pixpl, lines, gap */
                                	          5, 10,                             /* x0,y0, */
                                       	  WEGI_COLOR_BLACK, -1, -1 );  /* fontcolor, transcolor,opaque */
		/* draw a line at bottom of each page */
		fbset_color(WEGI_COLOR_BLACK);
		draw_line(&gv_fb_dev,5,yres-6, xres-5, yres-6);

		if(nret>0)
			off += nret;
	}

#else /* --- write imgbuf to back buffers --- */
	eimg=egi_imgbuf_readfile("/mmc/list.png");
	if(eimg==NULL) {
        	EGI_PLOG(LOGLV_ERROR, "%s: Fail to read and load file '%s'!", __func__, "/mmc/list.png");
		return -1;
	}
	memcpy(gv_fb_dev.map_bk, eimg->imgbuf, FBDEV_MAX_BUFFER*yres*line_length);
	egi_imgbuf_free(eimg); eimg=NULL;
#endif


	/* refresh FB to show first buff page */
	fb_shift_buffPage(&gv_fb_dev,0);
	fb_page_refresh(&gv_fb_dev);

	/* ---- Scrolling up/down 3 PAGEs by touch sliding ---- */

	i=0; 	/* Line index of all FB buffer pages */
	while(1)
      	{
		if(egi_touch_getdata(&touch_data)==false) {
			tm_delayms(10);
			continue;
		}

		/* switch touch status */
		switch(touch_data.status) {
			case released_hold:
				continue;
				break;
			case pressing:
	                        printf("pressing i=%d\n",i);
                        	mark=i;
                        	continue;
				break;
			case releasing:
				printf("releasing\n");
				continue;
				break;

			case pressed_hold:
				break;

			default:
				continue;
		}

                /* Update line index of FB back buffer */
                i=mark-touch_data.dy;
                printf("i=%d\n",i);

                /* Normalize 'i' to: [0  yres*FBDEV_MAX_BUFFER) */
                if(i<0) {
                        printf("i%%(yres*FBDEV_MAX_BUFF)=%d\n", i%(int)(yres*FBDEV_MAX_BUFFER));
                        i=(i%(int)(yres*FBDEV_MAX_BUFFER))+yres*FBDEV_MAX_BUFFER;
                        printf("renew i=%d\n", i);
                        mark=i+touch_data.dy;
                }
                else if (i > yres*FBDEV_MAX_BUFFER-1) {
                        i=0;    /* loop back to page 0 */
                        mark=touch_data.dy;
                        //continue;
                }

                /*  Refresh FB with offset line, now 'i' limits to [0  yres*FBDEV_MAX_BUFFER) */
                fb_slide_refresh(&gv_fb_dev, i);

		tm_delayms(5);
		//usleep(200000); //500);
      	}


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
	printf9"egi_end_touchread()...\n");
	egi_end_touchread();
        printf("egi_quit_log()...\n");
        egi_quit_log();
        printf("<-------  END  ------>\n");
	#endif

return 0;
}



/*--------------------------------------------
Thread function:
Create page offset table for wbook.

---------------------------------------------*/
static void * create_pgoffset_table(void *arg)
{
	int i;
	int nret=0;
	unsigned long off=0;

	pthread_detach(pthread_self());

   for(i=1; i<100; i++)
   {
	nret=FTsymbol_uft8strings_writeFB(NULL, egi_appfonts.bold,      /* FBdev, fontface */
               	                          18, 18, fp+off,  //wbook+off  /* fw,fh, pstr */
                       	                  240-5*2, (320-10*2)/(18+4), 4,/* pixpl, lines, gap */
                               	          5, 10,                        /* x0, y0, */
                                     	  WEGI_COLOR_BLACK, -1, -1 );  /* fontcolor, transcolor,opaque */
	if(nret<=0)
		break;

	off += nret;
	page_offset[i]=off;
	printf("page offset[%d]=%ld \n", i, page_offset[i]);

   }


	pthread_exit((void *)0);
}
