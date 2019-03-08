/*-----------------------------------------------------------
Note:
	Never forget why you start! ---Just For Fun!

Midas_Zhou
-----------------------------------------------------------*/
#ifndef  __FFPLAY_H__
#define  __FFPLAY_H__

#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
#include "egi_bmpjpg.h"
#include "egi_log.h"
#include <stdio.h>
#include <dirent.h>
#include <limits.h> /* NAME_MAX 255; PATH_MAX 4096 */
#include "play_ffpcm.h"

#define LCD_MAX_WIDTH 240
#define LCD_MAX_HEIGHT 320
#define FFPLAY_MUSIC_PATH "/mmc/"

/* ffplay control command signal */
enum ff_control_cmd {
        cmd_none,
        cmd_play,
        cmd_pause,
        cmd_quit, /* release all resource and quit ffplay */
        cmd_forward,
        cmd_prev,

        cmd_exit_display_thread, /* stop display thread */
};

static enum ff_control_cmd control_cmd;

/* ffplay mode */
enum ffplay_mode
{
        mode_loop_all,   /* loop all files in the list */
        mode_repeat_one, /* repeat current file */
        mode_shuffle,    /* pick next file randomly */
};


//#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48KHz 32bit audio
#define PIC_BUFF_NUM  4  /* total number of RGB picture data buffers. */

static uint8_t** pPICbuffs=NULL; /* PIC_BUF_NUM*Screen_size*16bits, data buff for several screen pictures */

static bool IsFree_PICbuff[PIC_BUFF_NUM]={false}; /* tag to indicate availiability of pPICbuffs[x].
					    * LoadPic2Buff() put 'false' tag
					    * thdf_Display_Pic() put 'true' tag,
					    */

/* information of a decoded picture, for pthread params */
struct PicInfo {
        /* coordinate for display window layout on LCD */
	int Hs; /* Horizontal start pixel column number */
	int He; /* Horizontal end */
	int Vs; /* Vertical start pixel row number */
	int Ve; /* Vertical end */
	int nPICbuff; /* slot number of buff data in pPICbuffs[] */
	uint8_t *data; /* RGB data, pointer to pPICbuffs[] page */
	int numBytes;  /* total bytes for a picture RGB data, depend on pixel format and pixel numbers */
};



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

/*-------------------------------------------------------------
   calculate and return time diff. in ms
---------------------------------------------------------------*/
int get_costtime(struct timeval tm_start, struct timeval tm_end)
{
        int time_cost;
        time_cost=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;

        return time_cost;
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

   EGI_IMGBUF imgbuf;
   imgbuf.width=ppic->He - ppic->Hs +1;
   imgbuf.height=ppic->Ve - ppic->Vs +1;

   printf("FFPLAY: -------thdf_Display_Pic(): imgbuf.width=%d, imgbuf.height=%d \n",
						imgbuf.width, imgbuf.height );

   /* check size limit */
   if(imgbuf.width>LCD_MAX_WIDTH || imgbuf.height>LCD_MAX_HEIGHT)
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

			imgbuf.imgbuf=(uint16_t *)pPICbuffs[i];
			//egi_imgbuf_display(&imgbuf, &gv_fb_dev, 0, 0);

			/* window_position displaying */
			egi_imgbuf_windisplay(&imgbuf, &gv_fb_dev, 0, 0, ppic->Hs, ppic->Vs,
									imgbuf.width, imgbuf.height);
		   	/* hold for a while :))) */
		   	usleep(20000);

		   	/* put a FREE tag after display, then it may be overwritten. */
	  	   	IsFree_PICbuff[i]=true;
		}
	   }
	   /* quit ffplay */
	   //if(fftok_QuitFFplay)
	   if(control_cmd == cmd_exit_display_thread )
		break;

	   usleep(2000);
  }
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
int Load_Pic2Buff(struct PicInfo *ppic,const uint8_t *data, int numBytes)
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


#if 0 ////////////// TO SEE IN egi_utils.c ////////////////////
/*--------------------------------------------------------------------------------------------
Find out specified type of files in a specified directory and save in ff_find_files[]

path:           Sear path without extension name.
fext:		File extension name, MUST exclude ".", Example: "wav" or "mp3"...
pcount:         Total number of files found, NULL to ignore.
		-1, search fails.

return value:
         pointer to char (*)[EGI_PATH_MAX+FPLAY_NAME_MAX]   --- OK
         NULL && pcount=-1;  --- Fails
---------------------------------------------------------------------------------------------*/
#define EGI_PATH_MAX 256 /* Max length for a file name */
#define EGI_NAME_MAX 128 /* Max length for a file path */
#define EGI_SEARCH_FILE_MAX (1<<10) /* to be 2**n, Max number of files for ff_fpath_buff[] */
#define EGI_FEXTNAME_MAX 10 /* !!! exclude '.', length of extension name */
static char* egi_alloc_search_files(const char* path, const char* fext,  int *pcount )
{
        DIR *dir;
        struct dirent *file;
        //int fnlen; /* file name length */
        int num=0; /* file numbers */
	//int extnlen=strlen(fext); /* extension name length */
	char *pt=NULL; /* pointer to '.', as for extension name */
	char (*fpbuff)[EGI_PATH_MAX+EGI_NAME_MAX]=NULL; /* pointer to final full_path buff */
	//char (*pn)[EGI_PATH_MAX+EGI_NAME_MAX]=NULL; /* pointer to a full_path in fpbuff[] */
	int km=0; /* doubling memory times */
	char *ptmp;

	/* 1. check input data */
	if( path==NULL || fext==NULL )
	{
		EGI_PLOG(LOGLV_ERROR, "ff_find_files(): Input path or extension name is NULL. \n");

		if(pcount!=NULL)*pcount=-1;
		return NULL;
	}

	/* 2. check input path leng */
	if( strlen(path) > EGI_PATH_MAX-1 )
	{
		EGI_PLOG(LOGLV_ERROR, "ff_find_files(): Input path length > EGI_PATH_MAX(%d). \n",
											EGI_PATH_MAX);
		if(pcount!=NULL)*pcount=-1;
		return NULL;
	}

        /* 3. open dir */
        if(!(dir=opendir(path)))
        {
                EGI_PLOG(LOGLV_ERROR,"ff_find_files(): %s, error open dir: %s!\n",strerror(errno),path);

		if(pcount!=NULL)*pcount=-1;
                return NULL;
        }

	/* 4. calloc one slot buff first */
	printf("start calloc()....\n");
	fpbuff= calloc(1,EGI_PATH_MAX+EGI_NAME_MAX);
	if(fpbuff==NULL)
	{
		EGI_PLOG(LOGLV_ERROR,"ff_alloc_search_files(): Fail to callo fpbuff!.\n");

		if(pcount!=NULL)*pcount=-1;
		return NULL;
	}
	km=0; /* 2^0, first double */

        /* 5. get file paths */
        while((file=readdir(dir))!=NULL)
        {
		/* 5.1 check number of files first, necessary to set limit????  */
                if(num >= EGI_SEARCH_FILE_MAX)/* break if fpaths buffer is full */
		{
			EGI_PLOG(LOGLV_WARN,"ff_alloc_search_files(): File fpath buffer is full! try to increase FFIND_MAX_FILENUM.\n");
                        break;
		}

		else if( num == (1<<km) )/* get limit of buff size */
		{
			/* double memory */
			km++;
			ptmp=(char *)realloc((char *)fpbuff,(1<<km)*(EGI_PATH_MAX+EGI_NAME_MAX) );

			/* if doubling mem fails */
			if(ptmp==NULL)
			{
				EGI_PLOG(LOGLV_ERROR,"ff_alloc_search_file(): Fail to realloc mem for fpbuff.\n");
				/* break, and return old fpbuff data though*/
				break;
			}

			/* get new pointer to the buff */
			fpbuff=( char(*)[EGI_PATH_MAX+EGI_NAME_MAX])ptmp;
			ptmp=NULL;

			EGI_PLOG(LOGLV_INFO,"ff_find_files(): fpbuff[] is reallocated with capacity to buffer totally %d items of full_path.\n",
													1<<km );
		}

                /* 5.2 check name string length */
                if( strlen(file->d_name) > EGI_NAME_MAX-1 )
		{
			EGI_PLOG(LOGLV_WARN,"ff_find_files(): %s.\n	File path is too long, fail to store.\n",
									file->d_name);
                        continue;
		}

		pt=strstr(file->d_name,"."); /* get '.' pointer */
		if(pt==NULL)
		{
			//printf("ff_find_files(): no extension '.' for %s\n",file->d_name);
			continue;
		}
		/* compare file extension name */
                if( strncmp(pt+1, fext, EGI_FEXTNAME_MAX)!=0 )
                         continue;

		/* Clear buff and save full path of the matched file */
		memset((char *)(fpbuff+num), 0, (EGI_PATH_MAX+EGI_NAME_MAX)*sizeof(char) );
		sprintf((char *)(fpbuff+num), "%s/%s", path, file->d_name);
		//printf("ff_alloc_search_files(): push %s ...OK\n",fpbuff+num);

                num++;
        }

	/* 6. feed back count to the caller */
	if(pcount != NULL)
	        *pcount=num; /* return count */

	/* 7. free fpbuff if no file found */
	if( num==0 && fpbuff != NULL )
	{
		free(fpbuff);
		fpbuff=NULL;
	}

	/* 8. close dir */
         closedir(dir);

	/* 9. return pointer to the buff */
         return (char *)fpbuff;
}
#endif 



#endif
