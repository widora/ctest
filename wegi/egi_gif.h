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

typedef struct egi_gif {
    bool    	VerGIF89;		     /* Version: GIF89 OR GIF87 */

    GifWord 	SWidth;         	     /* Size of virtual canvas */
    GifWord 	SHeight;
    GifWord 	SColorResolution;        /* How many colors can we generate? */
    GifWord 	SBackGroundColor;        /* Background color for virtual canvas */

    GifByteType 	AspectByte;      /* Used to compute pixel aspect ratio */
    ColorMapObject 	*SColorMap;      /* Global colormap, NULL if nonexistent. */
    int 		ImageCount;      /* Number of current image (both APIs) */
    GifImageDesc 	Image;           /* Current image (low-level API) */
    SavedImage 		*SavedImages;         /* Image sequence (high-level API) */
    int 		ExtensionBlockCount;  /* Count extensions past last image */
    ExtensionBlock 	*ExtensionBlocks;     /* Extensions past last image */

    EGI_IMGBUF		*Simgbuf;	      /* to hold GIF screen/canvas */

    int Error;                       	 /* Last error condition reported */
} EGI_GIF;

//static void PrintGifError(int ErrorCode);
//static void  	egi_gif_FreeSavedImages(SavedImage **psimg, int ImageCount);

EGI_GIF*  	egi_gif_readfile(const char *fpath);
void 		egi_gif_free(EGI_GIF **egif);

#endif
