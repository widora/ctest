#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "egi_timer.h"
#include "egi_fifo.h"
#include "egi_log.h"

//#include "egi_utils.h"

EGI_FIFO *fifo=NULL;


void fifo_pusher(void)
{
	int i=0;
	while(1)
	{
		//tm_delayms();

		if(egi_push_fifo(fifo, (unsigned char *)(&i), sizeof(int)) ==0)
		{
			printf("push fifo: i=%d\n",i);
			i++;
		}
	}
}

void fifo_puller(void)
{
	int data;

	while(1)
	{
		//tm_delayms(10);
		if(egi_pull_fifo(fifo,(unsigned char *)(&data),sizeof(int)) ==0)
			printf("pull fifo: data=%d\n",data);
	}


}



int main(void)
{

	pthread_t pt_pusher, pt_puller;


        /* --- start egi tick --- */
        tm_start_egitick();

	/* init logger */
  	if(egi_init_log() != 0)
	{
		printf("Fail to init logger,quit.\n");
		return -1;
	}

	/* init fifo */
	fifo=egi_malloc_fifo(64,sizeof(int));
	if(fifo==NULL)
	{
		printf("Fail to init fifo, quit.\n");
		return -1;
	}

	/* create threads */
	pthread_create(&pt_pusher,NULL,(void *)fifo_pusher, NULL);
	pthread_create(&pt_puller,NULL,(void *)fifo_puller, NULL);



#if 0 ////////// test fifo malloc and free  //////
  	while(1)
	{
		fifo=egi_malloc_fifo(256, 256);
		if(fifo==NULL)
		{
			printf("Fail to init fifo, quit.\n");
			return -1;
		}
		tm_delayms(50);
		printf("start egi_free_fifo()...\n");
		egi_free_fifo(fifo);
		printf("Finish one round.\n");

	}
#endif

	/* join threads */
	pthread_join(pt_pusher,NULL);
	pthread_join(pt_puller,NULL);

	/* free fifo */
	egi_free_fifo(fifo);

  	/* quit logger */
  	egi_quit_log();


	return 0;
}

