#ifndef  __FFPLAY_H__
#define  __FFPLAY_H__

#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "egi_bmpjpg.h"
#include <stdio.h>
#include "play_ffpcm.h"

#define PIC_MAX_WIDTH 240
#define PIC_MAX_HEIGHT 320

//#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48KHz 32bit audio
#define PIC_BUFF_NUM  4  //total number of RGB picture data buffers.

uint8_t** pPICbuffs; /* for one frame RGB data buffer */
bool IsFree_PICbuff[PIC_BUFF_NUM]={false}; //indicator for availiability.
//int nPICbuff; //indicator/tag of picture buffer,MAX. PIC_BUFF_NUM

/* information of a decoded picture, for pthread params */
struct PicInfo {
        /* Hb,Vb,Hs,He,Vs,Ve;  for LCD image layout */
	int Hs;
	int He;
	int Vs;
	int Ve;
	uint8_t *data; //pointer to PICbuff
	int numBytes;
	int nPICbuff; // slot number of pPICbuffs
};

bool tok_QuitFFplay=false;


/*------------------------------------------------------
WARNING:  for 1_producer and 1_consumer scenario only!!!

Allocate memory for PICbuffs
Return value:
	NULL   --- fails
	others --- OK
-------------------------------------------------------*/
uint8_t**  malloc_PICbuffs(int width, int height)
{
        int i,k;

        pPICbuffs=(uint8_t **)malloc( PIC_BUFF_NUM*sizeof(uint8_t *) );
        if(pPICbuffs == NULL) return NULL;

        for(i=0;i<PIC_BUFF_NUM;i++) {
                pPICbuffs[i]=(uint8_t *)malloc(width*height*3);

              /* if fails, free those buffs */
                if(pPICbuffs[i] == NULL) {
                        for(k=0;k<i;k++) {
                                free(pPICbuffs[k]);
                        }
                        free(pPICbuffs);
                        return NULL;
                }
       }

        /* set indicator */
        for(i=0;i<PIC_BUFF_NUM;i++) {
                IsFree_PICbuff[i]=true;
        }

        return pPICbuffs;
}

/*----------------------------------------
return a free PICbuff tag
Return value:
        >=0  OK
        <0   fails
---------------------------------------------*/
int get_FreePicBuff(void)
{
        int i;
        for(i=0;i<PIC_BUFF_NUM;i++) {
                if(IsFree_PICbuff[i]){
                        return i;
		}
        }

        return -1;
}

/*----------------------------------------------
   free PICbuffs
----------------------------------------------*/
void free_PicBuffs(void)
{
        int i;
        for(i=0;i<PIC_BUFF_NUM;i++)
                free(pPICbuffs[i]);
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

/*--------------------------------------------------
 <<<< a thread fucntion  >>>>

 In a loop to write pPICBuffs[]  data to display
---------------------------------------------------*/
void* thdf_Display_Pic(void * argv)
{
   int i;
   struct PicInfo *ppic =(struct PicInfo *) argv;

   EGI_IMGBUF imgbuf;
   imgbuf.width=ppic->He - ppic->Hs +1;
   imgbuf.height=ppic->Ve - ppic->Vs +1;

   /* check size limit */
   if(imgbuf.width>PIC_MAX_WIDTH || imgbuf.height>PIC_MAX_HEIGHT)
   {
	printf("thdf_Display_Pic(): movie size is too big to display.\n");
	exit(-1);
   }

   while(1)
   {
	   for(i=0;i<PIC_BUFF_NUM;i++)
	   {
		if( !IsFree_PICbuff[i] ) //only if pic data is loaded in the buff
		{
			//printf("imgbuf.width=%d, .height=%d \n",imgbuf.width,imgbuf.height);
			/* display picture buffer with full scree */
			imgbuf.imgbuf=(uint16_t *)pPICbuffs[i];
			//egi_imgbuf_display(&imgbuf, &gv_fb_dev, 0, 0);

			/* adjust window_position displaying */
			egi_imgbuf_windisplay(&imgbuf, &gv_fb_dev, 0, 0, ppic->Hs, ppic->Vs,
									imgbuf.width, imgbuf.height);

		   	//-----  write data in pPICbffs[i] to lcd ----
		   	//LCD_Write_Block(ppic->Hs,ppic->He,ppic->Vs,ppic->Ve, pPICbuffs[i], ppic->numBytes);
		   	usleep(50000);
		   	//----- put a FREE tag after write to displa
	  	   	IsFree_PICbuff[i]=true;
		}
	   }
	   /* quit ffplay */
	   if(tok_QuitFFplay)
		break;
	   usleep(2000);
  }
  return (void *)0;
}


/*-----------------------------------------------------------------------
 copy RGB data to data of a PicInfo
 Return value:
	>=0 Ok (slot number of PICBuffs)
	<0  fails
------------------------------------------------------------------------*/
int Load_Pic2Buff(struct PicInfo *ppic,const uint8_t *data, int numBytes)
{
	int nbuff;

	nbuff=get_FreePicBuff();
	ppic->nPICbuff=nbuff;
//	printf(" get_FreePicBuff() =%d\n",nbuff);

	//---- only if PICBuff has free slot
	if(nbuff >= 0){
		ppic->data=pPICbuffs[nbuff];//get pointer to the PICBuff
		memcpy(ppic->data,data,numBytes);
		IsFree_PICbuff[nbuff]=false; //put a NON_FREE tag to the buff slot
	}

	return nbuff;
}



#endif
