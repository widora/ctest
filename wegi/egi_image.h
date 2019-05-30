#ifndef __EGI_IMAGE_H__
#define __EGI_IMAGE_H__

#include "egi_color.h"

typedef struct
{
        int height;		 /* image height */
        int width;		 /* image width */
        EGI_16BIT_COLOR *imgbuf; /* color data, for RGB565 format */
	void *data; 		 /* color data, for pixel format other than RGB565 */
	unsigned char *alpha;    /* 8bit, alpha channel value, if applicable */
} EGI_IMGBUF;


#endif
