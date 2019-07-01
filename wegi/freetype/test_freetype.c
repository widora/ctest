/*-------------------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License version 2 as published by the Free Software
Foundation.


An Example of fonts demonstration based on example1.c in FreeType 2 library docs.
(Note: FreeType 2 is based on FTL/GPLv3 or GPLv2 ).


			<<<	--- Glossary ---	>>>

	( Refering to: https://freetype.org/freetype2/docs/tutorial )

character code:		A unique value that defines the character for a given encoding.

charmap:		A table in face object that converts character codes to glyph indices.
			Note:
			1. Most fonts contains a charmap that converts Unicode character codes to
		  	   glyph indices.
			2. FreeType tries to select a Unicode charmp when a new face object
			   is created. and it emulates a Unicode charmap if the font doesn't
			   contain such a charmap.

units_per_EM           The number of font units per EM square for a face, relevant for scalable
		       fonts only. Noticed that all characters have the same height.
		       Typically 1000 for type-1 fonts and 1024 or 2048 for TrueType fonts.

	  typedef struct  FT_CharMapRec_
	  {
	    FT_Face      face;
	    FT_Encoding  encoding;
	    FT_UShort    platform_id;
	    FT_UShort    encoding_id;
	  } FT_CharMapRec;


CJK:	Chinese, Japanese, Korean
TTF:	True Type Font
TTC:	True Type Collections
OTF:	Open Type Font (Post-script)
SIL:	The SIL Open Font License(OFL) is a free, libre and open source license.


			<<<	--- Some Open/Free Fonts --- 	>>>

Source_Han_San:		A set of OpenType/CFF Pan-CJK fonts. 		( SIL v1.1 )
Source_Han_Serif:	A set of OpenType/CFF Pan-CJK fonts. 		( SIL v1.1 )
Wang_Han_Zong:		A set of traditional/simple chinese fonts.	( GPL v2 )

Liberation-fonts:	A set of western fonts whose layout is compatible to Times New Roman,
			Arial, and Courier New. 			( SIL v1.1 )
Bitstream Vera Fonts	A set of western fonts for GNOME. 	( Bitstream Vera Fonts Copyright )



Notes:
1. Default encoding environment shall be set to be same, UFT-8 etc, for both codes compiling
   and running.
2. Refer to: zenozeng.github.io/Free-Chinese-Fonts for more GPL fonts.
3. Input text with chinese traditional style when use some chinese fonts, like
   WHZ fonts etc, or it may not be found in its font library.
4. SourceHanSans support both simple and traditional Chineses text encoding.
5. FT_Load_Char() calls FT_Get_Char_Index() and FT_Load_Glyph().
6. "For optional rendering on a screen the bitmap should be used as an alpha channle
   in linear blending with gamma correction." --- FreeType Tutorial 1

7. 				-----  WARNING  -----
   Due to lack of Gamma Correction(?) in color blend macro, there may be some dark/gray
   lines/dots after font bitmap and backgroud blending, especially for two contrasting
   colors/bright colors. Select a dark color for font and/or a compatible color for backgroud to
   weaken the effect.


Midas Zhou
midaszhou@yahoo.com
--------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <wchar.h>
#include <freetype2/ft2build.h>
#include "egi_common.h"
#include "egi_image.h"
#include "egi_utils.h"

#include FT_FREETYPE_H


//////////////// TODO: configure Openwrt  to support LOCAL ////////////
/*---------------------------------------------------
convert a multibyte string to a wide-character string

Note:
	Do not forget to free it after use.

Return:
	Pointer 	OK
	NULL		Fail
---------------------------------------------------*/
wchar_t * cstr_to_wcstr(const char *cstr)
{
	wchar_t *wcstr;
	int mbslen;

	if( cstr==NULL )
		return NULL;

  	/* Apply default locale TODO: not workable in Openwrt */
#if 1
        if ( setlocale(LC_ALL, "en_US.utf8") == NULL) {
		printf("%s: fail to setlocale!\n",__func__);
	        return NULL;
        }
#endif

        //mbslen = strlen(cstr);
	//printf("%s len is %d\n",cstr,mbslen);
#if 1
	mbslen = mbstowcs(NULL, cstr, 0);
        if ( mbslen == (size_t) -1 ) {
		printf("%s: mbslen=mbstowcs() invalid!\n",__func__);
	        return NULL;
        }
#endif

 	/* Add 1 to allow for terminating null wide character (L'\0'). */
        wcstr = calloc(mbslen + 1, sizeof(wchar_t));
        if (wcstr == NULL) {
		printf("%s: calloc() fails!\n",__func__);
		return NULL;
        }

	/* convert to wide-character string */
        if ( mbstowcs(wcstr, cstr, mbslen+1) == (size_t) -1 ) {
		perror("mbstowcs");
		printf("%s: mbstowcs() fails!\n",__func__);
		return NULL;
        }

	return wcstr;
}



/*------------------------------------------------------------------------------
        	   Add FreeType FT_Bitmap to EGI imgbuf

1. The input eimg should has been initialized by egi_imgbuf_init()
   with certain size of a canvas (height x width) inside.

2. The canvas size of the eimg shall be big enough to hold the bitmap,
   or pixels out of the canvas will be omitted.

@eimg		The EGI_IMGBUF to hold the bitmap
@xb,yb		coordinates of bitmap relative to EGI_IMGBUF canvas coord,
		left top as origin.
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

			/* buffer value(0-255) is gray value/alpha value */
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


/* ---------------     FT_GlyphSlot   ----------------------

typedef struct FT_GlyphSlotRec_*  FT_GlyphSlot;
typedef struct  FT_GlyphSlotRec_
  {
    FT_Library        library;
    FT_Face           face;
    FT_GlyphSlot      next;
    FT_UInt           reserved;       	retained for binary compatibility
    FT_Generic        generic;
    FT_Glyph_Metrics  metrics;
    FT_Fixed          linearHoriAdvance;
    FT_Fixed          linearVertAdvance;
    FT_Vector         advance;
    FT_Glyph_Format   format;
    FT_Bitmap         bitmap;
    FT_Int            bitmap_left;
    FT_Int            bitmap_top;
    FT_Outline        outline;
    FT_UInt           num_subglyphs;
    FT_SubGlyph       subglyphs;
    void*             control_data;
    long              control_len;
    FT_Pos            lsb_delta;
    FT_Pos            rsb_delta;
    void*             other;
    FT_Slot_Internal  internal;
  } FT_GlyphSlotRec;
---------------------------------------------*/

int main( int  argc,   char**  argv )
{
  FT_Library    library;
  FT_Face       face;		/* NOTE: typedef struct FT_FaceRec_*  FT_Face */
  FT_Face	face2;
  FT_CharMap*   pcharmaps=NULL;
  char		tag_charmap[6]={0};
  uint32_t	tag_num;

  FT_GlyphSlot  slot;
  FT_Matrix     matrix;         /* transformation matrix */
  FT_Vector     pen;            /* untransformed origin  */
  int		line_starty;    /* starting pen Y postion for rotated lines */
  FT_Error      error;

  char*         font_path;
  char*         text;

  char**	fpaths;
  int		count;
  char**	picpaths;
  int		pcount;

  //wchar_t	*wcstr;
  EGI_16BIT_COLOR font_color;
  int		glyph_index;
  int		deg; /* angle in degree */
  double        angle;
  int           n, num_chars;
  int 		i,j,k;
  int		np;
  float		step;
  int		ret;

#if 0  ////////////////  ENCODING TEST  /////////////////////
  wchar_t *words=L"严严";
  printf("0x%X\n", words[0]); 		    /* Print Unicode */
  printf("0x%X\n", words[1]);
  printf("0x%X\n", *(wchar_t *)(argv[1]) ); /* print UFT-8 LE, as argv is UFT-8 LE */
	exit(1);
#endif ////////////////  END ENCODING TEST  ////////////////

  EGI_IMGBUF  *eimg=NULL;
  eimg=egi_imgbuf_new();

  EGI_IMGBUF *logo_img=egi_imgbuf_new();
  if( egi_imgbuf_loadpng("/mmc/logo_openwrt.png", logo_img) )
		return -1;
  EGI_IMGBUF *pinguin_img=egi_imgbuf_new();
  if( egi_imgbuf_loadpng("/mmc/pinguin.png", pinguin_img) )
		return -1;


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


#if 1
//    wchar_t *wcstr=L"Heloolsdfsdf";
   wchar_t *wcstr=L"長亭外古道邊,芳草碧連天,晚風拂柳笛聲殘,夕陽山外山.";   /* for traditional chinese */
//   wchar_t *wcstr=L"长亭外古道边,芳草碧连天,晚风拂柳笛声残,夕阳山外山."; /* for simple chinese */
   char    *cstr="abcdefghijkABCDEFGHIJK";

#else  /* LOCALE configure fails */
   wchar_t *wcstr=NULL;
   wcstr=cstr_to_wcstr(argv[2]);
   if(wcstr==NULL)
	exit(1);
#endif

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
	if(j==count)
		j=0;

	np=egi_random_max(pcount)-1;

	printf(" <<<<<<<< fpaths[%d] DEMO FONT TYPE: '%s' >>>>>>> \n",j, fpaths[j]);

	/* 1. initialize FT library */
	error = FT_Init_FreeType( &library );
	if(error) {
		printf("%s: An error occured during FreeType library initialization.\n",__func__);
		return -1;
	}

	/* 2. create face object, face_index=0 */
 	//error = FT_New_Face( library, font_path, 0, &face );
 	error = FT_New_Face( library, fpaths[0], 0, &face2 );
	if(error==FT_Err_Unknown_File_Format) return -1;

 	error = FT_New_Face( library, fpaths[j], 0, &face );
	if(error==FT_Err_Unknown_File_Format) {
		printf("%s: Font file opens, but its font format is unsupported!\n",__func__);
		FT_Done_FreeType( library );
		return -2;
	}
	else if ( error ) {
		printf("%s: Fail to open or read font '%s'.\n",__func__, font_path);
		return -3;
	}
  	/* get pointer to the glyph slot */
  	slot = face->glyph;

	printf("-------- Load font '%s', index[0] --------\n", font_path);
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
	/* ---- print charmaps -----*/
	printf("   num_charmaps:	%d\n",	face->num_charmaps);
	for(i=0; i< face->num_charmaps; i++) { /* print all charmap tags */
		pcharmaps=face->charmaps;
		tag_num=htonl((uint32_t)(*pcharmaps)->encoding );
		memcpy( tag_charmap, &tag_num, 4); /* 4 bytes */
		printf("      			[%d] %s\n", i,tag_charmap ); /* unic as for Unicode */
		pcharmaps++;
	}
	tag_num=htonl((uint32_t)( face->charmap->encoding));
	memcpy( tag_charmap, &tag_num, 4); /* 4 bytes */
	printf("   charmap in use:	%s\n", tag_charmap ); /* unic as for Unicode */

	/* vertical distance between two consective lines */
	printf("   height(font units):	%d\n",	face->height);


deg=15-egi_random_max(30);
/*
////////////////     LOOP TEST     /////////////////
for( deg=0; deg<90; deg+=10 )
{
	if(deg>70){
		 deg=-5;
		 continue;
	}
*/

  /* Init eimg before load FTbitmap, old data erased if any. */
  //egi_imgbuf_init(eimg, 320, 240);

   /* load a pic to EGI_IMGBUF as for backgroup */
   ret=egi_imgbuf_loadpng( picpaths[np], eimg);
   if(ret!=0) {
	printf(" Fail to load png file '%s'\n", picpaths[np]);
	continue;
   }
   else {
	printf(" png '%s', HxW=%dx%d \n", picpaths[np], eimg->height, eimg->width);
   }


  /* 3. set up matrix for transformation */
  angle     = ( deg / 360.0 ) * 3.14159 * 2;
  printf(" ------------- angle: %d, %f ----------\n", deg, angle);
  matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
  matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
  matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
  matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

  /* 4. set pen position
   * the pen position in 26.6 cartesian space coordinates
     64 units per pixel for pen.x and pen.y   		  */
  pen.x = 5*64; //   300 * 64;
  pen.y = 5*64;  //   ( target_height - 200 ) * 64;
  line_starty=pen.y;

  /* clear screen */
  clear_screen(&gv_fb_dev, WEGI_COLOR_GRAYB);

  /* set font color */
  font_color= egi_color_random(light);

step=5.0; //1.25;
for(k=3; k<11; k++)	/* change font size */
{
   /* 5. set character size in pixels */
   error = FT_Set_Pixel_Sizes(face, step*k, step*k);
   /* OR set character size in 26.6 fractional points, and resolution in dpi */
   //error = FT_Set_Char_Size( face, 32*32, 0, 100,0 );

  //printf(" Size change [ k=%d ]\n",k);
//  printf(" len=%d, wcstr: %s\n", wcslen(wcstr), (char *)wcstr);
  pen.y=line_starty;
  //for ( n = 0; n < wcslen(wcstr); n++ )	/* load wchar and process one by one */
  for ( n = 0; n < strlen(cstr); n++ )	/* load wchar and process one by one */
  {
   	/* 6. re_set transformation before loading each wchar,
	 *    as pen postion and transfer_matrix may changed.
         */
    	FT_Set_Transform( face, &matrix, &pen );

	/* 7. load char and render to bitmap  */

//	glyph_index = FT_Get_Char_Index( face, wcstr[n] ); /*  */
//	printf("charcode[%X] --> glyph index[%d] \n", wcstr[n], glyph_index);
#if 1
	/*** Option 1:  FT_Load_Char( FT_LOAD_RENDER )
    	 *   load glyph image into the slot (erase previous one)
	 */
    	//error = FT_Load_Char( face, wcstr[n], FT_LOAD_RENDER );
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
    	//error = FT_Load_Char( face, wcstr[n], FT_LOAD_DEFAULT );
    	error = FT_Load_Char( face, cstr[n], FT_LOAD_DEFAULT );
    	if ( error ) {
		printf("%s: FT_Load_Char() error!\n");
      		continue;                 /* ignore errors */
	}

        FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL) ;
	/* ALSO_Ok: _LIGHT, _NORMAL; 	BlUR: _MONO	LIMIT: _LCD */
#endif


	/* 8. Draw to EGI_IMGBUF */
	ret=egi_imgbuf_blend_FTbitmap(eimg, slot->bitmap_left, 320-slot->bitmap_top,
						&slot->bitmap, font_color); //egi_color_random(deep) );
	if(ret!=0) {
		printf(" Fail to fetch Font type '%s' char index [%d]\n", fpaths[j], n);
	}

    	/* 9. increment pen position */
//	printf("bitmap_left=%d, bitmap_top=%d. \n",slot->bitmap_left, slot->bitmap_top);
//	printf("slot->advance.y=%d\n",slot->advance.y);
//	printf("slot->bitmap.rows=%d\n", slot->bitmap.rows);
    	pen.x += slot->advance.x;
    	pen.y += slot->advance.y; /* same in a line for the same char  */
  }
	/* 10. skip pen to next line */
	pen.x = 3<<6;

	line_starty += ((int)(step*k)+15)<<6; /* for inclined lines */
	//pen.y += ((k+5)<<6); /* 1 pixel= 64 units glyph metrics */

}  /* end for() of size change */


/*-------------------------------------------------------------------------------------------
int egi_imgbuf_windisplay(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
// no subcolor, no FB filo
int egi_imgbuf_windisplay2(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
----------------------------------------------------------------------------------------------*/

	/* put a logo */
	egi_imgbuf_blend_imgbuf(eimg, 10, 10, logo_img);
	egi_imgbuf_blend_imgbuf(eimg, 240-85, 320-85, pinguin_img);

	/* Dispay EGI_IMGBUF */
	egi_imgbuf_windisplay2(eimg, &gv_fb_dev, 0, 0, 0, 0, eimg->width, eimg->height);

	 tm_delayms(2000);


/*
}
////////////////     END LOOP TEST     /////////////////
*/


FT_FAILS:
  FT_Done_Face    ( face );
  FT_Done_Face    ( face2 );
  FT_Done_FreeType( library );


}

/* >>>>>>>>>>>>>>>>>  END FONTS DEMON TEST  >>>>>>>>>>>>>>>>>>>> */

 	/* free fpaths buffer */
        egi_free_buff2D((unsigned char **) fpaths, count);
        egi_free_buff2D((unsigned char **) picpaths, pcount);

	/* free EGI_IMGBUF */
	egi_imgbuf_free(eimg);
	egi_imgbuf_free(logo_img);
	egi_imgbuf_free(pinguin_img);

	/* Free EGI */
        release_fbdev(&gv_fb_dev);
        symbol_release_allpages();
        egi_quit_log();

  return 0;
}

/* EOF */
