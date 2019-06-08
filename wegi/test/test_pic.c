/*-----------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Test EGI_PIC functions

Midas Zhou
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "egi_header.h"
#include "utils/egi_utils.h"

int main(int argc, char **argv)
{
	int i,j;
	int ret;

        /* EGI general init */
        tm_start_egitick();
        if(egi_init_log("/mmc/log_color") != 0) {
                printf("Fail to init logger,quit.\n");
                return -1;
        }
        if(symbol_load_allpages() !=0 ) {
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
        init_fbdev(&gv_fb_dev);

	/* -------- to search jpg files ------ */
	char *path= argv[1]; //"/mmc/photos";
	char (*fpaths)[EGI_PATH_MAX+EGI_NAME_MAX]=NULL;
//	char **fpaths;
	int count=0;

	fpaths=egi_alloc_search_files(path, ".jpg, .png; .jpeg", &count);
	printf("Totally %d files are found.\n",count);
	for(i=0; i<count; i++)
		printf("%s\n",fpaths[i]);

	/* ------- display jpg files ------- */
	int pich=230;
	int picw=230;
	int poffx=4;
	int poffy=4;
	int symh=26;
	EGI_DATA_PIC *data_pic=NULL;
	EGI_EBOX *pic_box=NULL;

//	EGI_IMGBUF imgbuf={0};
	EGI_IMGBUF *imgbuf;

	EGI_POINT  pxy;
	/* set x0y0 of pic in box */
	EGI_BOX	 box={ {0-230/2,0-130/2}, {240-230/2-1,320-130/2-1} };
	//EGI_BOX  box={ {0-100,0-150},{240-1-100,320-1-150} };
	//EGI_BOX  box={ {0,0},{240-picw-poffx-1,320-pich-poffy-symh-1}};


for(i=0;i<count+1;i++)
{
	if(i==count) { /* reload file paths */
	   	i=0;
		free(fpaths);
		fpaths=egi_alloc_search_files(path, ".jpg, .png; .jpeg", &count);
		printf("--------- i=%d: Totally %d jpg files found -------.\n",i,count);
		continue;
	}

	imgbuf=egi_imgbuf_new();
	/* load jpg file to buf */
	if(egi_imgbuf_loadjpg(fpaths[i], imgbuf) !=0) {
		if(egi_imgbuf_loadpng(fpaths[i], imgbuf) !=0) {
			printf(" load imgbuf fail, try egi_imgbuf_free...\n");
			egi_imgbuf_free(imgbuf);
		   	continue;
		}
	}

	/* allocate data_pic */
        data_pic= egi_picdata_new( poffx, poffy,    	/* int offx, int offy */
 	               		 imgbuf->height, imgbuf->width, //pich, picw, /* heigth,width of displaying window */
                       		 0,0,	   		/* int imgpx, int imgpy */
				 -1,			/* image canvan color, <0 for transparent */
 	       	                 &sympg_testfont  	/* struct symbol_page *font */
	                        );
	/* set title */
	data_pic->title="EGI_PIC";
	/* copy image data and aplha */
	memcpy(data_pic->imgbuf->imgbuf, imgbuf->imgbuf, (imgbuf->height)*(imgbuf->width)*2);
	if(imgbuf->alpha != NULL)
		memcpy(data_pic->imgbuf->alpha, imgbuf->alpha, imgbuf->height*imgbuf->width); /* only if alpha available */

	egi_imgbuf_free(imgbuf);


	/* get a random point */
	egi_randp_inbox(&pxy, &box);
	pic_box=egi_picbox_new( "pic_box", /* char *tag, or NULL to ignore */
       				  data_pic,  /* EGI_DATA_PIC *egi_data */
			          1,	     /* bool movable */
			   	  pxy.x,pxy.y,//10,100,    /*  x0, y0 for host ebox*/
        			  -1,	     /* int frame */
				  -1	//WEGI_COLOR_GRAY /* int prmcolor,applys only if prmcolor>=0  */
	);
	printf("egi_picbox_activate()...\n");
	egi_picbox_activate(pic_box);

	/* scale pic to data_pic */
//	printf("egi_scale_pixbuf()...\n");
//	fb_scale_pixbuf(imgbuf->width, imgbuf->height, data_pic->imgbuf->width, data_pic->imgbuf->height,
//						imgbuf->imgbuf, data_pic->imgbuf->imgbuf); /* imgbuf */
	/* enforce alpha to be 0xff */
//	memset(data_pic->imgbuf->alpha,0xff, (data_pic->imgbuf->width)*(data_pic->imgbuf->height));

	/* release useless imgbuf then */
//	printf("egi_imgbuf_release...\n");
//	egi_imgbuf_release(&imgbuf);
//	egi_imgbuf_free(imgbuf);


 for(j=0; j<5; j++)
 {
	/* get a random point */
	printf("Start egi_randp_inbox()...\n");
	egi_randp_inbox(&pxy, &box);
	printf("egi_randp_inbox: pxy(%d,%d)\n", pxy.x, pxy.y);
	pic_box->x0=pxy.x;
	pic_box->y0=pxy.y;


	/* refresh picbox to show the picture */
	printf("start egi_picbox_refresh()...\n");
	egi_ebox_needrefresh(pic_box);
	printf("egi_picbox_refresh()...\n");
	egi_picbox_refresh(pic_box);
	printf("tm_delayms...\n");
	tm_delayms(500);
  }

	/* sleep to erase the image */
	egi_picbox_sleep(pic_box);
	/* release it */
	pic_box->free(pic_box);
}

	/* free fpaths */
//	for(i=0;i<maxfnum;i++) {
//		free(fpaths[i]);
//	}
	free(fpaths);

	release_fbdev(&gv_fb_dev);
	symbol_free_allpages();
	egi_quit_log();

	return 0;
}
