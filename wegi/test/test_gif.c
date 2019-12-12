/* -----------------------------------------------------------------------
A routine derived from GIFLIB examples: convert GIF to 24-bit RGB pixel triples.

     The GIFLIB distribution is Copyright (c) 1997  Eric S. Raymond
			SPDX-License-Identifier: MIT

	== Authors of Giflib ==
Gershon Elber <gershon[AT]cs.technion.sc.il>
        original giflib code
Toshio Kuratomi <toshio[AT]tiki-lounge.com>
        uncompressed gif writing code
        former maintainer
Eric Raymond <esr[AT]snark.thyrsus.com>
        current as well as long time former maintainer of giflib code
others ...


Note:
TODO:
 1. Is_ImgColorMap=TRUE NOT tested yet!

	typedef struct GraphicsControlBlock {
	    	int DisposalMode;
		#define DISPOSAL_UNSPECIFIED      0       // No disposal specified.
		#define DISPOSE_DO_NOT            1       // Leave image in place
		#define DISPOSE_BACKGROUND        2       // Set area too background color
		#define DISPOSE_PREVIOUS          3       // Restore to previous content
		bool UserInputFlag;      // User confirmation required before disposal
		int DelayTime;           // pre-display delay in 0.01sec units
	    	int TransparentColor;    // Palette index for transparency, -1 if none
		#define NO_TRANSPARENT_COLOR    -1
	} GraphicsControlBlock;


Midas Zhou
---------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fcntl.h>

#include "gif_lib.h"
#include "egi_common.h"

#define PROGRAM_NAME    "gif2rgb"
#define GIF_MESSAGE(Msg) fprintf(stderr, "\n%s: %s\n", "gif2rgb", Msg)
#define GIF_EXIT(Msg)    { GIF_MESSAGE(Msg); exit(-3); }
#define UNSIGNED_LITTLE_ENDIAN(lo, hi)	((lo) | ((hi) << 8))

void GifQprintf(char *Format, ...);
void PrintGifError(int ErrorCode);
void display_gif( int Width, int Height, int offx, int offy,
		  ColorMapObject *ColorMap, GifRowType *ScreenBuffer);

static EGI_IMGBUF *imgbuf=NULL; /* an IMGBUF to hold gif image canvas */
static bool Is_ImgColorMap=false;  /* If color map is image color map, NOT global screen color map, */
static int trans_color=-1;  /* Palette index for transparency, -1 if none, or NO_TRANSPARENT_COLOR */
static int SWidth, SHeight; /* screen(gif canvas) width and height */
static int offx, offy;	    /* gif block image width and height */


int main(int argc, char ** argv)
{

    int NumFiles=1;
    char *FileName=argv[1];
    bool OneFileFlag=true;
    char *OutFileName=argv[2];

    int	i, j, Size;
    int Width=0, Height=0;
    int Row=0, Col=0;
    int  ExtCode, Count;
    int DelayMs;	/* delay time in ms */

    GifRecordType RecordType;
    GifByteType *Extension;
    GifRowType *ScreenBuffer;  /* typedef unsigned char *GifRowType */
    GifFileType *GifFile;
    int
	InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
	InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
    int ImageNum = 0;
    ColorMapObject *ColorMap;
    int Error;

    GraphicsControlBlock gcb;
    int DisposalMode;

    /* --- Init FB DEV --- */
    printf("init_fbdev()...\n");
    if( init_fbdev(&gv_fb_dev) )            /* init sys FB */
                return -1;
    /* Set screen view type as LANDSCAPE mode */
    fb_position_rotate(&gv_fb_dev, 3);

    /* Open gif file */
    if (NumFiles == 1) {
	if ((GifFile = DGifOpenFileName(FileName, &Error)) == NULL) {
	    PrintGifError(Error);
	    exit(EXIT_FAILURE);
	}
    }
    else {
	/* Use stdin instead: */
	if ((GifFile = DGifOpenFileHandle(0, &Error)) == NULL) {
	    PrintGifError(Error);
	    exit(EXIT_FAILURE);
	}
    }

    if (GifFile->SHeight == 0 || GifFile->SWidth == 0) {
	fprintf(stderr, "Image of width or height 0\n");
	exit(EXIT_FAILURE);
    }


    /* Get color map */
    if(GifFile->Image.ColorMap) {
	Is_ImgColorMap=true;
	ColorMap=GifFile->Image.ColorMap;
		printf("Image Colorcount=%d\n", GifFile->Image.ColorMap->ColorCount);
    }
    else {
	Is_ImgColorMap=false;
		ColorMap=GifFile->SColorMap;
	printf("Global Colorcount=%d\n", GifFile->SColorMap->ColorCount);
    }

    /* check that the background color isn't garbage (SF bug #87) */
    if (GifFile->SBackGroundColor < 0 || GifFile->SBackGroundColor >= ColorMap->ColorCount) {
        fprintf(stderr, "Background color out of range for colormap\n");
        exit(EXIT_FAILURE);
    }

    /* get SWidth and SHeight */
    SWidth=GifFile->SWidth;
    SHeight=GifFile->SHeight;
    printf("\n--- SWxSH=%dx%d ---\n", SWidth, SHeight);

    /***   ---   Allocate screen as vector of column and rows   ---
     * Note this screen is device independent - it's the screen defined by the
     * GIF file parameters.
     */
    if ((ScreenBuffer = (GifRowType *)
	malloc(GifFile->SHeight * sizeof(GifRowType))) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");

    Size = GifFile->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/
    if ((ScreenBuffer[0] = (GifRowType) malloc(Size)) == NULL) /* First row. */
	GIF_EXIT("Failed to allocate memory required, aborted.");

    for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
	ScreenBuffer[0][i] = GifFile->SBackGroundColor;
    for (i = 1; i < GifFile->SHeight; i++) {
	/* Allocate the other rows, and set their color to background too: */
	if ((ScreenBuffer[i] = (GifRowType) malloc(Size)) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");

	memcpy(ScreenBuffer[i], ScreenBuffer[0], Size);
    }

    /* Scan the content of the GIF file and load the image(s) in: */
    do {
	if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
	    PrintGifError(GifFile->Error);
	    exit(EXIT_FAILURE);
	}
	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
		if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
		    PrintGifError(GifFile->Error);
		    exit(EXIT_FAILURE);
		}
		Row = GifFile->Image.Top; /* Image Position relative to Screen. */
		Col = GifFile->Image.Left;

		/* Get offx, offy, Original Row AND Col will increment later!!!  */
		offx = Col;
		offy = Row;
		Width = GifFile->Image.Width;
		Height = GifFile->Image.Height;

		/* check block image size and position */
		GifQprintf("\n%s: Image %d at (%d, %d) [%dx%d]:     ",
		    PROGRAM_NAME, ++ImageNum, Col, Row, Width, Height);
		if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
		   GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) {
		    fprintf(stderr, "Image %d is not confined to screen dimension, aborted.\n",ImageNum);
		    exit(EXIT_FAILURE);
		}

		/* Get color (index) data */
		if (GifFile->Image.Interlace) {
		    /* Need to perform 4 passes on the images: */
		    for (Count = i = 0; i < 4; i++)
			for (j = Row + InterlacedOffset[i]; j < Row + Height;
						 j += InterlacedJumps[i]  )
			{
			    GifQprintf("\b\b\b\b%-4d", Count++);
			    if ( DGifGetLine(GifFile, &ScreenBuffer[j][Col],
				 Width) == GIF_ERROR )
			    {
				 PrintGifError(GifFile->Error);
				 exit(EXIT_FAILURE);
			    }
			}
		}
		else {
		    for (i = 0; i < Height; i++) {
			GifQprintf("\b\b\b\b%-4d", i);
			if (DGifGetLine(GifFile, &ScreenBuffer[Row++][Col],
				Width) == GIF_ERROR) {
			    PrintGifError(GifFile->Error);
			    exit(EXIT_FAILURE);
			}
		    }
		}
		printf("\n");

		/* display */
   		display_gif(Width, Height, offx, offy, ColorMap, ScreenBuffer);
		/* hold on ... see displaying delay in case EXTENSION_RECORD_TYPE */
		//usleep(55000);
		break;

	    case EXTENSION_RECORD_TYPE:
		/*  extension blocks in file: */
		if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
		    PrintGifError(GifFile->Error);
		    exit(EXIT_FAILURE);
		}

		/*   --- parse extension code ---  */
                if (ExtCode == GRAPHICS_EXT_FUNC_CODE) {
                     if (Extension == NULL) {
                            printf("Invalid extension block\n");
                            GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
                            PrintGifError(GifFile->Error);
                            exit(EXIT_FAILURE);
                     }
                     if (DGifExtensionToGCB(Extension[0], Extension+1, &gcb) == GIF_ERROR) {
                            PrintGifError(GifFile->Error);
                            exit(EXIT_FAILURE);
                      }
		      #if 0 /* ----- for test ---- */
		      printf("\tDisposalMode: %d\n", gcb.DisposalMode);
                      printf("\tTransparency on: %s\n",
                      			gcb.TransparentColor != -1 ? "yes" : "no");
                      printf("\tDelayTime: %d\n", gcb.DelayTime);
                      printf("\tTransparent Index: %d\n", gcb.TransparentColor);
		      #endif

      		      /* Get trans_color */
		      trans_color=gcb.TransparentColor;

		      /* Get delay time in ms, and delay */
		      DelayMs=gcb.DelayTime*10;
		      //sleep(DelayMs/1000);
		      //usleep((DelayMs%1000)*1000);
		 }

		/* Read out next extension and discard, TODO: Not useful information????  */
		while (Extension!=NULL) {
                    if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
                        PrintGifError(GifFile->Error);
                        exit(EXIT_FAILURE);
                    }
                    if (Extension == NULL) {
			//printf("Extension is NULL\n");
                        break;
		    }
		    printf(" --------- \n");
		    if (Extension[0] & 0x01) {
        		   trans_color = Extension[3];
                    	   printf("---Transparent Index: %d\n", trans_color);
		    }
		    DisposalMode = (Extension[0] >> 2) & 0x07;
		    printf("---DisposalMode: %d\n", DisposalMode);
                }
		break;

	    case TERMINATE_RECORD_TYPE:
		break;

	    default:		    /* Should be trapped by DGifGetRecordType. */
		break;
	}
    } while (RecordType != TERMINATE_RECORD_TYPE);

#if 0  /* --- Move to just after opening the gif file --- */
    /* Lets dump it - set the global variables required and do it: */
    ColorMap = (GifFile->Image.ColorMap
		? GifFile->Image.ColorMap
		: GifFile->SColorMap);
    if (ColorMap == NULL) {
        fprintf(stderr, "Gif Image does not have a colormap\n");
        exit(EXIT_FAILURE);
    }

    /* check that the background color isn't garbage (SF bug #87) */
    if (GifFile->SBackGroundColor < 0 || GifFile->SBackGroundColor >= ColorMap->ColorCount) {
        fprintf(stderr, "Background color out of range for colormap\n");
        exit(EXIT_FAILURE);
    }
#endif

    (void)free(ScreenBuffer);

    if (DGifCloseFile(GifFile, &Error) == GIF_ERROR) {
	PrintGifError(Error);
	exit(EXIT_FAILURE);
    }

  return 0;
}




/*****************************************************************************
 Same as fprintf to stderr but with optional print.
******************************************************************************/
void GifQprintf(char *Format, ...)
{
bool GifNoisyPrint = true;

    va_list ArgPtr;

    va_start(ArgPtr, Format);

    if (GifNoisyPrint) {
        char Line[128];
        (void)vsnprintf(Line, sizeof(Line), Format, ArgPtr);
        (void)fputs(Line, stderr);
    }

    va_end(ArgPtr);
}

void PrintGifError(int ErrorCode)
{
    const char *Err = GifErrorString(ErrorCode);

    if (Err != NULL)
        fprintf(stderr, "GIF-LIB error: %s.\n", Err);
    else
        fprintf(stderr, "GIF-LIB undefined error %d.\n", ErrorCode);
}



void display_gif( int Width, int Height, int offx, int offy,
		  ColorMapObject *ColorMap, GifRowType *ScreenBuffer)
{
    int i,j;

    /* fill to imgbuf */
    if(imgbuf==NULL) {
	    imgbuf=egi_imgbuf_create(SHeight, SWidth, 255, 0); //height, width, alpha, color
	    if(imgbuf==NULL)
		return;

	    free(imgbuf->alpha);
	    imgbuf->alpha=NULL;
    }

   /* Limit Screen width x heigh */
   if(Width==SWidth)offx=0;
   if(Height==SHeight)offy=0;

   //printf("%s: input Height=%d, Width=%d offx=%d offy=%d\n", __func__, Height, Width, offx, offy);


    ////////// ----- NOTE: suppose (0,0) origin ---- ///////
    GifRowType GifRow;
    GifColorType *ColorMapEntry;

    int pos=0;

//    for(i=0; i<Height; i++)
      for(i=offy; i<offy+Height; i++)
      {
 	 if(Is_ImgColorMap) {
	    GifRow = ScreenBuffer[i-offy];
	 } else {
	    GifRow = ScreenBuffer[i];
	 }
	 for(j=offx; j<offx+Width; j++ ) {

	      pos=i*SWidth+j;

	      /* skip transparent color */
	      if( trans_color ==-1 || trans_color != GifRow[j] )
              {
		if(Is_ImgColorMap)
		         ColorMapEntry = &ColorMap->Colors[GifRow[j-offx]];
		else // Is screen color map
		         ColorMapEntry = &ColorMap->Colors[GifRow[j]];
                 *(imgbuf->imgbuf+pos)=COLOR_RGB_TO16BITS(  ColorMapEntry->Red,
							    ColorMapEntry->Green,
							    ColorMapEntry->Blue   );
	      }
//	     else
//		printf(" ======   trans_color=%d GirRow[j]=%d  ====== \n", trans_color,GifRow[j]);

          }
    }

   /* reset trans color */
   trans_color=-1;

   //egi_imgbuf_resize_update(&imgbuf, 240, 240);  // EGI_IMGBUF **pimg, width, height

    egi_imgbuf_windisplay( imgbuf, &gv_fb_dev, -1,            /* img, fb, subcolor */
                           0, 0,				/* xp, yp  */
			   SWidth>320 ? 0:(320-SWidth)/2,	/* xw */
			   SHeight>240 ? 0:(240-SHeight)/2,	/* yw */
			   //(320-SWidth)/2, (240-SHeight)/2,    /* xw, yw */
			   //Width, Height ); 			/* winw, winh */
                           imgbuf->width, imgbuf->height);    /* winw, winh */

    fb_page_refresh(&gv_fb_dev);

//    egi_imgbuf_free(imgbuf);
//    imgbuf=NULL;

}

