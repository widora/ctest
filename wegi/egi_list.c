/*----------------------- egi_list.c -------------------------------
         ---- A List item is a txt_ebox contains an icon! -----

1. A list consists of items.
2. Each item has an icon and several txt lines, all those txt lines is
   of the same txt ebox.
3. A txt ebox's height/width is also height/width of the list obj,
   that means a list item is a txt_ebox include an icon.
3. (NOPE!)A list has a full width of the screen, that is fd.vinfo.xres in pixels.
4. If an item txt line has NULL content, then it will be treated as the end
   line of that item txt_ebox. That means the number of txt line displayed 
   for each item is adjustable.

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

/*-----------   EGI_PATTERN :  LIST   -----------------
Create an list obj.
standard tag "list"


return:
	txt ebox pointer 	OK
	NULL			fai.
-----------------------------------------------------*/
EGI_LIST *egi_list_new (
	int x0, int y0, /* left top point */
        int inum,  /* item number of a list */
        int width, /* h/w for each list item */
        int height,
	int frame, /* -1 no frame for ebox, 0 simple .. */
	int llen, /* in byte, length for each txt line */
        struct symbol_page *font, /* txt font */
        int txtoffx, /* offset of txt from the ebox */
        int txtoffy,
        int iconoffx,
        int iconoffy
)
{
	int i;

	/* 1. malloc list structure */
	EGI_LIST *list=malloc(sizeof(EGI_LIST));
	if(list == NULL)
	{
		printf("egi_list_new(): fail to malloc a EGI_LIST *list.\n");
		return NULL;
	}
	memset(list,0,sizeof(EGI_LIST));

	/* 2. init **txtbox  */
	EGI_EBOX **txtboxes=malloc(inum*sizeof(EGI_EBOX *));
	if( txtboxes == NULL )
	{
		printf("egi_list_new(): fail to malloc a EGI_EBOX **txtbox.\n");
		return NULL;
	}
	memset(txtboxes,0,inum*sizeof(EGI_EBOX *));

	/* 3. init **icon */
	struct symbol_page **icons=malloc(inum*sizeof(struct symbol_page));
	if( icons == NULL )
	{
		printf("egi_list_new(): fail to malloc a symbol_page **icon.\n");
		return NULL;
	}
	memset(icons,0,inum*sizeof(struct symbol_page));

	/* 4. init bkcolor */
	uint16_t *bkcolor=malloc(inum*sizeof(uint16_t));
	/* init bkcolor with default bkcolor */
	memset(bkcolor,0,inum*sizeof(uint16_t));

	/* 5. assign  elements */
	list->x0=x0;
	list->y0=y0;
	list->inum=inum;
	list->width=width;
	list->height=height;
	list->llen=llen;
	list->font=font;
	list->txtoffx=txtoffx;
	list->txtoffy=txtoffy;
	list->iconoffx=iconoffx;
	list->iconoffy=iconoffy;

	list->icons=icons;
	list->bkcolor=bkcolor;

	/* 6. create txt_type eboxes  */
	for(i=0;i<inum;i++)
	{
		/* 6.1  create a data_txt */
		egi_pdebug(DBG_LIST,"egi_list_new(): start to egi_txtdata_new()...\n");
		EGI_DATA_TXT *data_txt=egi_txtdata_new(
				list->txtoffx,list->txtoffy, /* offset X,Y */
      	 		 	LIST_ITEM_MAXLINES, /*int nl, lines  */
       	 			list->llen, /*int llen, chars per line, however also limited by width */
        			list->font, /*struct symbol_page *font */
        			WEGI_COLOR_BLACK /* int16_t color */
			     );
		if(data_txt == NULL) /* if fail retry...*/
		{
			printf("egi_list_new(): data_txt=egi_txtdata_new()=NULL,  retry... \n");
			i--;
			continue;
		}

		/* 6.2 creates all those txt eboxes */
	        egi_pdebug(DBG_LIST,"egi_list_new(): start egi_list_new().....\n");
       		list->txt_boxes[i] = egi_txtbox_new(
                			"----", /* tag, or put later */
		                	data_txt, /* EGI_DATA_TXT pointer */
               				false, /* bool movable */
			                x0, y0+i*height, /* int x0, int y0 */
                			width, height, /* int width;  int height,which also related with symheight and offy */
                			frame, /* int frame, 0=simple frmae, -1=no frame */
                			bkcolor[i] /*int prmcolor*/
   		    		     );
		if(list->txt_boxes[i]==NULL)
		{
			printf("egi_list_new(): txt_eboxes[%d]=NULL,  retry... \n",i);
			free(data_txt);
			i--;
			continue;
		}

		/* 6.3 set tag */
		sprintf(list->txt_boxes[i]->tag,"item_%d",i);

	}

	return list;
}


