/*----------------------------------------------------------------------------
embedded graphic interface based on frame buffer, 16bit color tft-LCD.

Very simple concept:
0. Basic philosophy: Loop and Roll-back.
1. The basic elements of egi objects are egi_element boxes(ebox).
2. All types of EGI objects are inherited from basic eboxes. so later it will be
   easy to orgnize and manage them.
3. Only one egi_page is active on the screen.
4. An active egi_page occupys the whole screen.
5. An egi_page hosts several type of egi_ebox, such as type_txt,type_button,...etc.
6. First init egi_data_xxx for different type, then init the egi_element_box with it.

7. some ideas about egi_page ....
  7.1 Home egi_page (wallpaper, app buttons, head informations ...etc.)
  7.2 Current active egi_page (current running/displaying egi_page, accept all pad-touch events)
  7.3 Back Running egi_page (running in back groud, no pad-touch reaction. )


TODO:
	0. egi_txtbox_filltxt(),fill txt buffer of txt_data->txt.
	1. different symbol types in a txt_data......
	2. egi_init_data_txt(): llen according to ebox->height and width.---not necessary if multiline auto. 		   adjusting.
	3. To read FBDE vinfo to get all screen/fb parameters as in fblines.c, it's improper in other source files.

Midas Zhou
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h> /*malloc*/
#include <string.h> /* memset */
#include "egi.h"
#include "egi_txt.h"
#include "egi_debug.h"
#include "symbol.h"



/*-----------------------------------------------------------------------
allocate memory for ebox->bkimg with specified H and W, in 16bit color.
!!! height/width and ebox->height/width MAY NOT be the same.

ebox:	pointer to an ebox
height: height of an image.
width: width of an image.

Return:
	pointer to the mem.	OK
	NULL			fail
----------------------------------------------------------------------*/
void *egi_alloc_bkimg(struct egi_element_box *ebox, int width, int height)
{
	/* 1. check data */
	if( height<=0 || width <=0 )
	{
		printf("egi_alloc_bkimg(): ebox height or width is <=0!\n");
		return NULL;
	}

	/* 2. malloc exbo->bkimg for bk image storing */
	if( ebox->bkimg != NULL)
	{
		printf("egi_alloc_bkimg(): ebox->bkimg is not NULL, fail to malloc!\n");
		return NULL;
	}

	/* 3. alloc memory */

	PDEBUG("egi_alloc_bkimg(): start to malloc() ....!\n");
	ebox->bkimg=malloc(height*width*sizeof(uint16_t));

	if(ebox->bkimg == NULL)
	{
		printf("egi_alloc_bkimg(): fail to malloc for ebox->bkimg!\n");
		return NULL;
	}

	PDEBUG("egi_alloc_bkimg(): finish!\n");

	return ebox->bkimg;
}



/*---------- obselete!!!, substitued by egi_getindex_ebox() now!!! ----------------
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
1. find the ebox index according to given x,y
2. a sleeping ebox will be ignored.

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

	/* Now we only consider button ebox*/
	if(ebox->type==type_button)
	{
		for(i=0;i<num;i++)
		{
			if(ebox->status==status_sleep)continue; /* ignore sleeping ebox */

			if( x>=ebox[i].x0 && x<=ebox[i].x0+ebox[i].width \
				&& y>=ebox[i].y0 && y<=ebox[i].y0+ebox[i].height )
				return i;
		}
	}

	return -1;
}

/*------------------------------------------------
OBSOLETE!!!
all eboxes to be public !!!
return ebox status
-------------------------------------------------*/
enum egi_ebox_status egi_get_ebox_status(const struct egi_element_box *ebox)
{
	return ebox->status;
}


/*-----------------------------------------------------------------------
activate a button type ebox:
	1. get icon symbol information
	2. malloc bkimg and store bkimg.
	3. refresh the btnbox
	4. set status, ebox as active, and botton assume to be released.
TODO:

Return:
	0	OK
	<0	fails!
------------------------------------------------------------------------*/
int egi_btnbox_activate(struct egi_element_box *ebox)
{
	/* 1. confirm ebox type */
        if(ebox->type != type_button)
        {
                printf("egi_btnbox_activate(): Not button type ebox!\n");
                return -1;
        }

	struct egi_data_btn *data_btn=(struct egi_data_btn *)(ebox->egi_data);
	//int bkcolor=data_btn->icon->bkcolor;
	int symheight=data_btn->icon->symheight;
	int symwidth=data_btn->icon->symwidth[data_btn->icon_code];
	/* for button,ebox H&W is same as symbol H&W */
	ebox->height=symheight;
	ebox->width=symwidth;

	/* origin(left top), for btn H&W x0,y0 is same as ebox */
	int x0=ebox->x0;
	int y0=ebox->y0;


	/* 2. verify btn data if necessary. --No need here*/

	/* 3. malloc bkimg for the icon, not ebox, so use symwidth and symheight */
	if(egi_alloc_bkimg(ebox, symwidth, symheight)==NULL)
	{
		printf("egi_btnbox_activate(): fail to egi_alloc_bkimg()!\n");
		return -2;
	}

	/* 4. update bkimg box */
	ebox->bkbox.startxy.x=x0;
	ebox->bkbox.startxy.y=y0;
	ebox->bkbox.endxy.x=x0+symwidth-1;
	ebox->bkbox.endxy.y=y0+symheight-1;

#if 0 /* DEBUG */
	printf(" button activating... fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x, ebox->bkbox.endxy.y);
#endif
	/* 5. store bk image which will be restored when this ebox position/size changes */
	if( fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
				ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) <0)
		return -3;

	/* 6. set button status */
	ebox->status=status_active; /* if not, you can not refresh */
	data_btn->status=released_hold;

	/* 7. refresh btn ebox */
	if( egi_btnbox_refresh(ebox) != 0)
		return -4;

	printf("egi_btnbox_activate(): a '%s' ebox is activated.\n",ebox->tag);
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

Return:
	0	OK
	<0	fails!
------------------------------------------------------------------------*/
int egi_btnbox_refresh(struct egi_element_box *ebox)
{
	/* 0. confirm ebox type */
        if(ebox->type != type_button)
        {
                printf("egi_btnbox_activate(): Not button type ebox!\n");
                return -1;
        }
	/* 1. check the ebox status  */
	if( ebox->status != status_active )
	{
//		printf("ebox '%s' is not active! refresh action is ignored! \n",ebox->tag);
		return -2;
	}

	/* 2. restore bk image before refresh */
#if 0 /* DEBUG */
	printf("button refresh... fb_cpyfrom_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
#endif
        if( fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0)
		return -3;

	/* 3. get updated parameters */
	struct egi_data_btn *data_btn=(struct egi_data_btn *)(ebox->egi_data);
	int bkcolor=data_btn->icon->bkcolor;
	int symheight=data_btn->icon->symheight;
	int symwidth=data_btn->icon->symwidth[data_btn->icon_code];
	/* origin(left top) for btn H&W x0,y0 is same as ebox */
	int x0=ebox->x0;
	int y0=ebox->y0;

        /* ---- 4. redefine bkimg box range, in case it changes */
	/* check ebox height and font lines in case it changes, then adjust the height */
	/* updata bkimg->bkbox according */
	ebox->height=symheight;
        ebox->bkbox.startxy.x=x0;
        ebox->bkbox.startxy.y=y0;
        ebox->bkbox.endxy.x=x0+symwidth-1;
        ebox->bkbox.endxy.y=y0+symheight-1;

#if 0 /* DEBUG */
	printf("refresh() fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
#endif
        /* ---- 5. store bk image which will be restored when you refresh it later,
		this ebox position/size changes */
        if(fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                                ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0)
		return -4;

	/* 6. draw the button */
	symbol_writeFB(&gv_fb_dev,data_btn->icon, SYM_NOSUB_COLOR, bkcolor, x0, y0, data_btn->icon_code);

	/* 5. take action according to status:
		 void (* action)(enum egi_btn_status status);
	*/

	return 0;
}


/*-------------------------------------------------
release struct egi_btn_txt
--------------------------------------------------*/
void egi_free_data_btn(struct egi_data_btn *data_btn)
{



}



///xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  OK   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

/*----------------------------------------------------
ebox refresh: default method

reutrn:
	1	use default method
	0	OK
	<0	fail
------------------------------------------------------*/
int egi_ebox_refresh(struct egi_element_box *ebox)
{
	/* 1. put default methods here ...*/
	if(ebox->method.refresh == NULL)
	{
		printf("ebox '%s' has no defined method of refresh()!\n",ebox->tag);
		return 1;
	}

	/* 2. ebox object defined method */
	else
		return ebox->method.refresh(ebox);
}

/*----------------------------------------------------
ebox activate: default method

reutrn:
	1	use default method
	0	OK
	<0	fail
------------------------------------------------------*/
int egi_ebox_activate(struct egi_element_box *ebox)
{
	/* 1. put default methods here ...*/
	if(ebox->method.activate == NULL)
	{
		printf("ebox '%s' has no defined method of activate()!\n",ebox->tag);
		return 1;
	}

	/* 2. ebox object defined method */
	else
		return ebox->method.activate(ebox);
}

/*----------------------------------------------------
ebox refresh: default method

reutrn:
	1	use default method
	0	OK
	<0	fail
------------------------------------------------------*/
int egi_ebox_sleep(struct egi_element_box *ebox)
{
	/* 1. put default methods here ...*/
	if(ebox->method.sleep == NULL)
	{
		printf("ebox '%s' has no defined method of sleep()!\n",ebox->tag);
		return 1;
	}

	/* 2. ebox object defined method */
	else
		return ebox->method.sleep(ebox);
}


/*----------------------------------------------------
ebox refresh: default method

reutrn:
	1	use default method
	0	OK
	<0	fail
------------------------------------------------------*/
int egi_ebox_free(struct egi_element_box *ebox)
{
	/* check ebox */
	if(ebox == NULL)
	{
		printf("egi_ebox_free(): pointer ebox is NULL! fail to free it.\n");
		return -1;
	}

	/* 1. put default methods here ...*/
	if(ebox->method.free == NULL)
	{
		printf("ebox '%s' has no defined method of free(), now use default to free it ...!\n",ebox->tag);

		/* 1.1 free ebox tyep data */
		switch(ebox->type)
		{
			case type_txt:
				if(ebox->egi_data != NULL)
					egi_free_data_txt(ebox->egi_data);
				if(ebox->egi_data != NULL)
					free(ebox->egi_data);
				break;
			case type_button:
				if(ebox->egi_data != NULL)
					egi_free_data_btn(ebox->egi_data);
				break;
			case type_chart:
				break;
			default:
				printf("egi_ebox_free(): ebox '%s' type %d has not been created yet!\n",
										ebox->tag,ebox->type);
				return -2;
				break;
		}

		/* 1.2 free ebox bkimg */
		if(ebox->bkimg != NULL)
		{
			free(ebox->bkimg);
			ebox->bkimg=NULL;
		}
		/* 1.3 free ebox */
		free(ebox);
		ebox=NULL;

		return 1;
	}

	/* 2. else, use ebox object defined method */
	else
	{
		printf("use ebox '%s' defined free method to free it ...!\n",ebox->tag);
		return ebox->method.free(ebox);
	}
}


/*----------------------------------------------------------------------------
create a new ebox according to its type and parameters
WARNING: Memory for egi_data NOT allocated her, it's caller's job to allocate
	and assign it to the new ebox. they'll be freed all together in egi_ebox_free().

return:
	NULL		fail
	pointer		OK
----------------------------------------------------------------------------*/
struct egi_element_box * egi_ebox_new(enum egi_ebox_type type)  //, void *egi_data)
{
	/* malloc ebox */
	PDEBUG("egi_ebox_new(): start to malloc for a new ebox....\n");
	struct egi_element_box *ebox=malloc(sizeof(struct egi_element_box));

	if(ebox==NULL)
	{
		printf("egi_ebox_new(): fail to malloc for a new ebox!\n");
		return NULL;
	}
	memset(ebox,0,sizeof(struct egi_element_box)); /* clear data */


#if 0 	/* Not necessary, the egi_data to be allocated and assigned to ebox by the caller!!!!  malloc ebox type data */
	switch(type)
	{
		case type_txt:
			ebox->egi_data = malloc(sizeof(struct egi_data_txt));
			break;
		case type_button:
			ebox->egi_data = malloc(sizeof(struct egi_data_btn));
			break;
		case type_chart:
			break;
		default:
			printf("egi_ebox_new(): ebox type %d has not been created yet!\n",type);
			break;
	}
	if(ebox->egi_data==NULL)
	{
		printf("egi_ebox_new(): fail to malloc ebox->egi_data for a new ebox!\n");
		free(ebox);
		ebox=NULL;
		return NULL;
	}
	memset(ebox,0,sizeof(struct egi_data_txt)); /* clear data */
#endif


	/* assign default method for new ebox */
	PDEBUG("egi_ebox_new(): assing default method to ebox ....\n");
	ebox->activate=egi_ebox_activate;
	ebox->refresh=egi_ebox_refresh;
	ebox->sleep=egi_ebox_sleep;
	ebox->free=egi_ebox_free;

	/* others as default, after memset with 0:
	 inmovable
	 status=no_body
	 frame=simple type
	 prmcolor=BLACK
	 */

	return ebox;
}
