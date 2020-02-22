/*--------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example of a timer controled by input TXT.
!!! For 'One Server One Client' scenario only !!!

Note:
1. Run server in back ground:
	test_timer -s >/dev/null 2>&1 &  ( OR use screen )
2. Send TXT as command:
	test_timer "一小时3刻钟后提醒"

3. To incoorperate:
   screen -dmS TXTSVR ./txt_timer -s
   autorec_timer  /tmp/asr_snd.pcm  ( autorec_timer ---> asr_timer.sh ---> txt_timer(test_timer)  )

Midas Zhou
--------------------------------------------------------------------*/
#include <stdio.h>
#include <getopt.h>
#include "egi_common.h"
#include "egi_cstring.h"
#include "egi_FTsymbol.h"
#include "egi_gif.h"
#include "egi_shmem.h"
#include "egi_pcm.h"

#define SHMEM_SIZE 	(1024*4)
#define TXTDATA_OFFSET	1024
#define TXTDATA_SIZE 	1024
#define DEFAULT_ALARM_SOUND	"/mmc/alarm.wav"
#define DEFAULT_TICK_SOUND	"/mmc/tick.wav"
#define TIMER_BKGIMAGE		"/mmc/linux.jpg"
#define PCM_MIXER		"mymixer"

/* Timer data struct */
typedef struct {
	time_t  ts;  		/*  time span/duration value,  typedef long time_t   */
	time_t  tp;		/*  time point		  */

	bool	active;		/*  status/indicator 	  */
	bool	sigstart;	/*  signal to start timer */
	bool	sigstop;	/*  signal to stop timer  */
	bool    sigquit;	/*  signal to quit the timer thread */

	EGI_PCMBUF *pcmalarm;   /*  for alarm sound effect */
	EGI_PCMBUF *pcmtick;    /*  for ticking effect 	*/
	bool	ticksynch;	/*  for synchronizing ticking sound */
	bool	notick;

} etimer_t;

etimer_t timer1={0,0};

static void *timer_process(void *arg);
static void * thread_ticking(void *arg);



/*=====================================
		MAIN  PROG
======================================*/
int main(int argc, char **argv)
{
	int dts=1; /* Default displaying for 1 second */
	int opt;
	bool Is_Server=false;
	bool sigstop=false;
	char *ptxt=NULL;
	char *pdata=NULL;
	char *wavpath=NULL;
        char strtm[128]={0};  /* Buffer for extracted time-related string */
	pthread_t	timer_thread;

	if(argc < 1)
		return 2;

        /* parse input option */
        while( (opt=getopt(argc,argv,"hsw:t:q"))!=-1)
        {
                switch(opt)
                {
                       	case 'h':
                           printf("usage:  %s [-hsw:t:q] text \n", argv[0]);
                           printf("	-h   help \n");
			   printf("	-s   to start as server. If shmem exists in /dev/shm/, then it will be removed first.\n");
			   printf("	-w   to indeicate wav file as for alarm sound, or use default.\n");
			   printf("	-q   to end the server process.\n");
                           printf("	-t   holdon time, in seconds.\n");
                           printf("	text 	Text for displaying \n");
                           return 0;
			case 's':
			   Is_Server=true;
			   break;
			case 'w':
			   wavpath=optarg;
			   printf("Use alarm sound file '%s'\n",wavpath);
			   break;
		 	case 't':
			   dts=atoi(optarg);
			   printf("dts=%d\n",dts);
			   break;
                       	case 'q':
                           printf(" Signal to quit text displaying process...\n");
                           sigstop=true;
                           break;
                       	default:
                           break;
                }
        }
	if(optind<argc) {
		ptxt=argv[optind];
		printf("Input TXT: %s\n",ptxt);
	}

        /* ------------ SHM_MSG communication ---------- */
        EGI_SHMEM shm_msg= {
          .shm_name="timer_ctrl",
          .shm_size=SHMEM_SIZE,
        };


	/* Open shmem,  For server, If /dev/shm/timer_ctrl exits, remove it first.  */
	if( Is_Server ) {
		if( egi_shmem_remove(shm_msg.shm_name) ==0 )
			printf("Old shm dev removed!\n");
	}
        if( egi_shmem_open(&shm_msg) !=0 ) {
                printf("Fail to open shm_msg!\n");
                exit(1);
        }

	/* Assign shm data starting address */
	pdata=shm_msg.shm_map+TXTDATA_OFFSET;
	memset( shm_msg.msg_data->msg,0,64 );
	memset( pdata, 0, TXTDATA_SIZE); /* clear data block */


/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>   Start Client Job  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
        /* shm_msg communictate */
        if( shm_msg.msg_data->active ) {         /* If msg_data is ACTIVE already */
		printf("Start client job...\n");
                if(shm_msg.msg_data->msg[0] != '\0')
                        printf("Msg in shared mem '%s' is: %s\n", shm_msg.shm_name, shm_msg.msg_data->msg);
                if( ptxt != NULL ) {
			shm_msg.msg_data->signum=dts;
			strncpy( shm_msg.msg_data->msg, "New data available", 64);
                        strcpy( pdata, ptxt);
			printf("Finishing copying TXT to shmem!\n");
                }

                /* ---- signal to quit the main process ----- */
                if( sigstop ) {
                        printf("Signal to stop and wait....\n");
                        while( shm_msg.msg_data->active) {
                                shm_msg.msg_data->sigstop=true;
                                usleep(100000);
                        }
		}
               	egi_shmem_close(&shm_msg);
		printf("End client job!\n");
               	exit(1);
        }
/* >>>>>>>>>>>>>   END of Client Job  <<<<<<<<<<<<<<< */




/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>   Start Server Job   <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
	printf("Start server job...\n");
        if( !shm_msg.msg_data->active )
        {
                shm_msg.msg_data->active=true;
                printf("You are the first to handle this mem!\n");
        }


        /* <<<<<  EGI general init  >>>>>> */
#if 0
        printf("tm_start_egitick()...\n");
        tm_start_egitick();                     /* start sys tick */

        printf("egi_init_log()...\n");
        if(egi_init_log("/mmc/log_gif") != 0) {        /* start logger */
                printf("Fail to init logger,quit.\n");
                return -1;
        }
#endif

#if 1
        printf("symbol_load_allpages()...\n");
        if(symbol_load_allpages() !=0 ) {       /* load sys fonts */
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
#endif

#if 1
        printf("symbol_load_sysfonts()...\n");
        if(FTsymbol_load_sysfonts() !=0 ) {     /* load FT fonts LIBS */
                printf("Fail to load FT appfonts, quit.\n");
                return -2;
        }
#endif

        printf("init_fbdev()...\n");
        if( init_fbdev(&gv_fb_dev) )            /* init sys FB */
                return -1;

#if 0
        printf("start touchread thread...\n");
        egi_start_touchread();                  /* start touch read thread */
#endif
        /* <<<<------------------  End EGI Init  ----------------->>>> */

	/* Load sound pcmbuf for Timer */
	if(wavpath != NULL) {
		timer1.pcmalarm=egi_pcmbuf_readfile(wavpath);
        	if( timer1.pcmalarm==NULL )
                	printf("Fail to load sound file '%s' for alarm sound.\n", wavpath);
	}
	if(timer1.pcmalarm==NULL) {
		timer1.pcmalarm=egi_pcmbuf_readfile(DEFAULT_ALARM_SOUND);
        	if( timer1.pcmalarm==NULL )
                	printf("Fail to load sound file '%s' for default alarm sound.\n", DEFAULT_ALARM_SOUND);
        }
	timer1.pcmtick=egi_pcmbuf_readfile(DEFAULT_TICK_SOUND);
	if(timer1.pcmtick==NULL)
               	printf("Fail to load sound file '%s' for default ticking sound.\n", DEFAULT_TICK_SOUND);

	/* Setup FB */
	fb_set_directFB(&gv_fb_dev, false);
	fb_position_rotate(&gv_fb_dev,3);

	/* init FB working buffer with image */
        //fb_page_saveToBuff(&gv_fb_dev, 0);
        EGI_IMGBUF *eimg=egi_imgbuf_readfile(TIMER_BKGIMAGE);
        if(eimg==NULL)
                printf("Fail to read and load file '%s'!", TIMER_BKGIMAGE);
        egi_imgbuf_windisplay( eimg, &gv_fb_dev, -1,
                               0, 0, 0, 0,
                               gv_fb_dev.pos_xres, gv_fb_dev.pos_yres );

	/* Timer Circle */
	fbset_color(WEGI_COLOR_LTGREEN);
	/* FBDEV *dev, int x0, int y0, int r, float Sang, float Eang, unsigned int w */
	draw_arc(&gv_fb_dev, 100, 120, 90, -3.14159/2, 3.14159/2*3, 10);

	/* init FB back ground buffer page with working buffer */
        memcpy(gv_fb_dev.map_bk+gv_fb_dev.screensize, gv_fb_dev.map_bk, gv_fb_dev.screensize);

	/* write 00:00:00 */
        FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.bold,          	/* FBdev, fontface */
                                        30, 30, (const unsigned char *)"00:00:00",    	/* fw,fh, pstr */
                                        320-10, 5, 4,                           /* pixpl, lines, gap */
                                        38, 100,                                /* x0,y0, */
                                        WEGI_COLOR_GRAY, -1, -1,      /* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      /* int *cnt, int *lnleft, int* penx, int* peny */
	/* display */
        fb_page_refresh(&gv_fb_dev,0);

        /* init FB 3rd buffer page, with mark 00:00:00  */
        fb_page_saveToBuff(&gv_fb_dev, 2);

	/* Start timer thread */
	printf("Start Timer thread ...\n");
	if( pthread_create(&timer_thread,NULL,timer_process, (void *)&timer1) != 0 )
		printf("Fail to start timer process!\n");


	printf("Start loop ...\n");
while(1) {

	/* check EGI_SHM_MSG: Signal to quit */
        if(shm_msg.msg_data->sigstop ) {
        	printf(" ------ shm_msg: sigstop received! ------ \n");
                /* reset data */
                shm_msg.msg_data->signum=0;
                shm_msg.msg_data->active=false;
                shm_msg.msg_data->sigstop=false;

		/* To join timer thread */
		if(timer1.active) {
			timer1.sigstop=true;
			if( pthread_join(timer_thread, NULL) != 0) {
	                	printf("Fail to joint timer thread!\n");
        		} else {
				printf("Timer thread is joined!\n");
			}
		}

                break;
        }

	/* Check txt data for display  */
	if( pdata[0]=='\0') {
		//printf(" --- \n");
		usleep(200000);
		continue;
	} else {
		printf("shm_msg.msg_data->signum=%d\n", shm_msg.msg_data->signum);
		printf("shm_msg.msg_data->msg: %s\n", shm_msg.msg_data->msg);
        	printf("shm priv data: %s\n", pdata);
	}

	/* Simple NLU: parse input txt data and extract key words */

	/* CASE 1:  Set the timer --- */
	if(  timer1.active==false
	     	&& ( strstr(pdata,"定时") || strstr(pdata,"提醒") || strstr(pdata,"以后") )
          )
	{
		/* Extract time related string */
		memset(strtm,0,sizeof(strtm));
   		if( cstr_extract_ChnUft8TimeStr(pdata, strtm, sizeof(strtm))==0 )
        		printf("Extract time string: %s\n",strtm);

		/* Reset Timer and get time span in seconds */
		timer1.sigstop=false;
		timer1.sigquit=false;
		timer1.active=false;
		/* check ts */
   		timer1.ts=(time_t)cstr_getSecFrom_ChnUft8TimeStr(strtm);
		if(timer1.ts==0) {
			pdata[0]='\0';
			continue;
		}
		/* set sigstart at last! */
		timer1.sigstart=true;

   		printf("Totally %ld seconds.\n",(long)timer1.ts);

		/* Check for actions */


	}
	/* CASE 2:  --- Stop timer alarming sound --- */
	else if( timer1.active==true
		 	&& ( strstr(pdata,"取消") || strstr(pdata,"停止") )
		 	&& ( strstr(pdata,"时") || strstr(pdata,"提醒") )
	        )
	{
		timer1.sigstop=true;
	}
	/* CASE 3:  --- Timer is working! --- */
	else if ( timer1.active==true )
		printf("Timer is busy!\n");


#if 0  /////////////////////////////////////////////////////////////////////////////////////////////////////
	/*  Light size 80x80 */
	if(strstr(pdata,"关")) {
	    if(strstr(pdata,"灯")) {
		if( strstr(pdata,"红") ) {
			printf("Turn off red light!\n");
			draw_filled_rect2(&gv_fb_dev, WEGI_COLOR_GRAY5, 20, 140, 20+80, 140+80);
		}
		if(strstr(pdata,"绿") ) {
			printf("Turn off green light!\n");
			draw_filled_rect2(&gv_fb_dev, WEGI_COLOR_GRAY5, 20+100, 140, 20+100+80, 140+80);
		}
		if(strstr(pdata,"蓝") ) {
			printf("Turn off blue light!\n");
			draw_filled_rect2(&gv_fb_dev, WEGI_COLOR_GRAY5, 20+200, 140, 20+200+80, 140+80);
		}
		if(strstr(pdata,"所有") || strstr(pdata,"全部")) {
			printf("Turn on all lights!\n");
			draw_filled_rect2(&gv_fb_dev, WEGI_COLOR_GRAY5, 20, 140, 20+80, 140+80);
                        draw_filled_rect2(&gv_fb_dev, WEGI_COLOR_GRAY5, 20+100, 140, 20+100+80, 140+80);
                        draw_filled_rect2(&gv_fb_dev, WEGI_COLOR_GRAY5, 20+200, 140, 20+200+80, 140+80);
		}
	    }
	}
#endif	////////////////////////////////////////////////////////////////////////////////////////////////////


	/* reset data */
	pdata[0]='\0';

}

	/* joint timer thread */
	if( pthread_join(timer_thread, NULL) != 0)
               	printf("Fail to cancel timing and joint timer thread!\n");
      	else
		printf("Timer is cancelled!\n");


	/* free Timer data */
	if(timer1.pcmalarm)
		egi_pcmbuf_free(&timer1.pcmalarm);

	/* close shmem */
        egi_shmem_close(&shm_msg);

        /* <<<<<-----------------  EGI general release  ----------------->>>>> */
        printf("FTsymbol_release_allfonts()...\n");
        FTsymbol_release_allfonts();
        printf("symbol_release_allpages()...\n");
        symbol_release_allpages();
        printf("release_fbdev()...\n");
        fb_filo_flush(&gv_fb_dev); /* Flush FB filo if necessary */
        release_fbdev(&gv_fb_dev);
        printf("egi_end_touchread()...\n");
        egi_end_touchread();
        printf("egi_quit_log()...\n");
#if 0
        egi_quit_log();
        printf("<-------  END  ------>\n");
#endif
        return 0;
}



/*---------------------------------
Timer process function
----------------------------------*/
static void *timer_process(void *arg)
{
	long tset=0,ts=0;	/* set seconds, left seconds */
	long tm_end=0; 		/* end time in time_t */
	int hour,min,second;
	char strtm[128];
	float fang;	/* for arc */
	float sang;	/* for second hand */
//	bool enable_tick=false;
	pthread_t tick_thread;

	etimer_t *etimer=(etimer_t *)arg;

	/* Load page without 00:00:00  */
	fb_page_restoreFromBuff(&gv_fb_dev, 1);

   while(1) {
	/* Signal to start timer */
	if( etimer->sigstart && etimer->active==false && etimer->ts>0 ) {

		/* restore screen */
		fb_page_restoreFromBuff(&gv_fb_dev, 1);

		/* get timer set seconds */
		tset=ts=etimer->ts;
		tm_end=time(NULL)+tset;

		/* reset etimer params */
		etimer->active=true;
		etimer->sigstart=false;

		/* start ticking */
		etimer->notick=false;
		if( pthread_create(&tick_thread, NULL, thread_ticking, arg) != 0 )
			printf("%s: Fail to start ticking thread!\n",__func__);
	}
	/* Continue to wait if not active */
	else if(etimer->active==false) {
		egi_sleep(0,0,200);
		continue;
	}

	/* Signal to stop timer */
	if(etimer->sigstop) {
		etimer->notick=true;
		etimer->active=false; /* Just reset active */
		etimer->sigstop=false;

		/* restore screen */
		fb_page_restoreFromBuff(&gv_fb_dev, 2);

		continue;
	}

	/* Finish timing, reset timer */
	if( ts<0 ) {
		etimer->active=false;
		etimer->ts=0;
		continue;
	}

	/* convert to 00:00:00  */
	memset(strtm,0,sizeof(strtm));
	hour=ts/3600;
	min=(ts-hour*3600)/60;
	second=ts-hour*3600-min*60;
	sprintf(strtm,"%02d:%02d:%02d", hour,min,second );

        /*  init FB working buffer page with back ground buffer */
        memcpy(gv_fb_dev.map_bk, gv_fb_dev.map_bk+gv_fb_dev.screensize, gv_fb_dev.screensize);

	/* Draw second ticking hand */
	fbset_color(WEGI_COLOR_GRAY);
	//draw_wline(FBDEV *dev,int x1,int y1,int x2,int y2, unsigned w);
	/* center(100,120) r=80 */
	sang=(90.0-(tset-ts)%60/60.0*360)/180*3.14159;
	draw_wline(&gv_fb_dev, 100, 120, 100+80.0*cos(sang),120-80.0*sin(sang), 5);

	/* Timing Display */
	FTsymbol_uft8strings_writeFB( 	&gv_fb_dev, egi_sysfonts.bold,  	/* FBdev, fontface */
				      	30, 30,(const unsigned char *)strtm, 	/* fw,fh, pstr */
				      	320-10, 5, 4,                    	/* pixpl, lines, gap */
					38, 100,                           	/* x0,y0, */
                                     	WEGI_COLOR_YELLOW, -1, -1,      /* fontcolor, transcolor,opaque */
                                     	NULL, NULL, NULL, NULL);      /* int *cnt, int *lnleft, int* penx, int* peny */

	fbset_color(WEGI_COLOR_ORANGE);
	//fang=-3.14159/2+(2*3.14159)*(tset-ts)/tset;
	fang=3.14159*3/2-2*3.14159*ts/tset;
	draw_arc(&gv_fb_dev, 100, 120, 90, -3.14159/2, fang, 10);

	/* refresh FB */
	fb_page_refresh(&gv_fb_dev,0);

	/* synchronize with ticking */
	//if(ts%4==0)
	//	etimer->ticksynch=true;

	/* sleep and decrement, skip ts==0  */
	while( tm_end-ts >= time(NULL) && ts>0 ) {
		egi_sleep(0,0,200);
	}

	/* Play alarm sound, until sigstop */
	if(ts==0) {
		/* disable/stop ticking */
		etimer->notick=true;
		/* Loop playback, until sigstop */
		egi_pcmbuf_playback(PCM_MIXER, (const EGI_PCMBUF *)etimer->pcmalarm, 1024, 0, &etimer->sigstop, NULL);
	}

	ts--;
  }

	/* restore screen */
	fb_page_restoreFromBuff(&gv_fb_dev, 2);

	/* reset timer */
	etimer->ts=0;
	etimer->active=false;
	etimer->sigstop=false;

	printf("End timer process!\n");
	return (void *)0;
}

/*------------------------------------------
      Sound effect for alarm ticking
-------------------------------------------*/
static void * thread_ticking(void *arg)
{
     etimer_t *etimer=(etimer_t *)arg;

     pthread_detach(pthread_self());
     printf("%s: Start ticking.\n", __func__);
     egi_pcmbuf_playback(PCM_MIXER, (const EGI_PCMBUF *)etimer->pcmtick, 1024, 0, &etimer->notick, NULL); //&etimer->ticksynch);
     printf("%s: End ticking.\n", __func__);

     pthread_exit((void *)0);
}

