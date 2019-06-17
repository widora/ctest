/*-----------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Note:
	Never forget why you start! ---Just For Fun!

TODO:
1. Putting subtitle displaying codes in thdf_Display_Pic() may be better.


Midas_Zhou
-------------------------------------------------------------------------*/
#include <stdio.h>
#include <dirent.h>
#include <limits.h> /* system: NAME_MAX 255; PATH_MAX 4096 */

#if 0////////////////////////
#include "egi.h"
#include "egi_bjp.h"
#include "egi_log.h"
#include "egi_fbgeom.h"
#include "egi_symbol.h"
#include "egi_timer.h"
#endif //////////////////////////

#include "egi_common.h"
#include "sound/egi_pcm.h"

#include "ff_utils.h"

/* in seconds, playing time elapsed for Video */
int ff_sec_Velapsed;
int ff_sub_delays=3; /* delay sub display in seconds, relating to ff_sec_Velapsed */

/* seek position */
long ff_start_tmsecs=60*5; /* starting position */

/* ff control command */
enum ff_control_cmd control_cmd;

static uint8_t** pPICbuffs=NULL; /* PIC_BUF_NUM*Screen_size*16bits, data buff for several screen pictures */

static bool IsFree_PICbuff[PIC_BUFF_NUM]={false};  /* tag to indicate availiability of pPICbuffs[x].
						    * LoadPic2Buff() put 'false' tag
						    * thdf_Display_Pic() put 'true' tag,
						    */

/*--------------------------------------------------------------
WARNING: !!! for 1_producer and 1_consumer scenario only !!!
Allocate memory for PICbuffs[]

width,height:	picture size
pixel_size:	in byte, size for one pixel.

Return value:
	 NULL   --- fails
	!NULL 	--- OK
----------------------------------------------------------------*/
uint8_t**  malloc_PICbuffs(int width, int height, int pixel_size )
{
        int i,k;

        pPICbuffs=(uint8_t **)malloc( PIC_BUFF_NUM*sizeof(uint8_t *) );
        if(pPICbuffs == NULL) return NULL;

        for(i=0;i<PIC_BUFF_NUM;i++) {
                pPICbuffs[i]=(uint8_t *)malloc(width*height*pixel_size); /* for 16bits color */

                /* if fails, free those buffs */
                if(pPICbuffs[i] == NULL) {
                        for(k=0;k<i;k++) {
                                free(pPICbuffs[k]);
                        }
                        free(pPICbuffs);
                        return NULL;
                }
        }

        /* set tag */
        for(i=0;i<PIC_BUFF_NUM;i++) {
                IsFree_PICbuff[i]=true;
        }

        return pPICbuffs;
}

/*----------------------------------------
return a free PICbuff slot/tag number

Return value:
        >=0  OK
        <0   fails
---------------------------------------------*/
int get_FreePicBuff(void)
{
        int i;
        for(i=0;i<PIC_BUFF_NUM;i++) {
                if(IsFree_PICbuff[i]) {
                        return i;
		}
        }

        return -1;
}

/*----------------------------------
   free pPICbuffs
----------------------------------*/
void free_PicBuffs(void)
{
        int i;

	if(pPICbuffs == NULL)
		return;

        for(i=0;i<PIC_BUFF_NUM;i++)
	{
		//printf("PIC_BUFF_NUM: %d/%d start to free...\n",i,PIC_BUFF_NUM);
		if(pPICbuffs[i] != NULL)
	                free(pPICbuffs[i]);
		//printf("PIC_BUFF_NUM: %d/%d freed.\n",i,PIC_BUFF_NUM);
	}
        free(pPICbuffs);

	pPICbuffs=NULL;
}


/*--------------------------------------------------------
	     a thread fucntion
 In a loop to display pPICBuffs[]

WARNING: !!! for 1_producer and 1_consumer scenario only!!!
----------------------------------------------------------*/
void* thdf_Display_Pic(void * argv)
{
   int i;
   struct PicInfo *ppic =(struct PicInfo *) argv;

   EGI_IMGBUF *imgbuf=egi_imgbuf_new();
   if(imgbuf==NULL) {
	EGI_PLOG(LOGLV_INFO,"%s: fail to call egi_new_imgbuf().\n",__func__);
	return (void *)-1;
   }

   imgbuf->width=ppic->He - ppic->Hs +1;
   imgbuf->height=ppic->Ve - ppic->Vs +1;

   EGI_PLOG(LOGLV_INFO,"%s: thdf_Display_Pic(): imgbuf width=%d, imgbuf height=%d \n",
						__func__, imgbuf->width, imgbuf->height );

   /* check size limit */
   if(imgbuf->width>LCD_MAX_WIDTH || imgbuf->height>LCD_MAX_HEIGHT)
   {
	EGI_PLOG(LOGLV_WARN,"%s: movie size is too big to display.\n",__FUNCTION__);
	//exit(-1);
   }

   while(1)
   {
	   for(i=0;i<PIC_BUFF_NUM;i++) /* to display all pic buff in pPICbuff[] */
	   {
		if( !IsFree_PICbuff[i] ) /* only if pic data is loaded in the buff */
		{
			//printf("imgbuf.width=%d, .height=%d \n",imgbuf.width,imgbuf.height);
			imgbuf->imgbuf=(uint16_t *)pPICbuffs[i];

			/* window_position displaying */
			egi_imgbuf_windisplay(imgbuf, &gv_fb_dev, -1,
					0, 0, ppic->Hs, ppic->Vs, imgbuf->width, imgbuf->height);
		   	/* hold for a while :))) */
		   	usleep(20000);

		   	/* put a FREE tag after display, then it can be overwritten. */
	  	   	IsFree_PICbuff[i]=true;
		}
	   }

	   /* quit ffplay */
	   if(control_cmd == cmd_exit_display_thread )
		break;

	   usleep(2000);
  }

 egi_imgbuf_free(imgbuf);

  return (void *)0;
}


/*-----------------------------------------------------------------------
 Copy RGB data from *data to PicInfo.data

  ppic: 	a PicInfo struct
  data:		data source
  numbytes:	amount of data copied, in byte.

 Return value:
	>=0 Ok (slot number of PICBuffs)
	<0  fails
------------------------------------------------------------------------*/
int load_Pic2Buff(struct PicInfo *ppic,const uint8_t *data, int numBytes)
{
	int nbuff;

	nbuff=get_FreePicBuff(); /* get a slot number */
	ppic->nPICbuff=nbuff;
	//printf("Load_Pic2Buff(): get_FreePicBuff() =%d\n",nbuff);

	/* only if PICBuff has free slot */
	if(nbuff >= 0){
		ppic->data=pPICbuffs[nbuff]; /* get pointer to the PICBuff */
		//printf("Load_Pic2Buff(): start memcpy..\n");
		memcpy(ppic->data, data, numBytes);
		IsFree_PICbuff[nbuff]=false; /* put a NON_FREE tag to the buff slot */
	}

	return nbuff;
}



/*-------------------------------------------------------------
A thread function of displaying subtitles.
Read a SRT substitle file and display it on a dedicated area.

argv*:  data for subtitle path

Note:
1. Length of each subtitle line to be 32-1.
2. Argv to pass subtitle rst file path
3. \n or \r both to be deemed as line_return codes.
4. Use "-->" to confirm as a time stamp string.

WARNING: !!! for 1_producer and 1_consumer scenario only!!!
-------------------------------------------------------------*/
void* thdf_Display_Subtitle(void * argv)
{
        int subln=4; /* lines for subtitle */

        FILE *fil;
        char *subpath=(char *)argv; // "/mmc/ross.srt";
        char *pt=NULL;
        int  len; /* length of fgets string */
        char strtm[32]={0}; /* time stamp */
        int  nsect; /* section number */
        int  start_secs=0; /* sub section start time, in seconds */
        int  end_secs=0; /* sub section end time, in seconds */
	long off; /* offset to start position */
        char strsub[32*4]={0}; /* 64_chars x 4_lines, for subtitle content */
        EGI_BOX subbox={{0,150}, {240-1, 260}}; /* box area for subtitle display */

        /* open subtitle file */
       	fil=fopen(subpath,"r");
        if(fil==NULL) {
       	        printf("Fail to open subtitle:%s\n",strerror(errno));
               	return NULL;
        }

	/* seek to the start position */
	off=seek_Subtitle_TmStamp(subpath, ff_start_tmsecs);
	fseek(fil,off,SEEK_SET);

       	/* read subtitle section by section */
        while(!(feof(fil)))
        {
	        /* 2. get time stamp first! */
	        memset(strtm,0,sizeof(strtm));
	        fgets(strtm,sizeof(strtm),fil);/* time stamp */
	        if(strstr(strtm,"-->")!=NULL) {  /* to confirm the stime stamp string */
//	                printf("time stamp: %s\n",strtm);
        	        start_secs=atoi(strtm)*3600+atoi(strtm+3)*60+atoi(strtm+6);
//                	printf("Start(sec): %d\n",start_secs);
                       	end_secs=atoi(strtm+17)*3600+atoi(strtm+20)*60+atoi(strtm+23);
//                     	printf("End(sec): %d\n",end_secs);
	        }
		else
			continue;

        	/* 3. read a section of sub and display it */
	        fbset_color(WEGI_COLOR_BLACK);
//        	draw_filled_rect(&gv_fb_dev,subbox.startxy.x,subbox.startxy.y,
//								subbox.endxy.x,subbox.endxy.y);
        	draw_filled_rect2(&gv_fb_dev,WEGI_COLOR_BLACK,subbox.startxy.x,subbox.startxy.y,
								subbox.endxy.x,subbox.endxy.y);
	        len=0;
        	memset(strsub,0,sizeof(strsub));
	        do {   /* read a section of subtitle */
        	       pt=fgets(strsub+len,subln*32-len-1,fil);
                       if(pt==NULL)break;
//	               printf("fgets pt:%s\n",pt);
        	       len+=strlen(pt); /* fgets also add a '\0\ */
                       if(len>=subln*32-1)
                        	        break;
	        }while( *pt!='\n' && *pt!='\r' && *pt!='\0' ); /* return code, section end token */

		/* 4. wait for a right time to display the subtitles */
		do{
		     tm_delayms(200);
//		     printf("Elapsed time:%d  Start_secs:%d\n",ff_sec_Velapsed, start_secs);
		} while( start_secs > ff_sec_Velapsed - ff_sub_delays );
        	symbol_strings_writeFB(&gv_fb_dev, &sympg_testfont, 240, subln, -5, WEGI_COLOR_ORANGE,
                                                                                1, 0, 170, strsub,-1);
		/* 5. wait for a right time to let go to erase the sub. */
		do{
		     tm_delayms(200);
//		     printf("Elapsed time:%d  End_secs:%d\n",ff_sec_Velapsed, end_secs);
		} while( end_secs > ff_sec_Velapsed - ff_sub_delays );

		/* 6. check if any command comes */
	        if(control_cmd == cmd_exit_display_thread )
                	break;

       	        /* 1. section number or a return code, ignore it. */
                memset(strtm,0,sizeof(strtm));
       	        fgets(strtm,32,fil);
                if(*strtm=='\n' || *strtm=='\t')
       	                continue;  /* or continue if its a return */
          	else {
                   	 nsect=atoi(strtm);
//                       printf("Section: %d\n",nsect);
                }


        }/* end of sub file */

       	fclose(fil);
	return NULL;
}


/* --------------------------------------------------
Seek subtitle file, get offset with right time stamp

subpath:	path to the subtitle file.
tmsec:		time elapsed for the movie.

return:
	>0 	OK, offset
	<0 	Fails
----------------------------------------------------*/
long seek_Subtitle_TmStamp(char *subpath, unsigned int tmsec)
{
        char strtm[32]={0}; /* time stamp */
        int  start_secs=0; /* sub section start time, in seconds */
        int  end_secs=0; /* sub section end time, in seconds */
	long off=-1;
	FILE *fil;

	/* check data */
	if(subpath==NULL) return -2;
	if(tmsec==0) return 0L;

        /* open subtitle file */
       	fil=fopen(subpath,"r");
        if(fil==NULL) {
       	        printf("Fail to open subtitle:%s\n",strerror(errno));
               	return -3;
        }

	/* seek subtitle from start */
	fseek(fil,0L,SEEK_SET);
        while(!(feof(fil)))
        {
                memset(strtm,0,sizeof(strtm));
                fgets(strtm,sizeof(strtm),fil);/* time stamp */
                if(strstr(strtm,"-->")!=NULL) {  /* to confirm the time stamp string */
                        printf("Seek time stamp: %s\n",strtm);
                        start_secs=atoi(strtm)*3600+atoi(strtm+3)*60+atoi(strtm+6);
                        printf("Seek Start(sec): %d\n",start_secs);
                        end_secs=atoi(strtm+17)*3600+atoi(strtm+20)*60+atoi(strtm+23);
                        printf("Seek End(sec): %d\n",end_secs);
                }
		/* get offset position */
		if(start_secs > tmsec-ff_sub_delays) {
			off=ftell(fil);
			break;
		}
	}

	fclose(fil);

	return off;
}


