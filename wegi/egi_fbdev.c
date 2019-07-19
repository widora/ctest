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
Return:
        0       OK
        <0      Fails
---------------------------------------*/
int init_fbdev(FBDEV *fr_dev)
{
//        FBDEV *fr_dev=dev;
	int i;

        if(fr_dev->fbfd>0) {
           printf("Input FBDEV already open!\n");
           return -1;
        }

        fr_dev->fbfd=open(EGI_FBDEV_NAME,O_RDWR|O_CLOEXEC);
        if(fr_dev<0) {
          printf("Open /dev/fb0: %s\n",strerror(errno));
          return -1;
        }
        printf("Framebuffer device was opended successfully.\n");
        ioctl(fr_dev->fbfd,FBIOGET_FSCREENINFO,&(fr_dev->finfo));
        ioctl(fr_dev->fbfd,FBIOGET_VSCREENINFO,&(fr_dev->vinfo));
        fr_dev->screensize=fr_dev->vinfo.xres*fr_dev->vinfo.yres*fr_dev->vinfo.bits_per_pixel/8;

        /* mmap FB */
        fr_dev->map_fb=(unsigned char *)mmap(NULL,fr_dev->screensize,PROT_READ|PROT_WRITE,MAP_SHARED,
                                                                                        fr_dev->fbfd,0);
        if(fr_dev->map_fb==MAP_FAILED) {
                printf("Fail to mmap FB!\n");
                close(fr_dev->fbfd);
                return -2;
        }

	/* reset pos_rotate */
	fr_dev->pos_rotate=0;

        /* init fb_filo */
        fr_dev->filo_on=0;
        fr_dev->fb_filo=egi_malloc_filo(1<<13, sizeof(FBPIX), FILO_AUTO_DOUBLE);//|FILO_AUTO_HALVE
        if(fr_dev->fb_filo==NULL) {
                printf("Fail to malloc FB FILO!\n");
                munmap(fr_dev->map_fb,fr_dev->screensize);
                close(fr_dev->fbfd);
                return -3;
        }
        /* assign fb box */
        gv_fb_box.startxy.x=0;
        gv_fb_box.startxy.y=0;
        gv_fb_box.endxy.x=fr_dev->vinfo.xres-1;
        gv_fb_box.endxy.y=fr_dev->vinfo.yres-1;

	/* clear buffer */
	for(i=0; i<FBDEV_MAX_BUFFER; i++) {
		fr_dev->buffer[i]=NULL;
	}

//      printf("init_dev successfully. fr_dev->map_fb=%p\n",fr_dev->map_fb);
        printf(" \n------- FB Parameters -------\n");
        printf(" bits_per_pixel: %d bits \n",fr_dev->vinfo.bits_per_pixel);
        printf(" line_length: %d bytes\n",fr_dev->finfo.line_length);
        printf(" xres: %d pixels, yres: %d pixels \n", fr_dev->vinfo.xres, fr_dev->vinfo.yres);
        printf(" xoffset: %d,  yoffset: %d \n", fr_dev->vinfo.xoffset, fr_dev->vinfo.yoffset);
        printf(" screensize: %ld bytes\n", fr_dev->screensize);
        printf(" ----------------------------\n\n");

        return 0;
}

/*-------------------------
Release FB and free map
--------------------------*/
void release_fbdev(FBDEV *dev)
{
        if(!dev || !dev->map_fb)
                return;

        egi_free_filo(dev->fb_filo);

        munmap(dev->map_fb,dev->screensize);

        close(dev->fbfd);
        dev->fbfd=-1;
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
void fb_filo_flush(FBDEV *dev)
{
        FBPIX fpix;

        if(!dev || !dev->fb_filo)
                return;

        while( egi_filo_pop(dev->fb_filo, &fpix)==0 )
        {
                /* write back to FB */
                //printf("EGI FILO pop out: pos=%ld, color=%d\n",fpix.position,fpix.color);
                *((uint16_t *)(dev->map_fb+fpix.position)) = fpix.color;
        }
}

