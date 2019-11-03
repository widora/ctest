/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
-------------------------------------------------------------------*/
#include "egi.h"
#include "egi_fbdev.h"
#include "egi_fbgeom.h"
#include "egi_filo.h"
#include "egi_debug.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>


/* global variale, Frame buffer device */
FBDEV   gv_fb_dev={ .fbfd=-1, }; //__attribute__(( visibility ("hidden") )) ;

/*-------------------------------------
Initiate a FB device.
Return:
        0       OK
        <0      Fails
---------------------------------------*/
int init_fbdev(FBDEV *fb_dev)
{
//        FBDEV *fb_dev=dev;
	int i;

        if(fb_dev->fbfd > 0) {
           printf("Input FBDEV already open!\n");
           return -1;
        }

        fb_dev->fbfd=open(EGI_FBDEV_NAME,O_RDWR|O_CLOEXEC);
        if(fb_dev<0) {
          printf("Open /dev/fb0: %s\n",strerror(errno));
          return -1;
        }
        printf("%s:Framebuffer device opened successfully.\n",__func__);
        ioctl(fb_dev->fbfd,FBIOGET_FSCREENINFO,&(fb_dev->finfo));
        ioctl(fb_dev->fbfd,FBIOGET_VSCREENINFO,&(fb_dev->vinfo));
        fb_dev->screensize=fb_dev->vinfo.xres*fb_dev->vinfo.yres*fb_dev->vinfo.bits_per_pixel/8;

        /* mmap FB */
        fb_dev->map_fb=(unsigned char *)mmap(NULL,fb_dev->screensize,PROT_READ|PROT_WRITE, MAP_SHARED,
                                                                                        fb_dev->fbfd, 0);
        if(fb_dev->map_fb==MAP_FAILED) {
                printf("Fail to mmap FB: %s\n", strerror(errno));
                close(fb_dev->fbfd);
                return -2;
        }

	/* ---- mmap back mem, map_bk ---- */
	#ifdef LETS_NOTE
	fb_dev->map_bk=(unsigned char *)mmap(NULL,fb_dev->screensize, PROT_READ|PROT_WRITE,
									MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if(fb_dev->map_bk==MAP_FAILED) {
                printf("Fail to mmap back mem map_bk for FB: %s\n", strerror(errno));
                munmap(fb_dev->map_fb,fb_dev->screensize);
                close(fb_dev->fbfd);
                return -2;
	}
	#endif

	/* reset virtual FB, as EGI_IMGBUF */
	fb_dev->virt_fb=NULL;

	/* reset pos_rotate */
	fb_dev->pos_rotate=0;
        fb_dev->pos_xres=fb_dev->vinfo.xres;
        fb_dev->pos_yres=fb_dev->vinfo.yres;

        /* reset pixcolor and pixalpha */
	fb_dev->pixcolor_on=false;
        fb_dev->pixcolor=(30<<11)|(10<<5)|10;
        fb_dev->pixalpha=255;

        /* init fb_filo */
        fb_dev->filo_on=0;
        fb_dev->fb_filo=egi_malloc_filo(1<<13, sizeof(FBPIX), FILO_AUTO_DOUBLE);//|FILO_AUTO_HALVE
        if(fb_dev->fb_filo==NULL) {
                printf("%s: Fail to malloc FB FILO!\n",__func__);
                munmap(fb_dev->map_fb,fb_dev->screensize);
                close(fb_dev->fbfd);
                return -3;
        }

        /* assign fb box */
	if(fb_dev==&gv_fb_dev) {
	        gv_fb_box.startxy.x=0;
        	gv_fb_box.startxy.y=0;
	        gv_fb_box.endxy.x=fb_dev->vinfo.xres-1;
        	gv_fb_box.endxy.y=fb_dev->vinfo.yres-1;
	}

	/* clear buffer */
	for(i=0; i<FBDEV_MAX_BUFFER; i++) {
		fb_dev->buffer[i]=NULL;
	}

//      printf("init_dev successfully. fb_dev->map_fb=%p\n",fb_dev->map_fb);
        printf(" \n------- FB Parameters -------\n");
        printf(" bits_per_pixel: %d bits \n",fb_dev->vinfo.bits_per_pixel);
        printf(" line_length: %d bytes\n",fb_dev->finfo.line_length);
        printf(" xres: %d pixels, yres: %d pixels \n", fb_dev->vinfo.xres, fb_dev->vinfo.yres);
        printf(" xoffset: %d,  yoffset: %d \n", fb_dev->vinfo.xoffset, fb_dev->vinfo.yoffset);
        printf(" screensize: %ld bytes\n", fb_dev->screensize);
        printf(" ----------------------------\n\n");

        return 0;
}

/*-------------------------
Release FB and free map
--------------------------*/
void release_fbdev(FBDEV *dev)
{
	int i;

        if(!dev || !dev->map_fb)
                return;

	/* free FILO, reset fb_filo to NULL inside */
        egi_free_filo(dev->fb_filo);

	/* unmap FB and back mem */
        if( munmap(dev->map_fb,dev->screensize) != 0)
		printf("Fail to unmap FB: %s\n", strerror(errno));
        if( munmap(dev->map_bk,dev->screensize) !=0 )
		printf("Fail to unmap back mem for FB: %s\n", strerror(errno));

	/* free buffer */
	for( i=0; i<FBDEV_MAX_BUFFER; i++ )
	{
		if( dev->buffer[i] != NULL )
		{
			free(dev->buffer[i]);
			dev->buffer[i]=NULL;
		}
	}

        close(dev->fbfd);
        dev->fbfd=-1;
}



/*--------------------------------------------------
Initiate a virtual FB device with an EGI_IMGBUF

Return:
        0       OK
        <0      Fails
---------------------------------------------------*/
int init_virt_fbdev(FBDEV *fb_dev, EGI_IMGBUF *eimg)
{
	int i;

	/* check input data */
	if(eimg==NULL || eimg->width<=0 || eimg->height<=0 ) {
		printf("%s: Input EGI_IMGBUF is invalid!\n",__func__);
		return -1;
	}

	/* check alpha
	 * NULL is OK
	 */

	/* set virt */
	fb_dev->virt=true;

	/* disable FB parmas */
	fb_dev->fbfd=-1;
	fb_dev->map_fb=NULL;
	fb_dev->fb_filo=NULL;
	fb_dev->filo_on=0;

	/* reset virtual FB, as EGI_IMGBUF */
	fb_dev->virt_fb=eimg;

        /* reset pixcolor and pixalpha */
	fb_dev->pixcolor_on=false;
        fb_dev->pixcolor=(30<<11)|(10<<5)|10;
        fb_dev->pixalpha=255;

	/* set params for virt FB */
	fb_dev->vinfo.bits_per_pixel=16;
	fb_dev->finfo.line_length=eimg->width*2;
	fb_dev->vinfo.xres=eimg->width;
	fb_dev->vinfo.yres=eimg->height;
	fb_dev->vinfo.xoffset=0;
	fb_dev->vinfo.yoffset=0;
	fb_dev->screensize=eimg->height*eimg->width;

	/* reset pos_rotate */
	fb_dev->pos_rotate=0;
	fb_dev->pos_xres=fb_dev->vinfo.xres;
	fb_dev->pos_yres=fb_dev->vinfo.yres;

	/* clear buffer */
	for(i=0; i<FBDEV_MAX_BUFFER; i++) {
		fb_dev->buffer[i]=NULL;
	}

#if 0
        printf(" \n--- Virtal FB Parameters ---\n");
        printf(" bits_per_pixel: %d bits \n",		fb_dev->vinfo.bits_per_pixel);
        printf(" line_length: %d bytes\n",		fb_dev->finfo.line_length);
        printf(" xres: %d pixels, yres: %d pixels \n", 	fb_dev->vinfo.xres, fb_dev->vinfo.yres);
        printf(" xoffset: %d,  yoffset: %d \n", 	fb_dev->vinfo.xoffset, fb_dev->vinfo.yoffset);
        printf(" screensize: %ld bytes\n", 		fb_dev->screensize);
        printf(" ----------------------------\n\n");
#endif

	return 0;
}


/*---------------------------------
	Release a virtual FB
---------------------------------*/
void release_virt_fbdev(FBDEV *dev)
{
	dev->virt=false;
	dev->virt_fb=NULL;
}

/*--------------------------------
 Refresh FB mem with back memory
--------------------------------*/
void fb_refresh(FBDEV *dev)
{
	if(dev==NULL)
		return;

	if( dev->map_bk==NULL || dev->map_fb==NULL )
		return;

	memcpy(dev->map_fb, dev->map_bk, dev->screensize);

}

/*-------------------------------------------------------------
Put fb->filo_on to 1, as turn on FB FILO.

Note:
1. To activate FB FILO, depends also on FB_writing handle codes.

Midas Zhou
--------------------------------------------------------------*/
inline void fb_filo_on(FBDEV *dev)
{
        if(!dev || !dev->fb_filo)
                return;

        dev->filo_on=1;
}

inline void fb_filo_off(FBDEV *dev)
{
        if(!dev || !dev->fb_filo)
                return;

        dev->filo_on=0;
}


/*----------------------------------------------
Pop out all FBPIXs in the fb filo
Midas Zhou
----------------------------------------------*/
inline void fb_filo_flush(FBDEV *dev)
{
        FBPIX fpix;

        if(!dev || !dev->fb_filo)
                return;

        while( egi_filo_pop(dev->fb_filo, &fpix)==0 )
        {
                /* write back to FB */
                //printf("EGI FILO pop out: pos=%ld, color=%d\n",fpix.position,fpix.color);
		#ifdef LETS_NOTE  /*--- 4 bytes per pixel ---*/
//                *((uint32_t *)(dev->map_fb+fpix.position)) = fpix.argb; //COLOR_16TO24BITS(fpix.color) + (fpix.alpha<<24);
		*((uint32_t *)(dev->map_bk+fpix.position)) = fpix.argb;
		#else		/*--- 2 bytes per pixel ---*/
                *((uint16_t *)(dev->map_fb+fpix.position)) = fpix.color;
		#endif
        }
}

/*----------------------------------------------
Dump and get rid of all FBPIXs in the fb filo,
Do NOT write back to FB.
Midas Zhou
----------------------------------------------*/
void fb_filo_dump(FBDEV *dev)
{
        if(!dev || !dev->fb_filo)
                return;

        while( egi_filo_pop(dev->fb_filo, NULL)==0 ){};
}


/*--------------------------------------------------
Rotate FB displaying position relative to LCD screen.

Landscape displaying: pos=1 or 3
Postrait displaying: pos=0 or 2

---------------------------------------------------*/
void fb_position_rotate(FBDEV *dev, unsigned char pos)
{

        if(dev==NULL || dev->fbfd<0 ) {
		printf("%s: Input FBDEV is invalid!\n");
		return;
	}

        /* Set pos_rotate */
        dev->pos_rotate=(pos & 0x3); /* Restricted to 0,1,2,3 only */

	/* set pos_xres and pos_yres */
	if( (pos & 0x1) == 0 ) {		/* 0,2  Portrait mode */
		/* reset new resolution */
	        dev->pos_xres=dev->vinfo.xres;
        	dev->pos_yres=dev->vinfo.yres;

		/* reset gv_fb_box END point */
		if(dev==&gv_fb_dev) {
			gv_fb_box.endxy.x=dev->vinfo.xres-1;
			gv_fb_box.endxy.y=dev->vinfo.yres-1;
		}
	}
	else {					/* 1,3  Landscape mode */
		/* reset new resolution */
	        dev->pos_xres=dev->vinfo.yres;
        	dev->pos_yres=dev->vinfo.xres;

		/* reset gv_fb_box END point */
		if(dev==&gv_fb_dev) {
			gv_fb_box.endxy.x=dev->vinfo.yres-1;
			gv_fb_box.endxy.y=dev->vinfo.xres-1;
		}
	}
}
