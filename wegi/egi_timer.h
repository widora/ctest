#ifndef __EGI_TIMER_H__
#define __EGI_TIMER_H__

#include <sys/time.h>
#include <time.h>


/* shared data */
extern struct itimerval tm_val, tm_oval;
extern char tm_strbuf[];


/* functions */
void tm_get_strtime(char *tmbuf);
void tm_sigroutine(int signo);
void tm_settimer(int us);



#endif
