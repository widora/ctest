/* example1.c                                                      */
/*                                                                 */
/* This small program shows how to print a rotated string with the */
/* FreeType 2 library.                                             */


/*---------------------------------------------------------------------------


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


   FT_Get_Char_Index( FT_Face   face,
        	            FT_ULong  charcode );
   charcode=FT_Get_Char_Index( face, wchar);


TODO:
	1. Bitmap alpha value and gray value relationship?

Note:
1. Default code is UFT-8 for both input and output, as also for FreeType
   default set.
 
Midas Zhou
------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <wchar.h>
#include <freetype2/ft2build.h>
#include "egi_common.h"
#include "egi_image.h"

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



/*-------------------------------------------------------------------------------
        	   Draw FreeType  FT_Bitmap to EGI imgbuf

1. The input eimg should has been initialized by egi_imgbuf_init()
   with certain size of a canvas (height x width) inside.

2. The canvas size of the eimg shall be big enough to hold the bitmap,
   or the pixels out of the canvas will be omitted.

@eimg		The EGI_IMGBUF to hold the bitmap
@xb,yb		coordinates of bitmap relative to EGI_IMGBUF canvas coord,
		left top as origin.
@bitmap		pointer to a bitmap in a FT_GlyphSlot.
@subcolor	>=0 as substituting color
		<0  use bitmap buffer data as gray value. 0-255 (BLACK-WHITE)

return:
	0	OK
	<0	fails
--------------------------------------------------------------------------------*/
int egi_imgbuf_load_FTbitmap(EGI_IMGBUF* eimg, int xb, int yb, FT_Bitmap *bitmap,
								EGI_16BIT_COLOR subcolor)
{
	int i,j;
	EGI_16BIT_COLOR color;
	unsigned char alpha;
	int	sumalpha;
	int pos;

	if(eimg==NULL | eimg->imgbuf==NULL) {
		printf("%s: input EGI_IMBUG is NULL or uninitiliazed!\n", __func__);
		return -1;
	}
	if( bitmap==NULL || bitmap->buffer==NULL ) {
		printf("%s: input FT_Bitmap or its buffer is NULL!\n", __func__);
		return -2;
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



int
main( int     argc,
      char**  argv )
{
  FT_Library    library;
  FT_Face       face;		/* NOTE: typedef struct FT_FaceRec_*  FT_Face */

  FT_GlyphSlot  slot;
  FT_Matrix     matrix;         /* transformation matrix */
  FT_Vector     pen;            /* untransformed origin  */
  int		line_starty;    /* starting pen Y postion for rotated lines */
  FT_Error      error;

  char*         font_path;
  char*         text;

  //wchar_t	*wcstr;

  int		glyph_index;
  int		deg; /* angle in degree */
  double        angle;
  int           n, num_chars;
  int 		k,i;


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


#if 1
//    wchar_t *wcstr=L"Heloolsdfsdf";
  wchar_t *wcstr=L"长亭外古道边,芳草碧连天,晚风拂柳笛声残,夕阳山外山.";
#else

   wchar_t *wcstr=NULL;
   wcstr=cstr_to_wcstr(argv[2]);
   if(wcstr==NULL)
	exit(1);
#endif



	/* 1. initialize FT library */
	error = FT_Init_FreeType( &library );
	if(error) {
		printf("%s: An error occured during FreeType library initialization.\n",__func__);
		return -1;
	}

	/* 2. create face object, face_index=0 */
 	error = FT_New_Face( library, font_path, 0, &face );
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

	printf("-------- Load font '%s', Index 0 --------\n", font_path);
	printf("   num_faces:		%d\n",	face->num_faces);
	printf("   face_index:		%d\n",	face->face_index);
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
	printf("   num_charmaps:	%d\n",	face->num_charmaps);
	printf("   height:		%d\n",	face->height);



deg=0;
////////////////     LOOP TEST     /////////////////
for( deg=0; deg<90; deg+=10 )
{
	if(deg>70){
		 deg=-5;
		 continue;
	}

  /* Init eimg before load FTbitmap, old data erased if any. */
  egi_imgbuf_init(eimg, 320, 240);


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
  pen.y = 5*64-150*64;  //   ( target_height - 200 ) * 64;
  line_starty=pen.y;

  /* clear screen */
  clear_screen(&gv_fb_dev, WEGI_COLOR_GRAYB);

for(k=6; k<35; k++)	/* change font size */
{
   /* 5. set character size in pixels */
   error = FT_Set_Pixel_Sizes(face, 1.25*k, 1.25*k);
   /* OR set character size in 26.6 fractional points, and resolution in dpi */
   //error = FT_Set_Char_Size( face, 32*32, 0, 100,0 );

  printf("---------------- k=%d -------------\n",k);
//  printf(" len=%d, wcstr: %s\n", wcslen(wcstr), (char *)wcstr);
  pen.y=line_starty;
  for ( n = 0; n < wcslen(wcstr); n++ )	/* load wchar and process one by one */
  {
   	/* 6. re_set transformation before loading each wchar,
	 *    as pen postion and transfer_matrix may changed.
         */
    	FT_Set_Transform( face, &matrix, &pen );

	/* 7. load char and render to bitmap  */
#if 1
	/*** Option 1:  FT_Load_Char( FT_LOAD_RENDER )
    	 *   load glyph image into the slot (erase previous one)
	 */
    	error = FT_Load_Char( face, wcstr[n], FT_LOAD_RENDER );
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
    	error = FT_Load_Char( face, wcstr[n], FT_LOAD_DEFAULT );
    	if ( error ) {
		printf("%s: FT_Load_Char() error!\n");
      		continue;                 /* ignore errors */
	}

        FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL) ;
	/* ALSO_Ok: _LIGHT, _NORMAL; 	BlUR: _MONO	LIMIT: _LCD */
#endif

	/* 8. Draw to EGI_IMGBUF */
	egi_imgbuf_load_FTbitmap(eimg, slot->bitmap_left, 320-slot->bitmap_top,
						&slot->bitmap, egi_color_random(deep) ); //WEGI_COLOR_BLACK); //ORANGE);

    	/* 9. increment pen position */
//	printf("bitmap_left=%d, bitmap_top=%d. \n",slot->bitmap_left, slot->bitmap_top);
//	printf("slot->advance.y=%d\n",slot->advance.y);
//	printf("slot->bitmap.rows=%d\n", slot->bitmap.rows);
    	pen.x += slot->advance.x;
    	pen.y += slot->advance.y; /* same in a line for the same char  */
  }
	/* 10. skip pen to next line */
	pen.x = 5<<6;
	line_starty += ((k+5)<<6); /* for inclined lines */
	//pen.y += ((k+5)<<6); /* 1 pixel= 64 units glyph metrics */
}


/*-------------------------------------------------------------------------------------------
int egi_imgbuf_windisplay(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
// no subcolor, no FB filo
int egi_imgbuf_windisplay2(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
----------------------------------------------------------------------------------------------*/

	/* Dispay EGI_IMGBUF */
	egi_imgbuf_windisplay2(eimg, &gv_fb_dev, 0, 0, 0, 0, eimg->width, eimg->height);


  tm_delayms(1000);
}
////////////////     END LOOP TEST     /////////////////


FT_FAILS:
  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

	/* free EGI_IMGBUF */
	egi_imgbuf_free(eimg);

	/* Free EGI */
        release_fbdev(&gv_fb_dev);
        symbol_release_allpages();
        egi_quit_log();

  return 0;
}

/* EOF */
