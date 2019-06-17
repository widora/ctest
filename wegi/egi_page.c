/*--------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
---------------------------------------------------------------------*/
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
#include "egi_bjp.h"
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
	/* set page as its container */
	page->ebox->container=page;

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


/*--------------------------------------------
free a egi page
Return:
	0	OK
	<0	fails
--------------------------------------------*/
int egi_page_free(EGI_PAGE *page)
{
	int ret=0;
	int i;
	struct list_head *tnode, *tmpnode;
	EGI_EBOX *ebox;

	/* check data */
	if(page == NULL) {
		printf("egi_page_free(): page is NULL! fail to free.\n");
		return -1;
	}
	if( page->ebox==NULL ) {
		printf("%s: WARN: input page->ebox is NULL!\n",__func__);
	}

	/* change status, wait runner ....*/
	page->ebox->status=status_page_exiting;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	for(i=0;i<EGI_PAGE_MAXTHREADS;i++) {
		if(page->thread_running[i]) {
			EGI_PDEBUG(DBG_PAGE,"page ['%s'] wait to join runner thread [%d].\n"
										,page->ebox->tag,i);
			if( pthread_cancel(page->threadID[i]) !=0 )
			     EGI_PLOG(LOGLV_ERROR,"Fail to call pthread_cancel() for page ['%s'] threadID[%d] \n",
									page->ebox->tag, i);
			if( pthread_join(page->threadID[i],NULL) !=0 )
			     EGI_PLOG(LOGLV_ERROR,"Fail to call pthread_join() for page ['%s'] threadID[%d] \n",
									page->ebox->tag, i);
			else
				EGI_PDEBUG(DBG_PAGE,"page ['%s'] runner thread [%d] joined!.\n"
										,page->ebox->tag,i);
		}
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
//		EGI_PDEBUG(DBG_PAGE,"egi_page_travlist(): find child --- ebox: '%s' --- \n",ebox->tag);
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
	int xres __attribute__((__unused__))=gv_fb_dev.vinfo.xres;
	int yres __attribute__((__unused__))=gv_fb_dev.vinfo.yres;

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

	/* decroating job if any */
	egi_ebox_decorate(page->ebox);

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
2. refresh each of page's child eboxes only if its needrefresh
   flag is true.

return:
	1	need_refresh=false
	0	If any ebox has been refreshed.
	<0	fails
--------------------------------------------------------*/
int egi_page_refresh(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;
	int ret=1; /* set 1 first! */
	int xres __attribute__((__unused__))=gv_fb_dev.vinfo.xres;
	int yres __attribute__((__unused__))=gv_fb_dev.vinfo.yres;

	/* check data */
	if( page==NULL || page->ebox==NULL )
	{
		printf("egi_page_refresh(): input page or page->ebox is NULL!\n");
		return -1;
	}

	/* --------------- ***** FOR 'PAGE' REFRESH, wallpaper etc. ***** ------------ */
	/* only if need_refresh */
	if(page->ebox->need_refresh)
	{
		//printf("egi_page_refresh(): refresh page '%s' wallpaper.\n",page->ebox->tag);
		//EGI_PDEBUG(DBG_PAGE,"refresh page '%s' wallpaper.\n",page->ebox->tag);

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

		/* decroating job if any */
		egi_ebox_decorate(page->ebox);

		/* reset need_refresh */
		page->ebox->need_refresh=false;

		/* set ret */
		ret=0;
	}

	/* --------------- ***** FOR PAGE CHILD REFRESH ***** ------------*/
	/* check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_refresh(): page '%s' has an empty list_head .\n",page->ebox->tag);
		return -1*ret;
	}

	/* traverse the list and activate list eboxes, not safe */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		ret *= ebox->refresh(ebox);
#if 0 /* debug only */
		if(ret==0)
		    EGI_PDEBUG(DBG_PAGE,"egi_page_refresh(): refresh page '%s' list item ebox: '%s' with ret=%d \
			 \n ret=1 need_refresh=false \n", page->ebox->tag,ebox->tag,ret);
#endif
	}

	/* reset need_refresh */
	page->ebox->need_refresh=false;

	return ret; /* if any ebox refreshed, return 0 */
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
		EGI_PDEBUG(DBG_PAGE,"find child ebox: '%s' \n",ebox->tag);
	}

	return 0;
}


/*----------------------------------------------------------------------------------
pick a btn or slider pointer by its type and id number

@page	a page struct holding eboxes.
@type	type_btn OR type_slider
@id	NOTE: id of a EGI_DATA_BTN

return:
	pointer 	OK
	NULL		fails,or no match
-----------------------------------------------------------------------------------*/
EGI_EBOX *egi_page_pickbtn(EGI_PAGE *page,enum egi_ebox_type type,  unsigned int id)
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
		   EGI_PDEBUG(DBG_PAGE,"%s: find an ebox '%s' with data_btn->id=%d in page '%s'. \n",
									__func__, ebox->tag,id,page->ebox->tag);
			return ebox;
		}
	}

	EGI_PLOG(LOGLV_WARN,"%s: ebox '%s' with id=%d can NOT be found in page '%s'. \n",
									__func__, ebox->tag,id,page->ebox->tag);
	return NULL;
}


/*----------------------------------------------------------------------------------
pick a ebox pointer by its type and id number

@page	a page struct holding eboxes.
@type	type of ebox
@id	ID of the ebox.  ebox->id

return:
	pointer 	OK
	NULL		fails, or no match
-----------------------------------------------------------------------------------*/
EGI_EBOX *egi_page_pickebox(EGI_PAGE *page,enum egi_ebox_type type,  unsigned int id)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;

	/* 1. check data */
	if(id==0)
	{
		printf("%s: invalid for id=0!\n",__func__);
		return NULL;
	}
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
		if( ebox->type==type && ebox->id == id )
		{
//		   EGI_PDEBUG(DBG_PAGE,"Find an ebox '%s' with ebox->id=%d in page '%s'. \n",
//									ebox->tag,id,page->ebox->tag);
			return ebox;
		}
	}

	EGI_PLOG(LOGLV_WARN,"ebox '%s' with id=%d can NOT be found in page '%s'. \n",
									ebox->tag,id,page->ebox->tag);
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
	int i;
	int ret;
	uint16_t sx,sy;
	enum egi_touch_status last_status=released_hold;
	EGI_TOUCH_DATA touch_data;

	/* delay a while, to avoid touch-jittering ???? necessary ??????? */
	tm_delayms(200);

	/* for time struct */
//	struct timeval t_start,t_end; /* record two pressing_down time */
//	long tus;

	EGI_EBOX  *hitbtn; /* hit button_ebox */
	EGI_EBOX  *last_holdbtn=NULL; /* remember last hold btn */

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
			if( pthread_create( &page->threadID[i], NULL,(void *)page->runner[i],(void *)page)==0)
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

	/* Try to discard first obsolete data, just to inform egi_touch_loopread() to start loop_read */
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
			EGI_PDEBUG(DBG_PAGE,"last_status=%d \n",last_status);
			/* check if any ebox was hit */
		        hitbtn=egi_hit_pagebox(sx, sy, page, type_btn|type_slider);

			EGI_PDEBUG(DBG_PAGE,"hitbtn is '%s' \n", (hitbtn==NULL) ? "NULL" : hitbtn->tag);

			/* check if last hold btn losing foucs
			 * Note:
			 *	1. If it re_enters from SIGSTOP, this will trigger the very hitbtn
			 *	   which rasied SIGSTOP signal, to refresh and reset its need_refresh
			 *	   flag. So, when elements of the page are refreshed as SIGCONT
			 *	   handler expected, this hitbtn will dispear!!!
			 *	   We need to confirm 'last_holdbtn->need_refresh==false' here to rule
			 *	   out the situation, and make sure when egi_page_refresh(page) is called
			 *	   all elements are to refreshed in order.
			 */


			if( last_holdbtn != NULL && last_holdbtn != hitbtn
						 && last_holdbtn->need_refresh==false )
			{
				EGI_PDEBUG(DBG_PAGE,"last_holdbtn losed focus, refresh it...\n");
				egi_ebox_forcerefresh(last_holdbtn); /* refreshi it then */
				last_holdbtn=NULL;
			}

			/* trap into button reaction functions */
	       	 	if(hitbtn != NULL)
			{

//	EGI_PDEBUG(DBG_PAGE,"[page '%s'] [button '%s'] is touched! touch status is '%s'\n",
//						page->ebox->tag,hitbtn->tag,egi_str_touch_status(last_status));

		       /*  then trigger button-hit action:
		  	   1. 'pressing' and 'db_pressing' reaction events never coincide,
				'pressing' will prevail  ---
			*  2. !!!!WARNING: check touch_data in buttion reaction funciton at very begin,
			*  Status 'pressed_hold' will last for a while, so it will have chance to trigger
			*  button of previous page after triggered current page routine to quit.
			*  3. only 'pressed_hold','pressing','db_pressing' can trigger button reaction. ???
			*/
				/* display touch effect for button */
				if(hitbtn->type==type_btn) {

				    /* remember last hold btn */
				    if(last_status==pressed_hold) last_holdbtn=hitbtn;


 /* 1. When the page returns from SIGSTOP by signal SIGCONT, status 'pressed_hold' and 'releasing'
   *    will be received one after another,In rare case, it may receive 2 'pressed_hold'.
   *	and the signal handler will call egi_page_needrefresh().
   *    But 'releasing' will trigger btn refresh by egi_btn_touch_effect() just before page refresh.
   *    After refreshed, need_refresh will be reset for this btn, so when egi_page_refresh() is called
   *    later to refresh other elements, this btn will erased by page bkcolor/wallpaper.
   * 2. The 'releasing' status here is NOT a real pen_releasing action, but a status changing signal.
   *    Example: when status transfers from 'pressed_hold' to PEN_UP etc
   * 3. So, we need to bypass 'releasing' here by checking hitbtn->need_refresh!
   */
				    /* call touch_effect() */
		                    if( hitbtn->need_refresh==false   /* In case SIGCONT triggered */
                                        && ( ((EGI_DATA_BTN *)hitbtn->egi_data)->touch_effect != NULL ) )
					((EGI_DATA_BTN *)hitbtn->egi_data)->touch_effect(hitbtn,&touch_data);//last_status);

				}
				/* trigger reaction func */
 				if( hitbtn->reaction != NULL && (  last_status==pressed_hold ||
								   last_status==pressing ||
								   last_status==releasing ||
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
						egi_page_refresh(page);
						/* wait for 'release_hold' as begin of a new touch session
					         * the purpuse is to let last page's touching status pass away,
						 * especially 'pressed_hold' and 'releasing', which may trigger
						 * refreshed page again!!!
						 */
						do {
							tm_delayms(200);
						 	egi_touch_getdata(&touch_data);

						}while(touch_data.status != released_hold);

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

		       /* 	              -----   NOTE  -----
			* If a react function is triggered by a pressing touch, then after running
			* the function, a press_hold AND/OR a releasing touch status will likely be
		        * captured immediately and it triggers its reaction function just before following
			* egi_page_refresh().
			* If above mentioned press_hold/releasing reaction function also include refreshing
			* the ebox, that will cause following egi_page_refresh() useless. Because after refresh
			* the need_refresh flag will be reset afterall.
			* So, it must run just after egi_page_needrefresh(page);
			* OR OR OR --- All react functions to be triggered by a 'releasing' status. ??
			* However...a 'releasing' status always happens after a 'pressing' status, and if the
			* preceding 'pressing' triggers a PAGE, which unfortunatly has an ebox just at the same
			* location waiting for a 'releasing' to activate it. WOW.... It is activated at once!!!
			* Conclusion: activating touch_status(signal) for all eboxes shall be the same type!
	               */

			/* refresh page, OR sleep a while */
			if(egi_page_refresh(page)!=0) {
				/* hold on for a while, otherwise the screen will be  ...heheheheheh... */
#if 1
				tm_delayms(75); //55
#endif
#if 0 /* conflict with timer */
				egi_sleep(0,0,900);
#endif

			}


#if 0 /* conflict with timer */
	                printf("--------egi_page: egi_sleep 900ms ------------\n");
			egi_sleep(4,0,900);
	                printf("--------egi_page:  end egi_sleep 900ms------------\n");
#endif
			/* loop in refreshing listed eboxes */
		}

	}/* end while() */

	return pgret_OK;
	//return 0;
}


/*-----------------------------------------------------
Home page routine job

return:
	loop or >=0  	OK
	<0		fails
-----------------------------------------------------*/
int egi_homepage_routine(EGI_PAGE *page)
{
	int i;
	int ret;
	uint16_t sx,sy;
	enum egi_touch_status flip_status; /* for 'releasing' or 'pressing' */
	enum egi_touch_status last_status=released_hold;
	enum egi_touch_status check_status;
	EGI_TOUCH_DATA touch_data;
	int tdx,tdy;
	bool slide_touch=false;

	/* delay a while, to avoid touch-jittering ???? necessary ??????? */
	tm_delayms(200);

	/* for time struct */
//	struct timeval t_start,t_end; /* record two pressing_down time */
//	long tus;

	EGI_EBOX  *hitbtn=NULL; /* hit button_ebox */
	EGI_EBOX  *last_holdbtn=NULL; /* remember last hold btn */

	/* 1. check data */
	EGI_PDEBUG(DBG_PAGE,"start to check data for page.\n");
	if(page==NULL || page->ebox==NULL)
	{
		printf("egi_homepage_routine(): input page OR page->ebox  is NULL!\n");
		return -1;
	}

	/* 2. check list */
	EGI_PDEBUG(DBG_PAGE,"start to check ebox list for page.\n");
	if(list_empty(&page->list_head))
	{
		printf("egi_homepage_routine(): WARNING!!! page '%s' has an empty ebox list_head .\n",page->ebox->tag);
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

	/* Try to discard first obsolete data, just to inform egi_touch_loopread() to start loop_read */
	egi_touch_getdata(&touch_data);

	while(1)
	{
		/* 0. backup old touch status */
		if( touch_data.status==releasing ) {
			printf(" --- 'releasing' --- \n");
			flip_status=releasing;
		}
		else if ( touch_data.status==pressing ) {
			printf(" --- 'pressing' --- \n");
			flip_status=pressing;
		}

		/* 1. read touch data */
		if(!egi_touch_getdata(&touch_data) )
		{
//			EGI_PDEBUG(DBG_PAGE,"egi_page_routine(): egi_touch_getdata() no updated touch data found, retry...\n");
			continue;
		}
		sx=touch_data.coord.x;
		sy=touch_data.coord.y;
		last_status=touch_data.status;


		/*	-----  restore missing status  ------
		 *   The 'pressing' and 'releasing' signal may be missed due to current egi_touch.c
		 *  algrithm, especially when you add pressing force slowly at the screen.
		 *  we need to restore 'pressing' status and pass down the status.
		 */
		if( flip_status != pressing && last_status==pressed_hold ) {
		    /* restore 'pressing' status then*/
		    printf(" --- restore 'pressing' --- \n");
		    last_status=pressing;
		    flip_status=pressing;
		}
		else if(flip_status != releasing && last_status==released_hold) {
		    /* restore 'releasing' status then*/
		    printf(" --- restore 'releasing' --- \n");
		    last_status=releasing;
		    flip_status=releasing;
		}


		/* 2. trigger touch handling process then */
		if(last_status !=released_hold )
		{
			/* 2.1 Check if sliding operation begins, and update 'slide_touch' here !
			 *     1. Remind that 'pressing' is the only triggering signal for sliding operation,
			 *     and the handler will initialize data only at this signal.
			 *     2. If not dx detected within given time, sliding operation will never triggered
			 *       then.???
			 */
		        if( last_status==pressing || flip_status==pressing )  {
				/* peek next touch dx, but do not read out */
				tm_delayms(100);
				tdx=egi_touch_peekdx();
				/* check peek tdx, and also peek if 'releasing' after 'pressed_hold' */
				if(tdx > 3 || tdx < -3 ) {  //|| egi_touch_peekstatus()==releasing) {
					printf("--- start sliding ---\n");
					slide_touch=true;
				}
				/* else, pass down pressing status... */
//				else {
//					slide_touch=false;
//				}
			}

			/* 2.2 sliding handling func */
			if(slide_touch ) //&& ( last_status==pressed_hold || last_status==pressing || last_status==releasing) )
			{
				/* OR to ignore 'releasing' to let button icons stop at current position */
				if(last_status==releasing)
					printf(" --- sliding release! --- \n");

				page->slide_handler(page, &touch_data);
				egi_page_refresh(page); /* refresh page for other eboxes!!!  */

				if(last_status==releasing)  /* 'releasing' is and end to sliding operation */
					slide_touch=false;
				else
					continue;
			}


			/* 2.3 check if any ebox was hit */
		        hitbtn=egi_hit_pagebox(sx, sy, page, type_btn|type_slider);

			/* 2.4 check if last hold btn losing foucs */
			if( last_holdbtn != NULL && last_holdbtn != hitbtn )
				 	  // && last_holdbtn->need_refresh==false )
			{
				//printf(" --- btn lose focus! --- \n");
				egi_ebox_forcerefresh(last_holdbtn); /* refreshi it then */
				last_holdbtn=NULL;
			}

			/* 2.5 trap into button reaction functions */
	       	 	if(hitbtn != NULL)
			{
				/* display touch effect for button */
				if(hitbtn->type==type_btn) {

				    /* remember last hold btn */
				    if(last_status==pressed_hold) last_holdbtn=hitbtn;

				    /* call touch_effect()
				     * 'releasing' will trigger ebox refresh in egi_btn_touch_effect() !!!
				     */
//		                    if( hitbtn->need_refresh==false  &&  /* In case SIGCONT triggered */
				    if( ((EGI_DATA_BTN *)hitbtn->egi_data)->touch_effect != NULL )
					((EGI_DATA_BTN *)hitbtn->egi_data)->touch_effect(hitbtn,&touch_data);//last_status);
				}

				/* trigger reaction func */
 				if( hitbtn->reaction != NULL && (  last_status==pressed_hold ||
								   last_status==pressing ||
								   last_status==releasing ||
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
						egi_page_refresh(page);
						/* wait for 'release_hold' as begin of a new touch session
					         * the purpose is to let last page's touching status pass away,
						 * especially 'pressed_hold' and 'releasing', which may trigger
						 * refreshed page again!!!
						 */
						do {
							tm_delayms(100);
						 	egi_touch_getdata(&touch_data);

						}while(touch_data.status != released_hold);

					}
					else
					/* ELSE: for other cases, btnret_OK, btnret_ERR  */
					{
					/* needfresh flags for page or eboxes to be set by btn react. func. */
					    //let btn do it:	egi_page_needrefresh(page);
					}
				}

			 } /* end of hitbtn reaction, hitbtn != NULL */
		}

		else /* last_status == released_hold */
		{
			/* refresh page, OR sleep a while */
			if(egi_page_refresh(page)!=0) {
#if 1
				tm_delayms(75); //55
#endif
#if 0 /* conflict with timer */
				egi_sleep(0,0,900);
#endif

			}
		}


	}/* end while() */

	return pgret_OK;
	//return 0;
}
