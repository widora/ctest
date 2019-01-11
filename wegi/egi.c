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
  7.4 A egi page

TODO:
	0. egi_txtbox_filltxt(),fill txt buffer of txt_data->txt.
	1. different symbol types in a txt_data......
	2. egi_init_data_txt(): llen according to ebox->height and width.---not necessary if multiline auto. 		   adjusting.
	3. To read FBDE vinfo to get all screen/fb parameters as in fblines.c, it's improper in other source files.

Midas Zhou
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h> /*malloc*/
#include <string.h> /*memset*/
#include <unistd.h> /*usleep*/
#include "egi.h"
#include "egi_timer.h"
#include "egi_txt.h"
#include "egi_debug.h"
#include "list.h"
#include "egi_symbol.h"



/*---------------------------------------
return a random value not great than max
---------------------------------------*/
int egi_random_max(int max)
{
        int ret;
        struct timeval tmval;

        gettimeofday(&tmval,NULL);

        srand(tmval.tv_usec);
        ret = 1+(int)((float)max*rand()/(RAND_MAX+1.0));
        printf("random max ret=%d\n",ret);

        return ret;
}




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
void *egi_alloc_bkimg(EGI_EBOX *ebox, int width, int height)
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

	egi_pdebug(DBG_EGI,"egi_alloc_bkimg(): start to malloc() ....!\n");
	ebox->bkimg=malloc(height*width*sizeof(uint16_t));

	if(ebox->bkimg == NULL)
	{
		printf("egi_alloc_bkimg(): fail to malloc for ebox->bkimg!\n");
		return NULL;
	}

	egi_pdebug(DBG_EGI,"egi_alloc_bkimg(): finish!\n");

	return ebox->bkimg;
}



/*---------- obselete!!!, substitued by egi_getindex_ebox() now!!! ----------------
  check if (px,py) in the ebox
  return true or false
  !!!--- ebox local coordinate original is NOT sensitive in this function ---!!!
--------------------------------------------------------------------------------*/
bool egi_point_inbox(int px,int py, EGI_EBOX *ebox)
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
int egi_get_boxindex(int x,int y, EGI_EBOX *ebox, int num)
{
	int i=num;

	/* Now we only consider button ebox*/
	if(ebox->type==type_btn)
	{
		for(i=0;i<num;i++)
		{
			if(ebox[i].status==status_sleep)continue; /* ignore sleeping ebox */

			if( x>=ebox[i].x0 && x<=ebox[i].x0+ebox[i].width \
				&& y>=ebox[i].y0 && y<=ebox[i].y0+ebox[i].height )
				return i;
		}
	}

	return -1;
}



/*------------------------------------------------------------------
1. in a page, find the ebox index according to given x,y
2. a sleeping ebox will be ignored.

x,y: point at request
page:  a egi page containing eboxes

return:
	pointer to a ebox  	Ok
	NULL			fail
-------------------------------------------------------------------*/
EGI_EBOX *egi_hit_pagebox(int x, int y, EGI_PAGE *page, enum egi_ebox_type type)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;

        /* check page */
        if(page==NULL)
        {
                printf("egi_get_pagebtn(): page is NULL!\n");
                return NULL;
        }

        /* check list */
        if(list_empty(&page->list_head))
        {
                printf("egi_get_pagebtn(): page '%s' has no child ebox.\n",page->ebox->tag);
                return NULL;
        }

        /* traverse the list, not safe */
        list_for_each(tnode, &page->list_head)
        {
                ebox=list_entry(tnode, EGI_EBOX, node);
                //egi_pdebug(DBG_EGI,"egi_get_pagebtn(): find child --- ebox: '%s' --- \n",ebox->tag);

		if(ebox->type == type)
		{
	        	 if(ebox->status==status_sleep)
				continue; /* ignore sleeping ebox */

			 /* check whether the ebox is hit */
        	         if( x>=ebox->x0 && x<=ebox->x0+ebox->width \
                	                && y>=ebox->y0 && y<=ebox->y0+ebox->height )
                  	return ebox;
		}
	}

	return NULL;
}




/*------------------------------------------------
OBSOLETE!!!
all eboxes to be public !!!
return ebox status
-------------------------------------------------*/
enum egi_ebox_status egi_get_ebox_status(const EGI_EBOX *ebox)
{
	return ebox->status;
}







///xxxxxxxxxxxxxxxxxxxxxxxxxx(((   OK   )))xxxxxxxxxxxxxxxxxxxxxxxxx

/*-------------------------------------------
put tag for an ebox
--------------------------------------------*/
void egi_ebox_settag(EGI_EBOX *ebox, char *tag)
{
	/* 1. clear tag */
	memset(ebox->tag,0,EGI_TAG_LENGTH+1);

	/* 2. check data */
	if(ebox == NULL)
	{
		printf("egi_ebox_settag(): EGI_EBOX *ebox is NULL, fail to set tag.\n");
		return;
	}

	/* 3. set NULL */
	if(tag == NULL)
	{
		// OK, set NULL, printf("egi_ebox_settag(): char *tag is NULL, fail to set tag.\n");
		return;
	}

	/* 4. copy string to tag */
	strncpy(ebox->tag,tag,EGI_TAG_LENGTH);
}



/*----------------------------------------------------
ebox refresh: default method

reutrn:
	1	use default method
	0	OK
	<0	fail
------------------------------------------------------*/
int egi_ebox_refresh(EGI_EBOX *ebox)
{
	int ret;

	/* 1. put default methods here ...*/
	if(ebox->method.refresh == NULL)
	{
		egi_pdebug(DBG_EGI,"ebox '%s' has no defined method of refresh()!\n",ebox->tag);
		return 1;
	}

	/* 2. ebox object defined method */
	else
	{
			return ebox->method.refresh(ebox);
	}
}

/*----------------------------------------------------
ebox activate: default method

reutrn:
	1	use default method
	0	OK
	<0	fail
------------------------------------------------------*/
int egi_ebox_activate(EGI_EBOX *ebox)
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
int egi_ebox_sleep(EGI_EBOX *ebox)
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
ebox decorate: default method

reutrn:
	1	use default method
	0	OK
	<0	fail
------------------------------------------------------*/
int egi_ebox_decorate(EGI_EBOX *ebox)
{
	/* 1. put default methods here ...*/
	if(ebox->method.decorate == NULL)
	{
		printf("ebox '%s' has no defined method of decorate()!\n",ebox->tag);
		return 1;
	}

	/* 2. ebox object defined method */
	else
	{
		egi_pdebug(DBG_EGI,"egi_ebox_decorate(): start ebox->method.decorate(ebox)...\n");
		return ebox->method.decorate(ebox);
	}
}


/*----------------------------------------------------
ebox refresh: default method

reutrn:
	1	use default method
	0	OK
	<0	fail
------------------------------------------------------*/
int egi_ebox_free(EGI_EBOX *ebox)
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
		egi_pdebug(DBG_EGI,"ebox '%s' has no defined method of free(), now use default to free it ...!\n",ebox->tag);

		/* 1.1 free ebox tyep data */
		switch(ebox->type)
		{
			case type_txt:
				if(ebox->egi_data != NULL)
				{
					egi_pdebug(DBG_EGI,"egi_ebox_free():start to egi_free_data_txt(ebox->egi_data)  \
						 for '%s' ebox\n", ebox->tag);
					egi_free_data_txt(ebox->egi_data);
				 }
				break;
			case type_btn:
				if(ebox->egi_data != NULL)
					egi_pdebug(DBG_EGI,"egi_ebox_free():start to egi_free_data_btn(ebox->egi_data)  \
						 for '%s' ebox\n", ebox->tag);
					egi_free_data_btn(ebox->egi_data);
				break;
			case type_page:
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

		/* 1.3 free concept ebox */
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
EGI_EBOX * egi_ebox_new(enum egi_ebox_type type)  //, void *egi_data)
{
	/* malloc ebox */
	egi_pdebug(DBG_EGI,"egi_ebox_new(): start to malloc for a new ebox....\n");
	EGI_EBOX *ebox=malloc(sizeof(EGI_EBOX));

	if(ebox==NULL)
	{
		printf("egi_ebox_new(): fail to malloc for a new ebox!\n");
		return NULL;
	}
	memset(ebox,0,sizeof(EGI_EBOX)); /* clear data */

	ebox->type=type;


#if 0 	/* Not necessary, the egi_data to be allocated and assigned to ebox by the caller!!!!  malloc ebox type data */
	switch(type)
	{
		case type_txt:
			ebox->egi_data = malloc(sizeof(EGI_DATA_TXT));
			break;
		case type_button:
			ebox->egi_data = malloc(sizeof(EGI_DATA_BTN));
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
	memset(ebox,0,sizeof(EGI_DATA_TXT)); /* clear data */
#endif


	/* assign default method for new ebox */
	egi_pdebug(DBG_EGI,"egi_ebox_new(): assing default method to ebox ....\n");
	ebox->activate=egi_ebox_activate;
	ebox->refresh=egi_ebox_refresh;
	ebox->decorate=egi_ebox_decorate;
	ebox->sleep=egi_ebox_sleep;
	ebox->free=egi_ebox_free;

	/* others as default, after memset with 0:
	 inmovable
	 status=no_body
	 frame=simple type
	 prmcolor=BLACK
	 */

	egi_pdebug(DBG_EGI,"egi_ebox_new(): end the call. \n");

	return ebox;
}


/*------------------------------------------------------------------
dispearing effect for a full egi page, zoom out the page image to the top
left point

return:
	0		OK
	<0		fail
--------------------------------------------------------------------*/
int egi_page_dispear(EGI_EBOX *ebox)
{
        int wid,hgt;
	int bkwid,bkhgt; /* wid and hgt for backup */
	int screensize=gv_fb_dev.screensize;
	int xres=gv_fb_dev.vinfo.xres;
	int yres=gv_fb_dev.vinfo.yres;
	uint16_t *buf;
	uint16_t *sbuf; /* for scaled image */
	uint16_t *bkimg; /* bkimg for scaled area */

	/* check ebox is a full egi page */

	/* malloc buf */
	buf=malloc(screensize);
	if(buf==NULL)
	{
		printf("egi_page_dispear(): fail to malloc buf.\n");
		return -1;
	}
	sbuf=malloc(screensize);
	if(sbuf==NULL)
	{
		printf("egi_page_dispear(): fail to malloc sbuf.\n");
		return -2;
	}
	bkimg=malloc(screensize);
	if(bkimg==NULL)
	{
		printf("egi_page_dispear(): fail to malloc bkimg.\n");
		return -3;
	}

        /* 1. grap current page image */
        printf("start fb_cpyto_buf\n");
        fb_cpyto_buf(&gv_fb_dev, 0, 0, xres-1, yres-1, buf);
        printf("start for...\n");

	/* 2. restore original page image */
    	fb_cpyfrom_buf(&gv_fb_dev,ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
				ebox->bkbox.endxy.x,ebox->bkbox.endxy.y,ebox->bkimg);
        fb_cpyto_buf(&gv_fb_dev,0,0,xres-1,yres-1,bkimg);
	bkwid=xres;bkhgt=yres;

        /* 3. zoom out to left top point */
        for(wid=xres*3/4;wid>0;wid-=4)
        {
		/* 2.1 scale the image */
                hgt=wid*yres/xres; //4/3;
                fb_scale_pixbuf(xres,yres,wid,hgt,buf,sbuf);
                /* 2.2 restore ebox bk image */
                fb_cpyfrom_buf(&gv_fb_dev,0,0,bkwid-1,bkhgt-1,bkimg);
           //     fb_cpyfrom_buf(&gv_fb_dev,ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
	//					ebox->bkbox.endxy.x,ebox->bkbox.endxy.y,ebox->bkimg);
                /* 2.3 store the area which will be replaced by scaled image */
                fb_cpyto_buf(&gv_fb_dev,0,0,wid-1,hgt-1,bkimg);
		bkwid=wid;bkhgt=hgt;
                /* 2.3 put scaled image */
                fb_cpyfrom_buf(&gv_fb_dev,0,0,wid-1,hgt-1,sbuf);
//		usleep(100000);
        }

        /* 3. */
	free(buf);
	free(sbuf);

	return 0;
}
