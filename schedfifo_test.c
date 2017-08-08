//--refer to:  www.169it.com/tech-qa-linux/article-9475173645702120038.html
//--


#include <stdio.h> // perror()
#include <pthread.h> // pthread
#include <sched.h>  //SCHED_FIFO

//void thd1(void* lpparam)
static void thd1(void* lpparam)
{
int my_policy;
struct sched_param my_param;
//--------- check SCHEDULER type -----
pthread_getschedparam(pthread_self(),&my_policy,&my_param);
printf("thread_routine running at %s%d\n", \
	(my_policy == SCHED_FIFO ? "FIFO" \
	:(my_policy == SCHED_RR ? "RR" \
	:(my_policy == SCHED_OTHER ? "OTHER" \
	:"unknown"))),
	my_param.sched_priority);

//------------------------------------
	printf("----- thd1 parameter:%s\n",(char*)lpparam);
	sleep(1); //--sleep so main() has time to create thd2
	printf("----------start thd1\n"); //--print wanted result
	while(1)
		printf("--------------------------------------this is thd1!\n");
}

//void thd2(void* lpparam)
static void thd2(void* lpparam)
{

	printf("----- thd2 parameter:%s\n",(char*)lpparam);
	sleep(3);//--to occupy CUP after thd1 running for 2 seconds
        printf("----------start thd2 !\n"); //--print wanted result
	while(1){
		  //sleep(1); //---sleep here will let thd1 gain the preemption
		  //printf("----------thd2 preempt!\n"); //----printf(sleep embedded) will yield preemptiong !!!!!!  wanted result 
		}
}


int create_fifo_thd(void *routine,int dwPriority, void* param)
{
	pthread_attr_t Attr;
	pthread_attr_init(&Attr);
	pthread_attr_setinheritsched(&Attr,PTHREAD_EXPLICIT_SCHED);//!!!must set before setschedpolicy()
	pthread_attr_setschedpolicy(&Attr,SCHED_FIFO);
	pthread_attr_setscope(&Attr,PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&Attr,PTHREAD_CREATE_DETACHED);

	struct sched_param sch_param;
	sch_param.sched_priority=dwPriority;
	printf("FIFO priority =%d\n",sch_param.__sched_priority);
	pthread_attr_setschedparam(&Attr,&sch_param);
	pthread_t pid=0;
	pthread_create(&pid,&Attr,routine,(void *)param);

        //-----it's not necessary
	//if(pthread_setschedparam(pid,SCHED_FIFO,&sch_param)!=0)
	//printf("pthread_setschedparam failed\n");

	pthread_attr_destroy(&Attr);
	
	printf("pid=%d\n",(int)pid);
	return pid;
}


int main()
{
char param1[30]="this is param-1";
char param2[30]="this is param-2";

int my_policy;
struct sched_param my_param;
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
	pthread_getschedparam(pthread_self(),&my_policy,&my_param);
	printf("thread_routine running at %s%d\n", \
		(my_policy == SCHED_FIFO ? "FIFO" \
		:(my_policy == SCHED_RR ? "RR" \
		:(my_policy == SCHED_OTHER ? "OTHER" \
		:"unknown"))),
		my_param.sched_priority);
#else
	printf("POSIX_THREAD_PRIORITY_SCHEDULING not defined! thread routine running!\n");
#endif	


	int ret=0;
	ret=create_fifo_thd(thd1,11,(void *)param1); //thread1 FIFO priority 11
	if(0==ret){
		printf("%s :fail to create thread-1!\n",strerror(ret));
		return 0;
		}

	int maxpi;
	maxpi=sched_get_priority_max(SCHED_FIFO);
	//printf("sched get priority max:%d\n",maxpi);
	if(maxpi == -1){
		perror("sched_get_priority_max() failed\n");
	}

	ret=create_fifo_thd(thd2,maxpi,(void *)param2); //thread1 FIFO priority 
	if(0==ret){
		printf("fail to create thread-2!\n");
		return 0;
	}

	printf("pthreads create ok!\n");
	while(1)sleep(1);
}
