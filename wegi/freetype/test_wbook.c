/*-------------------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License version 2 as published by the Free Software
Foundation.
(Note: FreeType 2 is licensed under FTL/GPLv3 or GPLv2 ).

An Example of fonts demonstration based on example1.c in FreeType 2 library docs.


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

bitmap_left:		A member of FT_GlyphSlot, the bitmap's left bearing expressed in integer pixels,
			only valid if the format is BITMAP. It's the distance from the origin to the
			leftmost border of the glyph image.

bitmap_top:		A member of FT_GlyphSlot, the bitmap's top bearing expressed in integer pixels.
			It's the distance from the baseline to the top-most glyph scanline.

units_per_EM            The number of font units per EM square for a face, relevant for scalable
		        fonts only. Noticed that all characters have the same height.
		        Typically 1000 for type-1 fonts and 1024 or 2048 for TrueType fonts.

DPI:			Dot per inch, 1 point = 1/72 inch.
PPI:			Pixel per inch, pixel_size = point_size * DPI / 72
26.6 pixel format:	= 1/64th of a pixel per unit, where 64=2^6
			And fixed point type 16.16 is 1/65536 per unit, where 65536=2^16


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

UNICODE HAN DATABASE(UNIHAN): Unicode Standard Annex #38 http://www.unicode.org/reports/tr38/tr38-27.html
Basic Han Unicode range: U+4E00 - U+9FA5

			<<<	--- Some Open/Free Fonts --- 	>>>

Source_Han_San:		A set of OpenType/CFF Pan-CJK fonts. 		( SIL v1.1 )
Source_Han_Serif:	A set of OpenType/CFF Pan-CJK fonts. 		( SIL v1.1 )
Wang_Han_Zong:		A set of traditional/simple chinese fonts.	( GPL v2 )

Liberation-fonts:	A set of western fonts whose layout is compatible to Times New Roman,
			Arial, and Courier New. 			( SIL v1.1 )
Bitstream Vera Fonts	A set of western fonts for GNOME. 	( Bitstream Vera Fonts Copyright )


Notes:

1.  Default encoding environment shall be set to be the same, UFT-8 etc, for both codes compiling
    and running.
2.  Refer to: zenozeng.github.io/Free-Chinese-Fonts for more GPL fonts.
3.  Input text with chinese traditional style when use some chinese fonts, like
    WHZ fonts etc, or it may not be found in its font library.
4.  Source Han Sans support both simple and traditional Chineses text encoding.
5.  FT_Load_Char() calls FT_Get_Char_Index() and FT_Load_Glyph().
6.  "For optional rendering on a screen the bitmap should be used as an alpha channle
    in linear blending with gamma correction." --- FreeType Tutorial 1

7. 				-----  WARNING  -----
    Due to lack of Gamma Correction(?) in color blend macro, there may be some dark/gray
    lines/dots after font bitmap and backgroud blending, especially for two contrasting
    colors/bright colors. For example, white letters and black canvas.
    Select a dark color for font and/or a compatible color for backgroud to weaken the effect.

8.  TODO: Use libiconv to convert between char and wchar_t with different encodings.
9.  This demonstration program is only for scalable fonts! You must make some modification to
    apply for fixed type fonts.
10. Only horizontal text layout is considered.
11. Gap between two characters in horizontal text layout is included/presented in advance.x.

Midas Zhou
midaszhou@yahoo.com
--------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <wchar.h>
#include <sys/stat.h>
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>
#include "egi_common.h"
#include "egi_image.h"
#include "egi_utils.h"

#include FT_FREETYPE_H


/*-----------------------------------------------------------------------------------------------
1. Render a CJK character in UNICODE according to the specified face type, then write the bitmap
   to FB.
2. The caller shall check and skip unprintable symbols, and parse ASCII control codes.
   This function only deal with symbol bitmap if it exists.
3. Xleft will be subtrated by slot->advance.x first anyway, then write to FB only if Xleft >=0.
4. Doundary box is defined as bbox_W=MAX(advanceX,bitmap.width) bbox_H=fh.
5. CJK wchars may extrude the BBOX a little, and ASCII alphabets will extrude more, especially for
   'j','g',..etc, so keep enough gap between lines.
6. Or to get symheight just as in symbol_load_asciis_from_fontfile() as BBOX_H. However it's maybe
   a good idea to display CJK and wester charatcters separately by calling differenct functions.
   i.e. symbol_writeFB() for alphabets and symbol_unicode_writeFB() for CJKs.

@fbdev:         FB device
@face:          A face object in FreeType2 libliary.
@fh,fw:		Height and width of the wchar.
@wcode:		UNICODE number for the character.
@xleft:		Pixel space left in FB X direction (horizontal writing)
		Xleft will be subtrated by slot->advance.x first anyway, If xleft<0 then, it aborts.
@x0,y0:		Left top coordinate of the character bitmap,relative to FB coord system.
@transpcolor:   >=0 transparent pixel will not be written to FB, so backcolor is shown there.
                <0       no transparent pixel

@fontcolor:     font color(symbol color for a symbol)
                >= 0,  use given font color.
                <0  ,  use default color in img data.

use following COLOR:
#define SYM_NOSUB_COLOR -1  --- no substitute color defined for a symbol or font
#define SYM_NOTRANSP_COLOR -1  --- no transparent color defined for a symbol or font

@opaque:        >=0     set aplha value (0-255) for all pixels, and alpha data in sympage
			will be ignored.
                <0      Use symbol alpha data, or none.
                0       100% back ground color/transparent
                255     100% front color

--------------------------------------------------------------------------------------------------*/
void symbol_unicode_writeFB(FBDEV *fb_dev, FT_Face face, int fw, int fh, wchar_t wcode, int *xleft,
				int x0, int y0, int fontcolor, int transpcolor,int opaque)
{
  	FT_Error      error;
  	FT_GlyphSlot slot = face->glyph;
	struct symbol_page ftsympg={0};	/* a symbol page to hold the character bitmap */
	ftsympg.symtype=symtype_FT2;

	int advanceX;   /* X advance in pixels */
	int bbox_W;	/* boundary box width, taken bbox_H=fh */
	int delX;	/* adjust bitmap position in boundary box, according to bitmap_top */
	int delY;

	/* set character size in pixels */
	error = FT_Set_Pixel_Sizes(face, fw, fh);
   	/* OR set character size in 26.6 fractional points, and resolution in dpi
   	   error = FT_Set_Char_Size( face, 32*32, 0, 100,0 ); */
	if(error) {
		printf("%s: FT_Set_Pixel_Sizes() fails!\n",__func__);
		return;
	}

	/* Do not set transform, keep up_right and pen position(0,0)
    		FT_Set_Transform( face, &matrix, &pen ); */

	/* Load char and render */
    	error = FT_Load_Char( face, wcode, FT_LOAD_RENDER );
    	if (error) {
		printf("%s: FT_Load_Char() fails!\n");
		return;
	}

	/* Check whether xleft is used up first. */
	advanceX = slot->advance.x>>6;
	bbox_W = (advanceX > slot->bitmap.width ? advanceX : slot->bitmap.width);
	*xleft -= bbox_W;
	if( *xleft < 0 )
		return;
	/* taken bbox_H as fh */

	/* Assign alpha to ftsympg, Ownership IS NOT transfered! */
	ftsympg.alpha 	  = slot->bitmap.buffer;
	ftsympg.symheight = slot->bitmap.rows; //fh; /* font height in pixels is bigger than bitmap.rows! */
	ftsympg.ftwidth   = slot->bitmap.width; /* ftwidth <= (slot->advance.x>>6) */

#if 0 /* ----TEST: Display Boundary BOX------- */
	/* Note: Assume boundary box start from x0,y0(same as bitmap)
	 */
	draw_rect(fb_dev, x0, y0, x0+bbox_W, y0+fh );

#endif
	/* adjust bitmap position relative to boundary box */
	delX= slot->bitmap_left;
	delY= -slot->bitmap_top + fh;

	/* write to FB,  symcode =0, whatever  */
	symbol_writeFB(fb_dev, &ftsympg, fontcolor, transpcolor, x0+delX, y0+delY, 0, -1);
}


/*-----------------------------------------------------------------------
Get length of a character with UTF-8 encoding.

@cp:	A pointer to a character with UTF-8 encoding.

Note:
1. UTF-8 maps to UNICODE according to following relations:
				(No endianess problem for UTF-8 conding)

	   --- UNICODE ---	      --- UTF-8 CODING ---
	U+  0000 - U+  007F:	0XXXXXXX
	U+  0080 - U+  07FF:	110XXXXX 10XXXXXX
	U+  0800 - U+  FFFF:	1110XXXX 10XXXXXX 10XXXXXX
	U+ 10000 - U+ 1FFFF:	11110XXX 10XXXXXX 10XXXXXX 10XXXXXX

2. If illegal coding is found...

Return:
	>0	OK, string length in bytes.
	<0	Fails
------------------------------------------------------------------------*/
inline int cstr_charlen_uft8(const unsigned char *cp)
{
	if(cp==NULL)
		return -1;

	if(*cp < 0b10000000)
		return 1;	/* 1 byte */
	else if( *cp >= 0b11110000 )
		return 4;	/* 4 bytes */
	else if( *cp >= 0b11100000 )
		return 3;	/* 3 bytes */
	else if( *cp >= 0b11000000 )
		return 2;	/* 2 bytes */
	else
		return -2;	/* unrecognizable starting byte */
}



/*-----------------------------------------------------------------------
Get total number of characters in a string with UTF-8 encoding.

@cp:	A pointer to a string with UTF-8 encoding.

Note:
1. Count all ASCII characters, both printables and unprintables.

2. UTF-8 maps to UNICODE according to following relations:
				(No endianess problem for UTF-8 conding)

	   --- UNICODE ---	      --- UTF-8 CODING ---
	U+  0000 - U+  007F:	0XXXXXXX
	U+  0080 - U+  07FF:	110XXXXX 10XXXXXX
	U+  0800 - U+  FFFF:	1110XXXX 10XXXXXX 10XXXXXX
	U+ 10000 - U+ 1FFFF:	11110XXX 10XXXXXX 10XXXXXX 10XXXXXX


3. If illegal coding is found...

Return:
	>=0	OK, total numbers of character in the string.
	<0	Fails
------------------------------------------------------------------------*/
int cstr_strcount_uft8(const unsigned char *pstr)
{
	unsigned char *cp;
	int size;	/* in bytes, size of the character with UFT-8 encoding*/
	int count;

	if(pstr==NULL)
		return -1;

	cp=(unsigned char *)pstr;
	count=0;
	while(*cp) {
		size=cstr_charlen_uft8(cp);
		if(size>0) {
			//printf("%d\n",*cp);
			count++;
			cp+=size;
			continue;
		}
		else {
		   printf("%s: WARNGING! unrecognizable starting byte for UFT-8. Try to skip it.\n",__func__);
		   	cp++;
		}
	}

	return count;
}



#if 0 /////////////////// OBSELET ///////////////////////
int cstr_strlen_uft8(const unsigned char *pstr)
{
	unsigned char *cp;
	int count=0;

	if(cp==NULL)
		return -1;

	cp=(unsigned char *)pstr;
	while(*cp) {
		if( *cp>>7 == 0b0  ) {
#if 0 /*------  Exclude some unprintable ASCIIs  -----*/
	 		/* Only printable ASCII codes and NL(10), TAB.
			 * 9-TAB, 10-NextLine(NL), 13-ENTER(ER), 127-DEL
			 */
			if( ( *cp>=' ' && *cp !=127 ) || *cp=='\n' || *cp==9 ) {
				printf("offset=%d, ASCII: %d\n", cp-pstr, *cp);
				count++;
				cp++;
			}
#else /*------  Count all ASCIIs  -----*/
			/* count in all ASCII codes */
			count++;
			cp++;

#endif
		}
		else if ( *cp>>5 == 0b110 ) {
			count++;
			cp+=2; /* skip 2 byte to next starting byte */
		}
		else if (  *cp>>4 == 0b1110 ) {
			count++;
			cp+=3; /* skip 3 bytes */
		}
		else if ( *cp>>3 == 0b11110 ) {
			count++;
			cp+=4; /* skip 4 bytes */
		}
		/* Unrecognizable coding, carry on ... */
		else {
			printf("Warning!!! Unrecognizable starting byte 0x%x\n",(unsigned char)(*cp));
			cp++;
			continue;
		}
	}

	return count;
}
#endif ///////////////////////////////////////////////////////////////


/*----------------------------------------------------------------------
Convert a character from UFT-8 to UNICODE.

@src:	A pointer to characters with UFT-8 encoding.
@dest:	A pointer to mem space to hold converted characters with UNICODE.
	The caller shall allocate enough space for dest.
	!!!!TODO: Dest is now supposed to be in little-endian ordering. !!!!
@n:	Number of characters expected to be converted.

1. UTF-8 maps to UNICODE according to following relations:
				(No endianess problem for UTF-8 conding)

	   --- UNICODE ---	      --- UTF-8 CODING ---
	U+  0000 - U+  007F:	0XXXXXXX
	U+  0080 - U+  07FF:	110XXXXX 10XXXXXX
	U+  0800 - U+  FFFF:	1110XXXX 10XXXXXX 10XXXXXX
	U+ 10000 - U+ 1FFFF:	11110XXX 10XXXXXX 10XXXXXX 10XXXXXX


2. If illegal coding is found...

Return:
	>0 	OK, bytes of src consumed and converted into unicode.
	<0	Fails
-----------------------------------------------------------------------*/
inline int char_uft8_to_unicode(const unsigned char *src, wchar_t *dest)
{
	unsigned char *cp; /* tmp to dest */
	unsigned char *sp; /* tmp to src  */

	int size;	/* in bytes, size of the character with UFT-8 encoding*/

	if(src==NULL || dest==NULL )
		return -1;

	cp=(unsigned char *)dest;
	sp=(unsigned char *)src;

	size=cstr_charlen_uft8(src);

	if(size<0) {
		return size;	/* ERROR */
	}

	/* U+ 0000 - U+ 007F:	0XXXXXXX */
	else if(size==1) {
		*dest=0;
		*cp=*src;	/* The LSB of wchar_t */
	}

	/* U+ 0080 - U+ 07FF:	110XXXXX 10XXXXXX */
	else if(size==2) {
		/*dest=0;*/
		*dest= (*(sp+1)&0x3F) + ((*sp&0x1F)<<6);
	}

	/* U+ 0800 - U+ FFFF:	1110XXXX 10XXXXXX 10XXXXXX */
	else if(size==3) {
		*dest= (*(sp+2)&0x3F) + ((*(sp+1)&0x3F)<<6) + ((*sp&0xF)<<12);
	}

	/* U+ 10000 - U+ 1FFFF:	11110XXX 10XXXXXX 10XXXXXX 10XXXXXX */
	else if(size==4) {
		*dest= (*(sp+3)&0x3F) + ((*(sp+2)&0x3F)<<6) +((*(sp+2)&0x3F)<<12) + ((*sp&0x7)<<18);
	}

	return size;
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
  FT_BBox	bbox;		/* Boundary box */
  FT_Error      error;

  char*         font_path;

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
  uint32_t	ku; 	/* for unicode increament */
  float		step;
  int		ret;


  int fd;
  int fsize;
  struct stat sb;
  unsigned char *fp;
  int nread;

  unsigned char *text;
  int size;
  int fh,fw; /* font Height,Width */
  int gap;   /* line gap */
  int offp;
  int xleft;
  int px,py;
  int startx, starty;
  int wtotal;
  wchar_t *wtest=L"东方日出";


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
  	if ( argc < 2 )
  	{
    	fprintf ( stderr, "usage: %s font_path \n", argv[0] );
   	 	exit( 1 );
  	}

  	font_path      = argv[1];                           /* first argument     */
//  	text           = argv[2];                           /* second argument    */
//	if( strlen(text)<15 ) {
//		printf("Please enter a text more than 15 bytes!\n");
//		exit(1);
//	}

	/* open wchar book file */
	fd=open("/mmc/xyj_uft8.txt",O_RDONLY);
	if(fd<0) {
		perror("open file");
		return -1;
	}
	/* obtain file stat */
	if( fstat(fd,&sb)<0 ) {
		perror("fstat");
		return -2;
	}
	fsize=sb.st_size;
	/* mmap txt file */
	fp=mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
	if(fp==MAP_FAILED) {
		perror("mmap");
		return -3;
	}

/*--------- TEST: */
  printf("'\\n\':%d; '\\r':%d\n", '\n','\r');

	wchar_t wcstr[25]={0};
//       wchar_t *wcstr=NULL;
//     wchar_t *wcstr=L"abcdefghijJKCDEFGH";
//     wchar_t *wcstr=L"AJK長亭外古道邊,芳草碧連天,晚風拂柳笛聲殘,夕陽山外山.";   /* for traditional chinese */


#if 0 /*--------- TEST: input uft-8 text to wcstr */
  offp=0;
  for(i=0; i<15; i++) {
	    size=char_uft8_to_unicode(text+offp, wcstr+i);
	    printf(" Size=%d\n", size);
	    if(size>0)
		    offp+=size;
	    else {
		i--;
		offp++;
	    }
  }
 // exit(1);
#endif  /* -----------  END TEST ----------*/


   /* buff all png file path */
   picpaths=egi_alloc_search_files("/mmc/pic", ".png", &pcount); /* NOTE: path for fonts */
   printf("Totally %d png files are found.\n",pcount);
   if(pcount==0) exit(1);


   /* buff all ttf,otf font file path */

//   fpaths=egi_alloc_search_files(font_path, ".ttf, .otf", &count); /* NOTE: path for fonts */
//   printf("Totally %d ttf font files are found.\n",count);
//   if(count==0) exit(1);
//   for(i=0; i<count; i++)
//        printf("%s\n",fpaths[i]);


   ku=0x4E00;

/* >>>>>>>>>>>>>>>>>>>>  START FONTS DEMON TEST  >>>>>>>>>>>>>>>>>>>> */
//for(j=0; j<=count; j++) /* transverse all font files */
//{
//	if(j==count)
//		j=0;

	np=egi_random_max(pcount)-1;

	printf(" <<<<<<<< FONT TYPE: '%s' >>>>>>> \n",j, font_path);

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
		FT_Done_FreeType( library );
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


#if 0   /* ----------- TEST: unicode writeFb ----------*/
	xleft=240;
	font_color= egi_color_random(color_deep);
	clear_screen(&gv_fb_dev, COLOR_COMPLEMENT_16BITS(font_color));
	ku=egi_random_max(0x9FA5-0x4E00-1)+0x4E00-1;
	symbol_unicode_writeFB(&gv_fb_dev, face, 170, 170, ku, &xleft, //int fw, int fh, wchar_t wcode, int *xleft,
				 40, 70, font_color , -1, -1 ); 	   //int x0, int y0, int fontcolor, int transpcolor,int opaque)
	printf(" xleft=%d \n",xleft);
	tm_delayms(1000);
	goto FT_FAILS;
#endif  /*-------------TEST END------------------------*/


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
//   egi_imgbuf_init(eimg, 320, 240);

   /* load a pic to EGI_IMGBUF as for backgroup */
#if 0
   ret=egi_imgbuf_loadpng( picpaths[np], eimg);
   if(ret!=0) {
	printf(" Fail to load png file '%s'\n", picpaths[np]);
	continue;
   }
   else {
	printf(" png '%s', HxW=%dx%d \n", picpaths[np], eimg->height, eimg->width);
   }
#endif


#if 0  /*--------------- TEST UNICODE HAN CHARACTERS --------- */
   for( i=0; i<15; i++) {
		wcstr[i]=ku;
		ku++;
		if(ku==0x9FA5)
			ku=0x4E00;
   }
#endif

  //printf( "size k=%d, wcslen=%d, wcstr:'%s'\n", k, wcslen(wcstr), wcstr);
  offp=0;
  xleft=240;
  starty=10;
  py=starty;
  fh=20;fw=20;
  gap=4;

	/* set font color */
	font_color= WEGI_COLOR_BLACK; //egi_color_random(color_all);
	/* clear screen */
	clear_screen(&gv_fb_dev, 0xfff9); //0x0679); //COLOR_COMPLEMENT_16BITS(font_color));

  /* get total wchars in the book */
  wtotal=cstr_strcount_uft8(fp);
  printf(" Total wchar: %d\n",wtotal);

//  for ( n = 0; n < wcslen(wcstr); n++ )	/* load wchar and process one by one */
  for ( n = 0; n < wtotal; n++ )	/* load wchar and process one by one */
  {
///////////////////    TEST: symbol_unicode_writeFB()   /////////////////////

	size=char_uft8_to_unicode(fp+offp, wcstr);
	if(size==1)
		printf("ASCII code: %d \n",*wcstr);
	/* shift offset to next wchar */
	if(size>0) {
		offp+=size;
	}
	else {	/* step froward to locate next recognizable unicode wchar */
		//n--;
		offp++;
		continue;
	}

	/* if return next line */
	if(*wcstr=='\n') {
		/* change to next line, +gap */
		xleft=240;
		py+= fh+gap;
		/* ----- start a new page if py gets end ----- */
		if(py>=320-fh-gap) {
			tm_delayms(3000);
			py=starty; xleft=240;
			clear_screen(&gv_fb_dev, 0xfff9); //0x0679);
			continue;
		}
		continue;
	}
	/* else if control code or DEL, skip */
	else if( *wcstr < 32 || *wcstr==127  ) {
		printf(" code=%d\n", *wcstr);
		continue;
	}

	/* write to FB */
	symbol_unicode_writeFB(&gv_fb_dev, face, fw, fh, *wcstr, &xleft,
								 240-xleft, py, font_color , -1, -1 );
	/* check whether current line space is used up */
	if(xleft<=0) {
		if(xleft<0) /* if not written, reel back offp */
			offp-=size;
		/* change to next line, +gap */
		xleft=240;
		py+= fh+gap;
		/* ----- start a new page if py gets end ----- */
		if(py>=320-fh-gap) {
			tm_delayms(3000);
			py=starty; xleft=240;
			clear_screen(&gv_fb_dev, 0xfff9); //0x0679);
			continue;
		}
	}

//	tm_delayms(300);

//////////////////////////   END TEST   /////////////////////////////

  }


/*-------------------------------------------------------------------------------------------
int egi_imgbuf_windisplay(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
// no subcolor, no FB filo
int egi_imgbuf_windisplay2(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
                                        int xp, int yp, int xw, int yw, int winw, int winh);
----------------------------------------------------------------------------------------------*/

	/* put a logo */
//	egi_imgbuf_blend_imgbuf(eimg, 0, -10, logo_img);
//	egi_imgbuf_blend_imgbuf(eimg, 240-85, 320-85, pinguin_img);


	/* Dispay EGI_IMGBUF */
	//egi_imgbuf_windisplay2(eimg, &gv_fb_dev, 0, 0, 0, 0, eimg->width, eimg->height);

	 tm_delayms(3000);

/*
}
////////////////     END LOOP TEST     /////////////////
*/


FT_FAILS:
  FT_Done_Face    ( face );
  FT_Done_Face    ( face2 );
  FT_Done_FreeType( library );

//}

/* >>>>>>>>>>>>>>>>>  END FONTS DEMON TEST  >>>>>>>>>>>>>>>>>>>> */

 	/* free fpaths buffer */
//        egi_free_buff2D((unsigned char **) fpaths, count);
        egi_free_buff2D((unsigned char **) picpaths, pcount);

	/* free EGI_IMGBUF */
	egi_imgbuf_free(eimg);
	egi_imgbuf_free(logo_img);
	egi_imgbuf_free(pinguin_img);

	/* unmap and close fd */
	munmap(fp, fsize);
	close(fd);

	/* Free EGI */
        release_fbdev(&gv_fb_dev);
        symbol_release_allpages();
        egi_quit_log();

  return 0;
}

/* EOF */
