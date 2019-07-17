/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
-------------------------------------------------------------------*/
#include "egi_FTsymbol.h"
#include "egi_symbol.h"
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>
#include <arpa/inet.h>
#include FT_FREETYPE_H

/* <<<<<<<<<<<<<<<<<<   FreeType Fonts  >>>>>>>>>>>>>>>>>>>>>>*/

struct symbol_page sympg_ascii={0}; /* default  LiberationMono-Regular */

EGI_FONTS  egi_sysfonts =
{
	.fpath_regular ="/mmc/fonts/hansans/SourceHanSansSC-Normal.otf",
	.fpath_light   ="/mmc/fonts/hansans/SourceHanSansSC-ExtraLight.otf",
	.fpath_bold    ="/mmc/fonts/hansans/SourceHanSansSC-Bold.otf",
};


/*-------------------------------------
    Load all FT type symbol pages
return:
	0	OK
	<0	Fails
------------------------------------*/
int  FTsymbol_load_allpages(void)
{
	int ret=0;

        /* load ASCII font, bitmap size 18x18 in pixels, each line abt.18x2 pixels in height */
        ret=FTsymbol_load_asciis_from_fontfile( &sympg_ascii,
                                "/mmc/fonts/liber/LiberationMono-Regular.ttf", 18, 18);

	return ret;
}

/* --------------------------------------
     Release all TF type symbol pages
----------------------------------------*/
void FTsymbol_release_allpages(void)
{
        /* release FreeType2 symbol page */
      	symbol_release_page(&sympg_ascii);
	if(sympg_ascii.symwidth!=NULL)
		free(sympg_ascii.symwidth);
}



/*--------------------------------------------------
Initialize FT library and load faces.
return:
        0       OK
        !0      Fail
---------------------------------------------------*/
int FTsymbol_load_library( EGI_FONTS *symlib )
{
	FT_Error	error;

	/* check input data */
	if( symlib==NULL || symlib->fpath_regular==NULL
			 || symlib->fpath_light==NULL
			 || symlib->fpath_bold==NULL	)
	{
		printf("%s: Fail to check integrity of the input FTsymbol_library!\n",__func__);
		return -1;
	}

        /* 1. initialize FT library */
        error = FT_Init_FreeType( &symlib->library );
        if(error) {
                printf("%s: An error occured during FreeType library initialization.\n",__func__);
                return error;
        }

	/* 2. create face object: Regular */
        error = FT_New_Face( symlib->library, symlib->fpath_regular, 0, &symlib->regular );
        if(error==FT_Err_Unknown_File_Format) {
                printf("%s: Font file '%s' opens, but its font format is unsupported!\n",
								__func__, symlib->fpath_regular);
		goto FT_FAIL;
        }
        else if ( error ) {
                printf("%s: Fail to open or read font file '%s'.\n",__func__, symlib->fpath_regular);
		goto FT_FAIL;
        }

	/* 3. create face object: Light */
        error = FT_New_Face( symlib->library, symlib->fpath_light, 0, &symlib->light );
        if(error==FT_Err_Unknown_File_Format) {
                printf("%s: Font file '%s' opens, but its font format is unsupported!\n",
								__func__, symlib->fpath_light);
		goto FT_FAIL;
        }
        else if ( error ) {
                printf("%s: Fail to open or read font file '%s'.\n",__func__, symlib->fpath_light);
		goto FT_FAIL;
        }

	/* 4. create face object: Light */
        error = FT_New_Face( symlib->library, symlib->fpath_bold, 0, &symlib->bold );
        if(error==FT_Err_Unknown_File_Format) {
                printf("%s: Font file '%s' opens, but its font format is unsupported!\n",
								__func__, symlib->fpath_bold);
		goto FT_FAIL;
        }
        else if ( error ) {
                printf("%s: Fail to open or read font file '%s'.\n",__func__, symlib->fpath_bold);
		goto FT_FAIL;
        }

	return 0;

FT_FAIL:
 	FT_Done_Face    ( symlib->regular );
  	FT_Done_Face    ( symlib->light );
  	FT_Done_Face    ( symlib->bold );
  	FT_Done_FreeType( symlib->library );

	return error;
}


/*--------------------------------------------------
	Rlease FT library.
---------------------------------------------------*/
void FTsymbol_release_library( EGI_FONTS *symlib )
{
	if(symlib==NULL)
		return;

 	FT_Done_Face    ( symlib->regular );
  	FT_Done_Face    ( symlib->light );
  	FT_Done_Face    ( symlib->bold );
  	FT_Done_FreeType( symlib->library );

}


/*--------------------------------------------------------------------------------------------------------
(Note: FreeType 2 is licensed under FTL/GPLv3 or GPLv2 ).

1. This program is ONLY for horizontal text layout !!!

2. To get the MAX boudary box heigh: symheight, which is capable of containing each character in a font set.
   (symheight as for struct symbol_page)

   1.1	base_uppmost + base_lowmost +1    --->  symheight  (same height for all chars)
	base_uppmost = MAX( slot->bitmap_top )			    ( pen.y set to 0 as for baseline )
	base_lowmost = MAX( slot->bitmap.rows - slot->bitmap_top )  ( pen.y set to 0 as for baseline )

   	!!! Note: Though  W*H in pixels as in FT_Set_Pixel_Sizes(face,W,H) is capable to hold each
        char images, but it is NOT aligned with the common baseline !!!

   1.2 MAX(slot->advance.x/64, slot->bitmap.width)	--->  symwidth  ( for each charachter )

       Each ascii symbol has different value of its FT bitmap.width.

3. 0-31 ASCII control chars, 32-126 ASCII printable symbols.

4. For ASCII charaters, then height defined in FT_Set_Pixel_Sizes() or FT_Set_Char_Size() is NOT the true
   hight of a character bitmap, it is a norminal value including upper and/or low bearing gaps, depending
   on differenct font glyphs.

5. Following parameters all affect the final position of a character.
   5.1 slot->advance.x ,.y:  	Defines the current cursor/pen shift vector after loading/rendering a character.
				!!! This will affect the position of the next character. !!!
   5.2 HBearX, HBearY:	 	position adjust for a character, relative to the current cursor position(local
				origin).
   5.3 slot->bitmap_left, slot->bitmap_top:  Defines the left top point coordinate of a character bitmap,
				             relative to the global drawing system origin.


LiberationSans-Regular.ttf

Midas Zhou
midaszhou@yahoo.com
----------------------------------------------------------------------------------------------------------*/


/* -------------------------------------------------------------------------------------------
Load a ASCII symbol_page struct from a font file by calling FreeType2 libs.

Note:
0. This is for pure western ASCII alphabets font set.
1. Load ASCII characters from 0 - 127, bitmap data of all unprintable ASCII symbols( 0-31,127)
   will not be loaded to symfont_page, and their symwidths are set to 0 accordingly.

2. Take default face_index=0 when call FT_New_Face(), you can modify it if
   the font file contains more than one glphy face.

3. Inclination angle for each single character is set to be 0. If you want an oblique effect (for
   single character ), just assign a certain value to 'deg'.

4. !!! symfont_page->symwidth MUST be freed by calling free() separately,for symwidth is statically
   allocated in most case, and symbol_release_page() will NOT free it.


@symfont_page:   pointer to a font symbol_page.
@font_path:	 font file path.
@Wp, Hp:	 in pixels, as for FT_Set_Pixel_Sizes(face, Wp, Hp )

 !!!! NOTE: Hp is nominal font height and NOT symheight of symbole_page,
	    the later one shall be set to be the same for all symbols in the page.

Return:
        0       ok
        <0      fails
---------------------------------------------------------------------------------------------*/
int  FTsymbol_load_asciis_from_fontfile(  EGI_SYMPAGE *symfont_page, const char *font_path,
				       	   int Wp, int Hp )
{
	FT_Error      	error;
	FT_Library    	library;
	FT_Face       	face;		/* NOTE: typedef struct FT_FaceRec_*  FT_Face */
	FT_GlyphSlot  	slot;
	FT_Matrix     	matrix;         /* transformation matrix */
	FT_Vector	origin;

	int bbox_W; // bbox_H; 	/* Width and Height of the boundary box */
	int base_uppmost; 	/* in pixels, from baseline to the uppermost scanline */
	int base_lowmost; 	/* in pixels, from baseline to the lowermost scanline */
	int symheight;	  	/* in pixels, =base_uppmost+base_lowmost+1, symbol height for struct symbol_page */
	int symwidth_sum; 	/* sum of all charachter widths, as all symheight are the same,
			      	 * so symwidth_total*symheight is required mem space */
	int sympix_total;	/* total pixel number of all symbols in a sympage */
	int hi,wi;
	int pos_symdata, pos_bitmap;

 	FT_CharMap*   pcharmaps=NULL;
  	char          tag_charmap[6]={0};
  	uint32_t      tag_num;

  	int		deg;    /* angle in degree */
  	double          angle;  /* angle in double */
  	int 		i,n; //j,k,m,n;
	int		ret=0;

	/* A1. initialize FT library */
	error = FT_Init_FreeType( &library );
	if(error) {
		printf("%s: An error occured during FreeType library initialization.\n",__func__);
		return -1;
	}

	/* A2. create face object, face_index=0 */
 	error = FT_New_Face( library, font_path, 0, &face );
	if(error==FT_Err_Unknown_File_Format) {
		printf("%s: Font file opens, but its font format is unsupported!\n",__func__);
		FT_Done_FreeType( library );
		return -2;
	}
	else if ( error ) {
		printf("%s: Fail to open or read font file '%s'.\n",__func__, font_path);
		FT_Done_FreeType( library );
		return -3;
	}

  	/* A3. get pointer to the glyph slot */
  	slot = face->glyph;

	/* A4. print font face[0] parameters */
#if 1  /////////////////////////     PRINT FONT INFO     /////////////////////////////
	printf("   FreeTypes load font file '%s' :: face_index[0] \n", font_path);
	printf("   num_faces:		%d\n",	(int)face->num_faces);
	printf("   face_index:		%d\n",	(int)face->face_index);
	printf("   family name:		%s\n",	face->family_name);
	printf("   style name:		%s\n",	face->style_name);
	printf("   num_glyphs:		%d\n",	(int)face->num_glyphs);
	printf("   face_flags: 		0x%08X\n",(int)face->face_flags);
	printf("   units_per_EM:	%d\n",	face->units_per_EM);
	printf("   num_fixed_sizes:	%d\n",	face->num_fixed_sizes);
	if(face->num_fixed_sizes !=0 ) {
		for(i=0; i< face->num_fixed_sizes; i++) {
			printf("[%d]: H%d x W%d\n",i, face->available_sizes[i].height,
							    face->available_sizes[i].width);
		}
	}
		/* print available charmaps */
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
#endif ////////////////////////////   END PRINT   ///////////////////////////////////


   	/* A5. set character size in pixels */
   	error = FT_Set_Pixel_Sizes(face, Wp, Hp); /* width,height */
	/* OR set character size in 26.6 fractional points, and resolution in dpi */
   	//error = FT_Set_Char_Size( face, 32*32, 0, 100, 0 );

  	/* A6. set char inclination angle and matrix for transformation */
	deg=0.0;
  	angle     = ( deg / 360.0 ) * 3.14159 * 2;
  	printf("   Font inclination angle: %d, %f \n", deg, angle);
  	matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
  	matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
	matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
	matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

	/* --------------------------- NOTE -----------------------------------------------------
         * 	1. Define a boundary box with bbox_W and bbox_H to contain each character.
	 *	2. The BBoxes are NOT the same size, and neither top_lines nor bottom_lines
	 *	   are aligned at the same level.
	 *	3. The symbol_page boundary boxes are all with the same height, and their top_lines
	 *	   and bottum_lines are aligned at the same level!
	 -----------------------------------------------------------------------------------------*/

	/* ------- Get MAX. base_uppmost, base_lowmost ------- */
	base_uppmost=0;  /* baseline to top line of [symheight * bbox_W] */
	base_lowmost=0;  /* baseline to top line of [symheight * bbox_W] */

	/* set origin here, so that all character bitmaps are aligned to the same origin/baseline */
	origin.x=0;
	origin.y=0;
    	FT_Set_Transform( face, &matrix, &origin);

	/* 1. Tranvser all chars to get MAX height for the boundary box, base line all aligned. */
	symheight=0;
	symwidth_sum=0;
	for( n=0; n<128; n++) {  /* 0-NULL, 32-SPACE, 127-DEL */
	    	error = FT_Load_Char( face, n, FT_LOAD_RENDER );
	    	if ( error ) {
			printf("%s: FT_Load_Char() error!\n",__func__);
			exit(1);
		}

		/* 1.1 base_uppmost: dist from base_line to top */
		if(base_uppmost < slot->bitmap_top)
			base_uppmost=slot->bitmap_top;

		/* 1.2 base_lowmost: dist from base_line to bottom */
		/* !!!! unsigned int - signed int */
		if( base_lowmost < (int)(slot->bitmap.rows) - (int)(slot->bitmap_top) ) {
				base_lowmost=(int)slot->bitmap.rows-(int)slot->bitmap_top;
		}

		/* 1.3 get symwidth, and sum it up */
			/* rule out all unpritable characters, except TAB  */
		if( n<32 || n==127 ) {
			/* symwidth set to be 0, no mem space for it. */
			continue;
		}

		bbox_W=slot->advance.x>>6;
		if(bbox_W < slot->bitmap.width) /* adjust bbox_W */
			bbox_W =slot->bitmap.width;

		symwidth_sum += bbox_W;
	}

	/* 2. get symheight at last */
	symheight=base_uppmost+base_lowmost+1;
	printf("   Input Hp=%d; symheight=%d; base_uppmost=%d; base_lowmost=%d  (all in pixels)\n",
							Hp, symheight, base_uppmost, base_lowmost);
	symfont_page->symheight=symheight;

	/* 3. Set Maxnum for symbol_page*/
	symfont_page->maxnum=128-1; /* 0-127 */

	/* 4. allocate struct symbol_page .data, .alpha, .symwidth, .symoffset */
	sympix_total=symheight*symwidth_sum; /* total pixel number in the symbol page */

	/* release all data before allocate it */
	symbol_release_page(symfont_page);

	symfont_page->data=calloc(1, sympix_total*sizeof(uint16_t));
	if(symfont_page->data==NULL) { ret=-4; goto FT_FAILS; }

	symfont_page->alpha=calloc(1, sympix_total*sizeof(unsigned char));
	if(symfont_page->alpha==NULL) { ret=-4; goto FT_FAILS; }

	symfont_page->symwidth=calloc(1, (symfont_page->maxnum+1)*sizeof(int) );
	if(symfont_page->symwidth==NULL) { ret=-4; goto FT_FAILS; }

	symfont_page->symoffset=calloc(1, (symfont_page->maxnum+1)*sizeof(int));
	if(symfont_page->symoffset==NULL) { ret=-4; goto FT_FAILS; }


	/* 5. Tranverseall ASCIIs to get symfont_page.alpha, ensure that all base_lines are aligned. */
	/* NOTE:
	 *	1. TODO: Unprintable ASCII characters will be presented by FT as a box with a cross inside.
	 *	   It is not necessary to be allocated in mem????
	 *	2. For font sympage, only .aplha is available, .data is useless!!!
	 *	3. Symbol font boundary box [ symheight*bbox_W ] is bigger than each slot->bitmap box !
	 */

	symfont_page->symoffset[0]=0; 	/* default for first char */

	for( n=0; n<128; n++) {		/* 0-NULL, 32-SPACE, 127-DEL */


		/* 5.0 rule out all unpritable characters */
		if( n<32 || n==127 ) {
			symfont_page->symwidth[n]=0;
		        /* Not necessary to change symfont_page->symoffset[x], as its symwidth is 0. */
			continue;
		}

		/* 5.1 load N to ASCII symbol bitmap */
	    	error = FT_Load_Char( face, n, FT_LOAD_RENDER );
	    	if ( error ) {
			printf("%s: Fail to FT_Load_Char() n=%d as ASCII symbol!\n", __func__, n);
			exit(1);
		}

		/* 5.2 get symwidth for each symbol, =MAX(advance.x>>6, bitmap.width) */
		bbox_W=slot->advance.x>>6;
		if(bbox_W < slot->bitmap.width) /* adjust bbox_W */
			bbox_W =slot->bitmap.width;
		symfont_page->symwidth[n]=bbox_W;

		/* 5.3    <<<<<<<<<	get symfont_page.alpha	 >>>>>>>>>
			As sympage boundary box [ symheight*bbox_W ] is big enough to hold each
			slot->bitmap image, we divide symheight into 3 parts, with the mid part
			equals bitmap.rows, and get alpha value accordingly.

			ASSUME: bitmap start from wi=0 !!!

			variables range:: { hi: 0-symheight,   wi: 0-bbox_W }
		*/

		/* 1. From base_uppmost to bitmap_top:
		 *    All blank lines, and alpha values are 0 as default.
		 *
		 *   <------ So This Part Can be Ignored ------>
		 */

#if 0 /*   <------  This Part Can be Ignored ------> */
		for( hi=0; hi < base_uppmost-slot->bitmap_top; hi++) {
			for( wi=0; wi < bbox_W; wi++ ) {
				pos_symdata=symfont_page->symoffset[n] + hi*bbox_W + wi;
				/* all default value */
				symfont_page->alpha[pos_symdata] = 0;
			}
		}
#endif

		/* 2. From bitmap_top to bitmap bottom,
		 *    Actual data here.
		 */
	     for( hi=base_uppmost-slot->bitmap_top; hi < (base_uppmost-slot->bitmap_top) + slot->bitmap.rows; \
							 hi++ )
		{
			/* ROW: within bitmap.width, !!! ASSUME bitmap start from wi=0 !!! */
			for( wi=0; wi < slot->bitmap.width; wi++ ) {
				pos_symdata=symfont_page->symoffset[n] + hi*bbox_W +wi;
				pos_bitmap=(hi-(base_uppmost-slot->bitmap_top))*slot->bitmap.width+wi;
				symfont_page->alpha[pos_symdata]= slot->bitmap.buffer[pos_bitmap];

#if 0   /* ------ TEST ONLY, print alpha here ------ */

		  	if(n=='j')  {
				if(symfont_page->alpha[pos_symdata]>0)
					printf("*");
				else
					printf(" ");
				if( wi==slot->bitmap.width-1 )
					printf("\n");
	   		}

#endif /*  -------------   END TEST   -------------- */

			}
			/* ROW: blank area in BBOX */
			for( wi=slot->bitmap.width; wi<bbox_W; wi++ ) {
				pos_symdata=symfont_page->symoffset[n] + hi*bbox_W +wi;
				symfont_page->alpha[pos_symdata]=0;
			}
		}

		/* 3. From bitmap bottom to symheight( bottom),
		 *    All blank lines, and alpha values are 0 as default.
		 *
		 *   <------ So This Part Can be Ignored too! ------>
		 */

#if 0 /*   <------  This Part Can be Ignored ------> */
		for( hi=(base_uppmost-slot->bitmap_top) + slot->bitmap.rows; hi < symheight; hi++ )
		{
			for( wi=0; wi < bbox_W; wi++ ) {
				pos_symdata=symfont_page->symoffset[n] + hi*bbox_W + wi;
				/* set only alpha value */
				symfont_page->alpha[pos_symdata] = 0;
			}
		}
#endif

		/* calculate symoffset for next one */
		if(n<127)
			symfont_page->symoffset[n+1] = symfont_page->symoffset[n] + bbox_W * symheight;

	} /* end transversing all ASCII chars */


FT_FAILS:
  	FT_Done_Face    ( face );
  	FT_Done_FreeType( library );

  	return ret;
}


