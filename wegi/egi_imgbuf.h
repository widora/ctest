#ifndef __EGI_IMGBUF_H__
#define __EGI_IMGBUF_H__

#include "egi_color.h"
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>


typedef	struct {
		int x0;		/* subimage left top starting point */
		int y0;
		int w;		/* size of subimage */
		int h;
}EGI_IMGBOX;

typedef struct
{
	pthread_mutex_t	img_mutex;	/* mutex lock for imgbuf */
        int height;		 	/* image height */
        int width;		 	/* image width */
        EGI_16BIT_COLOR *imgbuf; 	/* color data, for RGB565 format */
	EGI_IMGBOX *subimgs;	 	/* sub_image boxes */
	int submax;		 	/* max index of subimg, from 0 */
	void *data; 		 	/* color data, for pixel format other than RGB565 */
	unsigned char *alpha;    	/* 8bit, alpha channel value, if applicable: alpha=0,100%backcolor, alpha=1, 100% frontcolor */
} EGI_IMGBUF;


#endif
