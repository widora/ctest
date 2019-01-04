#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "egi.h"
#include "egi_page.h"
#include "egi_debug.h"
#include "egi_color.h"
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
	EGI_PAGE *page;

	/* 2. malloc page */
	page=malloc(sizeof(struct egi_page));
	if(page == NULL)
	{
		printf("egi_page_init(): fail to malloc page.\n");
		return NULL;
	}

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

	/* 5. init list */
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
			PDEBUG("egi_page_free(): ebox '%s' is unlisted from page '%s' and freed.\n"
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
	PDEBUG("egi_page_addlist(): ebox '%s' is added to page '%s' \n",ebox->tag, page->ebox->tag); 

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
		printf("egi_page_travlist(): input EGI_PAGE *page is NULL!\n");
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
		PDEBUG("egi_page_travlist(): find child --- ebox: '%s' --- \n",ebox->tag);
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

	/* check data */
	if(page==NULL)
	{
		printf("egi_page_activate(): input EGI_PAGE *page is NULL!\n");
		return -1;
	}
	/* check list */
	if(list_empty(&page->list_head))
	{
		printf("egi_page_activate(): page '%s' has an empty list_head .\n",page->ebox->tag);
		return -2;
	}

	/* traverse the list and activate list eboxes, not safe */
	list_for_each(tnode, &page->list_head)
	{
		ebox=list_entry(tnode, EGI_EBOX, node);
		ret=ebox->activate(ebox);
		PDEBUG("egi_page_activate(): activate page list item ebox: '%s' with ret=%d \n",ebox->tag,ret);
	}


	return 0;
}


/*--------------------------------------------------
refresh a page and its eboxes in its list.

return:
	0	OK
	<0	fails
---------------------------------------------------*/
int egi_page_refresh(EGI_PAGE *page)
{
	struct list_head *tnode;
	EGI_EBOX *ebox;
	int ret;

	/* check data */
	if(page==NULL || page->ebox==NULL )
	{
		printf("egi_page_refresh(): input EGI_PAGE * page or page->ebox is NULL!\n");
		return -1;
	}

	/* load a picture or use prime color as wallpaper*/
	if(page->fpath != NULL)
		show_jpg(page->fpath, &gv_fb_dev, SHOW_BLACK_NOTRANSP, 0, 0);
	else /* use ebox prime color to clear(fill) screen */
	{
		if(page->ebox->prmcolor >= 0)
			 clear_screen(&gv_fb_dev, egi_colorgray_random(medium));
	}


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
		PDEBUG("egi_page_refresh(): refresh page list item ebox: '%s' with ret=%d \n",ebox->tag,ret);
	}


	return 0;
}
