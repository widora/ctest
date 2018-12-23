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
	if(ebox->bkimg != NULL)
	{
		printf("egi_alloc_bkimg(): ebox->bkimg is not NULL, fail to malloc!\n");
		return NULL;
	}

	/* 3. alloc memory */
	ebox->bkimg=malloc(height*width*sizeof(uint16_t));
	if(ebox->bkimg == NULL)
	{
		printf("egi_alloc_bkimg(): fail to malloc for ebox->bkimg!\n");
		return NULL;
	}

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



/*--------------------------------------------------------------------------------
initialize struct egi_data_txt according

offx,offy:			offset from prime ebox
int nl:   			number of txt lines
int llen:  			in byte, data length for each line, 
			!!!!! -	llen deciding howmany symbols it may hold.
struct symbol_page *font:	txt font
uint16_t color:     		txt color
char **txt:       		multiline txt data

Return:
        non-null        OK
        NULL            fails
---------------------------------------------------------------------------------*/
struct egi_data_txt *egi_init_data_txt(struct egi_data_txt *data_txt,
			int offx, int offy, int nl, int llen, struct symbol_page *font, uint16_t color)
{
	int i,j;


	/* check data first */
	if(data_txt == NULL)
	{
		printf("egi_init_data_txt(): data_txt is NULL!\n");
		return NULL;
	}
	if(data_txt->txt !=NULL) /* if data_txt defined statically, txt may NOT be NULL !!! */
	{
		printf("egi_init_data_txt(): ---WARNING!!!--- data_txt->txt is NOT NULL!\n");
		return NULL;
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

	/* --- assign var ---- */
	data_txt->nl=nl;
	data_txt->llen=llen;
	data_txt->font=font;
	data_txt->color=color;
	data_txt->offx=offx;
	data_txt->offy=offy;

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
		memset(data_txt->txt[i],0,llen*sizeof(char));
	}
	return data_txt;
}


/*-----------------------------------------------------------------------
activate a txt ebox:
	0. adjust ebox height and width according to its font line set
 	1. store back image of txtbox frame range.
	2. refresh the ebox.
	3. change status token to active,

TODO:
	if ebox_size is re-sizable dynamically, bkimg mem size MUST adjusted.

Return:
	0	OK
	<0	fails!

------------------------------------------------------------------------*/
int egi_txtbox_activate(struct egi_element_box *ebox)
{
//	int i,j;
	int x0=ebox->x0;
	int y0=ebox->y0;
	int height=ebox->height;
	int width=ebox->width;
	struct egi_data_txt *data_txt=(struct egi_data_txt *)(ebox->egi_data);
	int nl=data_txt->nl;
//	int llen=data_txt->llen;
//	int offx=data_txt->offx;
	int offy=data_txt->offy;
//	char **txt=data_txt->txt;
	int font_height=data_txt->font->symheight;

	/* 1. confirm ebox type */
        if(ebox->type != type_txt)
        {
                printf("egi_txtbox_activate(): '%s' is not a txt type ebox!\n",ebox->tag);
                return -1;
        }

	/* 2. wake up a sleeping ebox */
	if(ebox->status==status_sleep)
	{
		ebox->status=status_active;
		if(egi_txtbox_refresh(ebox)!=0)
		{
			printf("egi_txtbox_activate():fail to wake up sleeping ebox '%s'!\n",ebox->tag);
			return -1;
		}

		printf("egi_txtbox_activate(): wake up a sleeping '%s' ebox.\n",ebox->tag);
		return 0;
	}

        /* 3. check ebox height and font lines, then adjust the height */
        height= (font_height*nl+offy)>height ? (font_height*nl+offy) : height;
        ebox->height=height;

	//TODO: malloc more mem in case ebox size is enlarged later????? //
	/* 4. malloc exbo->bkimg for bk image storing */   
   if(ebox->movable) /* only if ebox is movale */
   {
		if(egi_alloc_bkimg(ebox, width, height)==NULL)
        	{
               	 	printf("egi_txtbox_activate(): fail to egi_alloc_bkimg() for '%s' ebox!\n",ebox->tag);
                	return -2;
        	}

#if 0
	/* 4. malloc exbo->bkimg for bk image storing */
	if(ebox->bkimg != NULL)
	{
		printf("egi_txtbox_activat(): '%s' ebox->bkimg is not NULL, fail to malloc!\n",ebox->tag);
		return -2;
	}


	ebox->bkimg=malloc(height*width*sizeof(uint16_t));
	if(ebox->bkimg == NULL)
	{
		printf("egi_txtbox_activat(): fail to malloc for '%s' ebox->bkimg!\n",ebox->tag);
		return -4;
	}
#endif

	/* 5. store bk image which will be restored when this ebox position/size changes */
	/* define bkimg box */
	ebox->bkbox.startxy.x=x0;
	ebox->bkbox.startxy.y=y0;
	ebox->bkbox.endxy.x=x0+width-1;
	ebox->bkbox.endxy.y=y0+height-1;
#if 0 /* DEBUG */
	printf("txt activate... fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x, ebox->bkbox.endxy.y);
#endif
	if(fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
				ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0 )
		return -5;
  } /* ebox->movable codes end */

	/* 6. change its status, if not, you can not refresh.  */
	ebox->status=status_active;


	/* 7. reset offset for txt file if fpath applys */
	((struct egi_data_txt *)(ebox->egi_data))->foff=0;

	/* 7. refresh displaying the ebox */
//	if( egi_txtbox_refresh(ebox) !=0);
//		return -6;

	printf("egi_txtbox_activate(): a '%s' ebox is activated.\n",ebox->tag);
	return 0;
}


/*-------------------------------------------------------------------------------
refresh a txt ebox.
	1.refresh ebox image according to following parameter updates:
		---txt(offx,offy,nl,llen)
		---size(height,width)
		---positon(x0,y0)
		---ebox color
	2.restore backgroud from bkimg and store new position backgroud to bkimg.
	3.refresh ebox color if ebox->prmcolor >0,
 	4.update txt.

Return
	2	fail to read txt file.
	1	sleeping
	0	OK
	<0	fail
-------------------------------------------------------------------------------*/
int egi_txtbox_refresh(struct egi_element_box *ebox)
{
	int i;

	/* 1. check data */
	if(ebox->type != type_txt)
	{
		printf("egi_txtbox_refresh(): Not txt type ebox!\n");
		return -1;
	}

	/* 2. check the ebox status */
	if( ebox->status != status_active )
	{
//		printf("This '%s' ebox is not active! refresh action is ignored! \n",ebox->tag);
		return -2;
	}

	/* 3. restore bk image before refresh */
   if(ebox->movable) /* only if ebox is movale */
   {
#if 0 /* DEBUG */
	printf("txt refresh... fb_cpyfrom_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
#endif
        if(fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) <0 )
		return -3;
   }/* ebox->movable end */

	/* 4. get updated ebox parameters */
	int x0=ebox->x0;
	int y0=ebox->y0;
	int height=ebox->height;
	int width=ebox->width;
	struct egi_data_txt *data_txt=(struct egi_data_txt *)(ebox->egi_data);
	int nl=data_txt->nl;
//	int llen=data_txt->llen;
	int offx=data_txt->offx;
	int offy=data_txt->offy;
	char **txt=data_txt->txt;
	int font_height=data_txt->font->symheight;

#if 0
	/* test--------------   print out box txt content */
	for(i=0;i<nl;i++)
		printf("egi_txtbox_refresh(): txt[%d]:%s\n",i,txt[i]);
#endif

        /* 5. redefine bkimg box range, in case it may change */
	/* check ebox height and font lines in case it may changes, then adjust the height */
	/* updata bkimg->bkbox according */
	height= (font_height*nl+offy)>height ? (font_height*nl+offy) : height;
	ebox->height=height;
        ebox->bkbox.startxy.x=x0;
        ebox->bkbox.startxy.y=y0;
        ebox->bkbox.endxy.x=x0+width-1;
        ebox->bkbox.endxy.y=y0+height-1;

   if(ebox->movable) /* only if ebox is movale */
   {
#if 0 /* DEBUG */
	printf("refresh() fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
#endif
        /* ---- 6. store bk image which will be restored when this ebox position/size changes */
        if( fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                                ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0)
		return -4;
   }

	/* ---- 7. refresh prime color under the symbol  before updating txt.  */
	if(ebox->prmcolor >= 0)
	{
		/* set color and draw filled rectangle */
		fbset_color(ebox->prmcolor);
		draw_filled_rect(&gv_fb_dev,x0,y0,x0+width-1,y0+height-1);
	}

	/* --- 8. draw frame according to its type  --- */
	if(ebox->frame == 0) /* 0: simple type */
	{
		fbset_color(0); /* use black as frame color  */
		draw_rect(&gv_fb_dev,x0,y0,x0+width-1,y0+height-1);
	}
	/* TODO: other type of frame .....*/

	/* ---- 9. if data_txt->fpath !=NULL, then re-read txt file to txt[][] */
	if(data_txt->fpath)
	{
		if(egi_txtbox_readfile(ebox,data_txt->fpath)<=0)
		{
			printf("egi_txtbox_refresh(): fail to read txt file: %s \n",data_txt->fpath);
			return 2;
		}
	}

	/* ---- 10. refresh TXT, write txt line to FB */
	for(i=0;i<nl;i++)
		/*  (fb_dev,font, font_color,transpcolor, x0,y0, char*)...
					1, for font symbol: tranpcolor is its img symbol bkcolor!!! */
		symbol_string_writeFB(&gv_fb_dev, data_txt->font, data_txt->color, 1, x0+offx, \
						 y0+offy+font_height*i, txt[i]);

	return 0;
}


/*-----------------------------------------------------
put a txt ebox to sleep
1. restore bkimg
2. reset status

return
	0 	OK
	<0 	fail

------------------------------------------------------*/
int egi_txtbox_sleep(struct egi_element_box *ebox)
{
   	if(ebox->movable) /* only for movable ebox */
   	{
		/* restore bkimg */
       		if(fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) <0 )
                	return -1;
   	}

	/* reset status */
	ebox->status=status_sleep;

	printf("egi_txtbox_sleep(): a '%s' ebox is put to sleep.\n",ebox->tag);
	return 0;
}


/*-----------------------------------------------------------------
Note:
1. Read a txt file and try to push it to egi_data_txt->txt[]
2. llen=data_txt->llen-1  one byte for string end 0.

Max. char number per line =llen;
Max. pixel number per line = bxwidth

Return:
	>0	number of chars read and stored to txt[].
	<=0	fail
---------------------------------------------------------------*/
int egi_txtbox_readfile(struct egi_element_box *ebox, char *path)
{
	FILE *fil;
	int i;
	char buf[32]={0};
	int nread=0;
	int ret=0;
	struct egi_data_txt *data_txt=(struct egi_data_txt *)(ebox->egi_data);
	int bxwidth=ebox->width; /* in pixel, ebox width for txt  */
	char **txt=data_txt->txt;
	int nt=0;/* index, txt[][nt] */
	int nl=data_txt->nl; /* from 0, number of txt line */
	int nlw=0; /* current written line of txt */
	int llen=data_txt->llen -1; /*in bytes(chars), length for each line, one byte for /0 */
	int ncount=0; /*in pixel, counter for used pixels per line, MAX=bxwidth.*/
	int *symwidth=data_txt->font->symwidth;/* width list for each char code */
	int maxnum=data_txt->font->maxnum;
	long foff=data_txt->foff; /* current file position */

	/* check ebox data here */
	if( txt==NULL )
	{
		printf("egi_txtbox_readfile(): data_txt->txt is NULL!\n");
		return -1;
	}

	/* reset txt buf */
	for(i=0;i<nl;i++)
		memset(txt[i],0,data_txt->llen); /* dont use llen, here llen=data_txt->llen-1 */

	/* open txt file */
	fil=fopen(path,"rb");
	if(fil==NULL)
	{
		perror("egi_txtbox_readfile()");
		return -2;
	}
	printf("succeed to open %s, current offset=%ld \n",path,foff);

	/* restore file position */
	fseek(fil,foff,SEEK_SET);

	while( !feof(fil) )
	{
		memset(buf,0,sizeof(buf));/* clear buf */

		nread=fread(buf,1,sizeof(buf),fil);
		if(nread <= 0) /* error */
			break;

		ret+=nread;

		/* here put char to egi_data_txt->txt */
		for(i=0;i<nread;i++)
		{
			//printf("buf[%d]='%c' ascii=%d\n",i,buf[i],buf[i]);

			/*  ------ 1. if it's a return code */
			/* TODO: substitue buf[i] with space ..... */
			if( buf[i]==10 )
			{
				//printf(" ------get a return \n");
				/* only if return code is NOT the first char of a line!!! */
				if(nt != 0)
					nlw +=1; /* new line */
				nt=0;ncount=0; /*reset one line char counter and pixel counter*/
				/* MAX. number of lines = nl */
				if(nlw>nl-1) /* no more line for txt ebox */
					break;//return ret; /* abort the job */
				continue;
			}

			/* ----- 2. if symbol code out of range */
			if( (uint8_t)buf[i] > maxnum ) 
			{
				printf("egi_txtbox_readfile():symbol/font/assic code number out of range.\n");
				continue;
			}

			/* ----- 3. check available pixel space for current line
			   Max. pixel number per line = bxwidth */
			/* ----- . if symbol code out of range */
			if( symwidth[ (uint8_t)buf[i] ] > bxwidth-ncount )
			{
				nlw +=1; /* new line */
				nt=0;ncount=0; /*reset line char counter and pixel counter*/
				/* MAX. number of lines = nl */
				if(nlw>nl-1) /* no more line for txt ebox */
					break;//return ret; /* abort the job */
				/*else, retry then */
				i--;
				continue;
			}

			/* increase total number of pixels in a txt line*/
			ncount+=symwidth[ (uint8_t)buf[i] ];
			//printf("one line pixel counter: ncount=%d\n",ncount);

			/* ----- 4.now push a char to txt[][] */
			txt[nlw][nt]=buf[i];
			nt++;

			/* ----- 5. check remained space
			check Max. char number per line =llen
			*/
			if( nt > llen-1 ) /* txt buf end */
			{
				nlw +=1; /* new line */
				nt=0;ncount=0; /*reset one line char counter and pixel counter*/
				/* MAX. number of lines = nl */
				if(nlw>nl-1) /* no more line for txt ebox */
					break; //return ret;
			}
		}/* END for() */

		/* check if txt line is used up */
		if(nlw>nl-1)
			break; /* end while() */

	} /* END while() */

	/* DEBUG */
#if 0
	for(i=0;i<nl;i++)
		printf("%s\n",txt[i]);
#endif

	/* save current file position */
	printf("ftell(fil)=%ld\n",ftell(fil));
	((struct egi_data_txt *)(ebox->egi_data))->foff +=ret; //ftell(fil);

	fclose(fil);
	return ret;
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


/////////////////////////////////////////////////////////////////////////////////////////
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


/*------------------------------------------------------------------
initialize an egi_element box according to its type

Return:
        non-null        OK
        NULL            fails
-------------------------------------------------------------------*/
struct egi_element_box *egi_init_ebox(struct egi_element_box *ebox) // enum egi_ebox type)
{
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


