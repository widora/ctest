/*----------------------------------------------------------------------------
For test only!

Midas
----------------------------------------------------------------------------*/
#ifndef __SYMBOL_H__
#define __SYMBOL_H__


#include "fblines.h"


/* symbol image size */
#define SYM_IMGPAGE_WIDTH 240
#define SYM_IMGPAGE_HEIGHT 320


#define TESTFONT_COLOR_FLIP 0 /* 1 use complementary color */


/* symbol type */
enum symbol_type
{
        type_font,
        type_icon,
};


/*
symbol page struct
*/
struct symbol_page
{
	/* symbol type */
	enum symbol_type symtype;
	/* symbol image file path */
	/* for a 320x240x2Byte page, 2Bytes for 16bit color */
	char *path;
	/* back color of the image, a font symbol may use bkcolor as transparent channel */
	uint16_t  bkcolor;
	/* page symbol mem data, store each symbol data consecutively, no hole. while img page file may have hole or blank row*/
	uint16_t *data;
	/* maximum number of symbols in this page, start from 0 */
	int  maxnum; /* maxnum+1 = total number */
	/* total symbol number for  each row, each row has 240 pixels. */
	int sqrow; /* for raw img page */
	/*same height for all symbols in a page */
	int symheight;
	/* use symb_index to locate a symbol in mem, and get its width */
	//struct symbol_index *symindex; /* symb_index[x], x is the corresponding code number of the symbol, like ASCII code */

	/* use pstart to locate a symbole in a row */
	int *symoffset; /* in pixel, offset from uint16_t *data for each smybol!! */
	int *symwidth; /* in pixel, symbol width may be different, while height MUST be the same */

	/* !!!!! following not used, if symbol encoded from 0, you can use array index number as code number.
	 Each row has same number of symbols, so you can use code number to locate a row in a img page
	 Code is a unique encoding number for each symbol in a page, like ASCII number.
	*/
//	int *symb_code;
};



extern struct symbol_page sympg_testfont;

/*------------------- functions ---------------------*/
uint16_t *symbol_load_page(struct symbol_page *sym_page);
void symbol_release_page(struct symbol_page *sym_page);
int symbol_check_page(const struct symbol_page *sym_page, char *func);
void symbol_save_pagemem(struct symbol_page *sym_page);
void symbol_print_symbol(const struct symbol_page *sym_page, int symbol, uint16_t transpcolor);
void symbol_writeFB(FBDEV *fb_dev, const struct symbol_page *sym_page,  \
                int transpcolor, int x0, int y0, int sym_code);
void symbol_string_writeFB(FBDEV *fb_dev, const struct symbol_page *sym_page,   \
                int transpcolor, int x0, int y0, const char* str);



#endif
