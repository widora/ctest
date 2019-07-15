/*----------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


For test only!

1. Symbols may be icons,fonts, image, a serial motion picture.... etc.
2. One img symbol page size is a 240x320x2Bytes data , 2Bytes for 16bit color.
   each row has 240 pixels. usually it is a pure color data file.
3. One mem symbol page size is MAX.240X320X2Bytes, usually it is smaller because
   its corresponding img symbol page has blank space.
   Mem symbol page is read and converted from img symbol page, mem symbol page
   stores symbol pixels data row by row consecutively for each symbol. and it
   is more efficient for storage and search.
   A mem symbol page may be saved as a file.
5. All symbols in a page MUST have the same height, and each row MUST has the same
   number of symbols.
6. The first symbol in a img page shuld not be used, code '0' usually will be treated
   as and end token for a char string.
7. when you edit a symbol image, don't forget to update:
	7.1 sympg->maxnum;
	7.2 xxx_width[N] >= sympg->maxnu;
	7.3 modify symbol structre in egi_symbol.h accordingly.

TODO:
0.  different type symbol now use same writeFB function !!!!! font and icon different writeFB func????
0.  if image file is not complete.
1.  void symbol_save_pagemem( )
2.  void symbol_writeFB( ) ... copy all data if no transp. pixel applied.
3.  data encode.
4.  symbol linear enlarge and shrink.
5. To read FBDE vinfo to get all screen/fb parameters as in fblines.c, it's improper in other source files.

Midas Zhou
midaszhou@yahoo.com
----------------------------------------------------------------------------*/
//#define _GNU_SOURCE	/* for O_CLOEXEC flag */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /*close*/
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "egi_fbgeom.h"
#include "egi_image.h"
#include "egi_symbol.h"
#include "egi_debug.h"
#include "egi_log.h"
#include "egi_timer.h"
#include "egi_math.h"


/*--------------------(  testfont  )------------------------
  1.  ascii 0-127 symbol width,
  2.  5-pixel blank space for unprintable symbols, though 0-pixel seems also OK.
  3.  Please change the 'space' width according to your purpose.
*/
static int testfont_width[16*8] = /* check with maxnum */
{
	/* give return code a 0 width in a txt display  */
//	5,5,5,5,5,5,5,5,5,5,0,5,5,5,5,5, /* unprintable symbol, give it 5 pixel wide blank */
//	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, /* unprintable symbol, give it 5 pixel wide blank */
	0,0,0,0,0,0,0,0,0,30,0,0,0,0,0,0, /* 9-TAB 50pixels,  unprintable symbol */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	5,7,8,10,11,15,14,5,6,6,10,10,5,6,5,8, /* space,!"#$%&'()*+,-./ */
	11,11,11,11,11,11,11,11,11,11,6,6,10,10,10,10, /* 0123456789:;<=>? */
	19,12,11,11,13,10,10,13,13,5,7,11,9,18,14,14, /* @ABCDEFGHIJKLMNO */
	11,14,11,10,10,13,12,19,11,10,10,6,8,6,10,10, /* PQRSTUVWXYZ[\]^_ */
	6,10,11,9,11,10,6,10,11,5,5,10,5,17,11,10, /* uppoint' abcdefghijklmnop */
	11,11,7,8,7,11,9,15,9,10,8,7,10,7,10,5 /*pqrstuvwxyz{|}~ blank */
};
/* symbole page struct for testfont */
struct symbol_page sympg_testfont=
{
	.symtype=symtype_font,
	.path="/home/testfont.img",
	.bkcolor=0xffff,
	.data=NULL,
	.maxnum=128-1,
	.sqrow=16,
	.symheight=26,
	.symwidth=testfont_width, /* width list */
};



/*--------------------------(  numbfont  )-----------------------------------
 	big number font 0123456789:
*/
static int numbfont_width[16*8] = /* check with maxnum */
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* unprintable symbol */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	15,15,15,15,15,15,15,15,15,15,15,0,0,0,0,0, /* 0123456789: */
};
/* symbole page struct for numbfont */
struct symbol_page sympg_numbfont=
{
	.symtype=symtype_font,
	.path="/home/numbfont.img",
	.bkcolor=0x0000,
	.data=NULL,
	.maxnum=16*4-1,
	.sqrow=16,
	.symheight=20,
	.symwidth=numbfont_width, /* width list */
};


/*--------------------------(  buttons H60 )-----------------------------------*/
static int buttons_width[4*5] =  /* check with maxnum */
{
	60,60,60,60,
	60,60,60,60,
	60,60,60,60,
	60,60,60,60,
	60,60,60,60,
};
struct symbol_page sympg_buttons=
{
	.symtype=symtype_icon,
	.path="/home/buttons.img",
	.bkcolor=0x0000,
	.data=NULL,
	.maxnum=4*5-1, /* 5 rows of ioncs */
	.sqrow=4, /* 4 icons per row */
	.symheight=60,
	.symwidth=buttons_width, /* width list */
};


/*--------------------------(  small buttons W48H60 )-----------------------------------*/
static int sbuttons_width[5*3] =  /* check with maxnum */
{
	48,48,48,48,
	48,48,48,48,
	48,48,48,48,
};
struct symbol_page sympg_sbuttons=
{
	.symtype=symtype_icon,
	.path="/home/sbuttons.img",
	.bkcolor=0x0000,
	.data=NULL,
	.maxnum=5*3-1, /* 5 rows of ioncs */
	.sqrow=5, /* 4 icons per row */
	.symheight=60,
	.symwidth=sbuttons_width, /* width list */
};


/*--------------------------(  30x30 icons for Home Head-Bar )-----------------------------------*/
static int icons_width[8*12] =  /* element number MUST >= maxnum */
{
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
        30,30,30,30,30,30,30,30,
};
struct symbol_page sympg_icons=
{
        .symtype=symtype_font,
        .path="/home/icons.img",
        .bkcolor=0x0000,
        .data=NULL,
        .maxnum=11*8-1, /* 11 rows of ioncs */
        .sqrow=8, /* 8 icons per row */
        .symheight=30,
        .symwidth=icons_width, /* width list */
};

/* put load motion icon array for CPU load
   !!! put code 0 as end of string */
char symmic_cpuload[6][5]= /* sym for motion icon */
{
	{40,41,42,43,0}, /* very light loadavg 1 */
	{44,45,46,47,0}, /* light  loadavg 2 */
	{48,49,50,51,0}, /* moderate  loadavg 3 */
	{52,53,54,55,0}, /* heavy loadavg 4 */
	{56,57,58,59,0}, /* very heavy loadavg 5 */
	{60,61,62,63,0},  /* red alarm loadavg 6 >5 overload*/
};

/* IoT mmic */
char symmic_iotload[9]=
{ 16,17,18,19,20,21,22,23,0}; /* with end token /0 */


/*------------------(  60x60 icons for PLAYS and ARROWS )-----------------*/
static int icons_2_width[4*5] =  /* element number MUST >= maxnum */
{
	60,60,60,60,
	60,60,60,60,
	60,60,60,60,
	60,60,60,60,
	60,60,60,60,
};
/* symbole page struct for testfont */
struct symbol_page sympg_icons_2=
{
        .symtype=symtype_icon,
        .path="/home/icons_2.img",
        .bkcolor=0x0000,
        .data=NULL,
        .maxnum=4*5-1, /* 5 rows of ioncs */
        .sqrow=4, /* 8 icons per row */
        .symheight=60,
        .symwidth=icons_2_width, /* width list */
};


/* for HeWeather Icons */
static int heweather_width[1*1] =
{
	60
};
struct symbol_page sympg_heweather =
{
        .symtype=symtype_icon,
        .path=NULL,			/* NOT applied */
        .bkcolor=-1,			/* <0, no transpcolor applied */
        .data=NULL,			/* 16bit per pixle image data */
	.alpha=NULL,			/* 8bit per pixle alpha data */
        .maxnum=1*1-1,
        .sqrow=1, 			/* 1 icons per row */
        .symheight=60,
        .symwidth=heweather_width, 	/* width list */
};


/* <<<<<<<<<<<<<<<<<<  	FreeType Fonts 	>>>>>>>>>>>>>>>>>>>>>>*/
struct symbol_page sympg_ascii={0}; /* default  LiberationMono-Regular */




/* -----  All static functions ----- */
static uint16_t *symbol_load_page(struct symbol_page *sym_page);

/* -------------------------------------------------------------
TODO: Only for one symbol NOW!!!!!

load a symbol_page struct from a EGI_IMGBUF
@sym_page:   pointer to a symbol_page.
@imgbuf:     a EGI_IMGBUF holding that image data

Return:
	0	ok
	<0	fails
--------------------------------------------------------------*/
int symbol_load_page_from_imgbuf(struct symbol_page *sym_page, EGI_IMGBUF *imgbuf)
{
	int i;
	int off;
	int data_size;

	if(imgbuf==NULL || imgbuf->imgbuf==NULL || sym_page==NULL) {
		printf("%s: Invalid input data!\n",__func__);
		return -1;
	}
	data_size=(imgbuf->height)*(imgbuf->width)*2; /* 16bpp color */

        /* malloc mem for symbol_page.symoffset */
        sym_page->symoffset = calloc(1, ((sym_page->maxnum)+1) * sizeof(int) );
        if(sym_page->symoffset == NULL) {
                        printf("%s: fail to malloc sym_page->symoffset!\n",__func__);
                        return -1;
        }

	/* set symoffset */
        off=0;
        for(i=0; i<=sym_page->maxnum; i++)
        {
                sym_page->symoffset[i]=off;
                off += sym_page->symwidth[i] * (sym_page->symheight) ;/* in pixel */
        }

	/* copy color data */
	printf("copy color data...\n");
	sym_page->data=calloc(1,data_size);
	if(sym_page->data==NULL) {
		printf("%s: Fail to alloc sym_page->data!\n",__func__);
		return -2;
	}
	memcpy(sym_page->data, imgbuf->imgbuf, data_size);

	/* copy alpha */
	printf("copy alpha value...\n");
	if(imgbuf->alpha) {
		sym_page->alpha=calloc(1,data_size>>1); /* 8bpp for alpha */
		if(sym_page->alpha==NULL) {
			printf("%s: Fail to alloc sym_page->alpha!\n",__func__);
			free(sym_page->data);
			return -3;
		}
		memcpy(sym_page->alpha, imgbuf->alpha, data_size>>1);
		sym_page->bkcolor=-1; /* use alpha instead of bkcolor */
	}

	return 0;
}


/*-------------------------------------------------------
Load all symbol files into mem pages
Don't forget to change symbol_free_allpages() accordingly
return:
	0	OK
	<0	Fail
-------------------------------------------------------*/
int symbol_load_allpages(void)
{
	int ret;

        /* load testfont */
        if(symbol_load_page(&sympg_testfont)==NULL)
                goto FAIL;
        /* load numbfont */
        if(symbol_load_page(&sympg_numbfont)==NULL)
                goto FAIL;
        /* load buttons icons */
        if(symbol_load_page(&sympg_buttons)==NULL)
                goto FAIL;
        /* load small buttons icons */
        if(symbol_load_page(&sympg_sbuttons)==NULL)
                goto FAIL;
        /* load icons for home head-bar*/
        if(symbol_load_page(&sympg_icons)==NULL)
                goto FAIL;
        /* load icons for PLAYERs */
        if(symbol_load_page(&sympg_icons_2)==NULL)
                goto FAIL;

	/* load ASCII font, bitmap size 18x18 in pixels, each line abt.18x2 pixels in height */
	ret=symbol_load_asciis_from_fontfile( &sympg_ascii,
				"/mmc/fonts/liber/LiberationMono-Regular.ttf", 18, 18);
	if( ret != 0 )
		goto FAIL;

	return 0;

FAIL:
	symbol_release_allpages();
	return -1;
}

/* --------------------------------------
	Free all mem pages
----------------------------------------*/
void symbol_release_allpages(void)
{
	symbol_release_page(&sympg_testfont);
	symbol_release_page(&sympg_numbfont);
	symbol_release_page(&sympg_buttons);
	symbol_release_page(&sympg_sbuttons);
	symbol_release_page(&sympg_icons);
	symbol_release_page(&sympg_icons_2);

	/* release FreeType2 fonts */
	symbol_release_page(&sympg_ascii);
}


/*----------------------------------------------------------------
   load an img page file
   1. direct mmap.
      or
   2. load to a mem page. (current implementation)

path: 	path to the symbol image file
num: 	total number of symbols,or MAX code number-1;
height: heigh of all symbols
width:	list for all symbol widthes
sqrow:	symbol number in each row of an img page

------------------------------------------------------------------*/
static uint16_t *symbol_load_page(struct symbol_page *sym_page)
{
	int fd;
	int datasize; /* memsize for all symbol data */
	int i,j;
	int x0=0,y0=0; /* origin position of a symbol in a image page, left top of a symbol */
	int nr,no;
	int offset=0; /* in pixel, offset of data mem for each symbol, NOT of img file */
	int all_height; /* height for all symbol in a page */
	int width; /* width of a symbol */

	if(sym_page==NULL)
		return NULL;

	/* open symbol image file */
	fd=open(sym_page->path, O_RDONLY|O_CLOEXEC);
	if(fd<0)
	{
		printf("fail to open symbol file %s!\n",sym_page->path);
			perror("open symbol image file");
		return NULL;
	}

#if 1   /*------------  check an ((unloaded)) page structure -----------*/

	/* check for maxnum */
	if(sym_page->maxnum < 0 )
	{
		printf("symbol_load_page(): symbol number less than 1! fail to load page.\n");
		return NULL;
	}
	/* check for data */
	if(sym_page->data != NULL)
	{
		printf("symbol_load_page(): sym_page->data is NOT NULL! symbol page may be already \
				loaded in memory!\n");
		return sym_page->data;
	}
	/* check for symb_index */
	if(sym_page->symwidth == NULL)
	{
		printf("symbol_load_page(): symbol width list is empty! fail to load symbole page.\n");
		return NULL;
	}

#endif

	/* get height for all symbols */
	all_height=sym_page->symheight;

	/* malloc mem for symbol_page.symoffset */
	sym_page->symoffset = malloc( ((sym_page->maxnum)+1) * sizeof(int) );
	if(sym_page->symoffset == NULL) {
			printf("symbol_load_page():fail to malloc sym_page->symoffset!\n");
			return NULL;
	}

	/* calculate symindex->sym_offset for each symbol
           and mem size needed for all symbols */
	datasize=0;
	for(i=0;i<=sym_page->maxnum;i++)
	{
		sym_page->symoffset[i]=datasize;
		datasize += sym_page->symwidth[i] * all_height ;/* in pixel */
	}

	/* malloc mem for symbol_page.data */
	sym_page->data= malloc( datasize*sizeof(uint16_t) ); /* memsize in pixel of 16bit color*/
	{
		if(sym_page->data == NULL)
		{
			EGI_PLOG(LOGLV_ERROR,"symbol_load_page(): fail to malloc sym_page->data! ???Maybe element number of symwidth[] is less than symbol_page.maxnum \n");
			symbol_release_page(sym_page);
			return NULL;
		}
	}


	/* read symbol pixel data from image file to sym_page.data */
        for(i=0; i<=sym_page->maxnum; i++) /* i for each symbol */
        {

		/* a symbol width MUST NOT be zero.!   --- zero also OK, */
/*
		if( sym_page->symwidth[i]==0 )
		{
			printf("symbol_load_page(): sym_page->symwidth[%d]=0!, a symbol width MUST NOT be zero!\n",i);
			symbol_release_page(sym_page);
			return NULL;
		}
*/
		nr=i/(sym_page->sqrow); /* locate row number of the page */
		no=i%(sym_page->sqrow); /* in symbol,locate order number of a symbol in a row */

		if(no==0) /* reset x0 for each row */
			x0=0;
		else
			x0 += sym_page->symwidth[i-1]; /* origin pixel order in a row */

                y0 = nr * all_height; /* origin pixel order in a column */
                //printf("x0=%d, y0=%d \n",x0,y0);

		offset=sym_page->symoffset[i]; /* in pixel, offset of the symbol in mem data */
		width=sym_page->symwidth[i]; /* width of the symbol */
#if 0 /*  for test -----------------------------------*/
		if(i=='M')
			printf(" width of 'M' is %d, offset is %d \n",width,offset);
#endif /*  test end -----------------------------------*/

                for(j=0;j<all_height;j++) /* for each pixel row of a symbol */
                {
                        /* in image file: seek position for pstart of a row, in bytes. 2bytes per pixel */
                        if( lseek(fd,(y0+j)*SYM_IMGPAGE_WIDTH*2+x0*2,SEEK_SET)<0 ) /* in bytes */
                        {
                                perror("lseek symbol image file");
				//EGI_PLOG(LOGLV_ERROR,"lseek symbol image fails.%s \n",strerror(errno));
				symbol_release_page(sym_page);
                                return NULL;
                        }

                        /* in mem data: read each row pixel data form image file to sym_page.data,
			2bytes per pixel, read one row pixel data each time */
                        if( read(fd, (uint8_t *)(sym_page->data+offset+width*j), width*2) < width*2 )
                        {
                                perror("read symbol image file");
	 			symbol_release_page(sym_page);
                                return NULL;
                        }

                }

        }
        printf("finish reading %s.\n",sym_page->path);

#if 0	/* for test ---------------------------------- */
	i='M';
	offset=sym_page->symoffset[i];
	width=sym_page->symwidth[i];
	for(j=0;j<all_height;j++)
	{
		for(k=0;k<width;k++)
		{
			if( *(uint16_t *)(sym_page->data+offset+width*j+k) != 0xFFFF)
				printf("*");
			else
				printf(" ");
		}
		printf("\n");
	}
#endif /*  test end -----------------------------------*/

	close(fd);
	EGI_PLOG(LOGLV_INFO,"symbol_load_page(): succeed to load symbol image file %s!\n", sym_page->path);
	//printf("sym_page->data = %p \n",sym_page->data);
	return (uint16_t *)sym_page->data;
}


/*--------------------------------------------------
	Release data in a symbol page
---------------------------------------------------*/
void symbol_release_page(struct symbol_page *sym_page)
{
	if(sym_page==NULL)
		return;

	if(sym_page->data != NULL) {
		//printf("%s: free(sym_page->data) ...\n",__func__);
		free(sym_page->data);
		sym_page->data=NULL;
	}

	if(sym_page->alpha != NULL) {
		//printf("%s: free(sym_page->alpah) ...\n",__func__);
		free(sym_page->alpha);
		sym_page->alpha=NULL;
	}

	if(sym_page->symoffset != NULL) {
		//printf("%s: free(sym_page->symoffset) ...\n",__func__);
		free(sym_page->symoffset);
		sym_page->symoffset=NULL;
	}

	/* TODO:  symwidth is NOT dynamically allocated */
	sym_page->symwidth=NULL;
//	if(sym_page->symwidth != NULL) {
//		printf("%s: free(sym_page->symwidth) ...\n",__func__);
//		free(sym_page->symwidth);
//		sym_page->symwidth=NULL;
//	}

	sym_page->path=NULL;


}


/*-----------------------------------------------------------------------
check integrity of a ((loaded)) page structure

sym_page: a loaded page
func:	  function name of the caller

return:
	0	OK
	<0	fails
-----------------------------------------------------------------------*/
int symbol_check_page(const struct symbol_page *sym_page, char *func)
{

	/* check sym_page */
	if(sym_page==NULL)
        {
                printf("%s(): symbol_page is NULL! .\n",func);
                return -1;
        }
        /* check for maxnum */
        if(sym_page->maxnum < 0 )
        {
                printf("%s(): symbol number less than 1! fail to load page.\n",func);
                return -2;
        }
        /* check for data */
        if(sym_page->data == NULL)
        {
                printf("%s(): sym_page->data is NULL! the symbol page has not been loaded?!\n",func);
	                return -3;
        }
        /* check for symb_index */
        if(sym_page->symwidth == NULL)
        {
                printf("%s(): symbol width list is empty!\n",func);
                return -4;
        }

	return 0;
}





/*--------------------------------------------------------------------------
print all symbol in a mem page.
print '*' if the pixel data is not 0.

sym_page: 	a mem symbol page pointer.
transpcolor:	color treated as transparent.
		black =0x0000; white = 0xffff;
----------------------------------------------------------------------------*/
void symbol_print_symbol( const struct symbol_page *sym_page, int symbol, uint16_t transpcolor)
{
        int i;
	int j,k;

	/* check page first */
	if(symbol_check_page(sym_page,"symbol_print_symbol") != 0)
		return;

	i=symbol;

#if 1 /* TEST ---------- */
	printf("symheight=%d, symwidth=%d \n", sym_page->symheight, sym_page->symwidth[i]);
#endif

	for(j=0;j<sym_page->symheight;j++) /*for each row of a symbol */
	{
		for(k=0;k<sym_page->symwidth[i];k++)
		{
			/* if not transparent color, then print the pixel */
			if(sym_page->alpha==NULL) {
				if( *(uint16_t *)( sym_page->data+(sym_page->symoffset)[i] \
						+(sym_page->symwidth)[i]*j +k ) != transpcolor ) {
                	                       printf("*");
				}
                        	       else
                                	       printf(" ");
			}
			else {  /* use alpha value */

				if( *(unsigned char *)(sym_page->alpha+(sym_page->symoffset)[i] \
						+(sym_page->symwidth)[i]*j +k ) > 0 )  {

						printf("*");
				}
					else
						printf(" ");
			}
		}
	     	printf("\n"); /* end of each row */
	}
}


/*-----------------------------------------------------
	save mem of a symbol page to a file
-------------------------------------------------------*/
void symbol_save_pagemem(struct symbol_page *sym_page)
{


}

/*------------------------------------------
	Get string length in pixel.
@str	string
@font	font for the string
-------------------------------------------*/
int symbol_string_pixlen(char *str, const struct symbol_page *font)
{
	int i;
        int len=strlen(str);
	int pixlen=0;

	if( len==0 || font==NULL)
		return 0;

        for(i=0;i<len;i++)
        {
                /* only if in code range */
                if( str[i] <= font->maxnum )
                           pixlen += font->symwidth[ (unsigned int)str[i] ];
        }

	return pixlen;
}


/*------------------------------------------------------------------------------------------
		write a symbol/font pixel data to FB device

1. Write a symbol data to FB.
2. Only alpha data is available in a FT2 symbol page, use WEGI_COLOR_BLACK as default front
   color.
4. Note: put page check in symbol_string_writeFB()!!!

@fbdev: 		FB device
@sym_page: 	symbol page

@transpcolor: 	>=0 transparent pixel will not be written to FB, so backcolor is shown there.
	     	<0	 no transparent pixel

@fontcolor:	font color(symbol color for a symbol)
		>= 0,  use given font color.
		<0  ,  use default color in img data.

use following COLOR:
#define SYM_NOSUB_COLOR -1  --- no substitute color defined for a symbol or font
#define SYM_NOTRANSP_COLOR -1  --- no transparent color defined for a symbol or font

@x0,y0: 		start position coordinate in screen, left top point of a symbol.
@sym_code: 	symbol code number

@opaque:	set aplha value (0-255)
		<0	No effect, or use symbol alpha value.
		0 	100% back ground color/transparent
		255	100% front color

-----------------------------------------------------------------------------------------*/
void symbol_writeFB(FBDEV *fb_dev, const struct symbol_page *sym_page, 	\
		int fontcolor, int transpcolor, int x0, int y0, unsigned int sym_code, int opaque)
{
	/* check data */
	if( sym_page==NULL || (sym_page->data==NULL && sym_page->alpha==NULL) ) {
		printf("%s: Input symbol page has no valid data inside.\n",__func__);
		return;
	}

	int i,j;
	FBPIX fpix;
	long int pos; /* offset position in fb map */
	int xres=fb_dev->vinfo.xres; /* x-resolusion = screen WIDTH240 */
	int yres=fb_dev->vinfo.yres;
	int mapx=0,mapy=0; /* if need ROLLBACK effect,then map x0,y0 to LCD coordinate range when they're out of range*/
	uint16_t pcolor;
	unsigned char palpha=0;
	uint16_t *data=sym_page->data; /* symbol pixel data in a mem page, for FT2 sympage, it's NULL! */
	int offset;
	long poff;
	int height=sym_page->symheight;
	int width;


	//long int screensize=fb_dev->screensize;

	/* check page */
#if 0  /* not here, put page check in symbol_string_writeFB() */
	if(symbol_check_page(sym_page, "symbol_writeFB") != 0)
		return;
#endif

	/* check sym_code */
	if( sym_code > sym_page->maxnum && sym_page->symtype!=symtype_FT2 ) {
		EGI_PLOG(LOGLV_ERROR,"symbole code number out of range! sympg->path: %s\n", sym_page->path);
		return;
	}

	/* get symbol/font width, only 1 character in FT2 symbol page NOW!!! */
	if(sym_page->symtype==symtype_FT2) {
		width=sym_page->ftwidth;
		offset=0;
	}
	else {
		width=sym_page->symwidth[sym_code];
		offset=sym_page->symoffset[sym_code];
	}

	/* get symbol pixel and copy it to FB mem */
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			/*  skip pixels according to opaque value, skipped pixels
							make trasparent area to the background */
//			if(opaque>0)
//			{
//				if( ((i%2)*(opaque/2)+j)%(opaque) != 0 ) /* make these points transparent */
//					continue;
//			}


#ifdef FB_SYMOUT_ROLLBACK  /* NOTE !!!  FB pos_rotate NOT applied  */

			/*--- map x for every (i,j) symbol pixel---*/
			if( x0+j<0 )
			{
				mapx=xres-(-x0-j)%xres; /* here mapx=1-240 */
				mapx=mapx%xres;
			}
			else if( x0+j>xres-1 )
				mapx=(x0+j)%xres;
			else
				mapx=x0+j;

			/*--- map Y for every (i,j) symbol pixel---*/
			if ( y0+i<0 )
			{
				/* in case y0=0 and i=0,then mapy=320!!!!, then function will returns 
				and abort drawing the symbol., don't forget -1 */
				mapy=yres-(-y0-i)%yres; /* here mapy=1-320 */
				mapy=mapy%yres;
			}
			else if(y0+i>yres-1)
				mapy=(y0+i)%yres;
			else
				mapy=y0+i;




#else /*--- if  NO ROLLBACK ---*/

			/* -----  FB ROTATION POSITION MAPPING -----
			 * IF 90 Deg rotated: Y maps to (xres-1)-FB.X,  X maps to FB.Y
			 */
		       switch(fb_dev->pos_rotate) {
        		        case 0:                 /* FB defaul position */
					mapx=x0+j;
					mapy=y0+i;
	                       		 break;
	        	        case 1:                 /* Clockwise 90 deg */
					mapx=(xres-1)-(y0+i);
					mapy=x0+j;
		                        break;
        		        case 2:                 /* Clockwise 180 deg */
					mapx=(xres-1)-(x0+j);
					mapy=(yres-1)-(y0+i);
        	               		 break;
		                case 3:                 /* Clockwise 270 deg */
					mapx=y0+i;
					mapy=(yres-1)-(x0+j);
		                        break;
		        }


			/* ignore out ranged points */
			if(mapx>(xres-1) || mapx<0 )
				continue;
			if(mapy>(yres-1) || mapy<0 )
				continue;

#endif
			/*x(i,j),y(i,j) mapped to LCD(xy),
				however, pos may also be out of FB screensize  */
			pos=mapy*xres+mapx; 	/* in pixel, LCD fb mem position */
			poff=offset+width*i+j; 	/* offset to pixel data */

			if(sym_page->alpha)
				palpha=*(sym_page->alpha+poff);  	/*  get alpha */

			/* for FT font sympage, only alpah value */
			if( data==NULL ) {
				pcolor=WEGI_COLOR_BLACK;
			}
			/* get symbol pixel in page data */
			else {
				pcolor=*(data+poff);
			}

			/* ------- assign color data one by one,faster then memcpy  --------
			   Wrtie to FB only if:
			   1.  alpha value exists.
			   2.  OR(no transp. color applied)
			   3.  OR (write only untransparent pixel)
			   otherwise,if pcolor==transpcolor, DO NOT write to FB
			*/
			if( sym_page->alpha || transpcolor<0 || pcolor!=transpcolor ) /* transpcolor applied befor COLOR FLIP! */
			{
				/* push original fb data to FB FILO, before write new color */
//				if( (transpcolor==7 || transpcolor==-7) && fb_dev->filo_on )
				if(fb_dev->filo_on) {
					fpix.position=pos<<1; /* pixel to bytes, !!! FAINT !!! */
					fpix.color=*(uint16_t *)(fb_dev->map_fb+(pos<<1));
					//printf("symbol push FILO: pos=%ld.\n",fpix.position);
					egi_filo_push(fb_dev->fb_filo,&fpix);
				}

				/* If use complementary color in image page,
				 * It will not apply if alpha value exists.
				 */
				if(TESTFONT_COLOR_FLIP && sym_page->alpha==NULL ) {
					pcolor = ~pcolor;
				}

				/* check available space for a 2bytes pixel color fb memory boundary,
				  !!! though FB has self roll back mechanism.  */
				pos<<=1; /*pixel to byte,  pos=pos*2 */
			        if( pos > (fb_dev->screensize-sizeof(uint16_t)) )
        			{
	                		printf("symbol_writeFB(): WARNING!!! symbol point reach boundary of FB mem.!\n");
					printf("pos=%ld, screensize=%ld    mapx=%d,mapy=%d\n",
						 pos, fb_dev->screensize, mapx, mapy);
                			return;
        			}

				/*  if use given symbol/font color  */
				if(fontcolor >= 0)
					pcolor=(uint16_t)fontcolor;

				/* if apply alpha: front pixel, background pixel,alpha value */
				if(opaque>=0) {
                    			pcolor=COLOR_16BITS_BLEND(  pcolor,
								    *(uint16_t *)(fb_dev->map_fb+pos),
								    opaque );
				}
				else if(sym_page->alpha) {
                    			pcolor=COLOR_16BITS_BLEND(  pcolor,
								    *(uint16_t *)(fb_dev->map_fb+pos),
								    palpha );
				}

				/* write to FB */
				*(uint16_t *)(fb_dev->map_fb+pos)=pcolor; /* in pixel, deref. to uint16_t */
			}
		}
	}
}


/*------------------------------------------------------------------------------
1. write a symbol/font string to FB device.
2. Write them at the same line.
3. If write symbols, just use symbol codes[] for str[].
4. If it's font, then use symbol bkcolor as transparent tunnel.

fbdev: 		FB device
sym_page: 	a font symbol page
fontcolor:	font color (or symbol color for a symbol)
		>= 0, use given font color.
		<0   use default color in img data
transpcolor: 	>=0 transparent pixel will not be written to FB, so backcolor is shown there.
		    for fonts and icons,
	     	<0	 --- no transparent pixel

use following COLOR:
#define SYM_NOSUB_COLOR -1  --- no substitute color defined for a symbol or font
#define SYM_NOTRANSP_COLOR -1 --- no transparent color defined for a symbol or font

x0,y0: 		start position coordinate in screen, left top point of a symbol.
str:		pointer to a char string(or symbol codes[]);

opaque:		set aplha value (0-255)
		<0	No effect, or use symbol alpha value.
		0 	100% back ground color/transparent
		255	100% front color
-------------------------------------------------------------------------------*/
void symbol_string_writeFB(FBDEV *fb_dev, const struct symbol_page *sym_page, 	\
		int fontcolor, int transpcolor, int x0, int y0, const char* str, int opaque)
{
	const char *p=str;
	int x=x0;

	/* check page data */
	if(symbol_check_page(sym_page, "symbol_writeFB") != 0)
		return;

	/* if the symbol is font then use symbol back color as transparent tunnel */
	//if(tspcolor >0 && sym_page->symtype == symtype_font )

	/* transpcolor applied for both font and icon anyway!!! */
	if(transpcolor>=0 && sym_page->bkcolor>=0 )
		transpcolor=sym_page->bkcolor;

	while(*p) /* code '0' will be deemed as end token here !!! */
	{
		symbol_writeFB(fb_dev,sym_page,fontcolor,transpcolor,x,y0,*p,opaque);/* at same line, so y=y0 */
		x+=sym_page->symwidth[(int)(*p)]; /* increase current x position */
		p++;
	}
}



/*-----------------------------------------------------------------------------------------
0. Extended ASCII symbols are not supported now!!!
1. write strings to FB device.
2. It will automatically return to next line if current line is used up,
   or if it gets a return code.
3. If write symbols, just use symbol codes[] for str[].
4. If it's font, then use symbol bkcolor as transparent tunnel.
5. Max dent space at each line end is 3 SPACEs, OR modify it.

fbdev: 		FB device
sym_page: 	a font symbol page
pixpl:		pixels per line.
lines:		number of lines available.
gap:		space between two lines, in pixel.
fontcolor:	font color (or symbol color for a symbol)
		>= 0, use given font color.
		<0   use default color in img data
transpcolor: 	>=0 transparent pixel will not be written to FB, so backcolor is shown there.
		    for fonts and icons,
	     	<0	 --- no transparent pixel
use following COLOR:
#define SYM_NOSUB_COLOR -1  --- no substitute color defined for a symbol or font
#define SYM_NOTRANSP_COLOR -1 --- no transparent color defined for a symbol or font
x0,y0: 		start position coordinate in screen, left top point of a symbol.
str:		pointer to a char string(or symbol codes[]);

opaque:		set aplha value (0-255)
		<0	No effect, or use symbol alpha value.
		0 	100% back ground color/transparent
		255	100% front color

TODO: TAB as 8*SPACE.

return:
		>=0   	bytes write to FB
		<0	fails
---------------------------------------------------------------------------------------------*/
int  symbol_strings_writeFB( FBDEV *fb_dev, const struct symbol_page *sym_page, unsigned int pixpl,
			     unsigned int lines,  unsigned int gap, int fontcolor, int transpcolor,
			     int x0, int y0, const char* str, int opaque )
{
	const char *p=str;
	const char *tmp;
	int x=x0;
	int y=y0;
	unsigned int pxl=pixpl; /* available pixels remainded in current line */
	unsigned int ln=0; /* lines used */
	int cw; 	/* char width, in pixel */
	int ww; 	/* word width, in pixel */
	bool check_word;

	/* check lines */
	if(lines==0)
		return -1;

	/* check page data */
	if(symbol_check_page(sym_page, "symbol_writeFB") != 0)
		return -2;

	/* if the symbol is font then use symbol back color as transparent tunnel */
	//if(tspcolor >0 && sym_page->symtype == symtype_font )

	/* use bkcolor for both font and icon anyway!!! */
	if(transpcolor>=0)
		transpcolor=sym_page->bkcolor;

	check_word=true;


	while(*p) /* code '0' will be deemed as end token here !!! */
	{
		/* skip extended ASCII symbols */
		if( *p > 127 ) {
			p++;
			continue;
		}

#if 0	/////////////  METHOD-1: Check CHARACTER after CHARACTER for necesary space  ////////////

		/* 1. Check whether remained space is enough for the CHARACTER,
  		 * or, if its a return code.
		 * 2. Note: If the first char for a new line is a return code, it returns again,
		 * and it may looks not so good!
		 */
		cw=sym_page->symwidth[(int)(*p)];
		if( pxl < cw || (*p)=='\n' )
		{
			ln++;
			if(ln>=lines) /* no lines available */
				return (p-str);
			y += gap + sym_page->symheight; /* move to next line */
			x = x0;
			pxl=pixpl;
		}

#else 	/////////////  METHOD-2:  Check WORD after WORD for necessary space  ////////////

	if( check_word || *p==' ' || *p=='\n' ) {	/* if a new word begins */

		/* 0. reset tmp and ww */
		tmp=(char *)p;
		ww=0;

		/* 1. If first char is not SPACE: get length of non_space WORD  */
		if(*p != ' ') {
		   	while(*tmp) {
				if( (*tmp != '\n') && (*tmp != ' ') ) {
					ww += sym_page->symwidth[(int)(*tmp)];
					tmp++;
				}
				else {
					break; /* break if SPACE or RETURN, as end of a WORD */
				}
			}
		}
		/* 2. Else if first char is SPACE: each SPACE deemed as one WORD */
		else {  /* ELSE IF *p == ' ' */
				ww += sym_page->symwidth[(int)(*p)];
		}

		/* 3. If not enough space for the WORD, or a RETURN for the first char */
		/* 3.1 if WORD length > pixpl */
		if( ww > pixpl ) {
                        /* This WORD is longer than a line!
			 * Do nothing, do not start a new line.
			 */
		}
		/* 3.2 set MAX pxl limit here for a long WORD at a line end,
		 * It will not start a new line here if pxl is big enough, but check cw by cw later.
		 * Just for good looking!
		 */
		else if( pxl < ww  &&  pxl > 3*sym_page->symwidth[' '] ) {   /* Max dents at line end, 3 SPACE */
			/* Do nothing, do not start a new line */
		}
		/* 3.3 Otherwise start a new line */
		else if( pxl < ww || *p == '\n' ) {
			ln++;
			if(ln>=lines) /* no lines available */
				return (p-str);
			y += gap + sym_page->symheight;
			x = x0;
			pxl=pixpl;
		}

	        /* reset check_word */
		check_word=false;
	}

	/*  if current char is SPACE or RETURN, we set check_work again for NEXT WORD!!!!
	 *  If current character is not SPACE/control_char, no need to check again.
	 */
	if( *p==' ' || *p=='\n' )
		check_word=true;

	/* process current character */
	cw=sym_page->symwidth[(int)(*p)];

	/* in case we set limit for ww>pxl in above, cw is a char of that long WORD */
	if(cw>pxl) {

		/* shift to next line */
		ln++;
		if(ln>=lines) /* no lines available */
			return (p-str);
		y += gap + sym_page->symheight;
		x = x0;
		pxl=pixpl;

		/* set check_word for next char */
		check_word=true;

		continue;
	}

	/* for control character */
//	if( cw==0 ) {
//		check_word=true;
//		p++;
//		continue;
//	}

#endif  /////////////////////////  END METHOD SELECTION //////////////////////////

		symbol_writeFB(fb_dev,sym_page,fontcolor,transpcolor,x,y,*p,opaque);
		x+=cw;
		pxl-=cw;
		p++;
	}

	return p-str;
}

/*-------------------------------------------------------------------------------
display each symbol in a char string to form a motion picture.

dt:		interval delay time for each symbol in (ms)
sym_page:       a font symbol page
transpcolor:    >=0 transparent pixel will not be written to FB, so backcolor is shown there.
                <0       --- no transparent pixel
use following COLOR:
#define SYM_NOSUB_COLOR -1  --- no substitute color defined for a symbol or font
#define SYM_NOTRANSP_COLOR -1 --- no transparent color defined for a symbol or font

x0,y0:          start position coordinate in screen, left top point of a symbol.
str:            pointer to a char string(or symbol codes[]);
		!!! code /0 as the end token of the string.

-------------------------------------------------------------------------------*/
void symbol_motion_string(FBDEV *fb_dev, int dt, const struct symbol_page *sym_page,   \
                			int transpcolor, int x0, int y0, const char* str)
{
        const char *p=str;

        /* check page data */
        if(symbol_check_page(sym_page, "symbol_writeFB") != 0)
                return;

        /* use bkcolor for both font and icon anyway!!! */
        if(transpcolor>=0)
                transpcolor=sym_page->bkcolor;

        while(*p) /* code '0' will be deemed as end token here !!! */
       	{
               	symbol_writeFB(fb_dev,sym_page,SYM_NOSUB_COLOR,transpcolor,x0,y0,*p,-1); /* -1, default font color */
		tm_delayms(dt);
           	p++;
       	}
}



/*--------------------------------------------------------------------------
Rotate a symbol, use its bkcolor as transcolor

sym_page: 	symbol page
x0,y0: 		start position coordinate in screen, left top point of a symbol.
sym_code:	symbole code
------------------------------------------------------------------------------*/
void symbol_rotate(const struct symbol_page *sym_page,
						 int x0, int y0, int sym_code)
{
        /* check page data */
        if(symbol_check_page(sym_page, "symbol_rotate") != 0)
                return;

	int i,j;
        uint16_t *data=sym_page->data; /* symbol pixel data in a mem page */
        int offset=sym_page->symoffset[sym_code];
        int height=sym_page->symheight;
        int width=sym_page->symwidth[sym_code];
	int max= height>width ? height : width;
	int n=((max/2)<<1)+1;/*  as form of 2*m+1  */
	uint16_t *symbuf;

	/* malloc symbuf */
	symbuf=malloc(n*n*sizeof(uint16_t));
	if(symbuf==NULL)
	{
		printf("symbol_rotate(): fail to malloc() symbuf.\n");
		return;
	}
        memset(symbuf,0,sizeof(n*n*sizeof(uint16_t)));

	/* copy data to fill symbuf with pixel number n*n */
        for(i=0;i<height;i++)
        {
                for(j=0;j<width;j++)
                {
			/* for n >= height and widt */
			symbuf[i*n+j]=*(data+offset+width*i+j);
		}
	}


        /* for image rotation matrix */
        struct egi_point_coord  *SQMat; /* the map matrix*/
        SQMat=malloc(n*n*sizeof(struct egi_point_coord));
	if(SQMat==NULL)
	{
		printf("symbol_rotate(): fail to malloc() SQMat.\n");
		return;
	}
        memset(SQMat,0,sizeof(*SQMat));

	/* rotation center */
        struct egi_point_coord  centxy={x0+n/2,y0+n/2}; /* center of rotation */
        struct egi_point_coord  x0y0={x0,y0};

	i=0;
	while(1)
	{
		i++;
              	/* get rotation map */
                mat_pointrotate_SQMap(n, 2*i, centxy, SQMat);/* side,angle,center, map matrix */
                /* draw rotated image */
                fb_drawimg_SQMap(n, x0y0, symbuf, SQMat); /* side,center,image buf, map matrix */
		tm_delayms(20);
	}
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
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>
#include <arpa/inet.h>
#include FT_FREETYPE_H

/* -------------------------------------------------------------------------------------------
Load a ASCII symbol_page struct from a font file by calling FreeType2 libs.

Note:
0. This is for pure western ASCII alphabets font set.
   For CJK font set, all alphabets are aligned with same baseline, so it's not necessary to calculate
   symheight.

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
int  symbol_load_asciis_from_fontfile(	struct symbol_page *symfont_page, const char *font_path,
				       	int Wp, int Hp
				      )
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
  	//int		np;
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
#if 1
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
#endif


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

			ASSUME: bitmap start from hi=0 !!!

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


