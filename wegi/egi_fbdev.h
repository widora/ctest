/*------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Referring to: http://blog.chinaunix.net/uid-22666248-id-285417.html
 本文的copyright归yuweixian4230@163.com 所有，使用GPL发布，可以自由拷贝，转载。
但转载请保持文档的完整性，注明原作者及原链接，严禁用于任何商业用途。

作者：yuweixian4230@163.com
博客：yuweixian4230.blog.chinaunix.net


Modified and appended by: Midas Zhou
-----------------------------------------------------------------------------*/
#ifndef __EGI_FBDEV_H__
#define __EGI_FBDEV_H__

#include <stdio.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdbool.h>
//#include "egi.h"  /* definition conflict */
#include "egi_filo.h"
#include "egi_imgbuf.h"
//#include "egi_image.h" /* definition conflict */

#define EGI_FBDEV_NAME "/dev/fb0"

#define FBDEV_MAX_BUFFER 3

typedef struct fbdev{

        int 		fbfd; 		/* FB device file descriptor, open "dev/fb0" */

        bool 		virt;           /* 1. TRUE: virtural fbdev, it maps to an EGI_IMGBUF
	                                 *   and fbfd will be ineffective.
					 *   vinfo.xres,vinfo.yres and vinfo.screensize MUST set.
					 *   as .width and .height of the EGI_IMGBUF.
					 *  2. FALSE: maps to true FB device, fbfd is effective.
					 *  3. FB FILO will be ineffective then.
                                	 */

        struct 		fb_var_screeninfo vinfo;
        struct 		fb_fix_screeninfo finfo;
        long int 	screensize;
        unsigned char 	*map_fb;  	/* mmap to FB data */
	EGI_IMGBUF	*virt_fb;	/* virtual FB data as a EGI_IMGBUF
					 * Ownership will NOT be taken from the caller, means FB will
				   	 * never try to free it, whatever.
					 */

	bool		pixcolor_on;	/* default/init as off */
	uint16_t 	pixcolor;	/* pixel color */
	unsigned char	pixalpha;	/* pixel alpha value in use, 0: 100% bkcolor, 255: 100% frontcolor */

	int   		pos_rotate;	/* 0: default X,Y coordinate of FB
					 * 1: clockwise rotation 90 deg: Y  maps to (vinfo.xres-1)-FB.X,
					 *				 X  maps to FB.Y
				         * 2: clockwise rotation 180 deg
					 * 3: clockwise rotation 270 deg
					 */
	int		pos_xres;	/* Resultion for X and Y direction, as per pos_rotate */
	int		pos_yres;

	/* pthread_mutex_t fbmap_lock; */
	EGI_FILO *fb_filo;
	int filo_on;		/* >0, activate FILO push */

	uint16_t *buffer[FBDEV_MAX_BUFFER];  /* FB image data buffer */

}FBDEV;

/* distinguished from PIXEL in egi_bjp.h */
typedef struct fbpixel {
	long int position;
	uint16_t color;
}FBPIX;

/* global variale, Frame buffer device */
extern FBDEV   gv_fb_dev;

/* functions */
int             init_fbdev(FBDEV *dev);
void            release_fbdev(FBDEV *dev);
int 		init_virt_fbdev(FBDEV *fr_dev, EGI_IMGBUF *eimg);
void		release_virt_fbdev(FBDEV *dev);
inline void     fb_filo_on(FBDEV *dev);
inline void     fb_filo_off(FBDEV *dev);
void            fb_filo_flush(FBDEV *dev);
void            fb_filo_dump(FBDEV *dev);
void		fb_position_rotate(FBDEV *dev, unsigned char pos);

#endif
