/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
-------------------------------------------------------------------*/

#ifndef __EGI_IMAGE_H__
#define __EGI_IMAGE_H__

#include "egi_imgbuf.h"
#include "egi_fbdev.h"
#include "egi_color.h"
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>

enum imgframe_type {
	frame_none=100,		/* incoporate with ebox->frame, <100  */
	frame_round_rect,
	frame_circle,
	frame_oval,
	frame_updown_mist,
};

EGI_IMGBUF*	egi_imgbuf_alloc(void); //new(void);
void 		egi_imgbuf_cleardata(EGI_IMGBUF *egi_imgbuf); /* free data inside */
void 		egi_imgbuf_free(EGI_IMGBUF *egi_imgbuf);
int 		egi_imgbuf_init(EGI_IMGBUF *egi_imgbuf, int height, int width);
EGI_IMGBUF*	egi_imgbuf_create( int height, int width, unsigned char alpha, EGI_16BIT_COLOR color );
int 		egi_imgbuf_setframe( EGI_IMGBUF *eimg, enum imgframe_type type,
                         	     int alpha, int pn, const int *param );
EGI_IMGBUF*	egi_imgbuf_newFrameImg( int height, int width,
                	             unsigned char alpha, EGI_16BIT_COLOR color,
                        	     enum imgframe_type type,
                                     int pn, const int *param );
EGI_IMGBUF  *egi_imgbuf_avgsoft(const EGI_IMGBUF *ineimg, int size);
int 		egi_imgbuf_blend_imgbuf(EGI_IMGBUF *eimg, int xb, int yb, EGI_IMGBUF *addimg );
int 		egi_imgbuf_windisplay(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
                	                        int xp, int yp, int xw, int yw, int winw, int winh);
/* no subcolor, no FB filo */
int egi_imgbuf_windisplay2(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
                                        int xp, int yp, int xw, int yw, int winw, int winh);

/* display sub_image in an EGI_IMAGBUF */
int egi_subimg_writeFB(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subnum,
                                                        int subcolor, int x0,   int y0);
/* reset color and alpha for all pixels */
int egi_imgbuf_reset(EGI_IMGBUF *egi_imgbuf, int subnum, int color, int alpha);

/* blend an EGI_IMGBUF with a FT_Bitmap */
int egi_imgbuf_blend_FTbitmap(EGI_IMGBUF* eimg, int xb, int yb, FT_Bitmap *bitmap,
								EGI_16BIT_COLOR subcolor);

#endif
