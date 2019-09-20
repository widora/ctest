/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
--------------------------------------------------------------------*/
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
	page=calloc(1, sizeof(struct egi_page));
	if(page == NULL)
	{
		printf("%s: Fail to calloc page.\n", __func__);
		return NULL;
	}

	/* 3. malloc page->ebox */
	page->ebox=egi_ebox_new(type_page);
	if( page == NULL)
	{
		printf("%s: egi_ebox_new() fails.\n", __func__);
		return NULL;
	}
	/* set page as its container */
	page->ebox->container=page;
	/* set type */
	page->ebox->type=type_page;

	/* 4. put tag here */
	egi_ebox_settag(page->ebox,tag);
	//strncpy(page->ebox->tag,tag,EGI_TAG_LENGTH); /* EGI_TAG_LENGTH+1 for a EGI_EBOX */

	/* 5. set prmcolor<0, so it will NOT draw prmcolor in page refresh()
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
Free a EGI_PAGE
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

	/* change status, wait to join runners ....*/
	page->ebox->status=status_page_exiting;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	/* cancel and join Runners */
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
	/* Destroy thread mutex locks and conds */
	if(pthread_mutex_destroy(&page->runner_mutex) !=0 ) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to call pthread_mutex_destroy()!\n", __func__ );
	}
	if(pthread_cond_destroy(&page->runner_cond) !=0 ) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to call pthread_cond_destroy()!\n", __func__ );
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
	if(page->ebox != NULL) {
		//free(page->ebox);
		egi_ebox_free(page->ebox);
	}

	/* free page */
	free(page);

	return ret;
}


/*----------------------------------------------------------
Suspend a runner of a PAGE, set signal variable sigSUSPEND[]
of the thread and wait until it confirms.

NOTE:
      The response time depends on the runner's loop period.

return:
	0	OK, thread suspended.
	<0	fails
----------------------------------------------------------*/
int egi_suspend_runner(EGI_PAGE *page, int runnerID)
{
	if(page==NULL)
		return -1;

	/* runner_ID is invalid or the thread is NOT running */
	if( runnerID < 0 || runnerID > EGI_PAGE_MAXTHREADS
	    		 || page->thread_running[runnerID]==false  ) {
		printf("%s: Runner ID is invalid or it's not running!\n",__func__);
		return -2;
	}

	/* 1. -------  set signal SigSUSPEND  ------- */

	if( pthread_mutex_lock(&page->runner_mutex) !=0 ) {
		printf("%s: Fail to lock pthread mutex!\n",__func__);
		return -3;
	}
/*  1.1 Start of Critical Zone  >>>>>>>>>  */

	/* 1.2 check status */
	if( page->thread_running[runnerID]==false ) {
		printf("%s: The page runner thread with runnerID=%d is NOT running!\n",__func__, runnerID);
		pthread_mutex_unlock(&page->runner_mutex);
		return -4;
	}
	if( page->thread_suspending[runnerID]==true ) {
		printf("%s: The page runner thread with runnerID=%d is already suspended!\n",__func__, runnerID);
		pthread_mutex_unlock(&page->runner_mutex);
		return -5;
	}

	/* 1.3 set signal sigSUSPEND */
	page->thread_SigSUSPEND[runnerID]=true;

/*  1.4<<<<<<<<<<  End of Critical Zone */
	pthread_mutex_unlock(&page->runner_mutex);

#if 0  //// If it gets mutex_lock before the runner_sigSuspend_handler, then it ends in deadlock! ////
	/* 2. -------  Wait to confirm suspending status  ------- */
	if( pthread_mutex_lock(&page->runner_mutex) !=0 ) {
		printf("%s: Fail to lock pthread mutex!\n",__func__);
		return -3;
	}
#endif

/*  2.1 Start of Critical Zone  >>>>>>>>>  */

	/* 2.2 wait to confirm suspending status */
	printf("%s: waiting runner ID=%d to suspend...\n",__func__, runnerID);
	while(page->thread_suspending[runnerID] != true) {
		usleep(5000);
	}

#if 0
/*  2.3 <<<<<<<<<<  End of Critical Zone */
	pthread_mutex_unlock(&page->runner_mutex);
#endif

	return 0;
}


/*------------------------------------------------------
Resume a runner from suspending.

return:
	0	OK.
	<0	fails
-------------------------------------------------------*/
int egi_resume_runner(EGI_PAGE *page, int runnerID)
{

	if(page==NULL)
		return -1;

	/* runner_ID is invalid or is NOT running */
	if( runnerID < 0 || runnerID > EGI_PAGE_MAXTHREADS
	    		 || page->thread_running[runnerID]==false  ) {
		printf("%s: Runner ID is invalid or it's not running!\n",__func__);
		return -2;
	}

	if( pthread_mutex_lock(&page->runner_mutex) !=0 ) {
		printf("%s: Fail to lock pthread mutex!\n",__func__);
		return -3;
	}
/*  Start of Critical Zone  >>>>>>>>>  */

	/* Reset SigSUSPEND first!!! then send pthread_cond_signal */
	EGI_PDEBUG(DBG_PAGE,"Call pthread_cond_signal to resume runner ID=%d.\n",runnerID);
	/* NOTE: A pthread_cond_signal() can only invoke one thread, When several runners are waiting
	         for the same condition variable and use differenct predicates to evaluate, the
		 signal may be received by a runner expecting other predicate, thus this cond_signal
		 will be wasted! So pthread_cond_signal() is not safe in asynchronous situation, so
		 we call pthread_cond_broadcast() instead.
	*/
	page->thread_SigSUSPEND[runnerID]=false; /* as predicate */
	//pthread_cond_signal(&page->runner_cond);
	pthread_cond_broadcast(&page->runner_cond);

/*  <<<<<<<<<<  End of Critical Zone */
	pthread_mutex_unlock(&page->runner_mutex);

	return 0;
}

/*------------------------------------------------------
Signal handler for page pthread runners.
If SigSUSPEND received, then this function will wait until
get mutex cond signal runner_cond.

 current only for thread_SigSUSPEND[]
 Add more if enum runner_signals needed.

return:
	0	OK
	<0	fails
------------------------------------------------------*/
int egi_runner_sigSuspend_handler(EGI_PAGE *page)
{
	int i;
	int runnerID=-1;
	pthread_t threadID;

	if(page==NULL)
		return -1;

	/* get thread ID */
	threadID=pthread_self();

	/* get runner ID */
	for(i=0; i<EGI_PAGE_MAXTHREADS; i++) {
		if(threadID==page->threadID[i]) {
			runnerID=i;
			break;
		}
	}
	if(runnerID<0) {
		printf("%s: Fail to get runner ID! Pls ensure the caller is a page runner.\n",__func__);
		return -2;
	}


	if( pthread_mutex_lock(&page->runner_mutex) !=0 ) {
		printf("%s: Fail to lock pthread mutex!\n",__func__);
		return -3;
	}
/*  Start of Critical Zone  >>>>>>>>>  */

	/* NOTE: "Some implementations, particularly on a multiprocessor, may sometimes cause multiple
	 * 	 threads to wake up when the condition variable is signaled simultaneously on different
	 *	 processors. so whenever a condition wait returns, the thread has to re-evaluate the
	 *	 predicate associated with the condition wait to determine whether is can safely proceed,
	 *	 should wait again, or should declare a timeout. It is thus recommend that a condition
	 *	 wait be enclosed in the equivalent of a "while loop" that checks the predicate "
	 */

	/* wait thread_SigSUSPEND[] to be reset to FALSE by other thread, who will call
	 * pthread_cond_signal() after that.
	 */
	while ( page->thread_SigSUSPEND[runnerID] ) {
		page->thread_suspending[runnerID]=true; /* reset status */
		EGI_PDEBUG(DBG_PAGE,"start pthread_cond_wait() for '%s' runner_cond... \n",page->ebox->tag);
		pthread_cond_wait(&page->runner_cond, &page->runner_mutex); /* put mutex first, and get mutex again when cond reaches */
	}

	/* reset status suspending */
	page->thread_suspending[runnerID]=false;

/*  <<<<<<<<<<  End of Critical Zone */
	pthread_mutex_unlock(&page->runner_mutex);

	return 0;
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
	EGI_EBOX *ebox=NULL;

	/* check data */
	if(page==NULL || page->ebox==NULL )
	{
		printf("%s: input page or page->ebox is NULL!\n",__func__);
		return -1;
	}
	/* check list */
	if(list_empty(&page->list_head))
	{
		printf("%s: page '%s' has an empty list_head.\n",__func__,page->ebox->tag);
		return -2;
	}

	/* traverse the list, not safe */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		EGI_PDEBUG(DBG_PAGE,"Find child --- ebox: '%s' --- \n",ebox->tag);
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
	EGI_IMGBUF *imgbuf;

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

        /*** 	Display page->ebox->frame_img, or page->fpath, or use prime color as wallpaper.
	 * !!!NOTE: In page->ebox.refresh(), the page->fpath will NOT be seen and handled, It's a page method!
	 *      We have to refresh page element(wallpaper) here, NOT in egi_page_refresh().
	 */
	if( page->ebox->frame_img !=NULL ) {
		EGI_PDEBUG(DBG_PAGE,"Apply page->ebox->frame_img for '%s' wallpaper.\n", page->ebox->tag);
		imgbuf=page->ebox->frame_img;
		/* egi_image_setFrame() if necessary */

		/* no subcolor, no FB filo */
   		egi_imgbuf_windisplay2(imgbuf, &gv_fb_dev, 0, 0, 0, 0, imgbuf->width, imgbuf->height);
	}
        else if( page->fpath != NULL) {
		EGI_PDEBUG(DBG_PAGE,"Load '%s' for '%s' wallpaper.\n", page->fpath, page->ebox->tag);
		/* Also see egi_page_refresh(EGI_PAGE *page) */
		/* load a picture or use prime color as wallpaper */
		if(page->fpath != NULL) {
			//show_jpg(page->fpath, &gv_fb_dev, SHOW_BLACK_NOTRANSP, 0, 0);
	        	imgbuf=egi_imgbuf_alloc(); //new();
		        /* First try to load as PNG file */
			printf("%s: Try to load as PNG file ...\n",__func__);
		        if(egi_imgbuf_loadpng(page->fpath, imgbuf) !=0) {
        		     // printf("%s: Load PNG imgbuf fail, try egi_imgbuf_loadjpg()...\n",__func__);
			     /* Then try to load as JPG file */
                	     if(egi_imgbuf_loadjpg(page->fpath, imgbuf) !=0) {
                        	 //printf("%s: Load JPG imgbuf fail, try egi_imgbuf_free()...\n",__func__);
                	     }
                	     else {
                        	 printf("%s: Finish loading JPG file '%s' as page wallpaper.\n",__func__,
												page->fpath);
                	     }
        		}
	        	else {
                       		printf("%s: Finish loading PNG file '%s' as page wallpaper.\n",__func__,
												page->fpath);
        		}
			/* Display it */
			if(imgbuf->imgbuf != NULL)  {
				/* no subcolor, no FB filo */
   				egi_imgbuf_windisplay2(imgbuf, &gv_fb_dev, 0, 0, 0, 0,
								imgbuf->width, imgbuf->height);
			}
			/* free imgbuf */
			EGI_PDEBUG(DBG_PAGE,"egi_imgbuf_free(imgbuf)...\n");
                        egi_imgbuf_free(imgbuf);
		}
	}
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
		ret +=ebox->activate(ebox);
		EGI_PDEBUG(DBG_PAGE,"activate page list item ebox: '%s' with ret=%d \n",ebox->tag,ret);
	}


	/* other misc jobs after listed ebox refreshed */
	if( page->page_refresh_misc != NULL )
		page->page_refresh_misc(page);


	return ret;
}


/*---------------------------------------------------------------------
1. Check need_refresh flag for page and refresh it if true.
2. Refresh each of page's child eboxes only if its needrefresh
   flag is true.
3. Refresh sequence:
   3.1 First refresh PAGE items, such as wallpaper, deco, misc..
   3.2 then refresh its child eboxes.

   3.3	 		<--- WARNINGS! --->
   3.3.1 Because PAGE ebox and each of its child eboxes has its
         own 'need_refresh' indicators, they will be checked
         and refreshed separately/independently.
         So if any child ebox has its image relied on PAGE image,
	 their 'need_refresh' indicators MUST be set together.

   3.3.2 To call egi_page_needrefresh() will activate 'need_refresh'
	 indicators for PAGE and its child eboxes.

   3.3.3 To call egi_ebox_forcerefresh() is seemed as dangerous,
         for it refreshs itself and ingnores its PAGE image coordination.

Return:
	1	need_refresh=false
	0	If any ebox has been refreshed.
	<0	fails
---------------------------------------------------------------------*/
int egi_page_refresh(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;
	int ret=1; /* set 1 first! */
	int xres __attribute__((__unused__))=gv_fb_dev.vinfo.xres;
	int yres __attribute__((__unused__))=gv_fb_dev.vinfo.yres;
	EGI_IMGBUF  *imgbuf;

	/* check data */
	if( page==NULL || page->ebox==NULL )
	{
		printf("egi_page_refresh(): input page or page->ebox is NULL!\n");
		return -1;
	}

	/* --------------- ***** 1. FOR 'PAGE' REFRESH, wallpaper etc. ***** ------------ */
	/* only if need_refresh */
	if(page->ebox->need_refresh)
	{
		//printf("egi_page_refresh(): refresh page '%s' wallpaper.\n",page->ebox->tag);
		EGI_PDEBUG(DBG_PAGE,"refresh page '%s' wallpaper.\n",page->ebox->tag);

		/* Also see egi_page_activate(EGI_PAGE *page) */
		/* load a picture or use prime color as wallpaper */
		if( page->ebox->frame_img !=NULL ) {
			EGI_PDEBUG(DBG_PAGE,"Apply page->ebox->frame_img for '%s' wallpaper.\n",
											 page->ebox->tag);
			imgbuf=page->ebox->frame_img;
			/* egi_image_setFrame() if necessary */

			/* no subcolor, no FB filo */
   			egi_imgbuf_windisplay2(imgbuf, &gv_fb_dev, 0, 0, 0, 0, imgbuf->width, imgbuf->height);
		}
		else if(page->fpath != NULL) {
			EGI_PDEBUG(DBG_PAGE,"Load '%s' for '%s' wallpaper.\n", page->fpath, page->ebox->tag);
			//show_jpg(page->fpath, &gv_fb_dev, SHOW_BLACK_NOTRANSP, 0, 0);
	        	imgbuf=egi_imgbuf_alloc();
		        /* First try to load as PNG file */
			printf("%s: Try to load as PNG file ...\n",__func__);
		        if(egi_imgbuf_loadpng(page->fpath, imgbuf) !=0) {
        		     // printf("%s: Load PNG imgbuf fail, try egi_imgbuf_loadjpg()...\n",__func__);
			     /* Then try to load as JPG file */
                	     if(egi_imgbuf_loadjpg(page->fpath, imgbuf) !=0) {
                        	 //printf("%s: Load JPG imgbuf fail, try egi_imgbuf_free()...\n",__func__);
                	     }
                	     else {
                        	 printf("%s: Finish loading JPG file '%s' as page wallpaper.\n",__func__,
												page->fpath);
                	     }
        		}
	        	else {
                       		printf("%s: Finish loading PNG file '%s' as page wallpaper.\n",__func__,
												page->fpath);
        		}

			/* Display it */
			if(imgbuf->imgbuf != NULL)  {
				/* no subcolor, no FB filo */
   				egi_imgbuf_windisplay2(imgbuf, &gv_fb_dev, 0, 0, 0, 0,
								imgbuf->width, imgbuf->height);
			}
			/* free imgbuf */
			EGI_PDEBUG(DBG_PAGE,"egi_imgbuf_free(imgbuf)...\n");
                        egi_imgbuf_free(imgbuf);
		}
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

		/* other misc. jobs after listed ebox refreshed */
		if( page->page_refresh_misc != NULL )
			page->page_refresh_misc(page);

		/* reset need_refresh */
		page->ebox->need_refresh=false;
		page->page_update=true; /* Synchronized with page->ebox->need_refresh currently
					 * It' an indicator just to inform its child eboxes (which
				 	 * are about to be refreshed in following codes.) that
				         * the PAGE back image (and other items) just refreshed,
					 * and their bkimgs may need to refresh too.
					 */

		/* set ret */
		ret=0;
	}

	/* --------------- ***** 2. FOR PAGE CHILD REFRESH ***** ------------*/
	/* check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_refresh(): page '%s' has an empty list_head .\n",page->ebox->tag);
		return -1*ret;
	}

	/* traverse the list and activate list eboxes, not safe */

	/* !!!! WRONG!!!!, if page->ebox->need_refresh is false, this token will mislead ebox
	 * to ignore fb_cpyfrom_buf()
	 * It MUST synchronize with page->ebox->need_refresh!!!
	 * see in above if(page->ebox->need_refresh){ ... }
	 */
        //page->page_update=true;

	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		ret *= ebox->refresh(ebox);
#if 1 /* debug only */
		if(ret==0)
		    EGI_PDEBUG(DBG_PAGE,"refresh page '%s' list item ebox: '%s' with ret=%d \
			 	 	ret=1 need_refresh=false \n", page->ebox->tag,ebox->tag,ret);
#endif
	}

	/*** COMMENTS:
	 *  It's too later to reset 'need_refresh' here, if any thread try to set 'need_refresh'
	 *  during PAGE child refreshing, it will be reset by following re_assignment!!!
	 *  Move it to end part of PAGE REFRESH codes, though it still poses race condition with other
	 *  threads trying to set 'need_refresh' at any time....
	 */
	/* reset need_refresh at last */
//	page->ebox->need_refresh=false;

	/* reset page_update, synchronized with page->ebox->need_refresh currently. */
	page->page_update=false;

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

	/*** 2. set page need_refresh flag
         *   Wait and let current refreshing finish.
         *   TODO: Race condition may still exist!
         */
	page->ebox->need_refresh=true;

	return 0;
}


/*-----------------------------------------------
Set all eboxes in a page to be need_refresh=true
return:
	0	OK
	<0	fails
-----------------------------------------------*/
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
        /***  Wait and let current refreshing finish.
         *    TODO: Race condition may still exist!
         */
	page->ebox->need_refresh=true;

	/* 4. traverse the list and set page need_refresh, not safe */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		/*  Wait and let current refreshing finish.
		 *  TODO: Race condition may still exist!
		 */
		ebox->need_refresh=true;
		EGI_PDEBUG(DBG_PAGE,"find child ebox: '%s' \n",ebox->tag);
	}

	return 0;
}


/*--------------------------------------------------------
pick a btn or slider pointer by its type and id number

@page	a page struct holding eboxes.
@type	type_btn OR type_slider
@id	NOTE: id of a EGI_DATA_BTN

return:
	pointer 	OK
	NULL		fails,or no match
--------------------------------------------------------*/
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


/*--------------------------------------------------------------
pick a ebox pointer by its type and id number

@page	a page struct holding eboxes.
@type	type of ebox
@id	ID of the ebox.  ebox->id, MUST >0.

return:
	pointer 	OK
	NULL		fails, or no match
-----------------------------------------------------------------*/
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


/*----------------------------------
Default page routine job

return:
	loop or >=0  	OK
	<0		fails
----------------------------------*/
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
	EGI_PDEBUG(DBG_PAGE,"start to load PAGE [%s]'s runner...\n",page->ebox->tag);
	for(i=0;i<EGI_PAGE_MAXTHREADS;i++)
	{
		if( page->runner[i] !=0 )
		{
			/* launch Runners in order */
			EGI_PDEBUG(DBG_PAGE,"Start creating runner: pthreadID[%d]=%u ...\n",
								i,(unsigned int)page->threadID[i] );
			if( pthread_create( &page->threadID[i],NULL,(void *)page->runner[i],(void *)page)==0)
			{
				page->thread_running[i]=true;
				printf("%s: Create pthreadID[%d]=%u successfully. \n", __func__,
								i, (unsigned int)page->threadID[i] );
			}
			else {
			      EGI_PLOG(LOGLV_ERROR,"%s: Fail to create pthread for runner[%d] of page[%s] \n", __func__,
					i, page->ebox->tag );
			      /* carry on anyway..... */
			}
		}
	}
	/* Initiate thread mutex locks, NOTE: also for egi_pagehome_routine() */
	EGI_PDEBUG(DBG_PAGE,"Start to initiate thread mutex lock for page runners.\n");
	if(pthread_mutex_init(&page->runner_mutex,NULL) !=0 ) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to call pthread_mutex_init()!\n", __func__ );
		return -1;
	}
	if(pthread_cond_init(&page->runner_cond,NULL) !=0 ) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to call pthread_cond_init()!\n", __func__ );
		return -1;
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
			 *	   handler expected, this hitbtn will disappear!!!
			 *	   We need to confirm 'last_holdbtn->need_refresh==false' here to rule
			 *	   out the situation, and make sure when egi_page_refresh(page) is called
			 *	   all elements are to refreshed in order.
			 *	2. Re_drawing an unmovable btn will change brightness of the icon, if
			 *	   the icon has opaque value.
			 */

			if( last_holdbtn != NULL && last_holdbtn != hitbtn
						 && last_holdbtn->need_refresh==false )
			{
				EGI_PDEBUG(DBG_PAGE,"last_holdbtn '%s' losed focus, refresh it...\n",
										last_holdbtn->tag);
				if( ((EGI_DATA_BTN *)(last_holdbtn->egi_data))->opaque <= 0 )
					/* To avoid refresh btn with opaque value, which shall be refreshed
					  with whole PAGE instead of just one btn!!! */
				{
			           printf("'%s' is %s \n",last_holdbtn->tag, last_holdbtn->movable ? "movable":"unmovable");
				   egi_ebox_forcerefresh(last_holdbtn); /* refreshi it then */
				}

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
			 /* ---- call touch_effect() ----- */
			 /* 1. 'pressing', 'releaseing' 'pressed_hold' all will trigger touch_effec(),
			  *     with differenct reactions defined.
			  * 2. Signals that trigger the touch_effec() may be by_passed by the reaction()
			  *    , so you should not expect that touch_effect() and reaction() will exectued
			  *    one after the other. in most case,when you slide on the btn, touch_effect()
			  *    will be triggered serval times, most possiblely by 'pressed_hold'.
			  * 3. When the btn icon is unmovale and has opaque(alpha) value, refreshing
			  *    it without refreshing the PAGE bk image will just addup/deepen color
			  *	value to FB.
			  */
		                    if( hitbtn->need_refresh==false   /* In case SIGCONT triggered */
                                       // && last_status==pressing      /* trigger once only after pressing */
					&&( ((EGI_DATA_BTN *)hitbtn->egi_data)->touch_effect != NULL ) ) {
					  EGI_PDEBUG(DBG_PAGE,"call '%s' touch_effect() \n", hitbtn->tag);
					((EGI_DATA_BTN *)hitbtn->egi_data)->touch_effect(hitbtn,&touch_data);//last_status);
				    }

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


/*--------------------------------------
Home page routine job

return:
	loop or >=0  	OK
	<0		fails
--------------------------------------*/
int egi_homepage_routine(EGI_PAGE *page)
{
	int i;
	int ret;
	uint16_t sx,sy;

	/* Indicating the nearest 'releasing' or 'pressing' status.
	 * Default set as 'releasing', as corresponds to last_status default 'released_hold'
	 */
	enum egi_touch_status flip_status=releasing;
	enum egi_touch_status last_status=released_hold; /* means the lasted status! */

	enum egi_touch_status check_status;
	EGI_TOUCH_DATA touch_data;
	int tdx,tdy;
	bool slide_touch=false;
	bool edge_restored=false;	/* Indicating that a missing edge change statu is restored */

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
			/* launch Runners in order */
			EGI_PDEBUG(DBG_PAGE,"Start creating runner: pthreadID[%d]=%u ...\n",
							__func__,i,(unsigned int)page->threadID[i] );
			if( pthread_create( &page->threadID[i],NULL,(void *)page->runner[i],(void *)page)==0)
			{
				page->thread_running[i]=true;
				printf("%s: Create pthreadID[%d]=%u successfully. \n", __func__,
								i, (unsigned int)page->threadID[i] );
			}
			else {
			      EGI_PLOG(LOGLV_ERROR,"%s: Fail to create pthread for runner[%d] of page[%s] \n", __func__,
					i, page->ebox->tag );
			      /* carry on anyway..... */
			}
		}
	}
	/* Initiate thread mutex locks, NOTE: also for egi_page_routine() */
	EGI_PDEBUG(DBG_PAGE,"Start to initiate thread mutex lock for page runners.\n");
	if(pthread_mutex_init(&page->runner_mutex,NULL) !=0 ) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to call pthread_mutex_init()!\n", __func__ );
		return -1;
	}
	if(pthread_cond_init(&page->runner_cond,NULL) !=0 ) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to call pthread_cond_init()!\n", __func__ );
		return -1;
	}


 	 /* ----------------    Touch Event Handling   ----------------  */

	/* Try to discard first obsolete data, just to inform egi_touch_loopread() to start loop_read */
	egi_touch_getdata(&touch_data);

	while(1)
	{
		/* 0. backup old touch status */
		if( touch_data.status==releasing ) {
			//printf(" --- 'releasing' --- \n");
			flip_status=releasing;
		}
		else if ( touch_data.status==pressing ) {
			//printf(" --- 'pressing' --- \n");
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

		/* print read_in touch status */
		if(last_status != released_hold)
			printf("routine: --- %s ---\n",egi_str_touch_status(last_status));

		/*  1.1     -----  restore missing status  ------
		 *   The 'pressing' and 'releasing' signal may be missed due to current egi_touch.c
		 *  algrithm, especially when you add pressing force slowly at the screen.
		 *  we need to restore 'pressing' status and pass down the status.
		 *  Or say 'edge restoring' for some edge_trigged actions.
		 */

		edge_restored=false;  /* reset indicator */
		if( flip_status != pressing && last_status==pressed_hold ) {
		    /* restore 'pressing' status then
		     * NOTE
		     *  1. When 'pressing' is detected by egi_touch_loopread(), dx,dy will be reset to 0.
		     *     the restored status will ingnored this.
		     */
		    printf(" --- restore 'pressing' --- \n");
		    last_status=pressing;
		    touch_data.status=pressing;
		    flip_status=pressing; /* update the nearset flip status */
		    edge_restored=true;
		}
		else if(flip_status != releasing && last_status==released_hold) {
		    /* restore 'releasing' status then*/
		    printf(" --- restore 'releasing' --- \n");
		    last_status=releasing;
		    touch_data.status=releasing;
		    flip_status=releasing;
		    edge_restored=true;
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
		        if( last_status==pressing ) {  //|| flip_status==pressing )  {
				/* peek next touch dx, but do not read out */
				tm_delayms(50);
				//tdx=egi_touch_peekdx();
				egi_touch_peekdxdy(&tdx,&tdy);
				/* check peek tdx, and also peek if 'releasing' after 'pressed_hold' */
				/* Note:
				 *  1. In heavy load conditon, tdx/tdy will fluctuate greatly even sliding
				 *     speed keeps constantly.
			         *  2. When tdx/tdy is too small from egi_touch_peekdxdy(), slide_touch status
				 *     will NOT be detected with following algrithm!!!
				 */
				printf("pressing check slide: dx=%d dy=%d \n",tdx, tdy);
				if( tdx>3 || tdx<-3 || tdy>3 || tdy<-3 ) {  //|| egi_touch_peekstatus()==releasing) {
					printf("--- start sliding ---\n");
					slide_touch=true;
				}
				/* else, pass down pressing status to slide_handler()... */
//				else {
//					slide_touch=false;
//				}
			}

			/* between two press_hold status, if dxdy is detected, also trigger slide_touch */
			else if( slide_touch != true && last_status==pressed_hold ) {
				/* Don't wait, peek imediately */
				egi_touch_peekdxdy(&tdx,&tdy);

				printf("pressed_hold check slide: dx=%d dy=%d \n",tdx, tdy);
				if( tdx>3 || tdx<-3 || tdy>3 || tdy<-3 ) {  //|| egi_touch_peekstatus()==releasing) {
					printf("--- hold sliding ---\n");
					slide_touch=true;
				}
			}


			/* 2.2 sliding handling func */
			if(slide_touch ) //&& ( last_status==pressed_hold || last_status==pressing || last_status==releasing) )
			{
				/* OR to ignore 'releasing' to let button icons stop at current position */
				if(last_status==releasing)
					printf(" --- sliding release! --- \n");

				else if(last_status==pressing)
					printf(" --- sliding press start! --- \n");

				if(page->slide_handler != NULL) {
					page->slide_handler(page, &touch_data);
					egi_page_refresh(page); /* refresh page for other eboxes!!!  */
				}

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
				/* 2.5.1 display touch effect for button */
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

				/* 2.5.2 trigger reaction func */
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
			if(egi_page_refresh(page)!=0) {  /* refresh ebox always */
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
