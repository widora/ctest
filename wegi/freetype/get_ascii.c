/*----------------------------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License version 2 as published by the Free Software
Foundation.
(Note: FreeType 2 is licensed under FTL/GPLv3 or GPLv2 ).

An Example of fonts demonstration based on example1.c in FreeType 2 library docs.

1. Size of character bitmap maps to symbol size for struct symbol_page:
   1.1 height in pixel as in FT_Set_Pixel_Sizes(face,W,H)   --->  symheight  (same height for all symbols)
       FT bitmap.rows of 'j' is the biggest in all ascii symbols, and nearly reaches pix size H.
   1.2 slot->advance.x  	 			    --->  symwidth
       Each ascii symbol has different value of its FT bitmap.width.

2. 0-31 ASCII control chars, 32-126 ASCII printable symbols.


Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <wchar.h>
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>
#include "egi_common.h"
#include "egi_image.h"
#include "egi_utils.h"

#include FT_FREETYPE_H


/*------------------------------------------------------------------------------------
        	   Add FreeType FT_Bitmap to EGI imgbuf

1. The input eimg should has been initialized by egi_imgbuf_init()
   with certain size of a canvas (height x width) inside.

2. The canvas size of the eimg shall be big enough to hold the bitmap,
   or pixels out of the canvas will be omitted.

3.   		!!! -----   WARNING   ----- !!!
   In order to algin all characters in same horizontal level, every char bitmap must be
   align to the same baseline, i.e. the vertical position of each char's origin
   (baseline) MUST be the same.
   So (xb,yb) should NOT be left top coordinate of a char bitmap,
   while use char's 'origin' coordinate relative to eimg canvan as (xb,yb) can align all
   chars at the same level !!!


@eimg		The EGI_IMGBUF to hold the bitmap
@xb,yb		coordinates of bitmap origin relative to EGI_IMGBUF canvas coord.!!!!

@bitmap		pointer to a bitmap in a FT_GlyphSlot.
		typedef struct  FT_Bitmap_
		{
			    unsigned int    rows;
			    unsigned int    width;
			    int             pitch;
			    unsigned char*  buffer;
			    unsigned short  num_grays;
			    unsigned char   pixel_mode;
			    unsigned char   palette_mode;
			    void*           palette;
		} FT_Bitmap;

@subcolor	>=0 as substituting color
		<0  use bitmap buffer data as gray value. 0-255 (BLACK-WHITE)

return:
	0	OK
	<0	fails
--------------------------------------------------------------------------------*/
int egi_imgbuf_blend_FTbitmap(EGI_IMGBUF* eimg, int xb, int yb, FT_Bitmap *bitmap,
								EGI_16BIT_COLOR subcolor)
{


	int i,j;
	EGI_16BIT_COLOR color;
	unsigned char alpha;
	unsigned long size; /* alpha size */
	int	sumalpha;
	int pos;

	if(eimg==NULL || eimg->imgbuf==NULL || eimg->height==0 || eimg->width==0 ) {
		printf("%s: input EGI_IMBUG is NULL or uninitiliazed!\n", __func__);
		return -1;
	}
	if( bitmap==NULL || bitmap->buffer==NULL ) {
		printf("%s: input FT_Bitmap or its buffer is NULL!\n", __func__);
		return -2;
	}
	/* calloc and assign alpha, if NULL */
	if(eimg->alpha==NULL) {
		size=eimg->height*eimg->width;
		eimg->alpha = calloc(1, size); /* alpha value 8bpp */
		if(eimg->alpha==NULL) {
			printf("%s: Fail to calloc eimg->alpha\n",__func__);
			return -3;
		}
		memset(eimg->alpha, 255, size); /* assign to 255 */
	}

	for( i=0; i< bitmap->rows; i++ ) {	      /* traverse bitmap height  */
		for( j=0; j< bitmap->width; j++ ) {   /* traverse bitmap width */
			/* check range limit */
			if( yb+i <0 || yb+i >= eimg->height ||
				    xb+j <0 || xb+j >= eimg->width )
				continue;

			/* buffer value(0-255) deemed as gray value OR alpha value */
			alpha=bitmap->buffer[i*bitmap->width+j];

			pos=(yb+i)*(eimg->width) + xb+j; /* eimg->imgbuf position */

			/* blend color	*/
			if( subcolor>=0 ) {	/* use subcolor */
				//color=subcolor;
				/* !!!WARNG!!!  NO Gamma Correctio in color blend macro,
				 * color blend will cause some unexpected gray
				 * areas/lines, especially for two contrasting colors.
				 * Select a suitable backgroud color to weaken this effect.
				 */
				color=COLOR_16BITS_BLEND( subcolor, eimg->imgbuf[pos], alpha );
							/* front, background, alpha */
			}
			else {			/* use Font bitmap gray value */
				color=COLOR_16BITS_BLEND( COLOR_RGB_TO16BITS(alpha,alpha,alpha),
							  eimg->imgbuf[pos], alpha );
			}
			eimg->imgbuf[pos]=color; /* assign color to imgbuf */

			/* blend alpha value */
			sumalpha=eimg->alpha[pos]+alpha;
			if( sumalpha > 255 ) sumalpha=255;
			eimg->alpha[pos]=sumalpha; //(alpha>0 ? 255:0); //alpha;
		}
	}

	return 0;
}



int main( int  argc,   char**  argv )
{
	FT_Library    library;
	FT_Face       face;		/* NOTE: typedef struct FT_FaceRec_*  FT_Face */

	FT_GlyphSlot  slot;
	FT_Matrix     matrix;         /* transformation matrix */
	FT_Vector     pen;            /* untransformed origin  */
	int		line_starty;    /* starting pen Y postion for rotated lines */
	FT_BBox	bbox;		/* Boundary box */
	FT_Error      error;

	/* size in pixels */
	int		nsize=8;
	int		size_pix[][2]= {

	16,16,	/* W*H */
	18,18,
	18,20,
	24,24,
	32,32,
	36,36,
	48,48,
	64,64
	};

 	FT_CharMap*   pcharmaps=NULL;
  	char          tag_charmap[6]={0};
  	uint32_t      tag_num;


  	char*         font_path;
  	char*         text;

  	char**	fpaths;
  	int		count;
  	char**	picpaths;
  	int		pcount;

  	EGI_16BIT_COLOR font_color;
  	int		glyph_index;
  	int		deg; /* angle in degree */
  	double        angle;
  	int           n, num_chars;
  	int 		i,j,k;
  	int		np;
  	float		step;
	int		ret;

	EGI_IMGBUF  *eimg=NULL;
	eimg=egi_imgbuf_new();

        /* EGI general init */
        tm_start_egitick();
        if(egi_init_log("/mmc/log_freetype") != 0) {
                printf("Fail to init logger,quit.\n");
                return -1;
        }
        if(symbol_load_allpages() !=0 ) {
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
        init_fbdev(&gv_fb_dev);


	/* get input args */
  	if ( argc != 3 )
  	{
    	fprintf ( stderr, "usage: %s font sample-text\n", argv[0] );
   	 	exit( 1 );
  	}

  	font_path      = argv[1];                           /* first argument     */
  	text          = argv[2];                           /* second argument    */
  	num_chars     = strlen( text );


   	/* TODO: convert char to wchar_t */
     	char    *cstr="Pause abcefghijJBHKMS_%$^#_{}|:>?";


   /* buff all png file path */
   picpaths=egi_alloc_search_files("/mmc/pic", ".png", &pcount); /* NOTE: path for fonts */
   printf("Totally %d png files are found.\n",pcount);
   if(pcount==0) exit(1);

   /* buff all ttf,otf font file path */
   fpaths=egi_alloc_search_files(font_path, ".ttf, .otf", &count); /* NOTE: path for fonts */
   printf("Totally %d ttf font files are found.\n",count);
   if(count==0) exit(1);
   for(i=0; i<count; i++)
        printf("%s\n",fpaths[i]);



/* >>>>>>>>>>>>>>>>>>>>  START FONTS DEMON TEST  >>>>>>>>>>>>>>>>>>>> */
for(j=0; j<=count; j++)
{
	if(j==count)j=0;

	/* 1. initialize FT library */
	error = FT_Init_FreeType( &library );
	if(error) {
		printf("%s: An error occured during FreeType library initialization.\n",__func__);
		return -1;
	}

	/* 2. create face object, face_index=0 */
 	//error = FT_New_Face( library, font_path, 0, &face );
 	error = FT_New_Face( library, fpaths[j], 0, &face );
	if(error==FT_Err_Unknown_File_Format) {
		printf("%s: Font file opens, but its font format is unsupported!\n",__func__);
		FT_Done_FreeType( library );
		return -2;
	}
	else if ( error ) {
		printf("%s: Fail to open or read font '%s'.\n",__func__, fpaths[j]);
		FT_Done_FreeType( library );
		return -3;
	}
  	/* get pointer to the glyph slot */
  	slot = face->glyph;

	printf("-------- Load font '%s', index[0] --------\n", fpaths[j]);
	printf("   num_faces:		%d\n",	face->num_faces);
	printf("   face_index:		%d\n",	face->face_index);
	printf("   family name:		%s\n",	face->family_name);
	printf("   style name:		%s\n",	face->style_name);
	printf("   num_glyphs:		%d\n",	face->num_glyphs);
	printf("   face_flags: 		0x%08X\n",face->face_flags);
	printf("   units_per_EM:	%d\n",	face->units_per_EM);
	printf("   num_fixed_sizes:	%d\n",	face->num_fixed_sizes);
	if(face->num_fixed_sizes !=0 ) {
		for(i=0; i< face->num_fixed_sizes; i++) {
			printf("[%d]: H%d x W%d\n",i, face->available_sizes[i].height,
							    face->available_sizes[i].width);
		}
	}
	/* print charmaps */
	printf("   num_charmaps:	%d\n",	face->num_charmaps);
	for(i=0; i< face->num_charmaps; i++) { /* print all charmap tags */
		pcharmaps=face->charmaps;
		tag_num=htonl((uint32_t)(*pcharmaps)->encoding );
		memcpy( tag_charmap, &tag_num, 4); /* 4 bytes TAG */
		printf("      			[%d] %s\n", i,tag_charmap ); /* 'unic' as for Unicode */
		pcharmaps++;
	}
	tag_num=htonl((uint32_t)( face->charmap->encoding));
	memcpy( tag_charmap, &tag_num, 4); /* 4 bytes TAG */
	printf("   charmap in use:	%s\n", tag_charmap ); /* 'unic' as for Unicode */

	/* vertical distance between two consective lines */
	printf("   height(V dist, in font units):	%d\n",	face->height);

	/* set angle */
	deg=0.0;

  	/* 3. set up matrix for transformation */
  	angle     = ( deg / 360.0 ) * 3.14159 * 2;
  	printf(" ------------- angle: %d, %f ----------\n", deg, angle);
  	matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
  	matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
	matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
	matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

	/* 4. set pen position
   	 * the pen position in 26.6 cartesian space coordinates
     	 * 64 units per pixel for pen.x and pen.y
 	 */
  	pen.x = 5*64;
  	pen.y = 5*64;
  	line_starty=pen.y;

  	/* clear screen */
  	clear_screen(&gv_fb_dev, WEGI_COLOR_BLACK);

	/* init imgbuf */
	egi_imgbuf_init(eimg, 320, 240);

  	/* set font color */
  	//font_color= egi_color_random(all);
	font_color = WEGI_COLOR_WHITE;

/* select differenct size */
for(k=0; k< nsize-1; k++) {

   	/* 5. set character size in pixels */
   	error = FT_Set_Pixel_Sizes(face, size_pix[k][0], size_pix[k][1]); /* width,height */
	/* OR set character size in 26.6 fractional points, and resolution in dpi */
   	//error = FT_Set_Char_Size( face, 32*32, 0, 100, 0 );

  	pen.y=line_starty;

	/* load chars in string one by one and process it */
	for ( n = 0; n < strlen(cstr); n++ )
	{
		   	/* 6. re_set transformation before loading each wchar,
			 *    as pen postion and transfer_matrix may changed.
		         */
		    	FT_Set_Transform( face, &matrix, &pen );

			/* 7. load char and render to bitmap  */

			// glyph_index = FT_Get_Char_Index( face, wcstr[n] ); /*  */
			// printf("charcode[%X] --> glyph index[%d] \n", wcstr[n], glyph_index);
#if 1
			/*** Option 1:  FT_Load_Char( FT_LOAD_RENDER )
		    	 *   load glyph image into the slot (erase previous one)
			 */
		    	error = FT_Load_Char( face, cstr[n], FT_LOAD_RENDER );
		    	if ( error ) {
				printf("%s: FT_Load_Char() error!\n");
      				continue;                 /* ignore errors */
			}
#else
			/*** Option 2:  FT_Load_Char(FT_LOAD_DEFAULT) + FT_Render_Glyph()
		         *    Call FT_Load_Glyph() with default flag and load the glyph slot
			 *    in its native format, then call FT_Render_Glyph() to convert
			 *    it to bitmap.
		         */
		    	error = FT_Load_Char( face, cstr[n], FT_LOAD_DEFAULT );
		    	if ( error ) {
				printf("%s: FT_Load_Char() error!\n");
		      		continue;                 /* ignore errors */
			}

		        FT_Render_Glyph( slot, FT_RENDER_MODE_NORMAL );
			/* ALSO_Ok: _LIGHT, _NORMAL; 	BlUR: _MONO	LIMIT: _LCD */
#endif

	/* get boundary box in grid-fitted pixel coordinates */
	printf("face boundary box: width=%d, height=%d.\n",
			face->bbox.xMax - face->bbox.xMin, face->bbox.yMax-face->bbox.yMin );

	/* Note that even when the glyph image is transformed, the metrics are NOT ! */
	printf("glyph metrics[in 26.6 format]: width=%d, height=%d.\n",
					slot->metrics.width, slot->metrics.height);

	printf(" '%c'--- W*H=%d*%d \n", cstr[n], size_pix[k][0], size_pix[k][1]);
	printf("bitmap.rows=%d, bitmap.width=%d \n", slot->bitmap.rows, slot->bitmap.width);
	printf("bitmap_left=%d, bitmap_top=%d. \n",slot->bitmap_left, slot->bitmap_top);
	printf("advance.x=%d\n",slot->advance.x);
	printf("advance.y=%d\n",slot->advance.y);

	/* 8. Draw to EGI_IMGBUF */
	error=egi_imgbuf_blend_FTbitmap(eimg, slot->bitmap_left, 220-slot->bitmap_top,
						&slot->bitmap, font_color); //egi_color_random(deep) );
	if(error) {
		printf(" Fail to fetch Font type '%s' char index [%d]\n", fpaths[j], n);
	}

    	/* 9. increment pen position */
    	pen.x += slot->advance.x;
    	pen.y += slot->advance.y; /* same in a line for the same char  */

  } /* end for() load and process chars */

	/* 10. skip pen to next line, in font units. */
	pen.x = 3<<6; /* start X in a new line */
	line_starty += ((int)(size_pix[k][0])+ 0 )<<6; /* LINE GAP,  +15 */
	/* 1 pixel= 64 units glyph metrics */

} /* end of k size */

/*-------------------------------------------------------------------------------------------
int egi_imgbuf_windisplay(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
// no subcolor, no FB filo
int egi_imgbuf_windisplay2(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
----------------------------------------------------------------------------------------------*/

	/* Dispay EGI_IMGBUF */
	egi_imgbuf_windisplay2(eimg, &gv_fb_dev, 0, 0, 0, 0, eimg->width, eimg->height);

	tm_delayms(2000);


FT_FAILS:
  FT_Done_Face    ( face );
  FT_Done_FreeType( library );


} /* end fpaths[j] */


/* >>>>>>>>>>>>>>>>>  END FONTS DEMON TEST  >>>>>>>>>>>>>>>>>>>> */

 	/* free fpaths buffer */
        egi_free_buff2D((unsigned char **) fpaths, count);
        egi_free_buff2D((unsigned char **) picpaths, pcount);

	/* free EGI_IMGBUF */
	egi_imgbuf_free(eimg);

	/* Free EGI */
        release_fbdev(&gv_fb_dev);
        symbol_release_allpages();
        egi_quit_log();

  return 0;
}

/* EOF */
