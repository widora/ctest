/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
-------------------------------------------------------------------*/
#ifndef __EGI_FTSYMBOL_H__
#define __EGI_FTSYMBOL_H__

#include "egi_symbol.h"
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>
#include <arpa/inet.h>
#include FT_FREETYPE_H

typedef struct FTsymbol_library EGI_FONTS;
struct FTsymbol_library {

	FT_Library     library;

	/* Regular type */
        FT_Face         regular;
	char 		*fpath_regular;

	/* Light type */
        FT_Face         light;
	char 		*fpath_light;

	/* Regular type */
        FT_Face         bold;
	char 		*fpath_bold;

	/* Special type */
        FT_Face         special;
	char 		*fpath_special;
};

extern EGI_SYMPAGE sympg_ascii; /* default  LiberationMono-Regular */
extern EGI_FONTS  egi_sysfonts; /* system font set */

int 	FTsymbol_load_library( EGI_FONTS *symlib );
void 	FTsymbol_release_library( EGI_FONTS *symlib );
int	FTsymbol_load_allpages(void);
void 	FTsymbol_release_allpages(void);
int  	FTsymbol_load_asciis_from_fontfile( EGI_SYMPAGE *symfont_page, const char *font_path, int Wp, int Hp );

void 	symbol_unicode_writeFB(FBDEV *fb_dev, FT_Face face, int fw, int fh, wchar_t wcode, int *xleft,
				int x0, int y0, int fontcolor, int transpcolor,int opaque);

int  	FTsymbol_unicstrings_writeFB( FBDEV *fb_dev, FT_Face face, int fw, int fh, const wchar_t *pwchar,
			       unsigned int pixpl,  unsigned int lines,  unsigned int gap,
                               int x0, int y0,
			       int fontcolor, int transpcolor, int opaque );

int  	FTsymbol_uft8strings_writeFB( FBDEV *fb_dev, FT_Face face, int fw, int fh, const unsigned char *pstr,
			       unsigned int pixpl,  unsigned int lines,  unsigned int gap,
                               int x0, int y0,
			       int fontcolor, int transpcolor, int opaque );


#endif
