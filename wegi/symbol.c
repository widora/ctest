/*----------------------------------------------------------------------------
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
6.


TODO:
0.  different type symbol now use same writeFB function !!!!! font and icon different writeFB func????
0.  if image file is not complete.
1.  void symbol_save_pagemem( )
2.  void symbol_writeFB( ) ... copy all data if no transp. pixel applied.
3.  data encode.
4.  symbol linear enlarge and shrink.
5. To read FBDE vinfo to get all screen/fb parameters as in fblines.c, it's improper in other source files.

Midas
----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /*close*/
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include "symbol.h"


/*--------------------(  testfont  )------------------------
  1.  ascii 0-127 symbol width,
  2.  5-pixel blank space for unprintable symbols, though 0-pixel seems also OK.
  3.  Please change the 'space' width according to your purpose.
*/
static int testfont_width[16*8] =
{
	/* give return code a 0 width in a txt display  */
//	5,5,5,5,5,5,5,5,5,5,0,5,5,5,5,5, /* unprintable symbol, give it 5 pixel wide blank */
//	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, /* unprintable symbol, give it 5 pixel wide blank */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* unprintable symbol */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	8,7,8,10,11,15,14,5,6,6,10,10,5,6,5,8, /* space,!"#$%&'()*+,-./ */
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
static int numbfont_width[16*8] =
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


/*--------------------------(  icon  )-----------------------------------*/
static int icon_width[60*2] =
{
	60,60,60,60,
	60,60,60,60,
	60,60,60,60,
};
/* symbole page struct for testfont */
struct symbol_page sympg_icon=
{
	.symtype=type_icon,
	.path="/home/icon.img",
	.bkcolor=0x0000,
	.data=NULL,
	.maxnum=4*3-1, /* 2 rows of ioncs */
	.sqrow=4, /* 4 icons per row */
	.symheight=60,
	.symwidth=icon_width, /* width list */
};






/*----------------------------------------------------------------
   load an img page file

   1. direct mmap.  (current implementation!)
      or
   2. load to a mem page.

path: 	path to the symbol image file
num: 	total number of symbols,or MAX code number-1;
height: heigh of all symbols
width:	list for all symbol widthes
sqrow:	symbol quantity in each row of an img page

------------------------------------------------------------------*/
uint16_t *symbol_load_page(struct symbol_page *sym_page)
{
	int fd;
	int datasize; /* memsize for all symbol data */
	int i,j;
	int x0,y0; /* origin position of a symbol in a image page, left top of a symbol */
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
			printf("symbol_load_page():fail to malloc sym_page->data!\n");
			symbol_release_page(sym_page);
			return NULL;
		}
	}



	/* read symbol pixel data from image file to sym_page.data */
        for(i=0; i<=sym_page->maxnum; i++) /* i for each symbol */
        {

		/* width of a symbol width MUST NOT be zero.!   --- zero also OK, */
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
	printf("symbol_load_page(): succeed to load symbol image file %s!\n", sym_page->path);
	//printf("sym_page->data = %p \n",sym_page->data);
	return (uint16_t *)sym_page->data;
}


/*--------------------------------------------------
	release mem for data in a symbol page
---------------------------------------------------*/
void symbol_release_page(struct symbol_page *sym_page)
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

        /* check for maxnum */
        if(sym_page->maxnum < 0 )
        {
                printf("%s(): symbol number less than 1! fail to load page.\n",func);
                return -1;
        }
        /* check for data */
        if(sym_page->data == NULL)
        {
                printf("%s(): sym_page->data is NULL! the symbol page has not been loaded?!\n",func);
	                return -2;
        }
        /* check for symb_index */
        if(sym_page->symwidth == NULL)
        {
                printf("%s(): symbol width list is empty!\n",func);
                return -3;
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

fbdev: 		FB device
sym_page: 	symbol page

transpcolor: 	>=0 transparent pixel will not be written to FB, so backcolor is shown there.
	     	<0	 no transparent pixel

fontcolor:	font color(symbol color for a symbol)
		>= 0, use given font color.
		<0   use default color in img data
use following COLOR:
#define SYM_NOSUB_COLOR -1  --- no substitute color defined for a symbol or font
#define SYM_NOTRANSP_COLOR -1  --- no transparent color defined for a symbol or font

x0,y0: 		start position coordinate in screen, left top point of a symbol.
sym_code: 	symbol code number
-------------------------------------------------------------------------------*/
void symbol_writeFB(FBDEV *fb_dev, const struct symbol_page *sym_page, 	\
		int fontcolor, int transpcolor, int x0, int y0, int sym_code)
{
	int i,j;
	FBDEV *dev = fb_dev;
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
#if 1 /* It wastes time. NO need here,we shall move this to .... */
	if(symbol_check_page(sym_page, "symbol_writeFB") != 0)
		return;
#endif

	/* check sym_code */
	if( sym_code < 0 || sym_code > sym_page->maxnum )
	{
		printf("symbole code number out of range!\n");
		return;
	}


	/* get symbol pixel and copy it to FB mem */
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
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
			mapy=y0+x;

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
                			printf("WARNING: symbol point reach boundary of FB mem.!\n");
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
		    for fonts symbol,
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
	int tspcolor=transpcolor;

	/* if the symbol is font then use symbol back color as transparent tunnel */
	if(tspcolor >0 && sym_page->symtype == type_font )
		tspcolor=sym_page->bkcolor;

	while(*p)
	{
		symbol_writeFB(fb_dev,sym_page,fontcolor,tspcolor,x,y0,*p);/* at same line, so y=y0 */
		x+=sym_page->symwidth[(int)(*p)]; /* increase current x position */
		p++;
	}
}
