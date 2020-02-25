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
	test_timer "取消提醒"

3. To incoorperate:
   screen -dmS TXTSVR ./txt_timer -s
   autorec_timer  /tmp/asr_snd.pcm  ( autorec_timer ---> asr_timer.sh ---> txt_timer(test_timer)  )

4. Two sound playing threads


Midas Zhou
--------------------------------------------------------------------*/
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include "egi_common.h"
#include "egi_cstring.h"
#include "egi_FTsymbol.h"
#include "egi_gif.h"
#include "egi_shmem.h"
#include "egi_pcm.h"

#define SHMEM_SIZE 		(1024*4)
#define TXTDATA_OFFSET		1024
#define TXTDATA_SIZE 		1024
#define DEFAULT_ALARM_SOUND	"/mmc/alarm.wav"
#define DEFAULT_TICK_SOUND	"/mmc/tick.wav"
#define TIMER_BKGIMAGE		"/mmc/linux.jpg"
#define PCM_MIXER		"mymixer"

/* Timer data struct */
typedef struct {
	time_t  ts;  		/*  time span/duration value for countdown, typedef long time_t   */
	time_t  tp;		/*  preset time point, tnow+ts=tp  */

	bool	statActive;	/*  status/indicator: true while timer is ticking  */
	bool	sigStart;	/*  signal: to start timer */
	bool	sigStop;	/*  signal: to stop timing and/or alarming  */
	bool    sigQuit;	/*  signal: to quit the timer thread */

	EGI_PCMBUF *pcmalarm;   /*  for alarm sound effect */
	EGI_PCMBUF *pcmtick;    /*  for ticking effect 	*/
	bool	tickSynch;	/*  signal: for synchronizing ticking sound */
	bool	sigNoTick;	/*  signal: to stop ticking sound */
	bool	statTick;	/*  status/indicator: true while playing pcmtick */
	bool	sigNoAlarm;	/*  signal: to stop alarming */
	bool	statAlarm;	/*  status/indicator: true while playing pcmalarm */

} etimer_t;
etimer_t timer1={0,0};

/* timer function */
static void *timer_process(void *arg);
static void *thread_ticking(void *arg);
static void *thread_alarming(void *arg);
static void writefb_datetime(void);

/* lights */
static EGI_16BIT_COLOR light_colors[]={ WEGI_COLOR_BLUE, WEGI_COLOR_RED, WEGI_COLOR_GREEN };


/*=====================================
		MAIN  PROG
======================================*/
int main(int argc, char **argv)
{
	int dts=1; /* Default displaying for 1 second */
	int opt;
	int i;
	double sang;
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
                           printf("	text 	TXT as for command \n");
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
                           printf("Signal to quit timer server...\n");
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


/* >>>>>>>>>>>>>>>   Start Client Job  <<<<<<<<<<<<<<< */
        if( Is_Server==false )
	{
	  if(shm_msg.msg_data->active ) {         /* If msg_data is ACTIVE already */
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
	    else {	/* If msg_data is ACTIVE already */
		printf("Server is already down!\n");
		exit(1);
	   }
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
        printf("symbol_load_allpages()...\n");
        if(symbol_load_allpages() !=0 ) {       /* load sys fonts */
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
#endif


        printf("FTsymbol_load_allpages()...\n");
        if(FTsymbol_load_allpages() !=0 ) {
		printf("Fail to load FTsym pages,quit.\n");
		return -2;
	}

        printf("symbol_load_sysfonts()...\n");
        if(FTsymbol_load_sysfonts() !=0 ) {     /* load FT fonts LIBS */
                printf("Fail to load FT appfonts, quit.\n");
                return -2;
        }

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
        EGI_IMGBUF *eimg=egi_imgbuf_readfile(TIMER_BKGIMAGE);
        if(eimg==NULL)
                printf("Fail to read and load file '%s'!", TIMER_BKGIMAGE);
        egi_imgbuf_windisplay( eimg, &gv_fb_dev, -1,
                               0, 0, 0, 0,
                               gv_fb_dev.pos_xres, gv_fb_dev.pos_yres );

	/* draw lights */
	for(i=0; i<3; i++)
		draw_filled_rect2(&gv_fb_dev, light_colors[i], 185+i*(30+15), 200, 185+30+i*(30+15), 200+30);

	/* Draw timing circle */
	fbset_color(WEGI_COLOR_GRAY3);//LTGREEN);
	/* FBDEV *dev, int x0, int y0, int r, float Sang, float Eang, unsigned int w */
	draw_arc(&gv_fb_dev, 100, 120, 90, -MATH_PI/2, MATH_PI/2*3, 10);
	fbset_color(WEGI_COLOR_GRAY);
	draw_arc(&gv_fb_dev, 100, 120, 80, -MATH_PI/2, MATH_PI/2*3, 10);


	/* Draw second_mark circle */
	fbset_color(WEGI_COLOR_WHITE);
	for(i=0; i<60; i++) {
		sang=(90.0-1.0*i/60.0*360)/180*MATH_PI;
		/* FBDEV *dev,int x1,int y1,int x2,int y2, unsigned w */
		draw_wline_nc(&gv_fb_dev, 100+75.0*cos(sang), 120-75.0*sin(sang), 100+85.0*cos(sang), 120-85.0*sin(sang), 3);
	}

	/* init FB back ground buffer page with working buffer */
        memcpy(gv_fb_dev.map_bk+gv_fb_dev.screensize, gv_fb_dev.map_bk, gv_fb_dev.screensize);

	/* write 00:00:00 */
        FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.bold,          	/* FBdev, fontface */
                                        30, 30, (const unsigned char *)"00:00:00",    	/* fw,fh, pstr */
                                        320-10, 5, 4,                           /* pixpl, lines, gap */
                                        38, 100,                                /* x0,y0, */
                                        WEGI_COLOR_GRAY, -1, -1,      	/* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL  );      /* int *cnt, int *lnleft, int* penx, int* peny */
	/* display */
        fb_page_refresh(&gv_fb_dev,0);

        /* init FB 3rd buffer page, with mark 00:00:00  */
        fb_page_saveToBuff(&gv_fb_dev, 2);

	/* Start timer thread */
	printf("Start Timer thread ...\n");
	if( pthread_create(&timer_thread,NULL,timer_process, (void *)&timer1) != 0 ) {
		printf("Fail to start timer process!\n");
		exit(-1);
	}

	printf("Start timer control service loop ...\n");
while(1) {

	/* check EGI_SHM_MSG: Signal to quit */
        if(shm_msg.msg_data->sigstop ) {
        	printf(" ------ shm_msg: sigstop received! ------ \n");
                /* reset data */
                shm_msg.msg_data->signum=0;
                shm_msg.msg_data->active=false;
                shm_msg.msg_data->sigstop=false;

		break;
	}

	/* Check txt data for display  */
	if( pdata[0]=='\0') {
		//printf(" --- \n");
		//usleep(200000);
		egi_sleep(0,0,100);
		continue;
	} else {
		printf("shm_msg.msg_data->signum=%d\n", shm_msg.msg_data->signum);
		printf("shm_msg.msg_data->msg: %s\n", shm_msg.msg_data->msg);
        	printf("shm priv data: %s\n", pdata);
	}

	/* Simple NLU: parse input txt data and extract key words */

	/* CASE 1:  Set the timer --- */
	if(  timer1.statActive==false
	     	&& ( strstr(pdata,"定时") || strstr(pdata,"提醒") || strstr(pdata,"以后") )
          )
	{
		/* Extract time related string */
		memset(strtm,0,sizeof(strtm));
   		if( cstr_extract_ChnUft8TimeStr(pdata, strtm, sizeof(strtm))==0 )
        		printf("Extract time string: %s\n",strtm);

		/* Reset Timer and get time span in seconds */
		timer1.sigStop=false;
		timer1.sigQuit=false;
		timer1.statActive=false;
		/* check ts */
   		timer1.ts=(time_t)cstr_getSecFrom_ChnUft8TimeStr(strtm);
		if(timer1.ts==0) {
			pdata[0]='\0';
			continue;
		}
		/* set sigStart at last! */
		timer1.sigStart=true;

   		printf("Totally %ld seconds.\n",(long)timer1.ts);

		/* Check for actions */


	}
	/* CASE 2:  --- Stop timer alarming sound --- */
	else if( timer1.statActive==true
		 	&& ( strstr(pdata,"结束") || strstr(pdata,"取消") || strstr(pdata,"停止") )
		 	&& ( strstr(pdata,"时") || strstr(pdata,"提醒")  || strstr(pdata,"定时") || strstr(pdata,"闹钟") || strstr(pdata,"响") )
	        )
	{
		timer1.sigStop=true;
	}
	/* CASE 3:  --- Timer is working! --- */
	else if ( timer1.statActive==true )
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
	printf("Try to join timer_thread...\n");
	timer1.sigStop=true; /* To end loop playing alarm PCM first if it's just doing so! ... */
	timer1.sigQuit=true;
	if( pthread_join(timer_thread, NULL) != 0)
               	printf("Fail to joint timer thread!\n");
       	else
		printf("Timer thread is joined!\n");


	/* free Timer data */
	printf("Try to free pcm data in etimer ...\n");
	if(timer1.pcmalarm)
		egi_pcmbuf_free(&timer1.pcmalarm);
	if(timer1.pcmtick)
		egi_pcmbuf_free(&timer1.pcmtick);

	/* close shmem */
	printf("Closing shmem...\n");
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
	long 	tset=0;			/* set seconds */
	long 	ts=0;			/* left seconds */
	time_t 	tm_end=0; 		/* end time in time_t */
	struct 	tm *localtm_end;	/* localt time in struct tm */
	int 	hour,min,second;
	char 	strCntTm[128];		/* time for countdown */
	char 	strSetTm[128];		/* preset time, in local time */
	float 	fang;			/* for arc */
	float 	sang;			/* for second hand */
	bool 	once_more=false;  	/* init as false */
	pthread_t tick_thread;
	pthread_t alarm_thread;
	etimer_t *etimer=(etimer_t *)arg;
	EGI_POINT tripts[3]={ {92, 132 }, {92+16, 132}, {92+16/2, 132+16} }; /* a triangle mark */

	/* Load page with 00:00:00  */
	fb_page_restoreFromBuff(&gv_fb_dev, 2);

   while(1) {
	/* Signal to quit */
	if( etimer->sigQuit==true ) {
		printf("%s:Signaled to quit timer thread!\n",__func__);
		/* Try to stop ticking */
		etimer->sigNoTick=true;
		if(etimer->statTick) {
			if(pthread_join(tick_thread,NULL)!=0)
				printf("%s:Fail to join tick_thread!\n",__func__);
			else
				etimer->statTick=false;
		}
		/* Try to stop alarming */
		etimer->sigNoAlarm=true;
		if(etimer->statAlarm) {
			if(pthread_join(alarm_thread,NULL)!=0)
				printf("%s:Fail to join alarm_thread!\n",__func__);
			else
				etimer->statAlarm=false;
		}

		etimer->sigQuit=false;
		break;
	}
	/* Signal to start timer */
	else if( etimer->sigStart && etimer->statActive==false && etimer->ts>0 ) {

		/* restore screen */
		fb_page_restoreFromBuff(&gv_fb_dev, 1);

		/* get timer set seconds */
		tset=ts=etimer->ts;
		tm_end=time(NULL)+tset;

		/* convert to preset time point,in local time */
		localtm_end=localtime(&tm_end);
		sprintf(strSetTm,"%02d:%02d:%02d",localtm_end->tm_hour, localtm_end->tm_min, localtm_end->tm_sec);

		/* reset etimer params */
		etimer->statActive=true;
		etimer->sigStart=false;

		/* start ticking */
		etimer->sigNoTick=false;
		if( pthread_create(&tick_thread, NULL, thread_ticking, arg) != 0 )
			printf("%s: Fail to start ticking thread!\n",__func__);
		else
			etimer->statTick=true;
	}
	/* Continue to wait if not active */
	else if(etimer->statActive==false) {
		/* restore back */
	        memcpy(gv_fb_dev.map_bk, gv_fb_dev.map_bk+2*gv_fb_dev.screensize, gv_fb_dev.screensize);

		/* Display current time */
		writefb_datetime();

		/* refresh FB */
		fb_page_refresh(&gv_fb_dev,0);

		egi_sleep(0,0,100);
		continue;
	}

	/* Signal to stop timer */
	if(etimer->sigStop) {
		/* Try to stop ticking */
		etimer->sigNoTick=true;
		if(etimer->statTick) {
			if(pthread_join(tick_thread,NULL)!=0)
				printf("%s:Fail to join tick_thread!\n",__func__);
			else
				etimer->statTick=false;
		}
		/* Try to stop alarming */
		etimer->sigNoAlarm=true;
		if(etimer->statAlarm) {
			if(pthread_join(alarm_thread,NULL)!=0)
				printf("%s:Fail to join alarm_thread!\n",__func__);
			else
				etimer->statAlarm=false;
		}

		/* Reset Timer signals and indicators */
		etimer->statActive=false;
		etimer->sigStop=false;
		etimer->sigNoTick=false;
		etimer->sigNoAlarm=false;

		/* restore screen */
		fb_page_restoreFromBuff(&gv_fb_dev, 2);

		continue;
	}

	/* convert to 00:00:00  */
	memset(strCntTm,0,sizeof(strCntTm));
	hour=ts/3600;
	min=(ts-hour*3600)/60;
	second=ts-hour*3600-min*60;
	sprintf(strCntTm,"%02d:%02d:%02d", hour,min,second );

        /*  init FB working buffer page with back ground buffer */
        memcpy(gv_fb_dev.map_bk, gv_fb_dev.map_bk+gv_fb_dev.screensize, gv_fb_dev.screensize);

	/* Draw second ticking hand */
	fbset_color(WEGI_COLOR_GREEN);
	/* center(100,120) r=80 */
	sang=(90.0-(tset-ts)%60/60.0*360)/180*MATH_PI;
	if(once_more){
		sang += 360.0/(2*60)/180*MATH_PI; /* for 0.5s */
	}
	/* FBDEV *dev,int x1,int y1,int x2,int y2, unsigned w */
	draw_wline_nc(&gv_fb_dev, 100+65*cos(sang), 120-65*sin(sang), 100+85.0*cos(sang),120-85.0*sin(sang), 7);

	/* Timing Display */
        FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.bold,          /* FBdev, fontface */
                                        20, 20,(const unsigned char *)"剩余时间",    /* fw,fh, pstr */
                                        320-10, 1, 4,                           /* pixpl, lines, gap */
                                        60, 68,                                /* x0,y0, */
                                        WEGI_COLOR_WHITE, -1, -1,      /* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      /* int *cnt, int *lnleft, int* penx, int* peny */
	/* Write countdown time */
	FTsymbol_uft8strings_writeFB( 	&gv_fb_dev, egi_sysfonts.bold,  	/* FBdev, fontface */
				      	28, 28,(const unsigned char *)strCntTm, 	/* fw,fh, pstr */
				      	320-10, 5, 4,                    	/* pixpl, lines, gap */
					40, 98,                          	/* x0,y0, */
                                     	WEGI_COLOR_ORANGE, -1, -1,      /* fontcolor, transcolor,opaque */
                                     	NULL, NULL, NULL, NULL);      /* int *cnt, int *lnleft, int* penx, int* peny */


	#if 0
        FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.bold,          /* FBdev, fontface */
                                        16, 16,(const unsigned char *)"设定时间",    /* fw,fh, pstr */
                                        320-10, 1, 4,                           /* pixpl, lines, gap */
                                        67, 132,                                /* x0,y0, */
                                        WEGI_COLOR_GRAY, -1, -1,      /* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      /* int *cnt, int *lnleft, int* penx, int* peny */

        FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.bold,          /* FBdev, fontface */
                                        18, 18,(const unsigned char *)strSetTm,    /* fw,fh, pstr */
                                        320-10, 1, 4,                           /* pixpl, lines, gap */
                                        62, 150,                                /* x0,y0, */
                                        WEGI_COLOR_GRAY, -1, -1,      /* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      /* int *cnt, int *lnleft, int* penx, int* peny */
	#else
	/* Write preset local time */
        FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.regular,          /* FBdev, fontface */
                                        20, 20,(const unsigned char *)strSetTm,    /* fw,fh, pstr */
                                        320-10, 1, 4,                           /* pixpl, lines, gap */
                                        60, 150,                                /* x0,y0, */
                                        WEGI_COLOR_ORANGE, -1, -1,      /* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      /* int *cnt, int *lnleft, int* penx, int* peny */

	#endif

	if(once_more) {
		fbset_color(WEGI_COLOR_GREEN);
		draw_filled_triangle(&gv_fb_dev, tripts); /* draw a triangle mark */
	}

	fbset_color(WEGI_COLOR_ORANGE);
	//fang=-MATH_PI/2+(2*MATH_PI)*(tset-ts)/tset;
	fang=MATH_PI*3/2-2*MATH_PI*ts/tset;
	draw_arc(&gv_fb_dev, 100, 120, 90, -MATH_PI/2, fang, 10);

	/* Display current time H:M:S */
	writefb_datetime();

	/* refresh FB */
	fb_page_refresh(&gv_fb_dev,0);

	/* --------- while alraming, loop back -------- */
	if(etimer->statAlarm && ts==0 )
		continue;

	/* synchronize with ticking */
	//if(ts%4==0)
	//	etimer->tickSynch=true;

	/* Refresh 2 times for 1 second */
	if(once_more) {
		egi_sleep(0,0,400);
		once_more=false;
		continue;
	} else {
		once_more=true;
	}


	/* Wait for next second, sleep and decrement, skip ts==0  */
	while( tm_end-ts >= time(NULL) && ts>0 ) {
		egi_sleep(0,0,100);
	}

	/* Stop ticking and start to alarm, skip while alarming  */
	if(ts==0 && etimer->statAlarm==false ) {
		/* Disable/stop ticking first */
		etimer->sigNoTick=true;
		if(pthread_join(tick_thread,NULL)!=0)
			printf("%s:Fail to join tick_thread!\n",__func__);
		else
			etimer->statTick=false;
		etimer->sigNoTick=false; /* reset sig */

		/* start alarming */
		if( pthread_create(&alarm_thread, NULL, thread_alarming, arg) != 0 )
			printf("%s: Fail to start alarming thread!\n",__func__);
		else
			etimer->statAlarm=true;

	}

	/* ts decrement */
	ts--;
	if(ts<0)ts=0; /* Loop for displaying 00:00:00 */
  }

	/* restore screen */
	fb_page_restoreFromBuff(&gv_fb_dev, 2);

	/* reset timer */
	etimer->ts=0;
	etimer->tp=0;
	etimer->statActive=false;
	etimer->statTick=false;
	etimer->statAlarm=false;
	etimer->sigStop=false;
	etimer->sigQuit=false;
	etimer->tickSynch=false;
	etimer->sigNoTick=false;
	etimer->sigNoAlarm=false;

	printf("End timer process!\n");

	return (void *)0;
}

/*------------------------------------------
      Sound effect for timer ticking
-------------------------------------------*/
static void *thread_ticking(void *arg)
{
     etimer_t *etimer=(etimer_t *)arg;

//    pthread_detach(pthread_self());  To avoid multimixer error
     egi_pcmbuf_playback(PCM_MIXER, (const EGI_PCMBUF *)etimer->pcmtick, 1024, 0, &etimer->sigNoTick, NULL); //&etimer->tickSynch);

//     pthread_exit((void *)0);
	return (void *)0;
}


/*------------------------------------------
      Sound effect for timer alarming
-------------------------------------------*/
static void *thread_alarming(void *arg)
{
     etimer_t *etimer=(etimer_t *)arg;

//    pthread_detach(pthread_self());  To avoid multimixer error
     egi_pcmbuf_playback(PCM_MIXER, (const EGI_PCMBUF *)etimer->pcmalarm, 1024, 0, &etimer->sigNoAlarm, NULL);

//     pthread_exit((void *)0);
	return (void *)0;
}



/*------------------------------------
     Write date and time to FB
-------------------------------------*/
static void writefb_datetime(void)
{
	char strHMS[32];  	/* Hour_Min_Sec */
	char ustrMD[64]; 	/* Mon_Day in uft-8 chn */

       	tm_get_strtime(strHMS);
	tm_get_ustrday(ustrMD);

	#if 0
        symbol_string_writeFB(&gv_fb_dev, &sympg_ascii, WEGI_COLOR_LTBLUE,	/* sympg_ascii abt.18x18 */
                                       SYM_FONT_DEFAULT_TRANSPCOLOR, 215, 165, strHMS, -1);
	#else
       	FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.bold,          /* FBdev, fontface */
               	                        20, 20,(const unsigned char *)strHMS,   /* fw,fh, pstr */
                       	                320-10, 1, 4,                           /* pixpl, lines, gap */
                               	        205, 165,                         /* x0,y0, */
                                       	WEGI_COLOR_GRAYB, -1, -1,      	  /* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      	  /* int *cnt, int *lnleft, int* penx, int* peny */
	#endif

       	FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.bold,          /* FBdev, fontface */
               	                        20, 20,(const unsigned char *)ustrMD,   /* fw,fh, pstr */
                       	                320-10, 1, 4,                           /* pixpl, lines, gap */
                               	        214, 140,                         /* x0,y0, */
                                       	WEGI_COLOR_GRAYB, -1, -1,      	  /* fontcolor, transcolor,opaque */
                                        NULL, NULL, NULL, NULL);      	  /* int *cnt, int *lnleft, int* penx, int* peny */
}
