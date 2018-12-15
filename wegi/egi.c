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
	1. egi_init_data_txt(): llen according to ebox->height and width.
	2. apply struct egi_data_txt->color for txt color,in egi_txtbox_refresh()



Midas Zhou
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h> /*malloc*/
//#include <stdbool.h>
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
			int nl, int llen, struct symbol_page *font, uint16_t color)
{
	int i,j;

	data_txt->nl=nl;
	data_txt->llen=llen;
	data_txt->font=font;
	data_txt->color=color;

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
	}

	return data_txt;
}


/*-----------------------------------------------------------------------
refresh a txt ebox:
	1.restore back img,
 	2.update its txt,


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
	char **txt=data_txt->txt;
	int font_height=data_txt->font->symheight;

	/* print out txt */
	for(i=0;i<nl;i++)
		printf("ebox txt[%d]:%s\n",i,txt[i]);

	/* write to FB */
	for(i=0;i<nl;i++)
		/*  (fb_dev,font, transpcolor, x0,y0, char*)...
					for font symbol: tranpcolor is its img symbol bkcolor!!! */
		symbol_string_writeFB(&gv_fb_dev, data_txt->font, -1, x0, y0+font_height*i, txt[i]);

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


