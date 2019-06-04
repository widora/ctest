#ifndef __EGI_IMAGE_H__
#define __EGI_IMAGE_H__

#include "egi_fbdev.h"
#include "egi_color.h"


typedef	struct {
		int x0;		/* subimage left top starting point */
		int y0;
		int w;		/* size of subimage */
		int h;
}EGI_IMGBOX;

typedef struct
{
        int height;		 /* image height */
        int width;		 /* image width */
        EGI_16BIT_COLOR *imgbuf; /* color data, for RGB565 format */
	EGI_IMGBOX *subimgs;	 /* sub_image boxes */
	int subtotal;		 /* total number of subimgs!!! */
	void *data; 		 /* color data, for pixel format other than RGB565 */
	unsigned char *alpha;    /* 8bit, alpha channel value, if applicable */
} EGI_IMGBUF;


int egi_imgbuf_windisplay(const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
                                        int xp, int yp, int xw, int yw, int winw, int winh);

/* no subcolor, no FB filo */
int egi_imgbuf_windisplay2(const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
                                        int xp, int yp, int xw, int yw, int winw, int winh);

/* display sub_image in a EGI_IMAGBUF */
int egi_subimg_writeFB(const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subnum,
                                                        int subcolor, int x0,   int y0);


#endif
