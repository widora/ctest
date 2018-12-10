/*-----------------------------------------------------------------
Dict for symbols

A dict is a 240x320x2-bytes symbol page, every symbol is
presented by 5-6-5RGB pixels. Symbols can be fonts,icons,pics.

--- Dicts:
h20w15:  20x15 fonts.
s60icon: 60x60 icons.



Midas Zhou
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* read,write,close */
#include <stdint.h>
#include <fcntl.h> /* open */
#include <linux/fb.h>
#include "fblines.h"
#include "dict.h"

/* symbols dict data  */
//uint16_t dict_h20w15[DICT_NUM_LIMIT][20*15]; /* 20*15*2 bytes each symbol */
uint16_t *dict_h20w15;

/*----------------------------------------------
 malloc dict_h20w15

 return:
	 NULL     fails
	 	  succeed
-----------------------------------------------*/
uint16_t *dict_init_h20w15(void)
{
	if(dict_h20w15 !=NULL) {
		printf("dict_h20w16 is not NULL, fail to malloc!\n");
		return dict_h20w15;
	}

	dict_h20w15=malloc(DICT_NUM_LIMIT*20*15*sizeof(uint16_t));

	if(dict_h20w15 == NULL) {
		printf("fail to malloc for dict_h20w15!\n");
		return NULL;
	}

	return dict_h20w15;
}

/*-------------------------------------------------
free dict_h20w15
-------------------------------------------------*/
void dict_release_h20w15(void)
{
	if(dict_h20w15 !=NULL)
		free(dict_h20w15);
}

/*-------------------------------------------------
  display dict img 
--------------------------------------------------*/
void dict_display_img(FBDEV *fb_dev,char *path)
{
	int i;
	int fd;
	FBDEV *dev = fb_dev;
	int xres=dev->vinfo.xres;
	int yres=dev->vinfo.yres;
	uint16_t buf;

	/* open dict img file */
	fd=open(path,O_RDONLY);
	if(fd<0) {
		perror("open dict file");
		//printf("Fail to open dict file %s!\n",path);
		dict_release_h20w15();
		exit(-1);
	}

	for(i=0;i<xres*yres;i++)
	{
		/* read from img file one byte each time */
		read(fd,&buf,2);/* 2 bytes each pixel */
		/* write to fb one byte each time */
               	*(uint16_t *)(dev->map_fb+2*i)=buf; /* write to fb */
	}

	close(fd);
}



/*---------------------------------------------------------
malloc dict and load 20*15 symbols from img data to dict_h20w15
-----!!!!!!!!!!!!!!!!!!

Return:
	NULL		 fails
	pointer to dict  OK

----------------------------------------------------------*/
uint16_t *dict_load_h20w15(char *path)
{
	int fd;
	int i,j,k;
	uint16_t *dict;

	int x0,y0; /* start position of a symbol,in pixel */

	/* malloc dict data */
	if(dict_init_h20w15()==NULL)
		return NULL;

	/* -------- get pointer to dict NOW ! */
	dict=dict_h20w15;

	/* open dict img file */
	fd=open(path,O_RDONLY);
	if(fd<0) {
		perror("open dict file");
		//printf("Fail to open dict file %s!\n",path);
		//dict_release_h20w15();//release in main();
		return NULL;
	}

	/* read each symbol to dict */
	for(i=0;i<DICT_NUM_LIMIT;i++) /* i each symbol */
	{
		x0=(i%(DICT_IMG_WIDTH/15))*15; /* in pixel, (DICT_IMG_WIDTH/15) symbols in each row */
		y0=(i/(DICT_IMG_WIDTH/15))*20; /* in pixel */
		printf("x0=%d, y0=%d \n",x0,y0);
		for(j=0;j<20;j++) /* 20 row ,20-pixels */
		{
			/* seek position in byte. 2bytes per pixel */
			if( lseek(fd,(y0+j)*DICT_IMG_WIDTH*2+x0*2,SEEK_SET)<0 ) /* in bytes */
			{
				perror("lseek dict file");
				return NULL;
			}
			/* read row data,15-pixels x 2bytes */
			if( read(fd, (uint8_t *)(dict+i*20*15+j*15), 15*2) < 0 )
			{
				perror("read dict file");
				return NULL;
			}
#if 0
			/* confirm dict data */
			for(k=0;k<15;k++)
			{
				if(*(uint16_t *)(dict+i*20*15+k))
					printf("*");
				else
					printf(" ");
			}

			printf("\n");
#endif
		}
	}
	printf("finish reading %s.\n",path);

	close(fd);

	return dict;
}


/*--------------------------------------------------
	print all 20x15 symbols in a dict
-------------------------------------------------*/
void dict_print_symb20x15(uint16_t *dict)
{
	int i,j,k;

	for(i=0;i<DICT_NUM_LIMIT;i++) /* i each symbol */
	{
		for(j=0;j<20;j++) /* row */
		{
			for(k=0;k<15;k++) /* pixels in a row */
			{
				if(*(uint16_t *)(dict+i*20*15+j*15+k))
					printf("*");
				else
					printf(" ");
			}
			printf("\n");
		}
	}
}



/*------------------------------------------------------------------------------
write symbol to frame buffer.

blackoff: >0 make black pixels transparent
	  0  keep black pixel
symbol: pointer to symbol data
x0,y0:	start position in screen.

---------------------------------------------------------------------------------*/
void dict_display_symb20x15(int blackoff, FBDEV *fb_dev, const uint16_t *dict, int x0, int y0)
{
	int i,j,k;
	FBDEV *dev = fb_dev;
	int xres=dev->vinfo.xres;
	int yres=dev->vinfo.yres;
        long int pos=0; /* position in fb */
	uint16_t color;

	//printf("xres=%d,yres=%d \n",xres,yres);

	for(i=0;i<20;i++)  /* in pixel, 20 row each symbol */
	{
		for(j=0;j<15;j++)  /* in pixel, 15 column each symbol */
		{
			color=*(uint16_t *)(dict+i*15+j);
			pos=(y0+i)*xres+x0+j;/* in pixel */

			if(blackoff && color)/* if make black transparent */
			{
                		*(uint16_t *)(dev->map_fb+pos*2)=color;
			}
			else if(!blackoff) /* else keep black */
			{
                		*(uint16_t *)(dev->map_fb+pos*2)=color;
			}
#if 0
			/* confirm dict data */
				if( *(uint16_t *)(dict+i*15+j) )
					printf("*");
				else
					printf(" ");
#endif
		}
		printf("\n");
	}
}
