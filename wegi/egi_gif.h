/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

  The GIFLIB distribution is Copyright (c) 1997  Eric S. Raymond
                    SPDX-License-Identifier: MIT


Midas-Zhou
------------------------------------------------------------------*/
#ifndef __EGI_GIF_H__
#define __EGI_GIF_H__

#include "gif_lib.h"
#include "egi_imgbuf.h"
//#include "egi_fbdev.h"
typedef struct fbdev FBDEV;

/*** 		--- NOTE ---
 * For big GIF file, be careful to use EGI_GIF, it needs large mem space!
 */
typedef struct egi_gif {
    bool    	VerGIF89;		     /* Version: GIF89 OR GIF87 */

    GifWord 	SWidth;         	     /* Size of virtual canvas */
    GifWord 	SHeight;
    GifWord 	SColorResolution;        /* How many colors can we generate? */
    GifWord 	SBackGroundColor;        /* Background color for virtual canvas */

    GifByteType 	AspectByte;      /* Used to compute pixel aspect ratio */
    ColorMapObject 	*SColorMap;      /* Global colormap, NULL if nonexistent. */
    int 		ImageCount;      /* Number of current image (both APIs)
					  * Index ready for display!!
					  */
    int			ImageTotal;	 /* Total number of images */
    GifImageDesc 	Image;           /* Current image (low-level API) */
    SavedImage 		*SavedImages;         /* Image sequence (high-level API) */
    int 		ExtensionBlockCount;  /* Count extensions past last image */
    ExtensionBlock 	*ExtensionBlocks;     /* Extensions past last image */

    EGI_IMGBUF		*Simgbuf;	      /* to hold GIF screen/canvas */

    int Error;                       	 /* Last error condition reported */
} EGI_GIF;

/*** 	----- static functions -----
static void PrintGifError(int ErrorCode);
static void  	egi_gif_FreeSavedImages(SavedImage **psimg, int ImageCount);
static void egi_gif_rasterWriteFB( FBDEV *dev, EGI_IMGBUF *Simgbuf, int Disposal_Mode, int x0, int y0,
                                   int BWidth, int BHeight, int offx, int offy,
                                   ColorMapObject *ColorMap, GifByteType *buffer,
                                   int trans_color, int user_trans_color, int bkg_color,
                                   bool DirectFB_ON, bool Bkg_Transp )
*/

EGI_GIF*  egi_gif_readfile(const char *fpath, bool ImgAlpha_ON);
void	egi_gif_free(EGI_GIF **egif);
void	egi_gif_displayFrame(FBDEV *fbdev, EGI_GIF *egif, bool loop_gif, bool DirectFB_ON, int x0, int y0 );

#endif
