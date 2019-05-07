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
----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /*close*/
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "egi_fbgeom.h"
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
	.symtype=type_font,
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

/* symbole page struct for testfont */
struct symbol_page sympg_numbfont=
{
	.symtype=type_font,
	.path="/home/numbfont.img",
	.bkcolor=0x0000,
	.data=NULL,
	.maxnum=16*4-1,
	.sqrow=16,
	.symheight=20,
	.symwidth=numbfont_width, /* width list */
};


/*--------------------------(  buttons  )-----------------------------------*/
static int buttons_width[4*5] =  /* check with maxnum */
{
	60,60,60,60,
	60,60,60,60,
	60,60,60,60,
	48,48,48,48,
	48,48,48,48,
};
/* symbole page struct for testfont */
struct symbol_page sympg_buttons=
{
	.symtype=type_icon,
	.path="/home/buttons.img",
	.bkcolor=0x0000,
	.data=NULL,
	.maxnum=4*5-1, /* 5 rows of ioncs */
	.sqrow=4, /* 4 icons per row */
	.symheight=60,
	.symwidth=buttons_width, /* width list */
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

/* symbole page struct for testfont */
struct symbol_page sympg_icons=
{
        .symtype=type_font,
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
static int icons_2_width[4*6] =  /* element number MUST >= maxnum */
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
        .symtype=type_icon,
        .path="/home/icons_2.img",
        .bkcolor=0x0000,
        .data=NULL,
        .maxnum=4*5-1, /* 11 rows of ioncs */
        .sqrow=4, /* 8 icons per row */
        .symheight=60,
        .symwidth=icons_2_width, /* width list */
};


/* -----  All static functions ----- */
static uint16_t *symbol_load_page(struct symbol_page *sym_page);
static void symbol_free_page(struct symbol_page *sym_page);


/*-------------------------------------------------------
Load all symbol files into mem pages
Don't forget to change symbol_free_allpages() accordingly
return:
	0	OK
	<0	Fail
-------------------------------------------------------*/
int symbol_load_allpages(void)
{
        /* load testfont */
        if(symbol_load_page(&sympg_testfont)==NULL)
                return -1;
        /* load numbfont */
        if(symbol_load_page(&sympg_numbfont)==NULL)
                return -2;
        /* load buttons icons */
        if(symbol_load_page(&sympg_buttons)==NULL)
                return -3;
        /* load icons for home head-bar*/
        if(symbol_load_page(&sympg_icons)==NULL)
                return -4;
        /* load icons for PLAYERs */
        if(symbol_load_page(&sympg_icons_2)==NULL)
                return -5;

	return 0;
}

/* --------------------------------------
	Free all mem pages
----------------------------------------*/
void symbol_free_allpages(void)
{
	symbol_free_page(&sympg_testfont);
	symbol_free_page(&sympg_numbfont);
	symbol_free_page(&sympg_buttons);
	symbol_free_page(&sympg_icons);
	symbol_free_page(&sympg_icons_2);
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

	/* open symbol image file */
	fd=open(sym_page->path, O_RDONLY);
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
	{
		if(sym_page->symoffset == NULL)
		{
			printf("symbol_load_page():fail to malloc sym_page->symoffset!\n");
			return NULL;
		}
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
			symbol_free_page(sym_page);
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
			symbol_free_page(sym_page);
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
				symbol_free_page(sym_page);
                                return NULL;
                        }

                        /* in mem data: read each row pixel data form image file to sym_page.data,
			2bytes per pixel, read one row pixel data each time */
                        if( read(fd, (uint8_t *)(sym_page->data+offset+width*j), width*2) < width*2 )
                        {
                                perror("read symbol image file");
	 			symbol_free_page(sym_page);
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
	Free mem for data in a symbol page
---------------------------------------------------*/
static void symbol_free_page(struct symbol_page *sym_page)
{
	if(sym_page->data != NULL)
	{
		free(sym_page->data);
		sym_page->data=NULL;
	}
	if(sym_page->symwidth != NULL)
	{
		free(sym_page->symwidth);
		sym_page->symwidth=NULL;
	}

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

	for(j=0;j<sym_page->symheight;j++) /*for each row of a symbol */
	{
		for(k=0;k<sym_page->symwidth[i];k++)
		{
			/* if not transparent color, then print the pixel */
			if( *(uint16_t *)( sym_page->data+(sym_page->symoffset)[i] \
					+(sym_page->symwidth)[i]*j +k ) != transpcolor )
                                       printf("*");
                               else
                                       printf(" ");
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


/*----------------------------------------------------------------------------------
	write a symbol/font pixel data to FB device

1. Write to FB in unit of symboy_width*pixel if no color treatment applied for symbols,
   or in pixel if color treatment is applied.
2. So the method of checking FB left space is different, depending on above mentioned
   writing methods.
3. Note: put page check in symbol_string_writeFB()!!!

fbdev: 		FB device
sym_page: 	symbol page

transpcolor: 	>=0 transparent pixel will not be written to FB, so backcolor is shown there.
	     	<0	 no transparent pixel

fontcolor:	font color(symbol color for a symbol)
		>= 0,  use given font color.
		<0  ,  use default color in img data.

use following COLOR:
#define SYM_NOSUB_COLOR -1  --- no substitute color defined for a symbol or font
#define SYM_NOTRANSP_COLOR -1  --- no transparent color defined for a symbol or font

x0,y0: 		start position coordinate in screen, left top point of a symbol.
sym_code: 	symbol code number

opaque:		set opaque value
	0 	display no effect
	other	...OK.  the greater the value, the vaguer the image.
-------------------------------------------------------------------------------*/
void symbol_writeFB(FBDEV *fb_dev, const struct symbol_page *sym_page, 	\
		int fontcolor, int transpcolor, int x0, int y0, int sym_code, int opaque)
{
	int i,j;
	FBDEV *dev = fb_dev;
	FBPIX fpix;
	long int pos; /* offset position in fb map */
	int xres=dev->vinfo.xres; /* x-resolusion = screen WIDTH240 */
	int yres=dev->vinfo.yres;
	int mapx=0,mapy=0; /* if need ROLLBACK effect,then map x0,y0 to LCD coordinate range when they're out of range*/
	uint16_t pcolor;
	uint16_t *data=sym_page->data; /* symbol pixel data in a mem page */
	int offset=sym_page->symoffset[sym_code];
	int height=sym_page->symheight;
	int width=sym_page->symwidth[sym_code];
	//long int screensize=fb_dev->screensize;

	/* check page */
#if 0  /* not here, put page check in symbol_string_writeFB() */
	if(symbol_check_page(sym_page, "symbol_writeFB") != 0)
		return;
#endif

	/* check sym_code */
	if( sym_code < 0 || sym_code > sym_page->maxnum )
	{
		EGI_PLOG(LOGLV_ERROR,"symbole code number out of range! sympg->path: %s\n", sym_page->path);
		return;
	}


	/* get symbol pixel and copy it to FB mem */
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			/*  skip pixels according to opaque value, skipped pixels
							make trasparent area to the background */
			if(opaque>0)
			{
				if( ((i%2)*(opaque/2)+j)%(opaque) != 0 ) /* make these points transparent */
					continue;
			}

#ifdef FB_SYMOUT_ROLLBACK
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

			mapx=x0+j;
			mapy=y0+i;
			/* ignore out ranged points */
			if(mapx>(xres-1) || mapx<0 )
				continue;
			if(mapy>(yres-1) || mapy<0 )
				continue;

#endif
			/*x(i,j),y(i,j) mapped to LCD(xy),
				however, pos may also be out of FB screensize  */
			pos=mapy*xres+mapx; /* in pixel, LCD fb mem position */
			pcolor=*(data+offset+width*i+j);/* get symbol pixel in page data */

			/* ------- assign color data one by one,faster then memcpy  --------
			   Wrtie to FB only if:
			   (no transp. color applied) OR (write only untransparent pixel) */
			if(transpcolor<0 || pcolor!=transpcolor ) /* transpcolor applied befor COLOR FLIP! */
			{
				/* push original fb data to FB FILO, before write new color */
//				if( (transpcolor==7 || transpcolor==-7) && fb_dev->filo_on )
				if(fb_dev->filo_on)
				{
					fpix.position=pos<<1; /* pixel to bytes, !!! FAINT !!! */
					fpix.color=*(uint16_t *)(dev->map_fb+(pos<<1));
					//printf("symbol push FILO: pos=%ld.\n",fpix.position);
					egi_filo_push(fb_dev->fb_filo,&fpix);
				}

				/* if use complementary color */
				if(TESTFONT_COLOR_FLIP)
				{
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
				if(fontcolor>=0)
					pcolor=(uint16_t)fontcolor;

				*(uint16_t *)(dev->map_fb+pos)=pcolor; /* in pixel, deref. to uint16_t */
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
-------------------------------------------------------------------------------*/
void symbol_string_writeFB(FBDEV *fb_dev, const struct symbol_page *sym_page, 	\
		int fontcolor, int transpcolor, int x0, int y0, const char* str)
{
	const char *p=str;
	int x=x0;

	/* check page data */
	if(symbol_check_page(sym_page, "symbol_writeFB") != 0)
		return;

	/* if the symbol is font then use symbol back color as transparent tunnel */
	//if(tspcolor >0 && sym_page->symtype == type_font )

	/* use bkcolor for both font and icon anyway!!! */
	if(transpcolor>=0)
		transpcolor=sym_page->bkcolor;

	while(*p) /* code '0' will be deemed as end token here !!! */
	{
		symbol_writeFB(fb_dev,sym_page,fontcolor,transpcolor,x,y0,*p,0);/* at same line, so y=y0 */
		x+=sym_page->symwidth[(int)(*p)]; /* increase current x position */
		p++;
	}
}


/*---------------------------------------------------------------------------
1. write strings to FB device.
2. It will auto. return to next line to write if current line is used up,
   or if it gets a return code.
3. If write symbols, just use symbol codes[] for str[].
4. If it's font, then use symbol bkcolor as transparent tunnel.

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
-------------------------------------------------------------------------------*/
void symbol_strings_writeFB( FBDEV *fb_dev, const struct symbol_page *sym_page, unsigned int pixpl,
			     unsigned int lines,  unsigned int gap, int fontcolor, int transpcolor,
			     int x0, int y0, const char* str )
{
	const char *p=str;
	int x=x0;
	int y=y0;
	unsigned int pxl=pixpl; /* available pixels in current line */
	unsigned int ln=0; /* lines used */
	int cw; /* char width, in pixel */

	/* check lines */
	if(lines==0)return;

	/* check page data */
	if(symbol_check_page(sym_page, "symbol_writeFB") != 0)
		return;

	/* if the symbol is font then use symbol back color as transparent tunnel */
	//if(tspcolor >0 && sym_page->symtype == type_font )

	/* use bkcolor for both font and icon anyway!!! */
	if(transpcolor>=0)
		transpcolor=sym_page->bkcolor;

	while(*p) /* code '0' will be deemed as end token here !!! */
	{
		/* 1. Check whether remained space is enough for the char,
  		 * or, if its a return code.
		 * 2. Note: If the first char for a new line is a return code, it returns again,
		 * and it may looks not so good!
		 */
		cw=sym_page->symwidth[(int)(*p)];
		if( pxl < cw || (*p)=='\n' )
		{
			ln++;
			if(ln>=lines) /* no lines available */
				return;
			y += gap + sym_page->symheight; /* move to next line */
			x = x0;
			pxl=pixpl;
		}
		symbol_writeFB(fb_dev,sym_page,fontcolor,transpcolor,x,y,*p,0);
		x+=cw;
		pxl-=cw;
		p++;
	}
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
               	symbol_writeFB(fb_dev,sym_page,SYM_NOSUB_COLOR,transpcolor,x0,y0,*p,0); /* -1, default font color */
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
