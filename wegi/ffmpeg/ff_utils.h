/*--------------------------------------------------------------------
Note:
	Never forget why you start! ---Just For Fun!

TODO:
1. Use a new FB_DEV to display subtitle. or it rises a race condition
   for the FB dev.
2.

Midas_Zhou
----------------------------------------------------------------------*/
#ifndef  __FF_UTILS_H__
#define  __FF_UTILS_H__

#include <stdio.h>
#include "egi_bmpjpg.h"
//#include "egi_log.h"
//#include "play_ffpcm.h"
#include "egi.h"
//#include "egi_fbgeom.h"
//#include "egi_symbol.h"
#include "egi_timer.h"

#define LCD_MAX_WIDTH 240
#define LCD_MAX_HEIGHT 320
#define FFPLAY_MUSIC_PATH "/mmc/"

/* in seconds, playing time elapsed for Video */
extern int ff_sec_Velapsed;
extern int ff_sub_delays; /* delay sub display in seconds, relating to ff_sec_Velapsed */
extern enum ff_control_cmd control_cmd;
extern long ff_start_tmsecs; /* starting position */

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

/* ffplay mode */
enum ffplay_mode
{
        mode_loop_all,   /* loop all files in the list */
        mode_repeat_one, /* repeat current file */
        mode_shuffle,    /* pick next file randomly */
};

//#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48KHz 32bit audio
#define PIC_BUFF_NUM  4  /* total number of RGB picture data buffers. */

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

/*  functions	*/
uint8_t**  malloc_PICbuffs(int width, int height, int pixel_size );
void free_PicBuffs(void);
int get_FreePicBuff(void);
void* thdf_Display_Pic(void * argv);
int Load_Pic2Buff(struct PicInfo *ppic,const uint8_t *data, int numBytes);
void* thdf_Display_Subtitle(void * argv);
long seek_subtitle_tmstamp(char *subpath, unsigned int tmsec);



#endif
