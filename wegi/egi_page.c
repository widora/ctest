#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sys_list.h"
#include "xpt2046.h"
#include "egi.h"
#include "egi_timer.h"
#include "egi_page.h"
#include "egi_debug.h"
#include "egi_color.h"
#include "egi_symbol.h"
#include "egi_bmpjpg.h"
#include "egi_touch.h"
#include "egi_log.h"

/*---------------------------------------------
init a egi page
tag: a tag for the page
Return
	page pointer		ok
	NULL			fail
--------------------------------------------*/
EGI_PAGE * egi_page_new(char *tag)
{
	int i;
	EGI_PAGE *page;

	/* 2. malloc page */
	page=malloc(sizeof(struct egi_page));
	if(page == NULL)
	{
		printf("egi_page_init(): fail to malloc page.\n");
		return NULL;
	}
	/* clear data */
	memset(page,0,sizeof(struct egi_page));

	/* 3. malloc page->ebox */
	page->ebox=egi_ebox_new(type_page);
	if( page == NULL)
	{
		printf("egi_page_init(): egi_ebox_new() fails.\n");
		return NULL;
	}

	/* 4. put tag here */
	egi_ebox_settag(page->ebox,tag);
	//strncpy(page->ebox->tag,tag,EGI_TAG_LENGTH); /* EGI_TAG_LENGTH+1 for a EGI_EBOX */


	/* 5. set prmcolor<0, so it will NOT  draw prmcolor in page refresh()
	   !!!! 0 will be deemed as pure black in refresh()  */
	page->ebox->prmcolor=-1;

	/* 6. put default routine method here */
	page->routine=egi_page_routine;

	/* 7. init pthreads. ??? Not necessary. since alread memset() in above ??? */
	for(i=0;i<EGI_PAGE_MAXTHREADS;i++)
	{
		page->thread_running[i]=false;
		page->runner[i]=NULL; /* thread functions */
	}

	/* 8. init list */
        INIT_LIST_HEAD(&page->list_head);

	return page;
}


/*---------------------------------------------
free a egi page
Return:
	0	OK
	<0	fails
--------------------------------------------*/
int egi_page_free(EGI_PAGE *page)
{
	int ret=0;
	struct list_head *tnode, *tmpnode;
	EGI_EBOX *ebox;

	/* check data */
	if(page == NULL)
	{
		printf("egi_page_free(): page is NULL! fail to free.\n");
		return -1;
	}
	if( page->ebox==NULL )
	{
		printf("%s: WARN: input page->ebox is NULL!\n",__func__);
	}

	/*  free every child in list */
	if(!list_empty(&page->list_head))
	{
		/* traverse the list, not safe */
		list_for_each_safe(tnode, tmpnode, &page->list_head)
        	{
               	 	ebox=list_entry(tnode,EGI_EBOX,node);
			EGI_PDEBUG(DBG_PAGE,"ebox '%s' is unlisted from page '%s' and freed.\n" 
									,ebox->tag,page->ebox->tag);
                	list_del(tnode);
                	ebox->free(ebox);
        	}
	}

	/* free self ebox */
	if(page->ebox != NULL)
		free(page->ebox);
	/* free page */
	free(page);

	return ret;
}


/*--------------------------------------------------
add a ebox into a page's head list.
return:
	0	OK
	>0	ingored
	<0	fails
---------------------------------------------------*/
int egi_page_addlist(EGI_PAGE *page, EGI_EBOX *ebox)
{
	/* check data */
	if( page==NULL || page->ebox==NULL )
	{
		printf("%s: input page or page->ebox is NULL!\n",__func__);
		return -1;
	}
	if(ebox==NULL)
	{
		printf("egi_page_addlist(): input ebox is NULL!\n");
		return 1;
	}


	/* set ebox->container */
	ebox->container=page;

	/* add to list tail */
	list_add_tail(&ebox->node, &page->list_head);
	EGI_PDEBUG(DBG_PAGE,"ebox '%s' is added to page '%s' \n",
								ebox->tag, page->ebox->tag);

	return 0;
}

/*--------------------------------------------------
traverse the page list and print it.
return:
	0	OK
	<0	fails
---------------------------------------------------*/
int egi_page_travlist(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;

	/* check data */
	if(page==NULL || page->ebox==NULL )
	{
		printf("%s: input page or page->ebox is NULL!\n",__func__);
		return -1;
	}
	/* check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_travlist(): page '%s' has an empty list_head.\n",page->ebox->tag);
		return -2;
	}

	/* traverse the list, not safe */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		printf("egi_page_travlist(): find child --- ebox: '%s' --- \n",ebox->tag);
	}


	return 0;
}



/*--------------------------------------------------
activate a page and its eboxes in its list.

return:
	0	OK
	<0	fails
---------------------------------------------------*/
int egi_page_activate(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;
	int ret=0;
	int xres=gv_fb_dev.vinfo.xres;
	int yres=gv_fb_dev.vinfo.yres;

	/* check data */
	if(page==NULL || page->ebox==NULL)
	{
		printf("egi_page_activate(): input page or page->ebox is NULL!\n");
		return -1;
	}
	/* check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_activate(): page '%s' has an empty list_head .\n",page->ebox->tag);
		return -2;
	}

	/* set page status */
	page->ebox->status=status_active;

        /* !!!!! in page->ebox.refresh(), the page->fpath will NOT be seen and handled, it's a page method.
	load a picture or use prime color as wallpaper*/
        if(page->fpath != NULL)
                show_jpg(page->fpath, &gv_fb_dev, SHOW_BLACK_NOTRANSP, 0, 0);

        else /* use ebox prime color to clear(fill) screen */
        {
		if( page->ebox->prmcolor >= 0)
		{
#if 0
			fbset_color( page->ebox->prmcolor );
			draw_filled_rect(&gv_fb_dev,0,0,xres-1,yres-1); /* full screen */
#endif
			clear_screen(&gv_fb_dev, page->ebox->prmcolor);
		}
        }


	/* traverse the list and activate list eboxes, not safe */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		ret=ebox->activate(ebox);
		EGI_PDEBUG(DBG_PAGE,"activate page list item ebox: '%s' with ret=%d \n",ebox->tag,ret);
	}


	return 0;
}


/*---------------------------------------------------------
1. check need_refresh flag for page and refresh it if true.
2. refresh page's child eboxes.

return:
	1	need_refresh=false
	0	OK
	<0	fails
--------------------------------------------------------*/
int egi_page_refresh(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;
	int ret;
	int xres=gv_fb_dev.vinfo.xres;
	int yres=gv_fb_dev.vinfo.yres;

	/* check data */
	if( page==NULL || page->ebox==NULL )
	{
		printf("egi_page_refresh(): input page or page->ebox is NULL!\n");
		return -1;
	}

	/* --------------- ***** FOR 'PAGE' REFRESH ***** ------------ */
	/* only if need_refresh */
	if(page->ebox->need_refresh)
	{
		//printf("egi_page_refresh(): refresh page '%s' wallpaper.\n",page->ebox->tag);
		EGI_PDEBUG(DBG_PAGE,"refresh page '%s' wallpaper.\n",page->ebox->tag);

		/* load a picture or use prime color as wallpaper*/
		if(page->fpath != NULL)
			show_jpg(page->fpath, &gv_fb_dev, SHOW_BLACK_NOTRANSP, 0, 0);

		else /* use ebox prime color to clear(fill) screen */
		{
			if( page->ebox->prmcolor >= 0)
			{
#if 0
				fbset_color( page->ebox->prmcolor );
				draw_filled_rect(&gv_fb_dev,0,0,xres-1,yres-1); /* full screen */
#endif
				clear_screen(&gv_fb_dev, page->ebox->prmcolor);
			}
		}

		/* reset need_refresh */
		page->ebox->need_refresh=false;
	}


	/* --------------- ***** FOR PAGE CHILD REFRESH ***** ------------*/
	/* check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_refresh(): page '%s' has an empty list_head .\n",page->ebox->tag);
		return -2;
	}

	/* traverse the list and activate list eboxes, not safe */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		ret=ebox->refresh(ebox);
		if(ret==0)
		    EGI_PDEBUG(DBG_TEST,"egi_page_refresh(): refresh page '%s' list item ebox: '%s' with ret=%d \
			 \n ret=1 need_refresh=false \n", page->ebox->tag,ebox->tag,ret);
	}

	/* reset need_refresh */
	page->ebox->need_refresh=false;

	return 0;
}

/*--------------------------------------------------------------
Just set need_refresh flag for the page, but do not set flag for
its children.

return:
        0       OK
        <0      fails
----------------------------------------------------------------*/
int egi_page_flag_needrefresh(EGI_PAGE *page)
{
        /* 1. check data */
        if(page==NULL || page->ebox==NULL)
        {
                printf("%s: input page or page->ebox is NULL!\n",__func__);
                return -1;
        }

	/* 2. set page need_refresh flag */
	page->ebox->need_refresh=true;

	return 0;
}


/*--------------------------------------------------------------
Set all eboxes in a page to be need_refresh=true
return:
	0	OK
	<0	fails
----------------------------------------------------------------*/
int egi_page_needrefresh(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;

	/* 1. check data */
	if(page==NULL || page->ebox==NULL)
	{
		printf("egi_page_needrefresh(): input page or page->ebox is NULL!\n");
		return -1;
	}

	/* 2. check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_needrefresh(): page '%s' has an empty list_head.\n",page->ebox->tag);
		return -2;
	}

	/* 3. set page->ebox */
	page->ebox->need_refresh=true;

	/* 4. traverse the list and set page need_refresh, not safe */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		ebox->need_refresh=true;
		EGI_PDEBUG(DBG_PAGE,"find child --- ebox: '%s' --- \n",ebox->tag);
	}

	return 0;
}


/*----------------------------------------------------------------------------------
pick an ebox pointer by its type and id number

return:
	pointer 	OK
	NULL		fails not no match
-----------------------------------------------------------------------------------*/
EGI_EBOX *egi_page_pickebox(EGI_PAGE *page,enum egi_ebox_type type,  unsigned int id)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;

	/* 1. check data */
	if(page==NULL || page->ebox==NULL )
	{
		printf("%s: input page or page->ebox is NULL!\n",__func__);
		return NULL;
	}

	/* 2. check list */
	if(list_empty(&page->list_head))
	{
		printf("%s: page '%s' has an empty list_head.\n",__func__,page->ebox->tag);
		return NULL;
	}

	/* 3. traverse the list to find ebox that matches the given id */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		if( ebox->type==type && ((EGI_DATA_BTN *)(ebox->egi_data))->id == id )
		{
		   EGI_PDEBUG(DBG_PAGE,"%s: find an ebox '%s' with id=%d in page '%s'. \n",
									__func__, ebox->tag,id,page->ebox->tag);
			return ebox;
		}
	}

	EGI_PLOG(LOGLV_WARN,"%s: ebox '%s' with id=%d can NOT be found in page '%s'. \n",
									__func__, ebox->tag,id,page->ebox->tag);
	return NULL;
}



/*-----------------------------------------------------
default page routine job

return:
	loop or >=0  	OK
	<0		fails
-----------------------------------------------------*/
int egi_page_routine(EGI_PAGE *page)
{
	int i,j;
	int ret;
	uint16_t sx,sy;
	enum egi_touch_status last_status=released_hold;
	EGI_TOUCH_DATA touch_data;

	/* delay a while, to avoid touch-jittering ???? necessary ??????? */
	tm_delayms(100);


	/* for time struct */
	struct timeval t_start,t_end; /* record two pressing_down time */
	long tus;

	EGI_EBOX  *hitbtn; /* hit button_ebox */

	/* 1. check data */
	EGI_PDEBUG(DBG_PAGE,"start to check data for page.\n");
	if(page==NULL || page->ebox==NULL)
	{
		printf("egi_page_routine(): input page OR page->ebox  is NULL!\n");
		return -1;
	}

	/* 2. check list */
	EGI_PDEBUG(DBG_PAGE,"start to check ebox list for page.\n");
	if(list_empty(&page->list_head))
	{
		printf("egi_page_routine(): WARNING!!! page '%s' has an empty ebox list_head .\n",page->ebox->tag);
	}

	EGI_PDEBUG(DBG_PAGE,"--------------- get into [PAGE %s]'s loop routine -------------\n",page->ebox->tag);

	/* 3. load page runner threads */
	EGI_PDEBUG(DBG_PAGE,"start to load [PAGE %s]'s runner...\n",page->ebox->tag); 
	for(i=0;i<EGI_PAGE_MAXTHREADS;i++)
	{
		if( page->runner[i] !=0 )
		{
			if( pthread_create( &page->threadID[i],NULL,(void *)page->runner[i],(void *)page)==0)
			{
				page->thread_running[i]=true;
				printf("egi_page_routine(): create pthreadID[%d]=%u successfully. \n",
								i, (unsigned int)page->threadID[i]);
			}
			else
			   printf("egi_page_routine(): fail to create pthread for runner[%d] of page[%s] \n",
					i,page->ebox->tag );
		}
	}


 	 /* ----------------    Touch Event Handling   ----------------  */

	/* discard first obsolete data, just to inform egi_touch_loopread() to start loop_read */
	egi_touch_getdata(&touch_data);

	while(1)
	{
		/* 1. read touch data */
		if(!egi_touch_getdata(&touch_data) )
		{
//			EGI_PDEBUG(DBG_PAGE,"egi_page_routine(): egi_touch_getdata() no updated touch data found, retry...\n");
			continue;
		}
		sx=touch_data.coord.x;
		sy=touch_data.coord.y;
		last_status=touch_data.status;

		/* 2. trigger touch handling process then */
		if(last_status !=released_hold )
		{
			/* check if any ebox was hit */
		        hitbtn=egi_hit_pagebox(sx, sy, page, type_btn);

			/* trap into button reaction functions */
	       	 	if(hitbtn != NULL)
			{
				EGI_PDEBUG(DBG_PAGE,"egi_page_routine(): [page '%s'] [button '%s'] is touched! touch status is '%s'\n",
  						page->ebox->tag,hitbtn->tag,egi_str_touch_status(last_status));
		       /*  then trigger button-hit action:
		  	   1. 'pressing' and 'db_pressing' reaction events never coincide,
				'pressing' will prevail  ---

			*  2. !!!!WARNING: check touch_data in buttion reaction funciton at very begin,
			*  Status 'pressed_hold' will last for a while, so it will have chance to trigger
			*  button of previous page after triggered current page routine to quit.
			*  3. only 'pressed_hold','pressing','db_pressing' can trigger button reaction.
			*/
 				if( hitbtn->reaction != NULL && (  last_status==pressed_hold ||
								   last_status==pressing ||
								   last_status==db_pressing  )  )
				{

					/*if ret<0, button pressed to exit current page
					   usually fall back to its page's routine caller to release page...
					*/
					ret=hitbtn->reaction(hitbtn, &touch_data);//last_status);

					/* IF: a button request to exit current page routine */
					if( ret==btnret_REQUEST_EXIT_PAGE )
					{
				       printf("[page '%s'] [button '%s'] ret: request to exit host page.\n",
										page->ebox->tag, hitbtn->tag);
						return pgret_OK; // or break;
					}

					/* ELSE IF: a button activated page returns. */
					else if ( ret==pgret_OK || ret==pgret_ERR )
					{
	  printf("[page '%s'] [button '%s'] ret: return from a button activated page, set needrefresh flag.\n",
										page->ebox->tag,hitbtn->tag);
						egi_page_needrefresh(page); /* refresh whole page */
					}
					else
					/* ELSE: for other cases, btnret_OK, btnret_ERR  */
					{
					/* needfresh flags for page or eboxes to be set by btn react. func. */
					    //let btn do it:	egi_page_needrefresh(page);
					}
				}

			 } /* end of hitbtn reaction */
		}
		else /* last_status == released_hold */
		{
			/* else, do other routine jobs */
                        //eig_pdebug(DBG_PAGE,"egi_page_routine(): --- XPT_READ_STATUS_PENUP ---\n");
			egi_page_refresh(page); /* only page->eboxs with needrefresh flag */

			/* hold on for a while, otherwise the screen will be  ...heheheheheh...
			 *
			 */
			tm_delayms(100); //55

			/* loop in refreshing listed eboxes */
		}

	}/* end while() */


	return pgret_OK;
	//return 0;
}
