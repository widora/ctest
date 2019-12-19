/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

     The GIFLIB distribution is Copyright (c) 1997  Eric S. Raymond
                        SPDX-License-Identifier: MIT


Midas-Zhou
-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "egi_gif.h"
#include "egi_common.h"

static void PrintGifError(int ErrorCode);
static void  egi_gif_FreeSavedImages(SavedImage **psimg, int ImageCount);


static void PrintGifError(int ErrorCode)
{
    const char *Err = GifErrorString(ErrorCode);

    if (Err != NULL)
        fprintf(stderr, "GIF-LIB error: %s.\n", Err);
    else
        fprintf(stderr, "GIF-LIB undefined error %d.\n", ErrorCode);
}


/*-----------------------------------------------------------
Read a GIF file and load data to EGI_GIF
@fpath: File path

Return:
	A pointer to EGI_GIF	OK
	NULL			Fail
-----------------------------------------------------------*/
EGI_GIF*  egi_gif_readfile(const char *fpath)
{
    EGI_GIF* egif=NULL;
    GifFileType *GifFile;
    int Error;


    /* calloc EGI_GIF */
    egif=calloc(1, sizeof(EGI_GIF));
    if(egif==NULL) {
	    printf("%s: Fail to calloc EGI_GIF.\n", __func__);
	    return NULL;
    }

    /* Open gif file */
    if ((GifFile = DGifOpenFileName(fpath, &Error)) == NULL) {
	    PrintGifError(Error);
	    free(egif);
	    return NULL;
    }

    /* Slurp reads an entire GIF into core, hanging all its state info off
     *  the GifFileType pointer, it may take a short of time...
     */
    printf("%s: Slurping GIF file and put images to memory...\n", __func__);
    if (DGifSlurp(GifFile) == GIF_ERROR) {
	PrintGifError(GifFile->Error);
    	if (DGifCloseFile(GifFile, &Error) == GIF_ERROR)
        	PrintGifError(Error);
	free(egif);
	return NULL;
    }

    /* check sanity */
    if( GifFile->ImageCount < 1 ) {
    	if (DGifCloseFile(GifFile, &Error) == GIF_ERROR)
        	PrintGifError(Error);
        free(egif);
	return NULL;
    }

    /* Get GIF version*/
    if( strcmp( EGifGetGifVersion(GifFile),"98" ) >= 0 )
	egif->VerGIF89=true;  /* GIF89 */
    else
	egif->VerGIF89=false; /* GIF87 */

    /* Assign EGI_GIF members. Ownership to be transfered later...*/
    egif->SWidth =       GifFile->SWidth;
    egif->SHeight =      GifFile->SHeight;
    egif->SColorResolution =     GifFile->SColorResolution;
    egif->SBackGroundColor =     GifFile->SBackGroundColor;
    egif->AspectByte =   GifFile->AspectByte;
    egif->SColorMap  =   GifFile->SColorMap;
    egif->ImageCount =   GifFile->ImageCount;
    egif->SavedImages=   GifFile->SavedImages;
    egif->ExtensionBlockCount = GifFile->ExtensionBlockCount;
    #if 1
    printf("%s --- GIF Params---\n",__func__);
    printf("		Version: %s", egif->VerGIF89 ? "GIF89":"GIF87");
    printf("		SWidth x SHeight: 	%dx%d\n", egif->SWidth, egif->SHeight);
    printf("   		SColorResolution: 	%d\n",egif->SColorResolution);
    printf("   		SBackGroundColor: 	%d\n", egif->SBackGroundColor);
    printf("   		AspectByte:		%d\n",egif->AspectByte);
    printf("   		ImageCount:       	%d\n",egif->ImageCount);
    #endif

    /* Decouple gif file handle, with respect to DGifCloseFile().
     * Ownership transfered from GifFile to EGI_GIF!
     */
    GifFile->SColorMap      =NULL;
    GifFile->SavedImages    =NULL;
    /* Note: Ownership of GifFile->ExtensionBlocks and GifFile->Image NOT transfered!
     * it will be freed by DGifCloseFile().
     */

    /* Now we can close GIF file, and leave decoupled data to EGI_GIF. */
    if (DGifCloseFile(GifFile, &Error) == GIF_ERROR) {
        PrintGifError(Error);
	// ...carry on ....
    }

    return egif;
}


/*-----------------------------------------------------------
Free array of struct SavedImage.

@sp:	Poiter to first SavedImage of the array.
@ImageCount: Total umber of  SavedImages
-----------------------------------------------------------*/
static void  egi_gif_FreeSavedImages(SavedImage **psimg, int ImageCount)
{
    int i;
    SavedImage* sp;

    if(psimg==NULL || *psimg==NULL || ImageCount < 1 )
	return;

    sp=*psimg;

    /* free array of SavedImage */
    for ( i=0; i<ImageCount; i++ ) {
        if (sp->ImageDesc.ColorMap != NULL) {
            GifFreeMapObject(sp->ImageDesc.ColorMap);
            sp->ImageDesc.ColorMap = NULL;
        }

        if (sp->RasterBits != NULL)
            free((char *)sp->RasterBits);

        GifFreeExtensions(&sp->ExtensionBlockCount, &sp->ExtensionBlocks);

	sp++; /* move to next SavedImage */
    }

    free(*psimg); /* free itself! */
    *psimg=NULL;
}


/*-----------------------------
Free An EGI_GIF struct.
------------------------------*/
void egi_gif_free(EGI_GIF **egif)
{
   if(egif==NULL || *egif==NULL)
	return;

    /* Free EGI_GIF, Note: Same procedure as per DGifCloseFile()  */
    if ((*egif)->SColorMap) {
        GifFreeMapObject((*egif)->SColorMap);
        (*egif)->SColorMap = NULL;
    }
    egi_gif_FreeSavedImages(&(*egif)->SavedImages, (*egif)->ImageCount);

    /* free imgbuf */
    if ((*egif)->Simgbuf != NULL ) {
	egi_imgbuf_free((*egif)->Simgbuf);
	(*egif)->Simgbuf=NULL;
    }

   /* free itself */
   free(*egif);
   *egif=NULL;
}





/*------------------------------------------------------------------------------------
Writing a frame/block of GIF image data to the Simgbuf and then display
the Simgbuf.

@egif:
@SWidth, SHeight:
@Disposal_Mode:  Defined in GIF file:
                                     0. No disposal specified
                                     1. Leave image in place
                                     2. Set area to background color(image)
                                     3. Restore to previous content

@Width, Height:  Width and Heigh for current GIF frame/block image size.
@offx, offy:	 offset of current image block relative to GIF virtual canvas left top point.
@ColorMap:	 Color map for current image block.
@buffer:         Gif block image rasterbits, as palette index.

@trans_color:    Transparent color index
		 <0, disable. or NO_TRANSPARENT_COLOR(=-1)
@user_trans_color:  --- FOR TEST ONLY ---
                 User defined palette index for the transparency
		 <0, disable.
                 When ENABLE, you shall use FB FILO or FB buff page copy! or moving trails
                 will overlapped! BUT, BUT!!! If image Disposal_Mode != 2,then the image
                 will mess up!!!
                 To check image[0] for wanted user_trans_color index!, for following image[]
                 usually contains trans_color only !!!
@bkg_color:	 Back ground color index, defined in gif file.

@ImgAlpha_ON:    User define, If true: apply imgbuf alpha
                 !! Turn off ONLY when the GIF file has no transparent
                 color at all, then it will speed up IMGBUF displaying.
@DirectFB_ON:    -- FOR TEST ONLY --
                 If TRUE: write data to FB directly, without FB buffer
                       --- NOTE ---
                 1. DirectFB_ON mode MUST NOT use FB_FILO !!! for GIF is a kind of
                    overlapping image operation, FB_FILO is just agains it!
                 2  Not for transparent GIF !!
                 3. Turn ON for samll size (maybe)and nontransparent GIF images
                    to speed up writing!!
                 4. For big size image, it may result in tear lines and flash(by FILO OP)
                    on the screen.
                 5. For images with transparent area, it MAY flash with bkgcolor!(by FILO OP)

@Bkg_Transp:     Usually it's FALSE!  trans_color will cause the same effect.
                 If true, make backgroud color transparent. User define.
                 WARN!: The GIF should has separated background color from image content,
                        otherwise the same color index in image will be transparent also!

-------------------------------------------------------------------------------------------*/
static void egi_gif_rasterWriteFB( EGI_GIF *egif, int Disposal_Mode,
				   int Width, int Height, int offx, int offy,
		  		   ColorMapObject *ColorMap, GifByteType *buffer,
				   int trans_color, int user_trans_color, int bkg_color,
				   bool ImgAlpha_ON, bool DirectFB_ON, bool Bkg_Transp)
{
    int i,j;
    int pos=0;
    int spos=0;
    GifColorType *ColorMapEntry;
    int xres=gv_fb_dev.pos_xres;
    int yres=gv_fb_dev.pos_yres;
    EGI_16BIT_COLOR img_bkcolor;
    EGI_IMGBUF *Simgbuf=egif->Simgbuf;
    int SWidth=egif->SWidth;
    int SHeight=egif->SHeight;

    /* check params */

    /* Create Simgbuf */
    if(Simgbuf==NULL) {
	     /* get bkg color, NOT necessary??? */
             ColorMapEntry = &ColorMap->Colors[bkg_color];
             img_bkcolor=COLOR_RGB_TO16BITS( ColorMapEntry->Red,
                                             ColorMapEntry->Green,
                                             ColorMapEntry->Blue);
	    /* create imgbuf, with bkg alpha == 0!!!*/
	    Simgbuf=egi_imgbuf_create(SHeight, SWidth, 0, img_bkcolor); //height, width, alpha, color
	    if(Simgbuf==NULL)
		return;

	    /* NOTE: if applay Simgbuf->alpha, then all initial pixels are invisible */
	    if(!ImgAlpha_ON) {
		    free(Simgbuf->alpha);
		    Simgbuf->alpha=NULL;
	   }
    }

   /* Limit Screen width x heigh, necessary? */
   if(Width==SWidth)offx=0;
   if(Height==SHeight)offy=0;
   //printf("%s: input Height=%d, Width=%d offx=%d offy=%d\n", __func__, Height, Width, offx, offy);

    //printf(" buffer[0]=%d\n", buffer[0]);

    for(i=0; i<Height; i++)
    {
	 /* update block of Simgbuf */
	 for(j=0; j<Width; j++ ) {
	      pos=i*Width+j;
              spos=(offy+i)*SWidth+(offx+j);
	      /* Nontransparent color: set color and set alpha to 255 */
	      if( trans_color < 0 || trans_color != buffer[pos]  )
              {
//		  if(Is_ImgColorMap) /* TODO:  not tested! */
//		         ColorMapEntry = &ColorMap->Colors[buffer[pos]];
//		  else 		   /* Is global color map. */
		         ColorMapEntry = &ColorMap->Colors[buffer[pos]];

                  Simgbuf->imgbuf[spos]=COLOR_RGB_TO16BITS( ColorMapEntry->Red,
							    ColorMapEntry->Green,
							    ColorMapEntry->Blue);
		  /* Only if imgbuf alpha is ON */
		  if(ImgAlpha_ON)
		  	Simgbuf->alpha[spos]=255;
	      }
	      /* Just after above!, If IMGBUF alpha applys, Transprent color: set alpha to 0 */
	      if(ImgAlpha_ON) {
		  /* Make background color transparent */
	          if( Bkg_Transp && ( buffer[pos] == bkg_color) ) {  /* bkg_color meaningful ONLY when global color table exists */
		      Simgbuf->alpha[spos]=0;
	          }
	          if( buffer[pos] == trans_color || buffer[pos] == user_trans_color) {   /* ???? if trans_color == bkg_color */
		      Simgbuf->alpha[spos]=0;
	          }
	      }
	      /*  ELSE : Keep old color and alpha value in Simgbuf->imgbuf */
          }
    }

    //newimg=egi_imgbuf_resize(&Simgbuf, 240, 240);  // EGI_IMGBUF **pimg, width, height

    if( Disposal_Mode==2 && !DirectFB_ON ) /* Set area to background color/image */
    {
	  /* NOTE!!! DirectFB_ON mode MUST NOT use FB_FILO! */
   	  /* Display imgbuf, with FB FILO ON, OR fill background with FB bkg buffer */
	  fb_filo_flush(&gv_fb_dev); /* flush and restore old FB pixel data */
	  fb_filo_on(&gv_fb_dev); /* start collecting old FB pixel data */
    }

    /* display the whole Gif screen/canvas */
    egi_imgbuf_windisplay( Simgbuf, &gv_fb_dev, -1,             /* img, fb, subcolor */
			   SWidth>xres ? (SWidth-xres)/2:0,     /* xp */
			   SHeight>yres ? (SHeight-yres)/2:0,   /* yp */
			   SWidth>xres ? 0:(xres-SWidth)/2,	/* xw */
			   SHeight>yres ? 0:(yres-SHeight)/2,	/* yw */
                           Simgbuf->width, Simgbuf->height    /* winw, winh */
			);

//    display_slogan();

    if( Disposal_Mode==2 && !DirectFB_ON )  /* Set area to background color/image */
      	fb_filo_off(&gv_fb_dev); /* start collecting old FB pixel data */

    /* refresh FB page */
    if(!DirectFB_ON)
    	fb_page_refresh(&gv_fb_dev);

    /* reset trans color index */
//    trans_color=-1;
}

