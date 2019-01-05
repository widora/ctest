#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h> /* usleep */
#include <stdbool.h>
#include "egi_timer.h"
#include "egi_symbol.h"
#include "fblines.h"
#include "dict.h"

struct itimerval tm_val, tm_oval;

char tm_strbuf[50]={0};
const char *str_weekday[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

/* global tick */
struct itimerval tm_tick_val,tm_tick_oval;
long long unsigned int tm_tick_count=0;

/*----------------------------------
 get local time in string in format:
 		H:M:S
------------------------------------*/
void tm_get_strtime(char *tmbuf)
{
	time_t tm_t; /* time in seconds */
	struct tm *tm_s; /* time in struct */

	time(&tm_t);
	tm_s=localtime(&tm_t);

	/*
		tm_s->tm_year start from 1900
		tm_s->tm_mon start from 0
	*/
	sprintf(tmbuf,"%02d:%02d:%02d\n",tm_s->tm_hour,tm_s->tm_min,tm_s->tm_sec);
}


/*----------------------------------
 get local time in string in format:
 	Year_Mon_Day  Weekday
------------------------------------*/
void tm_get_strday(char *tmdaybuf)
{
	time_t tm_t; /* time in seconds */
	struct tm *tm_s; /* time in struct */

	time(&tm_t);
	tm_s=localtime(&tm_t);

	sprintf(tmdaybuf,"%d-%d-%d   %s\n",tm_s->tm_year+1900,tm_s->tm_mon+1,tm_s->tm_mday,\
			str_weekday[tm_s->tm_wday] );
}




/* -----------------------------
 timer routine
-------------------------------*/
void tm_sigroutine(int signo)
{
	if(signo == SIGALRM)
	{
//		printf(" . tick . \n");

	/* ------- routine action every tick -------- */
#if 0	// put heavy action here is not a good ideal ??????  !!!!!!
        /* get time and display */
        tm_get_strtime(tm_strbuf);
        wirteFB_str20x15(&gv_fb_dev, 0, (30<<11|45<<5|10), tm_strbuf, 60, 320-38); 
        tm_get_strday(tm_strbuf);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont,0xffff,45,2,tm_strbuf);
        //symbol_string_writeFB(&gv_fb_dev, &sympg_testfont,0xffff,32,90,tm_strbuf);
#endif

	}

	/* restore tm_sigroutine */
	signal(SIGALRM, tm_sigroutine);
}

/*---------------------------------
set timer for SIGALRM
us: time interval in us.
----------------------------------*/
void tm_settimer(int us)
{
	/* time left before next expiration  */

	tm_val.it_value.tv_sec=0;
	tm_val.it_value.tv_usec=us;
	/* time interval for periodic timer */
	tm_val.it_interval.tv_sec=0;
	tm_val.it_interval.tv_usec=us;

	setitimer(ITIMER_REAL,&tm_val,NULL); /* NULL get rid of old time value */

}

/*-------------------------------------
set timer for SIGALRM of global tick
us: global tick time interval in us.
--------------------------------------*/
void tm_tick_settimer(int us)
{
	/* time left before next expiration  */
	tm_tick_val.it_value.tv_sec=0;
	tm_tick_val.it_value.tv_usec=us;
	/* time interval for periodic timer */
	tm_tick_val.it_interval.tv_sec=0;
	tm_tick_val.it_interval.tv_usec=us;
	/* use real time */
	setitimer(ITIMER_REAL,&tm_tick_val,NULL); /* NULL get rid of old time value */
}

/* -----------------------------
  global tick timer routine
-------------------------------*/
void tm_tick_sigroutine(int signo)
{
	if(signo == SIGALRM)
	{
		tm_tick_count+=1;
	}

	/* restore tm_sigroutine */
	signal(SIGALRM, tm_tick_sigroutine);

}


/*-----------------------------
	return tm_tick_count
------------------------------*/
long long unsigned int tm_get_tickcount(void)
{
	return tm_tick_count;
}

/*-----------------------------------------
delay ms, at lease TM_TICK_INTERVAL/1000 ms

-------------------------------------------*/
void tm_delayms(long ms)
{
	unsigned int nticks;

	if(ms < TM_TICK_INTERVAL/1000)
		nticks=TM_TICK_INTERVAL/1000;
	else
		nticks=ms*1000/TM_TICK_INTERVAL;

	long tm_now=tm_tick_count;

	while(tm_tick_count-tm_now < nticks)
	{
		usleep(1000);

	}
}


/*--------------------------------------------------------
return:
    TRUE: in every preset time interval gap(us), or time
interval exceeds gap(us).
    FALSE: otherwise
---------------------------------------------------------*/
bool tm_pulseus(long long unsigned int gap) /* gap(us) */
{
	static struct timeval tmnew,tmold;

	/* first init tmold */
	if(tmold.tv_sec==0 && tmold.tv_usec==0)
		gettimeofday(&tmold,NULL);

	/* get current time value */
	gettimeofday(&tmnew,NULL);

	/* compare timers */
	if(tmnew.tv_sec*1000000+tmnew.tv_usec >= tmold.tv_sec*1000000+tmold.tv_usec + gap)
	{
		tmold=tmnew;
		return true;
	}
	else
		return false;
}

