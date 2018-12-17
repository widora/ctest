/*----------------------------------------------------------------------------
embedded graphic interface based on frame buffer, 16bit color tft-LCD.

Very simple concept:
1. The basic elements of egi objects are egi_element boxes(ebox).
2. All types of EGI objects are inherited from basic eboxes. so later it will be
   easy to orgnize and manage them.
3. Only one egi_page is active on the screen.
4. An active egi_page occupys the whole screen.
5. An egi_page hosts several type of egi_ebox, such as type_txt,type_button,...etc.
6. First init egi_data_xxx for different type, then init the egi_element_box with it.


TODO:
	0. egi_txtbox_filltxt(),fill txt buffer of txt_data->txt.
	1. egi_init_data_txt(): llen according to ebox->height and width.
	2. apply struct egi_data_txt->color for txt color,in egi_txtbox_refresh()
3. To read FBDE vinfo to get all screen/fb parameters as in fblines.c, it's improper in other source files.



Midas Zhou
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h> /*malloc*/
#include <string.h> /* memset */
#include "egi.h"
#include "symbol.h"


/*----------obselete!!!, substitued by egi_getindex_ebox() now!!! ----------------
  check if (px,py) in the ebox
  return true or false
  !!!--- ebox local coordinate original is NOT sensitive in this function ---!!!
--------------------------------------------------------------------------------*/
bool egi_point_inbox(int px,int py, struct egi_element_box *ebox)
{
        int xl,xh,yl,yh;
        int x1=ebox->x0;
	int y1=ebox->y0;
        int x2=x1+ebox->width;
	int y2=y1+ebox->height;

        if(x1>=x2){
                xh=x1;xl=x2;
        }
        else {
                xl=x1;xh=x2;
        }

        if(y1>=y2){
                yh=y1;yl=y2;
        }
        else {
                yh=y2;yl=y1;
        }

        if( (px>=xl && px<=xh) && (py>=yl && py<=yh))
                return true;
        else
                return false;
}


/*------------------------------------------------------------------
find the ebox index according to given x,y
x,y: point at request
ebox:  ebox pointer
num: total number of eboxes referred by *ebox.

return:
	>=0   Ok, as ebox pointer index
	<0    not in eboxes
-------------------------------------------------------------------*/
int egi_get_boxindex(int x,int y, struct egi_element_box *ebox, int num)
{
	int i=num;

	for(i=0;i<num;i++)
	{
		if( x>=ebox[i].x0 && x<=ebox[i].x0+ebox[i].width \
			&& y>=ebox[i].y0 && y<=ebox[i].y0+ebox[i].height )
			return i;
	}

	return -1;
}



/*--------------------------------------------------------------------------------
initialize struct egi_data_txt according

offx,offy:			offset from prime ebox
int nl:   			number of txt lines
int llen:  			in byte, data length for each line
struct symbol_page *font:	txt font
uint16_t color:     		txt color
char **txt:       		multiline txt data

Return:
        non-null        OK
        NULL            fails
---------------------------------------------------------------------------------*/
struct egi_data_txt *egi_init_data_txt(struct egi_data_txt *data_txt,
			int offx, int offy, int nl, int llen, struct symbol_page *font, uint16_t color)
{
	int i,j;


	/* check data first */
	if(data_txt == NULL)
	{
		printf("egi_init_data_txt(): data_txt is NULL!\n");
		return NULL;
	}
	if(data_txt->txt !=NULL) /* if data_txt defined statically, txt may NOT be NULL !!! */
	{
		printf("egi_init_data_txt(): ---WARNING!!!--- data_txt->txt is NOT NULL!\n");
		//return NULL;
	}
	if( nl<1 || llen<1 )
	{
		printf("egi_init_data_txt(): data_txt->nl or llen is 0!\n");
		return NULL;
	}
	if( font == NULL )
	{
		printf("egi_init_data_txt(): data_txt->font is NULL!\n");
		return NULL;
	}

	/* --- assign var ---- */
	data_txt->nl=nl;
	data_txt->llen=llen;
	data_txt->font=font;
	data_txt->color=color;
	data_txt->offx=offx;
	data_txt->offy=offy;

	/* --- malloc data --- */
	data_txt->txt=malloc(nl*sizeof(char *));
	if(data_txt->txt == NULL) /* malloc **txt */
	{
		printf("egi_init_ebox(): fail to malloc egi_data_txt->txt!\n");
		return NULL;
	}
	for(i=0;i<nl;i++) /* malloc *txt */
	{
		data_txt->txt[i]=malloc(llen*sizeof(char));
		if(data_txt->txt[i] == NULL) /* malloc **txt */
		{
			printf("egi_init_ebox(): fail to malloc egi_data_txt->txt[]!\n");
			for(j=0;j<i;j++)
				free(data_txt->txt[j]);
			free(data_txt->txt);
			return NULL;
		}
		memset(data_txt->txt[i],0,llen*sizeof(char));
	}
	return data_txt;
}


/*-----------------------------------------------------------------------
activate a txt ebox:
	0. adjust ebox height and width according to its font line set
 	1. store back image of  txtbox frame range.
	2. refresh the ebox.
	3. change status token to active,

TODO:
	if ebox_size is fixed or is re-sizable, bkimg mem size MUST adjusted.

 
------------------------------------------------------------------------*/
void egi_txtbox_activate(struct egi_element_box *ebox)
{
	int i,j;
	int x0=ebox->x0;
	int y0=ebox->y0;
	int height=ebox->height;
	int width=ebox->width;
	struct egi_data_txt *data_txt=(struct egi_data_txt *)(ebox->egi_data);
	int nl=data_txt->nl;
	int llen=data_txt->llen;
	int offx=data_txt->offx;
	int offy=data_txt->offy;
	char **txt=data_txt->txt;
	int font_height=data_txt->font->symheight;

	/* confirm ebox type */
        if(ebox->type != type_txt)
        {
                printf("egi_txtbox_activate(): Not txt type ebox!\n");
                return;
        }


	/* malloc exbo->bkimg for bk image storing */
	if(ebox->bkimg != NULL)
	{
		printf("egi_txtbox_activat(): ebox->bkimg is not NULL, fail to malloc!\n");
		return;
	}


        /* check ebox height and font lines, then adjust the height */
        height= (font_height*nl+offy)>height ? (font_height*nl+offy) : height;
        ebox->height=height;

	// malloc more mem in case ebox size is enlarged later????? //
	//ebox->bkimg= malloc(240*320*2);
	ebox->bkimg=malloc(height*width*sizeof(uint16_t));

	if(ebox->bkimg == NULL)
	{
		printf("egi_txtbox_activat(): fail to malloc for ebox->bkimg!\n");
		return;
	}

	/* define bkimg box */
	ebox->bkbox.startxy.x=x0;
	ebox->bkbox.startxy.y=y0;
	ebox->bkbox.endxy.x=x0+width-1;
	ebox->bkbox.endxy.y=y0+height-1;

	printf("activate() fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x, ebox->bkbox.endxy.y);

	/* store bk image which will be restored when this ebox position/size changes */
	fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
				ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg);

	/* refresh displaying the ebox */
	egi_txtbox_refresh(ebox);
}



/*-----------------------------------------------------------------------
refresh a txt ebox:
	0.refresh ebox image according to following parameter updates:
		---txt(offx,offy,nl,llen)
		---size(height,width)
		---positon(x0,y0)
		---ebox color
	1.refresh back color if ebox->prmcolor >0,
 	2.update txt.
------------------------------------------------------------------------*/
void egi_txtbox_refresh(struct egi_element_box *ebox)
{
	if(ebox->type != type_txt)
	{
		printf("egi_txtbox_refresh(): Not txt type ebox!\n");
		return;
	}

	int i,j;
	int x0=ebox->x0;
	int y0=ebox->y0;
	int height=ebox->height;
	int width=ebox->width;
	struct egi_data_txt *data_txt=(struct egi_data_txt *)(ebox->egi_data);
	int nl=data_txt->nl;
	int llen=data_txt->llen;
	int offx=data_txt->offx;
	int offy=data_txt->offy;
	char **txt=data_txt->txt;
	int font_height=data_txt->font->symheight;
	//struct egi_box_coords bkbox=ebox->bkbox;

	/* test--------------   print out box txt content */
#if 0
	for(i=0;i<nl;i++)
		printf("egi_txtbox_refresh(): txt[%d]:%s\n",i,txt[i]);
#endif

	/* ---- 1. restore bk image before refresh */
	printf("refresh() fb_cpyfrom_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);

        fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg);

        /* ---- 2. redefine bkimg box range, in case it may change */
	/* check ebox height and font lines in case it may changes, then adjust the height */
	/* updata bkimg->bkbox according */
	height= (font_height*nl+offy)>height ? (font_height*nl+offy) : height;
	ebox->height=height;
        ebox->bkbox.startxy.x=x0;
        ebox->bkbox.startxy.y=y0;
        ebox->bkbox.endxy.x=x0+width-1;
        ebox->bkbox.endxy.y=y0+height-1;

	printf("refresh() fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);

        /* ---- 3. store bk image which will be restored when this ebox position/size changes */
        fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                                ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg);

	/* ---- 2. refresh prime color under the symbol  before updating txt.  */
	if(ebox->prmcolor >= 0)
	{
		/* set color and draw filled rectangle */
		fbset_color(ebox->prmcolor);
		draw_filled_rect(&gv_fb_dev,x0,y0,x0+width-1,y0+height-1);
		/* draw black frame rect */
		fbset_color(0); /* use black as frame color  */
		draw_rect(&gv_fb_dev,x0,y0,x0+width-1,y0+height-1);
	}

	/* ---- 3. refresh TXT, write txt line to FB */
	for(i=0;i<nl;i++)
		/*  (fb_dev,font, transpcolor, x0,y0, char*)...
					for font symbol: tranpcolor is its img symbol bkcolor!!! */
		symbol_string_writeFB(&gv_fb_dev, data_txt->font, -1, x0+offx, \
						 y0+offy+font_height*i, txt[i]);
}



/*----------------------------------------------------
refresh exbox according to its type

------------------------------------------------------*/
void egi_refresh(struct egi_element_box *ebox)
{

       switch(ebox->type)
        {
                case type_txt:
                {
                        break;
                }
                case type_picture:
                        break;
                case type_button:
                        break;
                case type_chart:
                        break;
                case type_motion:
                        break;

                default:
                        break;
        }
}




/*-------------------------------------------------
release struct egi_data_txt
--------------------------------------------------*/
void egi_free_data_txt(struct egi_data_txt *data_txt)
{
	int i;
	int nl=data_txt->nl;

	if(data_txt->txt != NULL)
	{
		for(i=0;i<nl;i++)
		{
			if(data_txt->txt[i] != NULL)
				free(data_txt->txt[i]);
		}

		free(data_txt);
	}
}


/*------------------------------------------------------
initialize an egi_element box according to its type

Return:
        non-null        OK
        NULL            fails
-------------------------------------------------------*/
struct egi_element_box *egi_init_ebox(struct egi_element_box *ebox) // enum egi_ebox type)
{
	int i,j;
	struct egi_element_box *ret=NULL;

        switch(ebox->type)
        {
		/*--------------- type_txt ebox init ------------------*/
                case type_txt:
                {
                        break;
                }
                case type_picture:
                        break;
                case type_button:
                        break;
                case type_chart:
                        break;
                case type_motion:
                        break;

                default:
                        break;
        }

	return ret;
}


/*------------------------------------------------------
free an egi_element box according to its type
-------------------------------------------------------*/
void egi_free_ebox(struct egi_element_box *ebox)
{
        switch(ebox->type)
        {
                case type_txt:
                {


                        break;
                }
                case type_picture:
                        break;
                case type_button:
                        break;
                case type_chart:
                        break;
                case type_motion:
                        break;

                default:
                        break;
        }



}


