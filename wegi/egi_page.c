#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "list.h"
#include "xpt2046.h"
#include "egi.h"
#include "egi_timer.h"
#include "egi_page.h"
#include "egi_debug.h"
#include "egi_color.h"
#include "egi_symbol.h"
#include "bmpjpg.h"

/*


*/

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

	/*  free every child in list */
	if(!list_empty(&page->list_head))
	{
		/* traverse the list, not safe */
		list_for_each_safe(tnode, tmpnode, &page->list_head)
        	{
               	 	ebox=list_entry(tnode,EGI_EBOX,node);
			egi_pdebug(DBG_PAGE,"egi_page_free(): ebox '%s' is unlisted from page '%s' and freed.\n" 
									,ebox->tag,page->ebox->tag);
                	list_del(tnode);
                	ebox->free(ebox);
        	}
	}

	/* free self ebox */
	free(page->ebox);
	/* free page */
	free(page);

	return ret;
}


/*--------------------------------------------------
add a ebox into a page's head list.
return:
	0	OK
	<0	fails
---------------------------------------------------*/
int egi_page_addlist(EGI_PAGE *page, EGI_EBOX *ebox)
{
	/* check data */
	if(page==NULL || ebox==NULL)
	{
		printf("egi_page_travlist(): page or ebox is NULL!\n");
		return -1;
	}

	list_add_tail(&ebox->node, &page->list_head);
	egi_pdebug(DBG_PAGE,"egi_page_addlist(): ebox '%s' is added to page '%s' \n",
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
	if(page==NULL)
	{
		printf("egi_page_travlist(): input egi_page *page is NULL!\n");
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
	if(page==NULL)
	{
		printf("egi_page_activate(): input egi_page *page is NULL!\n");
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
		egi_pdebug(DBG_PAGE,"egi_page_activate(): activate page list item ebox: '%s' with ret=%d \n",ebox->tag,ret);
	}


	return 0;
}


/*--------------------------------------------------
refresh a page and its eboxes in its list.

return:
	1	need_refresh=false
	0	OK
	<0	fails
---------------------------------------------------*/
int egi_page_refresh(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;
	int ret;
	int xres=gv_fb_dev.vinfo.xres;
	int yres=gv_fb_dev.vinfo.yres;

	/* check data */
	if(page==NULL || page->ebox==NULL )
	{
		printf("egi_page_refresh(): input egi_page * page or page->ebox is NULL!\n");
		return -1;
	}

	/* --------------- ***** FOR PAGE REFRESH ***** ------------ */
	/* only if need_refresh */
	if(page->ebox->need_refresh)
	{
		//printf("egi_page_refresh(): refresh page '%s' wallpaper.\n",page->ebox->tag);
		egi_pdebug(DBG_PAGE,"egi_page_refresh(): refresh page '%s' wallpaper.\n",page->ebox->tag);

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
		    egi_pdebug(DBG_TEST,"egi_page_refresh(): refresh page '%s' list item ebox: '%s' with ret=%d \
			 \n ret=1 need_refresh=false \n", page->ebox->tag,ebox->tag,ret);
	}

	/* reset need_refresh */
	page->ebox->need_refresh=false;

	return 0;
}


/*--------------------------------------------------------------
set all eboxes in a page to be need_refresh=true
return:
	0	OK
	<0	fails
----------------------------------------------------------------*/
int egi_page_needrefresh(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;

	/* 1. check data */
	if(page==NULL)
	{
		printf("egi_page_needrefresh(): input egi_page *page is NULL!\n");
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
		egi_pdebug(DBG_PAGE,"egi_page_needrefresh(): find child --- ebox: '%s' --- \n",ebox->tag);
	}

	return 0;
}


/*----------------------------------------------------------------------------------
pick an ebox pointer by its type and id number

return:
	pointer 	OK
	NULL		fails not no match
-----------------------------------------------------------------------------------*/
EGI_EBOX *egi_page_pickbtn(EGI_PAGE *page,enum egi_ebox_type type,  unsigned int id)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;

	/* 1. check data */
	if(page==NULL)
	{
		printf("egi_page_pickbtn(): input egi_page *page is NULL!\n");
		return NULL;
	}

	/* 2. check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_pickbtn(): page '%s' has an empty list_head.\n",page->ebox->tag);
		return NULL;
	}

	/* 3. traverse the list to find ebox that matches the given id */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		if( ebox->type==type && ((EGI_DATA_BTN *)(ebox->egi_data))->id == id )
		{
		   egi_pdebug(DBG_PAGE,"egi_page_pickbtn(): find an ebox '%s' with id=%d in page '%s'. \n",
										ebox->tag,id,page->ebox->tag);
			return ebox;
		}
	}

	egi_pdebug(DBG_PAGE,"egi_page_pickbtn():  ebox '%s' with id=%d can NOT be found in page '%s'. \n",
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
	int i,j;
	int ret;
	uint16_t sx,sy;
	enum egi_touch_status last_status=released_hold;

	/* for time struct */
	struct timeval t_start,t_end; /* record two pressing_down time */
	long tus; 

	EGI_EBOX  *hitbtn; /* hit button_ebox */

	/* 1. check data */
	if(page==NULL)
	{
		printf("egi_page_routine(): input egi_page *page is NULL!\n");
		return -1;
	}

	/* 2. check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_routine(): WARNING!!! page '%s' has an empty ebox list_head .\n",page->ebox->tag);
	}


	egi_pdebug(DBG_PAGE,"--------------- get into %s's loop routine -------------\n",page->ebox->tag);


	/* 3. load page runner threads */
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
				printf("egi_page_routine(): fail to create pthread ID=%d \n",i);
		}
	}

	/* 4. loop in touch checking and other routines.... */
	while(1)
	{
		/* 4.1. necessary wait for XPT */
	 	tm_delayms(2);

		/* 4.2. read XPT to get avg tft-LCD coordinate */
                //printf("start xpt_getavt_xy() \n");
                ret=xpt_getavg_xy(&sx,&sy); /* if fail to get touched tft-LCD xy */

		/* 4.3. touch reading is going on... */
                if(ret == XPT_READ_STATUS_GOING )
                {
                        //printf("XPT READ STATUS GOING ON....\n");
			/* DO NOT assign last_status=unkown here!!! because it'll always happen!!!
			   and you will never get pressed_hold status if you do so. */

                        continue; /* continue to loop to finish reading touch data */
                }

                /* 4.4. put PEN-UP status events here */
                else if(ret == XPT_READ_STATUS_PENUP )
                {
			if(last_status==pressing || last_status==pressed_hold)
			{
				last_status=releasing;
				printf("egi page-'%s' routine(4.4): ... ... ... pen releasing ... ... ...\n",
											page->ebox->tag);
			}
			else
				last_status=released_hold;

                        //eig_pdebug(DBG_PAGE,"egi_page_routine(): --- XPT_READ_STATUS_PENUP ---\n");
			egi_page_refresh(page);
			tm_delayms(100);/* hold on for a while, or the screen will be  */

		}

		/* 4.5. holdon(down) status events here, !!!!???? seems never happen !!???  */
		else if(ret == XPT_READ_STATUS_HOLDON)
		{
			last_status=pressed_hold;
			printf("egi page-'%s' routine(4.5): ... ... ... pen hold down ... ... ...\n",
											page->ebox->tag);

		}

		/* 4.6. get touch coordinates and trigger actions for hit button if any */
                else if(ret == XPT_READ_STATUS_COMPLETE) /* touch action detected */
                {
			/* update button last_status */
			if( last_status==pressing || last_status==db_pressing || last_status==pressed_hold )
			{
				last_status=pressed_hold;
				printf("egi page-'%s' routine(4.6): ... ... ... pen hold down ... ... ...\n",
											page->ebox->tag);
			}
			else
			{
				last_status=pressing;
				printf("egi page-'%s' routine(4.6): ... ... ... pen pressing ... ... ...\n",
							page->ebox->tag);

				/* check if it's a double-click   */
				t_start=t_end;
				gettimeofday(&t_end,NULL);
				tus=tm_diffus(t_end,t_start);
				//printf("------- diff us=%ld  ---------\n",tus);
				if( tus < TM_DBCLICK_INTERVAL )
				{
					printf("------- double click,tus=%ld ---------\n",tus);
					last_status=db_pressing;
				}
			}
                        //eig_pdebug(DBG_PAGE,"egi_page_routine(): --- XPT_READ_STATUS_COMPLETE ---\n");

		  /* ----------------    Touch Event Handling   ----------------  */
	                hitbtn=egi_hit_pagebox(sx, sy, page, type_btn);

			/* trap into button reaction functions */
	      	        if(hitbtn != NULL)
			{
				egi_pdebug(DBG_TEST,"egi_page_routine(): button '%s' of page '%s' is touched!\n",
										hitbtn->tag,page->ebox->tag);
				/* trigger button-hit action
				   return <0 to exit this rountine, roll back to forward routine then ...
				   NOTE: ---  'pressing' and 'db_pressing' reaction events never coincide,
					'pressing' will prevail  ---
				*/
	 			if(hitbtn->reaction != NULL &&
						(last_status==pressing || last_status==db_pressing ) )
				{
					/* reat_ret<0, button pressed to exit current page
					   usually fall back to its page's routine caller to release page...
					*/
					ret=hitbtn->reaction(hitbtn, last_status);
					if(ret<0)
					{
						printf("reaction of page '%s' button '%s' complete!\n",
										page->ebox->tag, hitbtn->tag);
						return -1;
					}
					/* react_ret=0, page exit!
					   the page activated in above reaction is released */
					else
					{
						if(ret==0)/* ret=0: refresh page and its eboxes */
						{
							egi_page_needrefresh(page);
						}

						else ; /* will not refresh the page */

					}
				}

			} /* end of button reaction */
	                continue;

			/* loop in refreshing listed eboxes */
		}
	}
	return 0;
}
