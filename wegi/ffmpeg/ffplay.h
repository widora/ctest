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
#include "play_ffpcm.h"

#define LCD_MAX_WIDTH 240
#define LCD_MAX_HEIGHT 320


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



#if 0
/* --------  !!!! OBSELETE, shell will parse *.xx to get all certain type of files  ----------
Find out specified type of files in a specified directory

fpath:           full path with file extension name for searching, for example " /music/*.mp3 ".
count:          total number of files found, NULL to ignore.
fpaths:         file path list
maxfnum:        max items of fpaths
maxflen:        max file name length

return value:
         0 --- OK
        <0 --- fails
---------------------------------------------------------------------------------------------*/
#define FFIND_MAX_FILENUM 256 /* Max number of file paths to be stored in a buffer */
#define FFIND_MAX_FPATHLEN 128 /* Max length for the full path of a file */
char ff_fpath_buff[FFIND_MAX_FILENUM][FFIND_MAX_FPATHLEN]={0};
static int ff_find_files(const char* fpath, int *count )
{
        DIR *dir;
	char path[FFIND_MAX_FPATHLEN]={0}; /* for path string, without extension name */
	char fext[10]={0}; /* for file extension name */
	int fext_len; /* extension name length */
        struct dirent *file;
        int fn_len; /* file name length */
        int num=0;

	printf("fpath:%s, len=%d\n",fpath,strlen(fpath));

	/* get extension name */
	char *tp=strstr(fpath,"*.");/* postion for '*.' */
	if(tp==NULL)
	{
		printf("ff_find_files(): Missing extension name or extension name error!  Example: /music/\*.mp3 \n");
		return -1;
	}
	tp++; /* pass '*' */
	strncpy(fext,tp,10-1);
	fext_len=strlen(fext);
	printf("fext: %s, fext_len: %d \n",fext, fext_len);

	/* separate to get path string */
	strncpy(path,fpath,strlen(fpath)-fext_len-1);
	printf("path: %s \n",path);

        /* open dir */
        if(!(dir=opendir(path)))
        {
                printf("ff_find_files(): error open dir: %s !\n",path);
                return -2;
        }

        /* get file paths */
        while((file=readdir(dir))!=NULL)
        {
                /* find out all files */
                fn_len=strlen(file->d_name);
                if(fn_len>FFIND_MAX_FPATHLEN-10)/* full file path length limit */
		{
			printf("ff_find_files(): %s.\n	File path is too long, fail to store.\n",file->d_name);
                        continue;
		}
                //if(strncmp(file->d_name+fn_len-4,".mp3",4)!=0 )
                if(strncmp(file->d_name+fn_len-fext_len,fext,fext_len)!=0 )
                         continue;
		memset((char *)&ff_fpath_buff[num][0],0,FFIND_MAX_FPATHLEN*sizeof(char));
		sprintf((char *)&ff_fpath_buff[num][0],"%s/%s",path,file->d_name);
		//printf("Find:	%s\n",&ff_fpath_buff[num][0]);
                num++;
                if(num==FFIND_MAX_FILENUM)/* break if fpaths buffer is full */
		{
			printf("ff_find_files(): File fpath buffer is full! try to increase FFIND_MAX_FILENUM.\n");
                        break;
		}
        }

	if(count !=NULL)
	        *count=num; /* return count */

         closedir(dir);
         return 0;
}
#endif





#endif
