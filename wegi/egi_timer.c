#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include "egi_timer.h"

struct itimerval tm_val, tm_oval;
char tm_strbuf[50]={0};


/*----------------------------------
 get local time in string in format:
 	Year Mon Day H:M:S
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
#if 0
	printf("%d-%d-%d %02d:%02d:%02d\n",tm_s->tm_year+1900,tm_s->tm_mon+1,tm_s->tm_mday,\
			tm_s->tm_hour,tm_s->tm_min,tm_s->tm_sec);
#endif
	sprintf(tmbuf,"%d %d %d %02d:%02d:%02d\n",tm_s->tm_year+1900,tm_s->tm_mon+1,tm_s->tm_mday,\
			tm_s->tm_hour,tm_s->tm_min,tm_s->tm_sec);

}

/* -----------------------------
 timer routine
-------------------------------*/
void tm_sigroutine(int signo)
{
	if(signo == SIGALRM)
	{
//		printf(" . tick . \n");

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



