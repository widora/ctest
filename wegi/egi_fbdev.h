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
#include "egi_filo.h"

#define EGI_FBDEV_NAME "/dev/fb0"

typedef struct fbdev{
        int fdfd; /* file descriptor, open "dev/fb0" */
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        long int screensize;
        unsigned char *map_fb;
	/* pthread_mutex_t fbmap_lock; */
	EGI_FILO *fb_filo;
	int filo_on;	/* >0,activate FILO push */
}FBDEV;

/* distinguished from PIXEL in egi_bjp.h */
typedef struct fbpixel {
	long int position;
	uint16_t color;
}FBPIX;


/* global variale, Frame buffer device */
extern FBDEV   gv_fb_dev;

#endif
