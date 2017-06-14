#include <stdio.h>
#include <pthread.h>
//#include <unistd.h> // for what ??
#include <stdlib.h> //for exit()
#include <sys/time.h>

#define  MUXNUMBER 10

pthread_mutex_t test_mutex; //--mutual exclusion locker
int testi=0;
int testis[10*1000];
int count=0;

struct timeval t_start,t_end;
long cost_timeus=0;
long cost_times=0;

void fillin_testis(void)
{
  int j;
  testis[testi]=2*testi;
  //usleep(100);
  testi++;
  for(j=0;j<10000;j++);

}

void thread_func()
{
  int m_count=0;
  while(m_count<1000)  //--every thread write 1000 times to testi[] 
  {
    pthread_mutex_lock(&test_mutex);  //--lock before write to public array
    fillin_testis();
    pthread_mutex_unlock(&test_mutex);
    m_count++;
  }
}

int main()
{
 pthread_t  pt[10];
 pthread_mutex_init(&test_mutex,NULL);
 int i,j,token;

//------- fillin  array and calculate time----
 gettimeofday(&t_start,NULL);
   for(i=0;i<10000;i++)
     {  
      testis[i]=i*2;
      for(j=0;j<10000;j++);
     }

 gettimeofday(&t_end,NULL);
 cost_times=t_end.tv_sec-t_start.tv_sec;
 cost_timeus=t_end.tv_usec-t_start.tv_usec;
 printf("Cost time 1: %ld s+ %ld us\n",cost_times,cost_timeus);


 gettimeofday(&t_start,NULL);
//-------- create threads----
 for(i=0;i<MUXNUMBER;i++)
  {
    if(pthread_create(&pt[i],NULL,(void*)thread_func,NULL)<0)
      {
        printf("Create threads error! \n");
        exit(1);
      }
  }
//-------- pthread join ----  
 for(i=0;i<MUXNUMBER;i++)
  {
    pthread_join(pt[i],NULL);
  }

 pthread_mutex_destroy(&test_mutex);

//------------- cal time cost ------
 gettimeofday(&t_end,NULL);
 cost_times=t_end.tv_sec-t_start.tv_sec;
 cost_timeus=t_end.tv_usec-t_start.tv_usec;
 printf("Cost time 2: %ld s+ %ld us\n",cost_times,cost_timeus);


//------ test result-----
/*
  token=0;
  for(i=0;i<10000;i++)
  {
    if(testis[i]!=i*2)
     {
        printf("testis[%d]=%d is error!\n",i,testis[i]);
        token=1;
      }
  }

  if(token==0){
     printf("Great! no error ocurred!\n");
    for(i=1000;i<1121;i++)
        printf("testis[%d]=%d \n",i,testis[i]);}
*/       

  return 0;
}   
    


