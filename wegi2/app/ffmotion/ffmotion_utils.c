/*-----------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Note:
	Never forget why you start! ---Just For Fun!

TODO:
1. Putting subtitle displaying codes in thdf_Display_Pic() may be better.
2. Apply EGI_FIFO for frame buffering instead of picbuff[];


Midas_Zhou
-------------------------------------------------------------------------*/
#include <stdio.h>
#include <dirent.h>
#include <limits.h> /* system: NAME_MAX 255; PATH_MAX 4096 */

#include "egi_common.h"
#include "utils/egi_utils.h"
#include "utils/egi_cstring.h"
#include "sound/egi_pcm.h"
#include "egi_FTsymbol.h"
#include "ffmotion.h"
#include "ffmotion_utils.h"


struct ffmotion_PicInfo motPicInfo;

/* in seconds, playing time elapsed for Video */
//int ff_sec_Velapsed;
int ff_sub_delays=3; /* delay sub display in seconds, relating to ff_sec_Velapsed */

/* ff control command */
enum ffmotion_cmd control_cmd;	/* Open to module caller for command convey  */

/* predefine seek position */
//long start_tmsecs=60*95; /* starting position */

/***
 * 1. Thanks to slow SPI transfering speed, producing speed is usually greater than consuming speed.
 * 2. Following keys to be initialized in ff_malloc_PICbuffs().
 */
static int ifplay;		/* Current playing frame index of pPICbuffs[] */
static unsigned long nfc;	/* Total frames read from pPICbuffs and consumed */
static unsigned long nfp;	/* Total frames produced and copied to pPICbuffs */

static uint8_t** pPICbuffs=NULL; /* PIC_BUF_NUM*Screen_size*16bits, data buff for several screen pictures
				  * malloc in thread_ffplay_music(), free in display_MusicPic()
				  */

static bool IsFree_PICbuff[PIC_BUFF_NUM]={true,true,true};  /* Tag to indicate availiability of pPICbuffs[x].
						    * True:  Obsoleted data inside, ready for new data input.
						    * False: New image data inside, ready for displaying.
						    * LoadPic2Buff() put 'false' tag
						    * thdf_Display_Pic() put 'true' tag,
						    */

static long seek_Subtitle_TmStamp(char *subpath, unsigned int tmsec);
static bool bkimg_updated;  	/* TRUE: Indicating back ground image for music playing is updated,
			     	 * It will reset to FALSE when display_MusicPic() exits.
			     	 */
static bool pic_off;	    	/* TRUE: Indicating there is no picture embedded in the audio file,
			     	 *       or it's disabled by user.
			     	 */

/*-----------------------------------------------------
Init FFplay context, allocate FFmotion_Ctx, and sort out
all media files in path.

@path           path for media files
@fext:          File extension name, MUST exclude ".",
                Example: "avi","mp3", "jpg, avi, mp3"

return:
        0       OK
        <0    Fails
------------------------------------------------------*/
int init_ffmotionCtx(char *fext)
{
        int fcount;
        char video_dir[EGI_PATH_MAX]={0};
        char url_addr[EGI_URL_MAX]={0};

	/* Free ctxt then calloc */
	if(FFmotion_Ctx != NULL)
		free_ffmotionCtx();

        FFmotion_Ctx=calloc(1,sizeof(FFMOTION_CONTEXT));
        if(FFmotion_Ctx==NULL) {
                EGI_PLOG(LOGLV_ERROR,"%s: Fail to calloc FFmotion_Ctx.",__func__);
                return -1;
        }
	/* Mem alloc for URL, ---- Now only 1 item ---- */
	FFmotion_Ctx->utotal=1;
	FFmotion_Ctx->url=egi_malloc_buff2D(FFmotion_Ctx->utotal, EGI_URL_MAX*sizeof(char));
        if(FFmotion_Ctx->url==NULL) {
                EGI_PLOG(LOGLV_ERROR,"%s: Fail to calloc FFmotion_Ctx->url.",__func__);
		free_ffmotionCtx();
                return -2;
        }

        /* Get video_dir from EGI config file, OR use default dir. */
        if ( egi_get_config_value("EGI_FFMOTION","video_dir",video_dir) != 0) {
                /* use default dir */
                strcpy(video_dir,"/mmc");
                EGI_PLOG(LOGLV_INFO,"%s: Fail to read config video_dir, use default dir: %s\n",
                                                                                __func__, video_dir);
        } else {
                EGI_PLOG(LOGLV_INFO,"%s: read egi.config and get video_dir: %s\n",__func__, video_dir);
        }

        /* search for files and put to ffCtx->fpath */
        FFmotion_Ctx->fpath=egi_alloc_search_files(video_dir, fext, &fcount);
        FFmotion_Ctx->ftotal=fcount;

	/* get URL address if configured in conf */
        if ( egi_get_config_value("EGI_FFMOTION","url_addr", FFmotion_Ctx->url[0] ) != 0) {
                egi_free_buff2D((unsigned char **)FFmotion_Ctx->url, FFmotion_Ctx->utotal);
		FFmotion_Ctx->url=NULL;
		FFmotion_Ctx->utotal=0;
                EGI_PLOG(LOGLV_WARN,"%s: Fail to read url_addr from egi.conf, apply video_dir for FFmotion.",
												__func__);
	}

        return 0;
}

/*-----------------------------------------
        Free a FFPLAY_CONTEXT struct
-----------------------------------------*/
void free_ffmotionCtx(void)
{
        if(FFmotion_Ctx==NULL) return;

	/* Free URL buffer */
        if( FFmotion_Ctx->utotal > 0 ) {
                egi_free_buff2D((unsigned char **)FFmotion_Ctx->url, FFmotion_Ctx->utotal);
		FFmotion_Ctx->url=NULL;
		FFmotion_Ctx->utotal=0;
	}

	/* Free medial file path buffer */
        if( FFmotion_Ctx->ftotal > 0 ) {
                egi_free_buff2D((unsigned char **)FFmotion_Ctx->fpath, FFmotion_Ctx->ftotal);
		FFmotion_Ctx->fpath=NULL;
		FFmotion_Ctx->ftotal=0;
	}

	/* Free iteslf */
        free(FFmotion_Ctx);

        FFmotion_Ctx=NULL;
}


/*----------------------------------------------------------
WARNING: !!! for 1_producer and 1_consumer scenario only !!!
Allocate memory for PICbuffs[]

@pic_size:	Total pixels of a picture.
@pixel_size:	in byte, size for one pixel.

Return value:
	 NULL   --- fails
	!NULL 	--- OK
----------------------------------------------------------*/
uint8_t**  malloc_PicBuffs(int pic_size, int pixel_size )
{
        int i,k;

        pPICbuffs=(uint8_t **)malloc( PIC_BUFF_NUM*sizeof(uint8_t *) );
        if(pPICbuffs == NULL) return NULL;

        for(i=0;i<PIC_BUFF_NUM;i++) {
                pPICbuffs[i]=(uint8_t *)calloc(1,pic_size*pixel_size); /* default BLACK */

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

   	/* init key values */
   	nfc=0;
   	nfp=0;
   	ifplay=0;

        return pPICbuffs;
}

/*-------------------------------------------
Return a free PICbuff slot/tag number.

Return value:
        >=0  OK
        <0   fails
---------------------------------------------*/
static inline int get_FreePicBuff(void)
{
        int i;
	int index;

        for(i=0;i<PIC_BUFF_NUM;i++) {
		/* check and get free slot number in preceding ifplay order. */
		index=(ifplay+i+1)&(PIC_BUFF_NUM-1);

                if(IsFree_PICbuff[index]) {
                        return index;
		}
        }

        return -1;
}


/*----------------------------------
   	Free pPICbuffs
----------------------------------*/
static void free_PicBuffs(void)
{
        int i;

	if(pPICbuffs == NULL)
		return;

        for(i=0; i<PIC_BUFF_NUM; i++)
	{
		//printf("PIC_BUFF_NUM: %d/%d start to free...\n",i,PIC_BUFF_NUM);
		if(pPICbuffs[i] != NULL)  {
	                free(pPICbuffs[i]);
			pPICbuffs[i]=NULL;
		}
		//printf("PIC_BUFF_NUM: %d/%d freed.\n",i,PIC_BUFF_NUM);
	}
        free(pPICbuffs);

	pPICbuffs=NULL;
}


/*----------------------------------------------------------
		     A thread function
In a loop to display pPICBuffs[]

WARNING: !!! for 1_producer and 1_consumer scenario only!!!
-----------------------------------------------------------*/
void* thdf_Display_motionPic(void * argv)
{
   if(FFmotion_Ctx==NULL) {
        printf("%s: FFmotion_Ctx is NULL!\n",__func__);
        return (void *)-1;
   }

   int  i;
   int  index;
   unsigned long nfc_tmp;
   bool still_image;

//   struct ffmotion_PicInfo *ppic =(struct ffmotion_PicInfo *) argv;

   EGI_IMGBUF *imgbuf=egi_imgbuf_alloc();
   if(imgbuf==NULL) {
        EGI_PLOG(LOGLV_INFO,"%s: fail to call egi_new_imgbuf().\n",__func__);
        return (void *)-1;
   }

   imgbuf->width=motPicInfo.dispBox.endxy.x-motPicInfo.dispBox.startxy.x +1;
   imgbuf->height=motPicInfo.dispBox.endxy.y-motPicInfo.dispBox.startxy.y +1;

   EGI_PLOG(LOGLV_INFO,"%s: imgbuf width=%d, imgbuf height=%d \n",
                                                __func__, imgbuf->width, imgbuf->height );

   /* check if it's image, TODO: still or motion image */
   if(IS_IMAGE_CODEC(motPicInfo.vcodecID)) {
          still_image=true;
          EGI_PLOG(LOGLV_INFO,"%s: Playing an still image.\n",__func__);
   }  else {
          still_image=false;
   }

   while(1)
   {
           nfc_tmp=nfc; /* starting from nfc_tmp, check PIC_BUFF_NUM buffs one by one */
           for(i=0;i<PIC_BUFF_NUM;i++) /* to display all pic buff in pPICbuff[] */
           {
                index= (nfc_tmp+i) & (PIC_BUFF_NUM-1);
                if( !IsFree_PICbuff[index] ) {
                        /* set index of pPICbuff[] for current playing frame*/
                        ifplay=index;

                        //printf("imgbuf.width=%d, .height=%d \n",imgbuf.width,imgbuf.height);
                        imgbuf->imgbuf=(uint16_t *)pPICbuffs[index]; /* Ownership transfered! */

			/* adjust luma */
			//egi_imgbuf_avgLuma(imgbuf, 125);

                        /* window_position displaying */
                        egi_imgbuf_windisplay(imgbuf, &ff_fb_dev, -1,
                                        0, 0, motPicInfo.dispBox.startxy.x, motPicInfo.dispBox.startxy.y,
					imgbuf->width, imgbuf->height);

                        /* put a FREE tag after display, then it can be overwritten. */
                        IsFree_PICbuff[index]=true;

                        tm_delayms(15); //25;
                        //usleep(20000);

                        /* increase number of frames consumed */
                        nfc++;
                }
           }

          /* revive slot [0] for still image */
          if( still_image )  {
                tm_delayms(500);
                nfc=1; /* since get_FreePicBuff() from 1 */
                IsFree_PICbuff[1]=false;
          }

           /* quit ffplay */
           if(control_cmd == cmd_exit_display_thread ) {
                EGI_PLOG(LOGLV_INFO,"%s: exit commmand is received!\n",__func__);
                break;
           }

          tm_delayms(25); //25
	  //printf("nfc=%ld\n",nfc);
          //usleep(2000);
  }

  free_PicBuffs();
  imgbuf->imgbuf=NULL; /* since freed by free_PicBuffs() */

  egi_imgbuf_free(imgbuf);

  return (void *)0;
}


/*------------------------------------------------------------------------
Copy RGB data from *data to ffmotion_PicInfo.data

  ppic: 	a ffmotion_PicInfo struct
  data:		data source
  numbytes:	amount of data copied, in byte.

TODO: Pic data loading must NOT exceed displaying by one circle of PIC buff.

 Return value:
	>=0 Ok (slot number of PICBuffs)
	<0  fails
--------------------------------------------------------------------------*/
int load_Pic2Buff(struct ffmotion_PicInfo *ppic,const uint8_t *data, int numBytes)
{
	int nbuff;

	nbuff=get_FreePicBuff(); /* get a slot number */
	//printf("Load_Pic2Buff(): get_FreePicBuff() =%d\n",nbuff);

	/* only if PICBuff has free slot, and no more than PIC_BUFF_NUM(one circle of buff), then renew ppic */
	if( nbuff >= 0 && nfp-nfc < PIC_BUFF_NUM  ){
		ppic->data=pPICbuffs[nbuff]; /* get pointer to the PICBuff */
		//printf("Load_Pic2Buff(): start memcpy..\n");
		memcpy(ppic->data, data, numBytes);
		IsFree_PICbuff[nbuff]=false; /* put a NON_FREE tag to the buff slot */
		ppic->nPICbuff=nbuff;	/* put slot number */

		/* increase total number of frames produced */
		nfp++;
	}

	return nbuff;
}


/*------------------------------------------------------------
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
	if(FFmotion_Ctx==NULL) {
		printf("%s: FFmotion_Ctx is NULL!\n",__func__);
		return (void *)-1;
	}

        int subln=4; 				/* lines for subtitle displaying */
        FILE *fil;
        char *subpath=(char *)argv;
        char *pt=NULL;
        int  len; 				/* length of fgets string */
        char strtm[32]={0}; 			/* time stamp */
        int  nsect; 				/* section number */
        int  start_secs=0; 			/* sub section start time, in seconds */
        int  end_secs=0; 			/* sub section end time, in seconds */
	long off; 				/* offset to start position */
        char strsub[32*4]={0}; 			/* 64_chars x 4_lines, for subtitle content */
        EGI_BOX subbox={{0,150}, {240-1, 240-10}}; /* box area for subtitle display */

        /* open subtitle file */
       	fil=fopen(subpath,"r");
        if(fil==NULL) {
       	        printf("Fail to open subtitle:%s\n",strerror(errno));
               	return (void *)-2;
        }

	/* seek to the start position */
	off=seek_Subtitle_TmStamp(subpath, FFmotion_Ctx->start_tmsecs);
	fseek(fil,off,SEEK_SET);

       	/* read subtitle section by section */
        while(!(feof(fil)))
        {

	        /* 2. get time stamp first! */
	        memset(strtm,0,sizeof(strtm));
	        fgets(strtm,sizeof(strtm),fil);/* time stamp */
	        if(strstr(strtm,"-->")!=NULL) {  /* to confirm the stime stamp string */
	                //printf("time stamp: %s\n",strtm);
        	        start_secs=atoi(strtm)*3600+atoi(strtm+3)*60+atoi(strtm+6);
                	//printf("Start(sec): %d\n",start_secs);
                       	end_secs=atoi(strtm+17)*3600+atoi(strtm+20)*60+atoi(strtm+23);
                     	//printf("End(sec): %d\n",end_secs);
	        }
		else
			continue;

        	/* 3. read a section of sub and display it */
	        //fbset_color(WEGI_COLOR_BLACK);
        	draw_filled_rect2(&gv_fb_dev,WEGI_COLOR_BLACK,subbox.startxy.x,subbox.startxy.y,
								subbox.endxy.x,subbox.endxy.y);
	        len=0;
        	memset(strsub,0,sizeof(strsub));
	        do {   /* read a section of subtitle */
        	       pt=fgets(strsub+len,subln*32-len-1,fil);
                       if(pt==NULL)break;
	               //printf("fgets pt:%s\n",pt);
        	       len+=strlen(pt); /* fgets also add a '\0\ */
                       if(len>=subln*32-1)
                        	        break;
	        }while( *pt!='\n' && *pt!='\r' && *pt!='\0' ); /* return code, section end token */

		/* 4. wait for a right time to display the subtitles */
		do{
		     /* check if any command comes */
		     if(control_cmd == cmd_exit_subtitle_thread ) {
			  EGI_PDEBUG(DBG_FFPLAY,"Exit commmand is received, exit thread now...!\n");
			  fclose(fil);
                	  pthread_exit( (void *)-1);
		     }

		     tm_delayms(200);
		     //printf("Elapsed time:%d  Start_secs:%d\n",ff_sec_Velapsed, start_secs);
		} while( start_secs > ff_sec_Velapsed - ff_sub_delays );

		/* 5. Disply subtitle */
       	        //symbol_strings_writeFB(&ff_fb_dev, &sympg_testfont, 240, subln, -5, WEGI_COLOR_ORANGE,
       	        symbol_strings_writeFB(&ff_fb_dev, &sympg_ascii, 240, subln, -2, WEGI_COLOR_ORANGE,
                                                                                1, 0, 160, strsub,-1);

		/* 6. wait for a right time to let go to erase the sub. */
		do{
		     /* check if any command comes */
		     if(control_cmd == cmd_exit_subtitle_thread ) {
			  EGI_PDEBUG(DBG_FFPLAY,"%s: exit commmand is received, exit thread now...!\n",
												__func__);
			  fclose(fil);
                	  pthread_exit( (void *)-2);
		     }

		     tm_delayms(200);
		     //printf("Elapsed time:%d  End_secs:%d\n",ff_sec_Velapsed, end_secs);
		} while( end_secs > ff_sec_Velapsed - ff_sub_delays );

       	        /* 7. section number or a return code, ignore it. */
                memset(strtm,0,sizeof(strtm));
       	        fgets(strtm,32,fil);
                if(*strtm=='\n' || *strtm=='\t')
       	                continue;  /* or continue if its a return */
          	else {
                   	 nsect=atoi(strtm);
                       //printf("Section: %d\n",nsect);
                }

        }/* end of sub file */

       	fclose(fil);

	return (void *)0;
}


/*--------------------------------------------------------
Seek subtitle file, get offset with right time stamp

subpath:	path to the subtitle file.
tmsec:		time elapsed for the movie.

return:
	>0 	OK, offset
	<0 	Fails
---------------------------------------------------------*/
static long seek_Subtitle_TmStamp(char *subpath, unsigned int tmsec)
{
        char strtm[32]={0}; /* time stamp */
        int  start_secs=0; /* sub section start time, seconds part */
	int  start_ms=0;   /* sub section start time, millisecond part */
        int  end_secs=0;   /* sub section end time, seconds part  */
	int  end_ms=0;	   /* sub section end time, millisecond part */
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
                        //printf("Seek time stamp: %s\n",strtm);
                        start_secs=atoi(strtm)*3600+atoi(strtm+3)*60+atoi(strtm+6);
			start_ms=atoi(strtm+9);
                        //printf("Seek Start(sec): %d\n",start_secs);
                        end_secs=atoi(strtm+17)*3600+atoi(strtm+20)*60+atoi(strtm+23);
			end_ms=atoi(strtm+9);
                        //printf("Seek End(sec): %d\n",end_secs);
                }
		/* get offset position */
		if(start_secs > tmsec-ff_sub_delays) {
			off=ftell(fil);
			printf("Seek start position at %dsec %dms.\n",start_secs, start_ms);
			break;
		}
	}

	fclose(fil);

	return off;
}

