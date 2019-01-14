#ifndef __EGI_TIMER_H__
#define __EGI_TIMER_H__

#include <sys/time.h>
#include <time.h>



#define TM_TICK_INTERVAL	2000 //5000  /* us */
#define TM_DBCLICK_INTERVAL	300000 /*in us,  Max for double click   */

/* shared data */
extern struct itimerval tm_val, tm_oval;
extern char tm_strbuf[];


/* functions */
void tm_get_strtime(char *tmbuf);
void tm_get_strday(char *tmdaybuf);
void tm_sigroutine(int signo);
void tm_settimer(int us);
void tm_tick_settimer(int us);
void tm_tick_sigroutine(int signo);
long long unsigned int tm_get_tickcount(void);
void tm_delayms(long ms);/* !!! To be abandoned !!!! Not good! */
bool tm_pulseus(long long unsigned int gap); /* gap(us) */
long tm_diffus(struct timeval t_start, struct timeval t_end);

#endif
