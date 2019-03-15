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
	int k;
	int i=0;

	/* first, push some data */
	for(i=0;i<64;i++)
	{
		if( egi_push_fifo(fifo, (unsigned char *)(&i), sizeof(int)) !=0 )
			i--;
	}

	while(1)
	{
//		tm_delayms(30);

		if( egi_push_fifo(fifo, (unsigned char *)(&i), sizeof(int) ) == 0)
		{
			//printf("push fifo: i=%d\n",i);
			printf("Push fifo: ------ OK, i=%d -------\n", i);
			i++;
		}

	}
}

void fifo_puller(void)
{
	int data;
	int i=0;
	int miss;

	while(1)
	{
//		tm_delayms(15);
		if(egi_pull_fifo(fifo, (unsigned char *)(&data), sizeof(int) ) !=0 )
			continue;

		printf("pull fifo: data=%d\n",data);

		if( data != i )
		{
			/* normal only overrun, if NOT */
			if( ((data-i)<<(32-9))!=0 )   // 2**9=512
			     EGI_PLOG(LOGLV_CRITICAL, "--- data-i != 512  data=%d, i=%d, ahead=%d --------\n",
											data, i, fifo->ahead);

			if( (data-i)<512 )
			     EGI_PLOG(LOGLV_CRITICAL, " ========= data-i< 512  data=%d, i=%d, ahead=%d ============n",
											data, i, fifo->ahead);
/*
			if( (data-i > 10) || (i-data > 10) )
			{
			   EGI_PLOG(LOGLV_ERROR," <<<<<< data crash: data=%d, i=%d  ahead=%d >>>>>>>\n",
											data, i, fifo->ahead);
			}

			if( data > i )
				miss += data-i;
			else
				miss += i-data;

*/
			miss++;
//			if( ( miss<<(32-8) ) ==0 )
//				EGI_PLOG(LOGLV_WARN,"Pull fifo: ------ miss i=%d  total_miss: %d -------\n", i,miss);

			/* reset i */
			i=data;
		}
		else
			printf("Pull fifo: ------ OK, i=%d -------\n", data);

		i++;
	}


}



int main(void)
{

	pthread_t pt_pusher, pt_puller;


        /* --- start egi tick --- */
        tm_start_egitick();

	/* init logger */
  	if(egi_init_log("/mmc/log_fifo") != 0)
	{
		printf("Fail to init logger,quit.\n");
		return -1;
	}

	/* init fifo */
	fifo=egi_malloc_fifo(512,sizeof(int));
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

