/*----------------------- egi_btn.c ------------------------------
egi btn tyep ebox functions

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "egi_color.h"
#include "egi.h"
#include "egi_btn.h"
#include "egi_debug.h"
#include "egi_symbol.h"
//#include "bmpjpg.h"


/*-------------------------------------
      txt ebox self_defined methods
--------------------------------------*/
static EGI_METHOD btnbox_method=
{
        .activate=egi_btnbox_activate,
        .refresh=egi_btnbox_refresh,
        .decorate=NULL,
        .sleep=NULL,
        .free=NULL, //egi_ebox_free,
};



/*-----------------------------------------------------------------------------
Dynamically create btn_data struct

id: 		id number of the button
shape: 		shape of the button
icon: 		symbol page for the icon
icon_code:	code of the symbol
font:		symbol page for the ebox->tag font

return:
        poiter          OK
        NULL            fail
-----------------------------------------------------------------------------*/
EGI_DATA_BTN *egi_btndata_new(int id, enum egi_btn_type shape,
			      struct symbol_page *icon, int icon_code,
			      struct symbol_page *font )
{
        /* malloc a egi_data_btn struct */
        egi_pdebug(DBG_BTN,"egi_btndata_new(): malloc data_btn ...\n");
        EGI_DATA_BTN *data_btn=malloc(sizeof(EGI_DATA_BTN));
        if(data_btn==NULL)
        {
                printf("egi_btndata_new(): fail to malloc egi_data_btn.\n");
		return NULL;
	}
	/* clear data */
	memset(data_btn,0,sizeof(EGI_DATA_BTN));

	/* assign struct number */
	data_btn->id=id;
	data_btn->shape=shape;
	data_btn->icon=icon;
	data_btn->icon_code=icon_code;
	data_btn->font=font;

	return data_btn;
}


/*-----------------------------------------------------------------------------
Dynamically create a new btnbox

return:
        poiter          OK
        NULL            fail
-----------------------------------------------------------------------------*/
EGI_EBOX * egi_btnbox_new( char *tag, /* or NULL to ignore */
        EGI_DATA_BTN *egi_data,
        bool movable,
        int x0, int y0,
        int width, int height, /* for frame drawing and prmcolor filled_rectangle */
        int frame,
        int prmcolor /* applys only if prmcolor>=0 and egi_data->icon != NULL */
)
{
        EGI_EBOX *ebox;

        /* 0. check egi_data */
        if(egi_data==NULL)
        {
                printf("egi_btnbox_new(): egi_data is NULL. \n");
                return NULL;
        }

        /* 1. create a new common ebox */
        egi_pdebug(DBG_BTN,"egi_btnbox_new(): start to egi_ebox_new(type_btn)...\n");
        ebox=egi_ebox_new(type_btn);// egi_data NOT allocated in egi_ebox_new()!!!
        if(ebox==NULL)
	{
                printf("egi_btnbox_new(): fail to execute egi_ebox_new(type_btn). \n");
                return NULL;
	}

        /* 2. default method assigned in egi_ebox_new() */

        /* 3. btn ebox object method */
        egi_pdebug(DBG_BTN,"egi_btnbox_new(): assign defined mehtod ebox->method=methd...\n");
        ebox->method=btnbox_method;

        /* 4. fill in elements for concept ebox */
	//strncpy(ebox->tag,tag,EGI_TAG_LENGTH); /* addtion EGI_TAG_LENGTH+1 for end token here */
	egi_ebox_settag(ebox,tag);
        ebox->egi_data=egi_data; /* ----- assign egi data here !!!!! */
        ebox->movable=movable;
        ebox->x0=x0;    ebox->y0=y0;
        ebox->width=width;      ebox->height=height;
        ebox->frame=frame;      ebox->prmcolor=prmcolor;

        /* 5. pointer default */
        ebox->bkimg=NULL;

        return ebox;
}


/*-----------------------------------------------------------------------
activate a button type ebox:
	1. get icon symbol information
	2. malloc bkimg and store bkimg.
	3. refresh the btnbox
	4. set status, ebox as active, and botton assume to be released.

TODO:
	1. if ebox size changes(enlarged), how to deal with bkimg!!??

Return:
	0	OK
	<0	fails!
------------------------------------------------------------------------*/
int egi_btnbox_activate(EGI_EBOX *ebox)
{
	/* check data */
        if( ebox == NULL)
        {
                printf("egi_btnbox_activate(): ebox is NULL!\n");
                return -1;
        }
	EGI_DATA_BTN *data_btn=(EGI_DATA_BTN *)(ebox->egi_data);
        if( data_btn == NULL)
        {
                printf("egi_btnbox_activate(): data_btn is NULL!\n");
                return -1;
        }


	/* 1. confirm ebox type */
        if(ebox->type != type_btn)
        {
                printf("egi_btnbox_activate(): Not button type ebox!\n");
                return -1;
        }


	/* only if it has an icon, get symheight and symwidth */
	if(data_btn->icon != NULL)
	{
		//int bkcolor=data_btn->icon->bkcolor;
		int symheight=data_btn->icon->symheight;
		int symwidth=data_btn->icon->symwidth[data_btn->icon_code];
		/* use bigger size for ebox height and width !!! */
		if(symheight > ebox->height || symwidth > ebox->width )
		{
			ebox->height=symheight;
			ebox->width=symwidth;
	 	}
	}

	/* else if no icon, use prime shape and size */


	/* origin(left top), for btn H&W x0,y0 is same as ebox */
	int x0=ebox->x0;
	int y0=ebox->y0;

	/* 2. verify btn data if necessary. --No need here*/
        if( ebox->height==0 || ebox->width==0)
        {
                printf("egi_btnbox_activate(): height or width is 0 in ebox '%s'!\n",ebox->tag);
                return -1;
        }


   if(ebox->movable) /* only if ebox is movale */
   {
	/* 3. malloc bkimg for the icon, not ebox, so use symwidth and symheight */
	if(egi_alloc_bkimg(ebox, ebox->height, ebox->width)==NULL)
	{
		printf("egi_btnbox_activate(): fail to egi_alloc_bkimg()!\n");
		return -2;
	}

	/* 4. update bkimg box */
	ebox->bkbox.startxy.x=x0;
	ebox->bkbox.startxy.y=y0;
	ebox->bkbox.endxy.x=x0+ebox->width-1;
	ebox->bkbox.endxy.y=y0+ebox->height-1;

	#if 0 /* DEBUG */
	egi_pdebug(DBG_BTN," button activating... fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x, ebox->bkbox.endxy.y);
	#endif
	/* 5. store bk image which will be restored when this ebox position/size changes */
	if( fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
				ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) <0)
		return -3;

    } /* end of movable codes */


	/* 6. set button status */
	ebox->status=status_active; /* if not, you can not refresh */
	data_btn->status=released_hold;

	/* 7. set need_refresh */
	ebox->need_refresh=true;

	/* 8. refresh btn ebox */
	if( egi_btnbox_refresh(ebox) != 0)
		return -4;


	egi_pdebug(DBG_BTN,"egi_btnbox_activate(): a '%s' ebox is activated.\n",ebox->tag);
	return 0;
}


/*-----------------------------------------------------------------------
refresh a button type ebox:
	1.refresh button ebox according to updated parameters:
		--- position x0,y0, offx,offy
		--- symbol page, symbol code ...etc.
		... ...
	2. restore bkimg and store bkimg.
	3. drawing the icon
	4. take actions according to btn_status (released, pressed).

TODO:
	1. if ebox size changes(enlarged), how to deal with bkimg!!??

Return:
	1	need_refresh=false
	0	OK
	<0	fails!
------------------------------------------------------------------------*/
int egi_btnbox_refresh(EGI_EBOX *ebox)
{
	int i;
	int bkcolor=0;
	int symheight;
	int symwidth;


	/* check data */
        if( ebox == NULL)
        {
                printf("egi_btnbox_refresh(): ebox is NULL!\n");
                return -1;
        }

	/* confirm ebox type */
        if(ebox->type != type_btn)
        {
                printf("egi_btnbox_refresh(): Not button type ebox!\n");
                return -1;
        }

	/*  check the ebox status  */
	if( ebox->status != status_active )
	{
		egi_pdebug(DBG_BTN,"ebox '%s' is not active! refresh action is ignored! \n",ebox->tag);
		return -2;
	}

	/* only if need_refresh is true */
	if(!ebox->need_refresh)
	{
		egi_pdebug(DBG_BTN,"egi_btnbox_refresh(): need_refresh=false, abort refresh.\n");
		return 1;
	}


   if(ebox->movable) /* only if ebox is movale */
   {
	/* 2. restore bk image use old bkbox data, before refresh */
	#if 0 /* DEBUG */
	egi_pdebug(DBG_BTN,"button refresh... fb_cpyfrom_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
	#endif
        if( fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0)
		return -3;
   } /* end of movable codes */


	/* 3. get updated data */
	EGI_DATA_BTN *data_btn=(EGI_DATA_BTN *)(ebox->egi_data);
        if( data_btn == NULL)
        {
                printf("egi_btnbox_refresh(): data_btn is NULL!\n");
                return -1;
        }


	/* only if it has an icon */
	if(data_btn->icon != NULL)
	{
		bkcolor=data_btn->icon->bkcolor;
		symheight=data_btn->icon->symheight;
		symwidth=data_btn->icon->symwidth[data_btn->icon_code];
		/* use bigger size for ebox height and width !!! */
		if(symheight > ebox->height || symwidth > ebox->width )
		{
			ebox->height=symheight;
			ebox->width=symwidth;
	 	}
	}
	/* else if no icon, use prime shape */

	/* check ebox size */
        if( ebox->height==0 || ebox->width==0)
        {
                printf("egi_btnbox_refresh(): height or width is 0 in ebox '%s'!\n",ebox->tag);
                return -1;
        }

	/* origin(left top) for btn H&W x0,y0 is same as ebox */
	int x0=ebox->x0;
	int y0=ebox->y0;

   if(ebox->movable) /* only if ebox is movale */
   {
       /* ---- 4. redefine bkimg box range, in case it changes */
	/* check ebox height and font lines in case it changes, then adjust the height */
	/* updata bkimg->bkbox according */
        ebox->bkbox.startxy.x=x0;
        ebox->bkbox.startxy.y=y0;
        ebox->bkbox.endxy.x=x0+ebox->width-1;
        ebox->bkbox.endxy.y=y0+ebox->height-1;

	#if 1 /* DEBUG */
	egi_pdebug(DBG_BTN,"egi_btnbox_refresh(): fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
	#endif
        /* ---- 5. store bk image which will be restored when you refresh it later,
		this ebox position/size changes */
        if(fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                                ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0)
	{
		printf("egi_btnbox_refresh(): fb_cpyto_buf() fails.\n");
		return -4;
	}
   } /* end of movable codes */


        /* ---- 6. set color and drawing shape ----- */
        if(ebox->prmcolor >= 0 && data_btn->icon == NULL )
        {
		int height=ebox->height;
		int width=ebox->width;

                /* set color */
           	fbset_color(ebox->prmcolor);
		switch(data_btn->shape)
		{
			case square:
			        draw_filled_rect(&gv_fb_dev,x0,y0,x0+width-1,y0+height-1);
        			/* --- draw frame --- */
       			        if(ebox->frame >= 0) /* 0: simple type */
       				{
                			fbset_color(0); /* use black as frame color  */
                			draw_rect(&gv_fb_dev,x0,y0,x0+width-1,y0+height-1);
			        }
				break;
			case circle:
				draw_filled_circle(&gv_fb_dev, x0+width/2, y0+height/2,
								width>height?height/2:width/2);
        			/* --- draw frame --- */
       			        if(ebox->frame >= 0) /* 0: simple type */
       				{
                			fbset_color(0); /* use black as frame color  */
					draw_circle(&gv_fb_dev, x0+width/2, y0+height/2,
								width>height?height/2:width/2);
			        }
				break;

			default:
				printf("egi_btnbox_refresh(): shape not defined! \n");
				break;
		}
        }

	/* 7. draw the button symbol if it has an icon */
	if(data_btn->icon != NULL)
		symbol_writeFB(&gv_fb_dev,data_btn->icon, SYM_NOSUB_COLOR, bkcolor,
								x0, y0, data_btn->icon_code,data_btn->opaque);


	/* 8. draw ebox->tag on the button if necessary */

   /* 8.1 check whether sympg is set */
   if(data_btn->font == NULL)
		printf("egi_btnbox_refresh(): data_btn->font is NULL, fail to put tag on button.\n");

   else if(data_btn->showtag==true)
   {
	/* 8.2 get length of tag */
	int taglen=0;
	while( ebox->tag[taglen] )
	{
		taglen++;
	}
	//printf("egi_btnbox_refresh(): length of ebox->tag:'%s' is %d (chars).\n", ebox->tag, taglen);

	/* only if tag has content */
	if(taglen>0)
	{
		int strwidth=0; /* total number of pixels of all char in ebox->tag */
		// int symheight=data_btn->icon->symheight; already above assigned.
		int shx=0, shy=0; /* shift x,y to fit the tag just in middle of ebox shape */
		/* WARNIGN: Do not mess up with sympg icon!  sympy font is right here! */
		int *wdata=data_btn->font->symwidth; /*symwidth data */
		int  fontheight=data_btn->font->symheight;

		for(i=0;i<taglen;i++)
		{
			/* only if in code range */
			if( (ebox->tag[i]) <= (data_btn->font->maxnum) )
				strwidth += wdata[ (unsigned int)ebox->tag[i] ];
		}
		shx = (ebox->width - strwidth)/2;
		shx = shx>0 ? shx:0;
		shy = (ebox->height - fontheight)/2;
		shy = shy>0 ? shy:0;
/*
use following COLOR:
#define SYM_NOSUB_COLOR -1  --- no substitute color defined for a symbol or font
#define SYM_NOTRANSP_COLOR -1 --- no transparent color defined for a symbol or font
void symbol_string_writeFB(FBDEV *fb_dev, const struct symbol_page *sym_page,   \
                int fontcolor, int transpcolor, int x0, int y0, const char* str)
*/
		symbol_string_writeFB(&gv_fb_dev, data_btn->font, SYM_NOSUB_COLOR, 1,
 							ebox->x0+shx, ebox->y0+shy, ebox->tag);

	} /* endif: taglen>0 */

   }/* endif: data_btn->font != NULL */


	/* 9. decorate functoins
	*/
	if(ebox->decorate)
		ebox->decorate(ebox);

	/* 10. finally, reset need_refresh */
	ebox->need_refresh=false;

	return 0;
}


/*-------------------------------------------------
release struct egi_btn_txt
--------------------------------------------------*/
void egi_free_data_btn(EGI_DATA_BTN *data_btn)
{
	if(data_btn != NULL)
		free(data_btn);

	data_btn=NULL;
}

