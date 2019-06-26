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

TODO:
	1. Bitmap alpha value and gray value relationship?
	2. Fonts gap?


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

#define WIDTH   110 //640
#define HEIGHT  40  //480

/* origin is the upper left corner */
unsigned char image[HEIGHT][WIDTH];


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





/*------------------------------------------------------------------------
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
------------------------------------------------------------------------------*/
int egi_imgbuf_draw_FTbitmap(EGI_IMGBUF* eimg, int xb, int yb, FT_Bitmap *bitmap,
								EGI_16BIT_COLOR subcolor)
{
	int i,j;
	EGI_16BIT_COLOR color;
	unsigned char alpha;
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

			alpha=bitmap->buffer[i*bitmap->width+j];

			if(subcolor>=0)
				color=subcolor;
			else
				color=COLOR_RGB_TO16BITS(alpha,alpha,alpha);

			/* assig data to eimg->imgbuf and alpha */
			pos=(yb+i)*(eimg->width) + xb+j;
			eimg->imgbuf[pos]=color;
			eimg->alpha[pos]=alpha; //(alpha>0 ? 255:0); //alpha;
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
  FT_Error      error;

  char*         filename;
  char*         text;

//  wchar_t	*wcstr;

  int		glyph_index;
  double        angle;
  int           target_height;
  int           n, num_chars;
  int 		k,i;


  EGI_IMGBUF  *eimg=NULL;
  eimg=egi_imgbuf_new();
  egi_imgbuf_init(eimg, 320, 240);


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

  	filename      = argv[1];                           /* first argument     */
  	text          = argv[2];                           /* second argument    */
  	num_chars     = strlen( text );
  	angle         = ( 0.0 / 360 ) * 3.14159 * 2;      /* use 25 degrees     */

	/* 1. initialize FT library */
	error = FT_Init_FreeType( &library );
	if(error) {
		printf("%s: An error occured during FreeType library initialization.\n",__func__);
		return -1;
	}

	/* 2. create face object, face_index=0 */
 	error = FT_New_Face( library, filename, 0, &face );
	if(error==FT_Err_Unknown_File_Format) {
		printf("%s: Font file opens, but its font format is unsupported!\n",__func__);
		FT_Done_FreeType( library );
		return -2;
	}
	else if ( error ) {
		printf("%s: Fail to open or read font '%s'.\n",__func__, filename);
		return -3;
	}

	printf("-------- Font '%s', Index 0 --------\n", filename);
	printf("   num_faces:		%d\n",face->num_faces);
	printf("   face_index:		%d\n",face->face_index);
	printf("   style name:		%s\n",face->style_name);
	printf("   num_glyphs:		%d\n",face->num_glyphs);
	printf("   face_flags: 		0x%08X\n",face->face_flags);
	printf("   units_per_EM:	%d\n",face->units_per_EM);
	printf("   num_fixed_sizes:	%d\n",face->num_fixed_sizes);
	if(face->num_fixed_sizes !=0 ) {
		for(i=0; i< face->num_fixed_sizes; i++) {
			printf("[%d]: H%d x W%d\n",i, face->available_sizes[i].height,
							    face->available_sizes[i].width);
		}
	}
	printf("   num_charmaps:	%d\n",face->num_charmaps);
	printf("   height:		%d\n",face->height);

/*------------------------------------------------------------
	FT_Set_Char_Size( FT_Face     face,
        	            FT_F26Dot6  char_width,
                	    FT_F26Dot6  char_height,
	                    FT_UInt     horz_resolution,
        	            FT_UInt     vert_resolution );

	FT_Set_Pixel_Sizes( FT_Face  face,
        	              FT_UInt  pixel_width,
                	      FT_UInt  pixel_height );

-------------------------------------------------------------*/

  /* use 50pt at 100dpi */
  //  error = FT_Set_Char_Size( face, 32*32, 0, //50 * 64, 0,  /*  W*H */
  //                           100, 0 );                /* set character size */
//  error = FT_Set_Pixel_Sizes(face, 32, 32);

  /* error handling omitted */

  slot = face->glyph;

  /* set up matrix */
  matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
  matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
  matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
  matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

  /* the pen position in 26.6 cartesian space coordinates; */
  /* start at (300,200) relative to the upper left corner  */
  /* 1/64 uint for pen.x/pen.y   			    */
  pen.x = 5*64; //   300 * 64;
  pen.y = 5*64;  //   ( target_height - 200 ) * 64;

  printf("%s\n",argv[2]);

#if 1
  wchar_t *wcstr=L"长亭外古道边,芳草碧连天,晚风拂柳笛声残,夕阳山外山.";
#else
   wchar_t *wcstr=NULL;
  wcstr=cstr_to_wcstr(argv[2]);
  if(wcstr==NULL)
	exit(1);
#endif


  /* --------------------------------------------
  FT_Get_Char_Index( FT_Face   face,
        	             FT_ULong  charcode );
  --------------------------------------------- */

clear_screen(&gv_fb_dev, WEGI_COLOR_GRAYB);

for(k=6; k<25; k++)
//k=8;
{

   /* set character size in pixels */
   error = FT_Set_Pixel_Sizes(face, 1.25*k, 1.25*k);
   /* set character size in 26.6 fractional points, and resolution in dpi */
   //error = FT_Set_Char_Size( face, 32*32, 0, 100,0 );

  printf("---------------- k=%d -------------\n",k);
  for ( n = 0; n < wcslen(wcstr); n++ )
  {
    	/* set transformation */
    	FT_Set_Transform( face, &matrix, &pen );

#if 1
    	/* load glyph image into the slot (erase previous one) */
    	error = FT_Load_Char( face, wcstr[n], FT_LOAD_RENDER );
    	if ( error ) {
		printf("%s: FT_Load_Char() error!\n");
      		continue;                 /* ignore errors */
	}
#else

//	charcode=FT_Get_Char_Index( face, wchar);
//      printf(" 周 charcode=%ld\n", charcode);
    	error = FT_Load_Char( face, wcstr[n], FT_LOAD_DEFAULT );
    	if ( error ) {
		printf("%s: FT_Load_Char() error!\n");
      		continue;                 /* ignore errors */
	}

//	FT_Load_Glyph( face, glyph_index,
//                    load_flags | FT_LOAD_TARGET_LIGHT );

        FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL) ;
	// ALSO_Ok: _LIGHT, _NORMAL; 	BlUR: _MONO	LIMIT: _LCD
#endif


  /*    If @FT_Load_Glyph is called with default flags (see                */
  /*    @FT_LOAD_DEFAULT) the glyph image is loaded in the glyph slot in   */
  /*    its native format (e.g., an outline glyph for TrueType and Type~1  */
  /*    formats).                                                          */
  /*                                                                       */
  /*    This image can later be converted into a bitmap by calling         */
  /*    @FT_Render_Glyph.  This function finds the current renderer for    */
  /*    the native image's format, then invokes it.                        */
  /*                                                                       */
  /*    The renderer is in charge of transforming the native image through */
  /*    the slot's face transformation fields, then converting it into a   */
  /*    bitmap that is returned in `slot->bitmap'.                         */
  /*                                                                       */
  /*    Note that `slot->bitmap_left' and `slot->bitmap_top' are also used */
  /*    to specify the position of the bitmap relative to the current pen  */
  /*    position (e.g., coordinates (0,0) on the baseline).  Of course,    */
  /*    `slot->format' is also changed to @FT_GLYPH_FORMAT_BITMAP.         */

	/* Draw to EGI_IMGBUF */
	egi_imgbuf_draw_FTbitmap(eimg, slot->bitmap_left, 320-slot->bitmap_top,
						&slot->bitmap, WEGI_COLOR_BLACK); //ORANGE);

    	/* increment pen position */
	printf("bitmap_left=%d, bitmap_top=%d. \n",slot->bitmap_left, slot->bitmap_top);
	printf("slot->advance.y=%d\n",slot->advance.y);
	printf("slot->bitmap.rows=%d\n", slot->bitmap.rows);
    	pen.x += slot->advance.x;
    	pen.y += slot->advance.y;
  }
	pen.x = 5<<6;
	pen.y += ((k+5)<<6); /* 1 pixel= 64 units glyph metrics */
}

// 	 show_image();

/*-------------------------------------------------------------------------------------------
int egi_imgbuf_windisplay(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
// no subcolor, no FB filo
int egi_imgbuf_windisplay2(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
----------------------------------------------------------------------------------------------*/

	/* Dispay EGI_IMGBUF */
	egi_imgbuf_windisplay2(eimg, &gv_fb_dev, 0, 0, 0, 0, eimg->width, eimg->height);


FT_FAILS:
  FT_Done_Face    ( face );
  FT_Done_FreeType( library );


	/* Free EGI */
        release_fbdev(&gv_fb_dev);
        symbol_release_allpages();
        egi_quit_log();

  return 0;
}

/* EOF */
