/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This module is a wrapper a GIFLIB routines and functions.

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
static void egi_gif_FreeSavedImages(SavedImage **psimg, int ImageCount);
static void PrintGifError(int ErrorCode)
{
    const char *Err = GifErrorString(ErrorCode);

    if (Err != NULL)
        printf("GIF-LIB error: %s.\n", Err);
    else
        printf("GIF-LIB undefined error %d.\n", ErrorCode);
}


/*------------------------------------------------------------------------------
Read a GIF file and load data to EGI_GIF

@fpath: File path

@ImgAlpha_ON:    (User define)
		 Usually to be set as TRUE.
		 Suggest to turn OFF only when no transparency setting in the GIF file,
		 to show SBackGroundColor.

		 NOTE: Transparency_on dosen't means the GIF image itself is transparent!
		   it refers to gcb.TransparentColor for pixles in each sequence block images!
		   gcb.TransparentColor refers to previous block image ONLY, NOT to the
		   backgroup of the whole GIF image!
		   A transparent GIF is applied only if it is so designed, and its SBackGroundColor
		   is usually also transparent, so the background of the GIF image is visible there.

 		 TRUE:  Apply imgbuf alpha.
			Then alphas of transparent pixels in each sequence block images
			will be set to 0, and draw_dot() will NOT be called for them,
			so it will speed up displaying!

		 FALSE: SBackGroundColor will take effect.
                        Turn OFF only when the GIF file has no transparent color
                        index at all, draw_dot() will be called to draw every
		        dot in each block image.

Return:
	A pointer to EGI_GIF	OK
	NULL			Fail
--------------------------------------------------------------------------------*/
EGI_GIF*  egi_gif_readfile(const char *fpath, bool ImgAlpha_ON)
{
    EGI_GIF* egif=NULL;
    GifFileType *GifFile;
    int Error;

    GifColorType *ColorMapEntry;
    EGI_16BIT_COLOR img_bkcolor;

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

    /* TODO: Big size warning */


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

    /* xxx */
    egif->ImgTransp_ON=ImgAlpha_ON;

    /* Assign EGI_GIF members. Ownership to be transfered later...*/
    egif->SWidth =       GifFile->SWidth;
    egif->SHeight =      GifFile->SHeight;
    egif->SColorResolution =     GifFile->SColorResolution;
    egif->SBackGroundColor =     GifFile->SBackGroundColor;
    egif->AspectByte =   GifFile->AspectByte;
    egif->SColorMap  =   GifFile->SColorMap;
    egif->ImageCount =   0;
    egif->ImageTotal =  GifFile->ImageCount; 	/* after slurp, GifFile->ImageCount is total number */
    egif->SavedImages=   GifFile->SavedImages;
    egif->ExtensionBlockCount = GifFile->ExtensionBlockCount;

    #if 1
    printf("%s --- GIF Params---\n",__func__);
    printf("		Version: %s", egif->VerGIF89 ? "GIF89":"GIF87");
    printf("		SWidth x SHeight: 	%dx%d\n", egif->SWidth, egif->SHeight);
    printf("		SColorMap:		%s\n", egif->SColorMap==NULL ? "No":"Yes" );
    printf("   		SColorResolution: 	%d\n", egif->SColorResolution);
    printf("   		SBackGroundColor: 	%d\n", egif->SBackGroundColor);
    printf("   		AspectByte:		%d\n", egif->AspectByte);
    printf("   		ImageTotal:       	%d\n", egif->ImageTotal);
    #endif

    /* --- initiate Simgbuf --- */

    /* get bkg color, ---- NOT necessary??? ---- */
    ColorMapEntry = &(egif->SColorMap->Colors[egif->SBackGroundColor]);
    img_bkcolor=COLOR_RGB_TO16BITS( ColorMapEntry->Red,
                                    ColorMapEntry->Green,
                                    ColorMapEntry->Blue );

    /* create imgbuf, with bkg alpha == 0!!!*/
//    egif->Simgbuf=egi_imgbuf_create(egif->SHeight, egif->SWidth, 0, img_bkcolor); //height, width, alpha, color
    egif->Simgbuf=egi_imgbuf_create(egif->SHeight, egif->SWidth,
					egif->ImgTransp_ON==true ? 0:255, img_bkcolor); //height, width, alpha, color
    if(egif->Simgbuf==NULL) {
	printf("%s: Fail to create Simgbuf!\n", __func__);
	/* close GIF file */
	if (DGifCloseFile(GifFile, &Error) == GIF_ERROR) {
       		PrintGifError(Error);
  	}

        free(egif);
        return NULL;
    }

    /* NOTE: if applay Simgbuf->alpha, then all initial pixels are invisible with initial alpha are 0s */
//    if(!ImgAlpha_ON) {
//    	free(egif->Simgbuf->alpha);
//	egif->Simgbuf->alpha=NULL;
//    }

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
static void  egi_gif_FreeSavedImages(SavedImage **psimg, int ImageTotal)
{
    int i;
    SavedImage* sp;

    if(psimg==NULL || *psimg==NULL || ImageTotal < 1 )
	return;

    sp=*psimg;

    /* free array of SavedImage */
    for ( i=0; i<ImageTotal; i++ ) {
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
    egi_gif_FreeSavedImages(&(*egif)->SavedImages, (*egif)->ImageTotal);

    /* free imgbuf */
    if ((*egif)->Simgbuf != NULL ) {
	egi_imgbuf_free((*egif)->Simgbuf);
	(*egif)->Simgbuf=NULL;
    }

   /* free itself */
   free(*egif);
   *egif=NULL;
}



/*---------------------------------------------------------------------------------------
Update Simgbuf with raster data of a sequence GIF frame and its colormap. If fbdev is
not NULL, then display the Simgbuf.

NOTE:
1. The sequence image raster data will first write to Simgbuf, which functions as GIF canvas,
   then write to FB/(FB buffer) to display the whole canvas content.

2. For Disposal_Mode 2 && DirectFB_OFF:
   !!! XXX it will uses FB FILO to restore the original screen image before displaying
   each GIF sequence image. So if Disposal_Mode changes it will mess up then  XXX !!!
   So use FB buff memcpy to restore image!


@fbdev:		FBDEV.
		If NULL, update Simgbuf, but do NOT write to FB.
@Simgbuf:	An EGI_IMGBUF to hold all pixel data of a GIF screen/canvas.
		Size SWidth x SHeight.
		Hooked to an EGI_GIF struct.

@Disposal_Mode:  How to dispose current block image ( Defined in GIF extension control blocks )
                                     0. No disposal specified
                                     1. Leave image in place
                                     2. Set area to background color/(OR background image)
                                     3. Restore to previous content

@xp,yp:		The origin of GIF canvas relative to FB/LCD coordinate system.
@xw,yw:         Displaying window origin, relate to the LCD coord system.
@winw,winh:     width and height(row/column for fb) of the displaying window.
                !!! Note: You'd better set winw,winh not exceeds acutual LCD size, or it will
                waste time calling draw_dot() for pixels outsie FB zone. --- OK

@BWidth, BHeight:  Width and Heigh for current GIF frame/block image size.  <= SWidth,SHeigh.
@offx, offy:	 offset of current image block relative to GIF virtual canvas left top point.
@ColorMap:	 Color map for current image block.
@buffer:         Gif block image rasterbits, as palette index.

@trans_color:    Transparent color index ( Defined in GIF extension control blocks )
		 <0, disable. or NO_TRANSPARENT_COLOR(=-1)

@user_trans_color:   (User define)  --- FOR TEST ONLY ---
                 Palette index for the transparency
		 <0, disable.
                 When ENABLE, you shall use FB FILO or FB buff page copy! or moving trails
                 will overlapped! BUT, BUT!!! If image Disposal_Mode != 2,then the image
                 will mess up!!!
                 To check image[0] for wanted user_trans_color index!, for following image[]
                 usually contains trans_color only !!!

@bkg_color:	 Back ground color index, ( Defined in GIF file header )
		 <0, disable.

@DirectFB_ON:    (User define)  	-- FOR TEST ONLY --
		 Normally to be FALSE, or for nontransparent(transparency off) GIF.
		 This will NOT affect Simgbuf.
                 If TRUE: write data to FB directly, without FB working buffer.
                       --- NOTE ---
                 1. DirectFB_ON mode MUST NOT use FB_FILO !!! for GIF is a kind of
                    overlapping image operation, FB_FILO is just agains it!
                 2  !!! Not for transparent GIF !!!
                 3. Turn ON for samll size (maybe)and nontransparent GIF images
                    to speed up writing!!
                 4. For big size image, it may result in tear lines and flash(by FILO OP)
                    on the screen.
                 5. For images with transparent area, it MAY flash with bkgcolor!(by FILO OP)

@BkgTransp_ON:     (User define) Usually it's FALSE!  trans_color will cause the same effect.
                 If TRUE: make backgroud color transparent.
                 WARN!: The GIF should has separated background color from image content,
                        otherwise the same color index in image will be transparent also!

-------------------------------------------------------------------------------------------*/
inline static void egi_gif_rasterWriteFB( FBDEV *fbdev, EGI_IMGBUF *Simgbuf, int Disposal_Mode,
				   int xp, int yp, int xw, int yw, int winw, int winh,
				   int BWidth, int BHeight, int offx, int offy,
		  		   ColorMapObject *ColorMap, GifByteType *buffer,
				   int trans_color, int user_trans_color, int bkg_color,
				   bool DirectFB_ON, bool ImgTransp_ON, bool BkgTransp_ON )
{
    /* check params */
    if( Simgbuf==NULL || ColorMap==NULL || buffer==NULL )
		return;

    int i,j;
    int pos=0;
    int spos=0;
    GifColorType *ColorMapEntry;
    int SWidth=Simgbuf->width;		/* Screen/canvas size */
    int SHeight=Simgbuf->height;
    EGI_16BIT_COLOR bkcolor;		/* 16bit color, NOT index */

   /* Limit Screen width x heigh, necessary? */
   if(BWidth==SWidth)offx=0;
   if(BHeight==SHeight)offy=0;
   //printf("%s: input Height=%d, Width=%d offx=%d offy=%d\n", __func__, Height, Width, offx, offy);

#if 1
    /*** 	--- Disposal_Mode ---
     * 		0. No disposal specified
     *		1. Leave image in place
     */
    if( Disposal_Mode==2 && !DirectFB_ON ) /* 2. Set area to background color/image */
    {
	/* TODO: to be testetd, how about Disposal_Mode changes ??? */
	/* NOTE!!! DirectFB_ON mode MUST NOT use FB_FILO! */

        #if 0 /* FAILS!!! When Disposal_mode changes to other than 2, this will fail then! */
   	/* Display imgbuf, with FB FILO ON, OR fill background with FB bkg buffer */
	fb_filo_flush(fbdev); /* flush and restore old FB pixel data */
	fb_filo_on(fbdev); /* start collecting old FB pixel data */

	#else /* OK */
	/* clear all alphas */
	ColorMapEntry = &ColorMap->Colors[bkg_color];
	bkcolor=COLOR_RGB_TO16BITS( ColorMapEntry->Red,
                                    ColorMapEntry->Green,
               	                    ColorMapEntry->Blue );

	if(ImgTransp_ON)	/* Make canvan transparent! */
		egi_imgbuf_resetColorAlpha(Simgbuf, -1, 0);
	else {			/* Fill with SBackGroundColor */
		egi_imgbuf_resetColorAlpha(Simgbuf, bkcolor, 255);
	}

//      memcpy(fbdev->map_bk, fbdev->map_buff+fbdev->screensize, fbdev->screensize);
	#endif
    }

    /* TODO: Disposal_Mode 3. Restore to previous image
     * "The mode Restore to Previous is intended to be used in small sections of the graphic
     *	 ... this mode should be used sparingly" --- < Graphics Interchange Format Version 89a >
     */
    else if( Disposal_Mode==3)
	printf("%s: !!! Skip Disposal_Mode 3 !!!\n",__func__);

#endif

    /* update Simgbuf */
    for(i=0; i<BHeight; i++)
    {
	 /* update block of Simgbuf */
	 for(j=0; j<BWidth; j++ ) {
	      pos=i*BWidth+j;
              spos=(offy+i)*SWidth+(offx+j);
	      /* Nontransparent color: set color and set alpha to 255 */
	      if( trans_color < 0 || trans_color != buffer[pos]  )
              {
//		  if(Is_ImgColorMap) /* TODO:  not tested! */
///		         ColorMapEntry = &ColorMap->Colors[buffer[pos]];
//		  else 		   /* Is global color map. */
		         ColorMapEntry = &ColorMap->Colors[buffer[pos]];

                  Simgbuf->imgbuf[spos]=COLOR_RGB_TO16BITS( ColorMapEntry->Red,
							    ColorMapEntry->Green,
							    ColorMapEntry->Blue);
		  /* Only if imgbuf alpha is ON */
//		  if(Simgbuf->alpha)
		  	Simgbuf->alpha[spos]=255;
	      }

	      /* Just after above!, If IMGBUF alpha applys, Transprent color: set alpha to 0 */
	      //if(Simgbuf->alpha) {
	      if(ImgTransp_ON) {
		  /* Make background color transparent */
	          if( BkgTransp_ON && ( buffer[pos] == bkg_color) ) {  /* bkg_color meaningful ONLY when global color table exists */
		      Simgbuf->alpha[spos]=0;
	          }
	          if( buffer[pos] == trans_color || buffer[pos] == user_trans_color) {   /* ???? if trans_color == bkg_color */
		      Simgbuf->alpha[spos]=0;
	          }
	      }
	      /*  ELSE : Keep old color and alpha value in Simgbuf->imgbuf */
          }
    }

    /* --- Return if FBDEV is NULL --- */
    if(fbdev == NULL)
	return;


    /* Set area to background color/image */
    if( Disposal_Mode==2 && !DirectFB_ON )
     	memcpy(fbdev->map_bk, fbdev->map_buff+fbdev->screensize, fbdev->screensize);

    /* display the whole Gif screen/canvas */
    egi_imgbuf_windisplay( Simgbuf, fbdev, -1,              /* img, fb, subcolor */
			   xp, yp,				/* xp, yp */
			   xw, yw,				/* xw, yw */
                     	   winw, winh   /* winw, winh */
			);

    if( Disposal_Mode==2 && !DirectFB_ON )  /* Set area to background color/image */
	 fb_filo_off(fbdev); /* start collecting old FB pixel data */

    /* refresh FB page */
    if(!DirectFB_ON)
    	fb_page_refresh(fbdev);

}


/*---------------------------------------------------------------------------------------------------
Display current frame of GIF, then increase count of EGI_GIF, If dev==NULL, then update eigf->Simgbuf
only!

Note: If dev is NOT NULL, it will affect whole FB data whith memcpy and page fresh operation!


@dev:	     	 FB Device.
		 If NULL, update egif->Simgbuf, but will DO NOT write to FB.
@egif:	     	 The subjective EGI_GIF.
@nloop:		 loop time for each function call:
                 <0 : loop displaying forever
		 0  : display one frame only, and then ImageCount++.
		 >0 : nloop times
//@bool_loop:  	 If true, loop back to first frame if finish one round, by reset egif->ImageCount
		 to 0.
@x0,y0:	  	 The position of GIF canvas relative to FB/LCD coordinate system.
@DirectFB_ON:    (User define) -- FOR TEST ONLY --
                 If TRUE: write data to FB directly, without FB buffer.
		 See NOTE of egi_gif_rasterWriteFB() for more details

----------------------------------------------------------------------------------------------------*/
void egi_gif_displayFrame(FBDEV *fbdev, EGI_GIF *egif, int nloop, bool DirectFB_ON,
				   		 int xp, int yp, int xw, int yw, int winw, int winh )

{
    int j,k;
    int BWidth=0, BHeight=0;  	/* image block size */
    int offx, offy;		/* image block offset relative to gif canvas */
    int DelayMs=0;            	/* delay time in ms */

    SavedImage *ImageData;      /*  savedimages EGI_GIF */
    ExtensionBlock  *ExtBlock;  /*  extension block in savedimage */
    ColorMapObject *ColorMap;   /*  Color map */
    GraphicsControlBlock gcb;   /* extension control block */

    int  Disposal_Mode=0;       /*** Defined in GIF extension control block:
                                   0. No disposal specified
                                   1. Leave image in place
                                   2. Set area to background color(image)
                                   3. Restore to previous content
				*/

    int trans_color=NO_TRANSPARENT_COLOR;
    GifByteType *buffer; /* For METHOD_2 CODE BLOCK */
    bool ImgTransp_ON=egif->ImgTransp_ON;

    /* sanity check */
    if( egif==NULL || egif->SavedImages==NULL ) {
	printf("%s: input EGI_GIF(data) is NULL.\n", __func__);
	return;
    }
    if( egif->ImageCount > egif->ImageTotal-1)
		egif->ImageCount=0;

 /* Do nloop times, or just one frame if nloop==0 */
 k=0;
 do {
     /* Get saved image data sequence */
     ImageData=&egif->SavedImages[egif->ImageCount];
     buffer=ImageData->RasterBits;

     /* Get image colorMap */
     if(ImageData->ImageDesc.ColorMap) {
//      Is_ImgColorMap=true;
        ColorMap=ImageData->ImageDesc.ColorMap;
	printf("  --->  Local colormap: Colorcount=%d   <--\n", ColorMap->ColorCount);
     }
     else {  /* Apply global colormap */
//	Is_ImgColorMap=false;
	ColorMap=egif->SColorMap;
     }

     #if 1	/* For TEST */
     printf("\n Get ImageData %d/%d ... \n", 	egif->ImageCount, egif->ImageTotal-1);
     printf("ExtensionBlockCount=%d\n", 	ImageData->ExtensionBlockCount);
     printf("ColorMap.SortFlag:	%s\n", 		ColorMap->SortFlag?"YES":"NO");
     printf("ColorMap.BitsPerPixel:	%d\n", 	ColorMap->BitsPerPixel);
     #endif

     /* get Block image offset and size */
     offx=ImageData->ImageDesc.Left;
     offy=ImageData->ImageDesc.Top;
     BWidth=ImageData->ImageDesc.Width;
     BHeight=ImageData->ImageDesc.Height;
     printf("Block: offset(%d,%d) size(%dx%d) \n", offx, offy, BWidth, BHeight);

     /* Get extension block */
     if( ImageData->ExtensionBlockCount > 0 )
     {
	/* Search for control block */
	for( j=0; j< ImageData->ExtensionBlockCount; j++ )
	{
	    ExtBlock=&ImageData->ExtensionBlocks[j];

	    switch(ExtBlock->Function)
	    {

	   /***   	       ---  Extension Block Classification ---
	    *
	    * 	0x00 - 0x7F(0-127):     the Graphic Rendering Blocks, excluding the Trailer(0x3B,59)
	    *	0x80 - 0xF9(128-249): 	the Control Blocks
	    *   0xFA - 0xFF:		the Special Purpose Blocks
	    *
	    *		      ( FUNC CODE defined in GIFLIB )
	    *	CONTINUE_EXT_FUNC_CODE    	0x00  continuation subblock
	    *   COMMENT_EXT_FUNC_CODE     	0xfe  comment
	    *   GRAPHICS_EXT_FUNC_CODE    	0xf9  graphics control (GIF89)
	    *	PLAINTEXT_EXT_FUNC_CODE   	0x01  plaintext
	    *	APPLICATION_EXT_FUNC_CODE 	0xff  application block (GIF89)
	    */

		case GRAPHICS_EXT_FUNC_CODE:
	       		if (DGifExtensionToGCB(ExtBlock->ByteCount, ExtBlock->Bytes, &gcb) == GIF_ERROR)
			{
        	                printf("%s: DGifExtensionToGCB() Fails!\n",__func__);
	        	   	continue;
			 }
        	        /* ----- for test ---- */
	                #if 1
        	        printf("Transparency on: %s\n",
           				gcb.TransparentColor != -1 ? "yes" : "no");
	                printf("Transparent Index: %d\n", gcb.TransparentColor);
        	        printf("DelayTime: %d\n", gcb.DelayTime);
              		printf("DisposalMode: %d\n", gcb.DisposalMode);
	                #endif

        	        /* Get disposal mode */
                	Disposal_Mode=gcb.DisposalMode;
	                /* Get trans_color */
        	      	trans_color=gcb.TransparentColor;

	               	/* Get delay time in ms, and delay */
        	        DelayMs=gcb.DelayTime*10;
//			egi_sleep(0,0,DelayMs);
			//tm_delayms(DelayMs);

			break;
		case CONTINUE_EXT_FUNC_CODE:
			printf(" --- CONTINUE_EXT_FUNC_CODE ---\n");
			break;
		case COMMENT_EXT_FUNC_CODE:
			printf(" --- COMMENT_EXT_FUNC_CODE ---\n");
			break;
		case PLAINTEXT_EXT_FUNC_CODE:
			printf(" --- PLAINTEXT_EXT_FUNC_CODE ---\n");
			break;
		case APPLICATION_EXT_FUNC_CODE:
			printf(" --- APPLICATION_EXT_FUNC_CODE ---\n");
			break;

		default:
			break;

	    } /* END of switch(), EXT_FUNC_CODE */


	} /* END for(j):  read and parse extension block*/

     }
     /* ELSE: NO extension blocks */
     else {
		ExtBlock=NULL;
     }


#if 1   ///////////////////  METHOD_1:    CALL egi_gif_rasterWriteFB()    ////////////////

     /* Write gif frame/block to FB, ALWAYS after a control block! */
      egi_gif_rasterWriteFB( fbdev, egif->Simgbuf, Disposal_Mode,   /* Simgbuf, Disposal_Mode */
			     xp,yp,xw,yw,winw,winh,		    /* xp,yp,xw,yw,winw,winh */
			     BWidth, BHeight, offx, offy,           /* BWidth, BHeight, offx, offy */
		  	     ColorMap, ImageData->RasterBits,  	    /*  ColorMap, buffer, */
		     trans_color, -1, egif->SBackGroundColor,   /* trans_color, user_trans_color, bkg_color */
			     DirectFB_ON, ImgTransp_ON, false ); /* DirectFB_ON, ImgTransp_ON, BkgTransp_ON */

	egi_sleep(0,0,DelayMs);

#else  /////////////////  METHOD_2:  Code Block, A litter faster?    /////////////////////

    int i;
    int bkg_color=-1;
    int user_trans_color=-1;
    bool BkgTransp_ON=false;
    bool Is_ImgColorMap;
    int pos=0;
    int spos=0;
    int SWidth=egif->SWidth;		/* Screen/canvas size */
    int SHeight=egif->SHeight;
    GifColorType *ColorMapEntry;
    EGI_IMGBUF *Simgbuf= egif->Simgbuf;


   /* Limit Screen width x heigh, necessary? */
   if(BWidth==SWidth)offx=0;
   if(BHeight==SHeight)offy=0;
   //printf("%s: input Height=%d, Width=%d offx=%d offy=%d\n", __func__, Height, Width, offx, offy);

    /* update Simgbuf */
    for(i=0; i<BHeight; i++)
    {
	 /* update block of Simgbuf */
	 for(j=0; j<BWidth; j++ ) {
	      pos=i*BWidth+j;
              spos=(offy+i)*SWidth+(offx+j);
	      /* Nontransparent color: set color and set alpha to 255 */
	      if( trans_color < 0 || trans_color != buffer[pos]  )
              {
//		  if(Is_ImgColorMap) /* TODO:  not tested! */
///		         ColorMapEntry = &ColorMap->Colors[buffer[pos]];
//		  else 		   /* Is global color map. */
		         ColorMapEntry = &ColorMap->Colors[buffer[pos]];

                  Simgbuf->imgbuf[spos]=COLOR_RGB_TO16BITS( ColorMapEntry->Red,
							    ColorMapEntry->Green,
							    ColorMapEntry->Blue );
		  /* Only if imgbuf alpha is ON */
		 // if(Simgbuf->alpha)
		  	Simgbuf->alpha[spos]=255;
	      }


	      /* Just after above!, If IMGBUF alpha applys, Transprent color: set alpha to 0 */
	      // if(Simgbuf->alpha) {
	      if(ImgTransp_ON) {
		  /* Make background color transparent */
	          if( BkgTransp_ON && (buffer[pos] == bkg_color) ) {  /* bkg_color meaningful ONLY when global color table exists */
			//if(Disposal_Mode==2)
		      		Simgbuf->alpha[spos]=0;
	          }
	          if( buffer[pos] == trans_color || buffer[pos] == user_trans_color) {   /* ???? if trans_color == bkg_color */
			//if(Disposal_Mode==2)
		      		Simgbuf->alpha[spos]=0;
	          }
	      }
	      /*  ELSE : Keep old color and alpha value in Simgbuf->imgbuf */
          }
    }

    /* return if FBDEV is NULL */
    if(fbdev == NULL)
	return;

    //newimg=egi_imgbuf_resize(&Simgbuf, 240, 240);  // EGI_IMGBUF **pimg, width, height
    if( Disposal_Mode==2 && !DirectFB_ON ) /* Set area to background color/image */
    {
	  /* TODO: to be testetd, how about Disposal_Mode changes ??? */
	  /* NOTE!!! DirectFB_ON mode MUST NOT use FB_FILO! */

          #if 0 /* FAILS!!! When Disposal_mode changes to other than 2, this will fail then! */
   	  /* Display imgbuf, with FB FILO ON, OR fill background with FB bkg buffer */
	  fb_filo_flush(fbdev); /* flush and restore old FB pixel data */
	  fb_filo_on(fbdev); /* start collecting old FB pixel data */
	  #else
          memcpy(fbdev->map_bk, fbdev->map_buff+fbdev->screensize, fbdev->screensize);
	  #endif
    }
    /* TODO: Mode 3. */

    /* display the whole Gif screen/canvas */
    egi_imgbuf_windisplay( Simgbuf, fbdev, -1,             /* img, fb, subcolor */
			   xp, yp,				/* xp, yp */
			   xw, yw,				/* xw, yw */
                     	   winw, winh   /* winw, winh */
			);

    if( Disposal_Mode==2 && !DirectFB_ON )  /* Set area to background color/image */
	 fb_filo_off(fbdev); /* start collecting old FB pixel data */

    /* refresh FB page */
    if(!DirectFB_ON)
    	fb_page_refresh(fbdev);

#endif  /////////////////////   END METHOD SELECT    /////////////////////////

     /* ? reset trans_color ? */

     /* ImageCount incremental */
     egif->ImageCount++;

    if( egif->ImageCount > egif->ImageTotal-1) {
	/* End of one round loop */
	egif->ImageCount=0;
	k++;
    }

 }  while( k < nloop );  /*  ---- Do nloop  times! ---- */


}
