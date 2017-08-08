#include <signal.h>
#include <sys/time.h>
#include <stdio.h>

int handle_count;

void set_timer(void)
{
 struct itimerval itv;
 itv.it_interval.tv_sec=10; //--this value will be reloaded to it_value  automatically once  countdown finish.
 itv.it_interval.tv_usec=0;
 itv.it_value.tv_sec=5; //first set count-down value, it will auto.set to it_interval_sec once countdown finish
 itv.it_value.tv_usec=0;
 if(setitimer(ITIMER_REAL,&itv,NULL)<0)printf("Fail to set timer\n");
 /*
 ITIMER_REAL decrements in real time, and delivers SIGALRM upon expiration
 ITIMER_VIRTUAL decrements only when the process is executing,and delivers SIGALRM
 ITIMER_PROF decrements both when the process executes and when the system is exectuing on behalf of the porcess
*/

}

void alarm_handle(int sig)
{
  handle_count++;
  printf("Handle_count: %d \015",handle_count);
  fflush(stdout);
}


void main(void)
{
  struct itimerval itv;
  signal(SIGALRM,alarm_handle);
  set_timer();
  printf("--------------------- timer --------------------------\n");
  fflush(stdout);

  while(1)
  {

     getitimer(ITIMER_REAL,&itv);
     printf("Alarm count:%d             pass second is %d \015",handle_count,(int)itv.it_value.tv_sec);
     fflush(stdout);

     sleep(1);

   }  

}
