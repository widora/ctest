/*----------------------- egi_pic.c ------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou

egi pic type ebox functions

TODO: take data_pic->width,height or use imgbuf->width, height ?

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "egi_color.h"
#include "egi.h"
#include "egi_pic.h"
#include "egi_debug.h"
#include "egi_symbol.h"
#include "egi_bmpjpg.h"


/*-------------------------------------
      pic ebox self_defined methods
--------------------------------------*/
static EGI_METHOD picbox_method=
{
        .activate=egi_picbox_activate,
        .refresh=egi_picbox_refresh,
        .decorate=NULL,
        .sleep=NULL,
        .free=NULL, /* to see egi_ebox_free() in egi.c */
};



/*-----------------------------------------------------------------------------
Dynamically create pic_data struct

font_code:
int offx: 		offset from host ebox left top
int offy:
int height:		height and width for a EGI_IMGBUF
int width:
//char *title: 		title of the the picture showing above the window, or NULL
			default NULL
font: 			symbol page for the title, or NULL

//EGI_IMGBUF imgbuf:	image buffer for a picture, in R5G6B5 pixel format
			1.imgbuf->width or height must >0
			2.imgbuf->imgbuf may be NULL.

//char *fpath:		fpath to a picture file, or NULL

return:
        poiter          OK
        NULL            fail
-----------------------------------------------------------------------------*/
EGI_DATA_PIC *egi_picdata_new( int offx, int offy,
			       int height, int width,
			       int imgpx, int imgpy,
			       struct symbol_page *font
			     )
{
	/* check data */
	if(height==0 || width==0 )
	{
		printf("egi_picdata_new(): EGI_DATA_PIC height or width is 0, fail to proceed.\n");
		return NULL;
	}

	/* malloc a EGI_IMGBUF struct */
	EGI_PDEBUG(DBG_PIC,"egi_picdata_new(): malloc EGI_IMGBUF...\n");
	EGI_IMGBUF *imgbuf = malloc(sizeof(EGI_IMGBUF));
	if(imgbuf == NULL)
        {
                printf("egi_picdata_new(): fail to malloc EGI_IMGBUF.\n");
		return NULL;
	}
	memset(imgbuf,0,sizeof(EGI_IMGBUF));

	/* set height and width for imgbuf */
	imgbuf->height=height;
	imgbuf->width=width;

	/* malloc imgbuf->imgbuf */
	EGI_PDEBUG(DBG_PIC,"egi_picdata_new(): malloc imgbuf->imgbuf...\n");
	imgbuf->imgbuf = malloc(height*width*sizeof(uint16_t));
	if(imgbuf->imgbuf == NULL)
	{
		printf("egi_picdata_new(): fail to malloc imgbuf->imgbuf.\n");
		free(imgbuf);
		return NULL;
	}
	memset(imgbuf->imgbuf,0,height*width*sizeof(uint16_t));


        /* malloc a egi_data_pic struct */
        EGI_PDEBUG(DBG_PIC,"egi_picdata_new(): malloc data_pic ...\n");
        EGI_DATA_PIC *data_pic=malloc(sizeof(EGI_DATA_PIC));
        if(data_pic==NULL)
        {
                printf("egi_picdata_new(): fail to malloc egi_data_pic.\n");
		free(imgbuf->imgbuf);
		free(imgbuf);
		return NULL;
	}
	/* clear data */
	memset(data_pic,0,sizeof(EGI_DATA_PIC));

	/* assign struct number */
	data_pic->offx=offx;
	data_pic->offy=offy;
	data_pic->imgpx=imgpx;
	data_pic->imgpy=imgpy;
	data_pic->font=font;
	data_pic->imgbuf=imgbuf;
	data_pic->title=NULL;

	return data_pic;
}


/*-----------------------------------------------------------------------------
Dynamically create a new pic type ebox

egi_data: pic data
movable:  whether the host ebox is movable,relating to bkimg mem alloc.
x0,y0:    host ebox origin position.
frame:	  frame type of the host ebox.
prmcolor:  prime color for the host ebox.

return:
        poiter          OK
        NULL            fail
-----------------------------------------------------------------------------*/
EGI_EBOX * egi_picbox_new( char *tag, /* or NULL to ignore */
        EGI_DATA_PIC *egi_data,
        bool movable,
        int x0, int y0,
        int frame,
        int prmcolor /* applys only if prmcolor>=0 and egi_data->icon != NULL */
)
{
        EGI_EBOX *ebox;

        /* 0. check egi_data */
        if(egi_data==NULL)
        {
                printf("egi_picbox_new(): egi_data is NULL. \n");
                return NULL;
        }

        /* 1. create a new common ebox */
        EGI_PDEBUG(DBG_PIC,"egi_picbox_new(): start to egi_ebox_new(type_pic)...\n");
        ebox=egi_ebox_new(type_pic);// egi_data NOT allocated in egi_ebox_new()!!!
        if(ebox==NULL)
	{
                printf("egi_picbox_new(): fail to execute egi_ebox_new(type_pic). \n");
                return NULL;
	}


        /* 2. default method to be assigned in egi_ebox_new(),...see egi_ebox_new() */

        /* 3. pic ebox object method */
        EGI_PDEBUG(DBG_PIC,"egi_picbox_new(): assign self_defined methods: ebox->method=methd...\n");
        ebox->method=picbox_method;

        /* 4. fill in elements for concept ebox */
	egi_ebox_settag(ebox,tag);
        ebox->egi_data=egi_data; /* ----- assign egi data here !!!!! */
        ebox->movable=movable;
        ebox->x0=x0;
	ebox->y0=y0;

	/* 5. host ebox size is according to pic offx,offy and symheight */
	int symheight=0;
	if(egi_data->font != NULL)
	{
		symheight=egi_data->font->symheight;
	}

	ebox->width=egi_data->imgbuf->width+2*(egi_data->offx);

	if(egi_data->font != NULL && egi_data->title != NULL)
		ebox->height=egi_data->imgbuf->height+2*(egi_data->offy)+symheight;/* one line of title string */
	else
		ebox->height=egi_data->imgbuf->height+2*(egi_data->offy);


	/* 6. set frame and color */
        ebox->frame=frame;
	ebox->prmcolor=prmcolor;

        /* 7. set pointer default value*/
        //ebox->bkimg=NULL; Not neccessary, already set by memset() to 0 in egi_ebox_new().

        return ebox;
}


/*-----------------------------------------------------------------------
activate a picture type ebox:
	1. malloc bkimg and store bkimg.
	2. set status, ebox as active.
	3. refresh the picbox

TODO:
	1. if ebox size changes(enlarged), how to deal with bkimg!!??

Return:
	0	OK
	<0	fails!
------------------------------------------------------------------------*/
int egi_picbox_activate(EGI_EBOX *ebox)
{
	/* check data */
        if( ebox == NULL)
        {
                printf("egi_picbox_activate(): ebox is NULL!\n");
                return -1;
        }
	EGI_DATA_PIC *data_pic=(EGI_DATA_PIC *)(ebox->egi_data);
        if( data_pic == NULL)
        {
                printf("egi_picbox_activate(): data_pic is NULL!\n");
                return -1;
        }


	/* 1. confirm ebox type */
        if(ebox->type != type_pic)
        {
                printf("egi_picbox_activate(): Not button type ebox!\n");
                return -1;
        }

	/* host ebox size alread assigned according to pic offx,offy and symheight in egi_picbox_new() ...*/


	/* 2. verify pic data if necessary. --No need here*/
        if( ebox->height==0 || ebox->width==0)
        {
                printf("egi_picbox_activate(): height or width is 0 in ebox '%s'! fail to activate.\n",ebox->tag);
                return -1;
        }


   if(ebox->movable) /* only if ebox is movale */
   {
	/* 3. malloc bkimg for the host ebox */
	if(egi_alloc_bkimg(ebox, ebox->height, ebox->width)==NULL)
	{
		printf("egi_picbox_activate(): fail to egi_alloc_bkimg()!\n");
		return -2;
	}

	/* 4. update bkimg box */
	ebox->bkbox.startxy.x=ebox->x0;
	ebox->bkbox.startxy.y=ebox->y0;
	ebox->bkbox.endxy.x=ebox->x0+ebox->width-1;
	ebox->bkbox.endxy.y=ebox->y0+ebox->height-1;

	#if 0 /* DEBUG */
	EGI_PDEBUG(DBG_PIC," pic ebox activating... fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x, ebox->bkbox.endxy.y);
	#endif

	/* 5. store bk image which will be restored when this ebox's position/size changes */
	if( fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
				ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) <0)
		return -3;

    } /* end of movable codes */

	/* 6. set status */
	ebox->status=status_active; /* if not, you can not refresh */

	/* 7. set need_refresh */
	ebox->need_refresh=true; /* if not, ignore refresh */

	/* 8. refresh pic ebox */
	if( egi_picbox_refresh(ebox) != 0)
		return -4;


	EGI_PDEBUG(DBG_PIC,"egi_picbox_activate(): a '%s' ebox is activated.\n",ebox->tag);
	return 0;
}


/*-----------------------------------------------------------------------
refresh a pic type ebox:
	1.refresh a pic ebox according to updated parameters:
		--- position x0,y0, offx,offy
		--- symbol page, symbol code
		--- fpath for a pic/movie ...etc.
		... ...
	2. restore bkimg and store bkimg.
	3. drawing a picture, or start a motion pic, or play a movie ...etc.
	4. take actions according to pic_status (released, pressed).???

TODO:
	1. if ebox size changes(enlarged), how to deal with bkimg!!??

Return:
	1	need_refresh=false
	0	OK
	<0	fails!
------------------------------------------------------------------------*/
int egi_picbox_refresh(EGI_EBOX *ebox)
{
//	int i;
//	int symheight;
//	int symwidth;


	/* check data */
        if( ebox == NULL)
        {
                printf("egi_picbox_refresh(): ebox is NULL!\n");
                return -1;
        }

	/* confirm ebox type */
        if(ebox->type != type_pic)
        {
                printf("egi_picbox_refresh(): Not pic type ebox!\n");
                return -2;
        }

	/*  check the ebox status  */
	if( ebox->status != status_active )
	{
		EGI_PDEBUG(DBG_PIC,"ebox '%s' is not active! fail to refresh. \n",ebox->tag);
		return -3;
	}

	/* only if need_refresh is true */
	if(!ebox->need_refresh)
	{
		EGI_PDEBUG(DBG_PIC,"egi_picbox_refresh(): need_refresh=false, refresh action is ignored.\n");
		return 1;
	}

   if(ebox->movable) /* only if ebox is movale */
   {
	/* 2. restore bk image use old bkbox data, before refresh */
	#if 0 /* DEBUG */
	EGI_PDEBUG(DBG_PIC,"pic refresh... fb_cpyfrom_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
	#endif
        if( fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0)
		return -3;
   } /* end of movable codes */


	/* 3. get updated data */
	EGI_DATA_PIC *data_pic=(EGI_DATA_PIC *)(ebox->egi_data);
        if( data_pic == NULL)
        {
                printf("egi_picbox_refresh(): data_pic is NULL!\n");
                return -4;
        }
	/* check ebox size */
        if( ebox->height==0 || ebox->width==0)
        {
                printf("egi_btnbox_refresh(): height or width is 0 in ebox '%s'!\n",ebox->tag);
                return -1;
        }

	/* get parameters */
	int symheight=0;
	if(data_pic->font != NULL)
	    symheight=data_pic->font->symheight;

	int wx0=ebox->x0+data_pic->offx; /* displaying_window origin */

	int wy0=ebox->y0+data_pic->offy;
	if(data_pic->title != NULL)
	    wy0=ebox->y0+data_pic->offy+symheight;

	int imgh=data_pic->imgbuf->height;
	int imgw=data_pic->imgbuf->width;


   if(ebox->movable) /* only if ebox is movale */
   {
       /* ---- 4. redefine bkimg box range, in case it changes... */
	/* check ebox height and font lines in case it changes, then adjust the height */
	/* updata bkimg->bkbox according */
        ebox->bkbox.startxy.x=ebox->x0;
        ebox->bkbox.startxy.y=ebox->y0;
        ebox->bkbox.endxy.x=ebox->x0+ebox->width-1;
        ebox->bkbox.endxy.y=ebox->y0+ebox->height-1;

	#if 0 /* DEBUG */
	EGI_PDEBUG(DBG_PIC,"egi_picbox_refresh(): fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
	#endif
        /* ---- 5. store bk image which will be restored when you refresh it later,
		this ebox position/size changes */
        if(fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                                ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0)
	{
		printf("egi_picbox_refresh(): fb_cpyto_buf() fails.\n");
		return -4;
	}
   } /* end of movable codes */


        /* 6. set prime color and drawing shape  */
        if(ebox->prmcolor >= 0 )
        {
                /* set color */
           	fbset_color(ebox->prmcolor);
		/* draw ebox  */
	        draw_filled_rect(&gv_fb_dev, ebox->x0, ebox->y0,
						ebox->x0+ebox->width-1, ebox->y0+ebox->height-1);

        	/* draw frame */
       		if(ebox->frame >= 0) /* 0: simple type */
       		{
                	fbset_color(0); /* use black as frame color  */
                	draw_rect(&gv_fb_dev, ebox->x0, ebox->y0,
						ebox->x0+ebox->width-1, ebox->y0+ebox->height-1);
		}
        }


	/* 7. put title  */
	if(data_pic->title != NULL && data_pic->font != NULL)
	{
		symbol_string_writeFB(&gv_fb_dev, data_pic->font, SYM_NOSUB_COLOR, SYM_FONT_DEFAULT_TRANSPCOLOR,
					 ebox->x0+data_pic->offx, ebox->y0 + data_pic->offy/2,
//			 data_pic->offy > symheight ? (ebox->y0+(data_pic->offy-symheight)/2) : ebox->y0, /*title position */
				     	 data_pic->title);
	}


  	/* 8. draw picture in the displaying ebox window */
	/*-------------------------------------------------------------------------------------
	egi_imgbuf:     an EGI_IMGBUF struct which hold bits_color image data of a picture.
	(xp,yp):        coodinate of the displaying window origin(left top) point, relative to
        	        the coordinate system of the picture(also origin at left top).
	(xw,yw):        displaying window origin, relate to the LCD coord system.
	winw,winh:              width and height of the displaying window.

	int egi_imgbuf_windisplay(const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int xp, int yp,
                                int xw, int yw, int winw, int winh)
	---------------------------------------------------------------------------------------*/
	/*  display imgbuf if not NULL */
	if( data_pic->imgbuf != NULL && data_pic->imgbuf->imgbuf != NULL )
	{
		egi_imgbuf_windisplay(data_pic->imgbuf, &gv_fb_dev,
					data_pic->imgpx, data_pic->imgpy,
					wx0, wy0, imgw, imgh );
	}
	/* else if fpath != 0, load jpg file */
	else if(data_pic->fpath != NULL)
	{
		/* scale image to fit to the displaying window */


		/* keep original picture size */


	}
	else /* draw a black window if imgbuf is NULL */
	{
               	fbset_color(0); /* use black as frame color  */
               	draw_filled_rect(&gv_fb_dev,wx0,wy0,wx0+imgw-1,wy0+imgh-1);
	}


	/* 9. decorate functoins */
	if(ebox->decorate)
		ebox->decorate(ebox);

	/* 10. finally, reset need_refresh */
	ebox->need_refresh=false;

	return 0;
}


/*-----------------------------------------------------
put a txt ebox to sleep
1. restore bkimg
2. reset status

return
        0       OK
        <0      fail
------------------------------------------------------*/
int egi_picbox_sleep(EGI_EBOX *ebox)
{
	if(ebox==NULL)
	{
		printf("egi_picbox_sleep(): ebox is NULL, fail to make it sleep.\n");
		return -1;
	}

        if(ebox->movable) /* only for movable ebox */
        {
                /* restore bkimg */
                if(fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) <0 )
                {
                        printf("egi_picbox_sleep(): fail to restor bkimg for a '%s' ebox.\n",ebox->tag);
                        return -2;
                }
        }

        /* reset status */
        ebox->status=status_sleep;

        EGI_PDEBUG(DBG_PIC,"egi_picbox_sleep(): a '%s' ebox is put to sleep.\n",ebox->tag);
        return 0;
}


/*-------------------------------------------------
release struct egi_data_pic
--------------------------------------------------*/
void egi_free_data_pic(EGI_DATA_PIC *data_pic)
{
	if(data_pic != NULL)
	{
		if(data_pic->imgbuf != NULL)
		{
			if(data_pic->imgbuf->imgbuf != NULL)
			{
				free(data_pic->imgbuf->imgbuf);
			}
			free(data_pic->imgbuf);
		}
		free(data_pic);
	}

	data_pic=NULL;
}

