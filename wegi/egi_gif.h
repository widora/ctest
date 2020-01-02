/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This module is a wrapper of GIFLIB routines and functions.

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
    bool    	VerGIF89;		 /* Version: GIF89 OR GIF87 */
    bool	ImgTransp_ON;		 /* Try image transparency */

    GifWord 	SWidth;         	 /* Size of virtual canvas */
    GifWord 	SHeight;
    GifWord	BWidth;			 /* Size of current block image */
    GifWord	BHeight;
    GifWord	offx;			 /* Current block offset relative to canvas origin */
    GifWord	offy;
    GifWord 	SColorResolution;        /* How many colors can we generate? */
    GifWord 	SBackGroundColor;        /* Background color for virtual canvas */

    GifByteType 	AspectByte;      /* Used to compute pixel aspect ratio */
    ColorMapObject 	*SColorMap;      /* Global colormap, NULL if nonexistent. */
    int			ImageCount;      /* Number of current image (both APIs)
					  * Index ready for display!!
					  */
    int			ImageTotal;	 /* Total number of images */
    GifImageDesc 	Image;           /* Current image (low-level API) */
    SavedImage 		*SavedImages;         /* Image sequence (high-level API) */
    int 		ExtensionBlockCount;  /* Count extensions past last image */
    ExtensionBlock 	*ExtensionBlocks;     /* Extensions past last image */

    EGI_IMGBUF		*Simgbuf;	      /* to hold GIF screen/canvas */

//    int Error;                       	 /* Last error condition reported */

    pthread_t		thread_display;	 /* displaying thread ID */
    bool		thread_running;	 /* True if thread is running */

} EGI_GIF;


/*------------------------------------------
Context for GIF thread.
Parameters: Refert to egi_gif_displayFrame( )
-------------------------------------------*/
typedef struct egi_gif_context
{
        FBDEV   *fbdev;
        EGI_GIF *egif;
        int     nloop;
        bool    DirectFB_ON;
        int     User_DisposalMode;
        int     User_TransColor;
        int     User_BkgColor;
        int     xp;
        int     yp;
        int     xw;
        int     yw;
        int     winw, winh;
} EGI_GIF_CONTEXT;



/*** 	----- static functions -----
static void GifQprintf(char *Format, ...);
static void PrintGifError(int ErrorCode);
static void  	egi_gif_FreeSavedImages(SavedImage **psimg, int ImageCount);

inline static void egi_gif_rasterWriteFB( FBDEV *fbdev, EGI_IMGBUF *Simgbuf, int Disposal_Mode,
                                   int xp, int yp, int xw, int yw, int winw, int winh,
                                   int BWidth, int BHeight, int offx, int offy,
                                   ColorMapObject *ColorMap, GifByteType *buffer,
                                   int trans_color, int User_TransColor, int bkg_color,
                                   bool ImgTransp_ON, bool BkgTransp_ON );

static void *egi_gif_threadDisplay(void *argv);

*/

int  	  egi_gif_readFile(const char *fpath, bool Silent_Mode, bool ImgTransp_ON, int *ImageCount);
EGI_GIF*  egi_gif_slurpFile(const char *fpath, bool ImgTransp_ON);
void	  egi_gif_free(EGI_GIF **egif);
void 	  egi_gif_displayFrame(FBDEV *fbdev, EGI_GIF *egif, int nloop, bool DirectFB_ON,
                                             int User_DisposalMode, int User_TransColor,int User_BkgColor,
                                             int xp, int yp, int xw, int yw, int winw, int winh );

int 	  egi_gif_runDisplayThread( FBDEV *fbdev, EGI_GIF *egif, int nloop, bool DirectFB_ON,
                        	    int User_DisposalMode, int User_TransColor, int User_BkgColor,
                        	    int xp, int yp, int xw, int yw, int winw, int winh );

#endif
