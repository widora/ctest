/*----------------------- egi_list.c -------------------------------

TODO:
1. Color set for each line of txt to be fixed later in egi_txt.c.

Midas Zhou
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "egi_color.h"
#include "egi.h"
#include "egi_txt.h"
#include "egi_objtxt.h"
#include "egi_timer.h"
#include "egi_debug.h"
//#include "egi_timer.h"
#include "egi_symbol.h"
#include "egi_list.h"



/*-------------------------------------
      list ebox self_defined methods
--------------------------------------*/
static EGI_METHOD listbox_method=
{
        .activate=egi_listbox_activate,
        .refresh=egi_listbox_refresh,
        .decorate=NULL,
        .sleep=NULL,
        .free=NULL, //see egi_ebox_free() in egi.c
};



/*-------------------------------------------------
Create an list ebox.
standard tag "list"

default ebox prmcolor: 	WEGI_COLOR_GRAY
default font color: 	WEGI_COLOR_BLACK

return:
	txt ebox pointer 	OK
	NULL			fai.
-----------------------------------------------------*/
EGI_EBOX *egi_listbox_new (
	int x0, int y0, /* left top point */
        int inum,  	/* item number of a list */
        int width, 	/* H/W for each list item ebox, W/H of the hosting ebox depends on it */
        int height,
	int frame, 	/* -1 no frame for ebox, 0 simple .. */
	int nl,		/* number of txt lines for each txt ebox */
	int llen,  	/* in byte, length for each txt line */
        struct symbol_page *font, /* txt font */
        int txtoffx,	 /* offset of txt from the ebox, all the same */
        int txtoffy,
        int iconoffx, 	/* offset of icon from the ebox, all the same */
        int iconoffy
)
{
	int i;


        EGI_EBOX *ebox;

        /* 1. create a new common ebox with type_list */
        egi_pdebug(DBG_LIST,"egi_listbox_new(): start to egi_ebox_new(type_list)...\n");
        ebox=egi_ebox_new(type_list);// egi_data NOT allocated in egi_ebox_new()!!! , (void *)egi_data);
        if(ebox==NULL)
        {
                printf("egi_listbox_new(): fail to execute egi_ebox_new(type_list). \n");
                return NULL;
        }

        /* 2. default methods have been assigned in egi_ebox_new() */

        /* 3. list ebox object method */
        egi_pdebug(DBG_LIST,"egi_listbox_new(): assign defined mehtod ebox->method=methd...\n");
        ebox->method=listbox_method;

        /* 4. fill in elements for main ebox */
        egi_ebox_settag(ebox,"list");

        ebox->x0=x0;
    	ebox->y0=y0;
        ebox->movable=false; /* fixed type */
        ebox->width=width;
	ebox->height=height*inum;
        ebox->frame=-1; /* no frame, let item ebox prevail */
	ebox->prmcolor=-1; /* no prmcolor, let item ebox prevail */

	/* to assign ebox->egi_data latter ......*/


	/* 5 malloc data_txt for list item eboxes,
	   Temprary this function only, to be released   */
	EGI_DATA_TXT **data_txt=malloc(inum*sizeof(EGI_DATA_TXT *));
	if( data_txt == NULL )
	{
		printf("egi_listbox_new(): fail to malloc  **data_txt.\n");
		return NULL;
	}
	memset(data_txt,0,inum*sizeof(EGI_DATA_TXT *));
printf("egi_listbox_new(): malloc **data_txt end...\n");

	/* 6. malloc txt_boxes for list item eboxes */
	EGI_EBOX **txt_boxes=malloc(inum*sizeof(EGI_EBOX *));
	if( txt_boxes == NULL )
	{
		printf("egi_listbox_new(): fail to malloc a EGI_EBOX **txt_box.\n");
		return NULL;
	}
	memset(txt_boxes,0,inum*sizeof(EGI_EBOX *));
printf("egi_listbox_new(): malloc txtboxes end...\n");

	/* 7. init **icon */
	struct symbol_page **icons=malloc(inum*sizeof(struct symbol_page *));
	if( icons == NULL )
	{
		printf("egi_listbox_new(): fail to malloc a symbol_page **icon.\n");
		return NULL;
	}
	memset(icons,0,inum*sizeof(struct symbol_page *));
printf("egi_listbox_new(): malloc icons end...\n");

	/* 8. init *icon_code */
	int *icon_code =malloc(inum*sizeof(int));
	if( icon_code == NULL )
	{
		printf("egi_listbox_new(): fail to malloc *icon_code.\n");
		return NULL;
	}
	memset(icon_code,0,inum*sizeof(int));
printf("egi_listbox_new(): malloc icon_code end...\n");


	/* 9. init bkcolor
	   Temprary this function only, to be released   */
	uint16_t *prmcolor=malloc(inum*sizeof(uint16_t));
	/* init prmcolor with default bkcolor */
	memset(prmcolor,0,inum*sizeof(uint16_t));
	for(i=0;i<inum;i++)
		prmcolor[i]=LIST_DEFAULT_BKCOLOR;
printf("egi_listbox_new(): malloc prmcolor end...\n");


	/* 10. init egi_data_list */
	EGI_DATA_LIST *data_list=malloc(sizeof(EGI_DATA_LIST));
	if( data_list == NULL )
	{
		printf("egi_listbox_new(): fail to malloc *data_list.\n");
		return NULL;
	}
	memset(data_list,0,sizeof(EGI_DATA_LIST));
printf("egi_listbox_new(): malloc data_list end...\n");

	/* 11. assign to data_list */
	data_list->inum=inum;
	data_list->txt_boxes=txt_boxes;
	data_list->icons=icons;
	data_list->icon_code=icon_code;
	data_list->iconoffx=iconoffx;
	data_list->iconoffy=iconoffy;



	/* 12. create txt_type eboxes for item txt_ebox->egi_data  */
	for(i=0;i<inum;i++)
	{
		/* 12.1  create a data_txt */
		egi_pdebug(DBG_LIST,"egi_listbox_new(): start to data_txt[%d]=egi_txtdata_new()...\n",i);
		data_txt[i]=egi_txtdata_new(
				txtoffx,txtoffy, /* offset X,Y */
      	 		 	nl, /*int nl, lines  */
       	 			llen, /*int llen, chars per line, however also limited by width */
        			font, /*struct symbol_page *font */
        			WEGI_COLOR_BLACK /* int16_t color */
			     );
		if(data_txt[i] == NULL) /* if fail retry...*/
		{
			printf("egi_listbox_new(): data_txt[%d]=egi_txtdata_new()=NULL,  retry... \n",i);
			i--;
			continue;
		}
printf("egi_listbox_new(): data_txt[%d]=egi_txtdata_new() end...\n",i);


		/* 12.2 creates all those txt eboxes */
	        egi_pdebug(DBG_LIST,"egi_listbox_new(): start egi_txtbox_new().....\n");
       		(data_list->txt_boxes)[i] = egi_txtbox_new(
                			"----", /* tag, or put later */
		                	data_txt[i], /* EGI_DATA_TXT pointer */
               				true, /* bool movable, for txt changing/transparent... */
			                x0, y0+i*height, /* int x0, int y0 */
                			width, height, /* int width;  int height,which also related with symheight and offy */
                			frame, /* int frame, 0=simple frmae, -1=no frame */
                			prmcolor[i] /*int prmcolor*/
  		    		  );

		if( (data_list->txt_boxes)[i] == NULL )
		{
			printf("egi_listbox_new(): data_list->txt_eboxes[%d]=NULL,  retry... \n",i);
			free(data_txt[i]);
			i--;
			continue;
		}
printf("egi_listbox_new(): data_list->txt_boxes[%d]=egi_txtbox_new() end...\n",i);


		/* 12.3 set-tag */
		sprintf(data_list->txt_boxes[i]->tag,"item_%d",i);
printf("egi_list_new(): data_list->txt_boxes[%d] set_tag  end...\n",i);

	}

	/* 13. assign list_data to type_list ebox */
	ebox->egi_data=(void *)data_list;

	/* 14. free data_txt */
	free(data_txt);

	/* 15. free prmcolor */
	free(prmcolor);

	return ebox;
}


/*----------------------------------------------
release EGI_DATA_LIST
----------------------------------------------*/
void egi_free_data_list(EGI_DATA_LIST *data_list)
{
	if(data_list==NULL)
		return;

	int i;
 	int inum=data_list->inum;

	/* free txt_boxes */
	for(i=0;i<inum;i++)
	{
		if( data_list->txt_boxes[i] != NULL )
			(data_list->txt_boxes[i])->free(data_list->txt_boxes[i]);
	}
	/* free **txt_boxes */
	free(data_list->txt_boxes);

	/* free icons and icon_code */
	if(data_list->icons != NULL)
		free(data_list->icons);
	if(data_list->icon_code != NULL)
		free(data_list->icon_code);

	/* free data_list */
	free(data_list);
}


/*----------------------------------------------
to activate a list:
1. activate each txt ebox in the list.
2. draw icon for drawing item.

Retrun:
	0	OK
	<0	fail
----------------------------------------------*/
int egi_listbox_activate(EGI_EBOX *ebox)
{
	int i;
	int inum;
	EGI_DATA_LIST *data_list;

	/* 1. check list  */
	if(ebox==NULL)
	{
		printf("egi_listbox_activate(): input list ebox is NULL! fail to activate.");
		return -1;
	}

	/* 2. check ebox type */
	if(ebox->type != type_list)
	{
		printf("egi_listbox_activate(): input ebox is not list type! fail to activate.");
		return -3;
	}

	/* 3. check egi_data */
	if(ebox->egi_data==NULL)
	{
		printf("egi_listbox_activate(): input ebox->egi_data is NULL! fail to activate.");
		return -2;
	}

	data_list=(EGI_DATA_LIST *)ebox->egi_data;
	inum=data_list->inum;

	/* 4. activate each txt_ebox in the data_list */
	for(i=0; i<inum; i++)
	{

		/* 4.1 activate txt ebox */
		if( egi_txtbox_activate(data_list->txt_boxes[i]) < 0)
		{
			printf("egi_listbox_activate(): fail to activate data_list->txt_boxes[%d].\n",i);
			return -4;
		}

		/* 4.2 FB write icon */
		if(data_list->icons[i])
		{
			symbol_writeFB(&gv_fb_dev, data_list->icons[i], SYM_NOSUB_COLOR, data_list->icons[i]->bkcolor,
 					data_list->txt_boxes[i]->x0, data_list->txt_boxes[i]->y0,
					data_list->icon_code[i], 0 ); /* opaque 0 */
		}
	}

	return 0;
}


/*---------------------------------------------------------
to refresh a list ebox:
0. TODO: update/push data to fill list
1. refresh each txt ebox in the data_list.
2. update and draw icon for each item.

Retrun:
	0	OK
	<0	fail
----------------------------------------------------------*/
int egi_listbox_refresh(EGI_EBOX *ebox)
{
	int  i;
	int inum;
	EGI_DATA_LIST *data_list;

        /* 1. check list ebox  */
        if(ebox==NULL)
        {
                printf("egi_listbox_refresh(): input list ebox is NULL! fail to refresh.");
                return -1;
        }

        /* 2. check egi_data */
        if(ebox->egi_data==NULL)
        {
                printf("egi_listbox_refresh(): input ebox->egi_data is NULL! fail to activate.");
                return -2;
        }

        data_list=(EGI_DATA_LIST *)ebox->egi_data;
        inum=data_list->inum;

        /* 2. check data_list->txt_boxes */
	if(data_list == NULL )
	{
                printf("egi_listbox_refresh(): input ebox->data_list is NULL! fail to refresh.");
                return -3;
	}
        if(data_list->txt_boxes==NULL)
        {
                printf("egi_listbox_refresh(): input data_list->txt_boxes is NULL! fail to refresh.");
                return -3;
        }

	/* 3. update and push data to the list */
	//TODO

	/* 4. refresh each txt eboxe */
	for(i=0; i<inum; i++)
	{

		/* 4.1 refresh txt */
		if( egi_txtbox_refresh(data_list->txt_boxes[i]) < 0 )
		{
			printf("egi_listbox_refresh(): fail to refresh data_list->txt_boxes[%d].\n",i);
			return -4;
		}
		/* 4.2 FB write icon */
		if(data_list->icons[i])
		{
			symbol_writeFB(&gv_fb_dev, data_list->icons[i], SYM_NOSUB_COLOR, data_list->icons[i]->bkcolor,
 					data_list->txt_boxes[i]->x0, data_list->txt_boxes[i]->y0,
					data_list->icon_code[i], 0 );
		}

	}


	return 0;
}


/*-------------------------------------------------------------------------
to update prmcolor and data for items in a list type ebox:

EGI_EBOX: 	list ebox to be updated
n:		number of the list's item considered
txt:		txt to push to:
		data_list->txt_boxes[i]->egi_data->txt
prmcolor:	prime color for item ebox

Retrun:
	0	OK
	<0	fail
-------------------------------------------------------------------------*/
int egi_listbox_updateitem(EGI_EBOX *ebox, int n, uint16_t prmcolor, char **txt)
{
	int i;
	int inum;
	EGI_DATA_LIST *data_list;


	/* 0. check data */
	if(txt==NULL || txt[0]==NULL)
	{
                printf("egi_listbox_updateitem(): input txt is NULL! fail to push data to list item.");
                return -1;
	}

        /* 1. check list ebox */
        if(ebox==NULL)
        {
                printf("egi_listbox_updateitem(): input list is NULL! fail to push data to list item.");
                return -2;
        }

	/* 2. check ebox type */
	if(ebox->type != type_list)
	{
		printf("egi_listbox_updateitem(): input ebox is not list type! fail to update.");
		return -3;
	}

        data_list=(EGI_DATA_LIST *)ebox->egi_data;
        inum=data_list->inum;

        /* 3. check data_list and data_list->txt_boxes */
	if(data_list == NULL )
	{
                printf("egi_listbox_updateitem(): input ebox->data_list is NULL! fail to update.");
                return -3;
	}
        if(data_list->txt_boxes==NULL)
        {
                printf("egi_listbox_updateitem(): input data_list->txt_boxes is NULL! fail to update.");
                return -3;
        }


	/* 4. check number */
	if(n > data_list->inum)
        {
                printf("egi_listbox_updateitem(): n is greater than list->inum.");
                return -4;
        }

        /* 5. check list->txt_boxes */
        if(data_list->txt_boxes==NULL)
        {
                printf("egi_listbox_updateitem(): input data_list->txt_boxes is NULL! fail to update item.");
                return -5;
        }

	/* 6. check data txt */
	EGI_DATA_TXT *data_txt=(EGI_DATA_TXT *)(data_list->txt_boxes[n]->egi_data);
	if(data_txt == NULL)
	{
                printf("egi_listbox_updateitem(): input data_list->txt_boxes->data_txt is NULL! fail to update item.");
                return -6;
	}

	/* 7. update prmcolor of the item ebox */
	data_list->txt_boxes[n]->prmcolor=prmcolor;

	/* 8. push txt into data_txt->txt */
	for(i=0; i<(data_txt->nl); i++)
	{
		strncpy(data_txt->txt[i], txt[i], data_txt->llen-1);/* 1 byte for end token */
		egi_pdebug(DBG_LIST,"egi_listbox_updateitem(): txt_boxes[%d], txt[%d]='%s'\n",
										n,i,data_txt->txt[i]);
	}

	return 0;

}
