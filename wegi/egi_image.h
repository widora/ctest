#ifndef __EGI_IMAGE_H__
#define __EGI_IMAGE_H__

#include "egi_color.h"

typedef struct
{
        int height;
        int width;
        EGI_16BIT_COLOR *imgbuf; /* image data, for RGB565 format */
	void *data; /* image data, for other pixel format */
} EGI_IMGBUF;


#endif
