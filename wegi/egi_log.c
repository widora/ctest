/*----------------------------------------------------------------------------
Try to write a thread_safe log system. :))))

1. Run egi_init_log() first, then push log string into log_buff
   by calling egi_push_log().

2. A egi_log_thread_write() will be running as a thread,it monitors
   log_buff_count and write log_buff items into log file.


Note:
1. log_buff_mutex will not be destroyed after egi_quit_log().
2. Log items are not written  sorted by time, because of buff FILO and thread operation.

TODO:
1. egi_init_log() can be called only once! It's NOT reentrant!!!!
   !!! consider to destroy and re-initiliate log_buff_mutex. !!!
2. sort lof_buff wirte 
3. file write is buffed, fflush...
4  egi_push_log() after when egi_quit_log(), must wait for all egi_push_log().
--------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h> /* va_list, va_start */
#include <pthread.h>
#include "egi_log.h"
#include "egi_timer.h"

static pthread_t log_write_thread;

/* NOT APPLIED YET!:  following variables to be mutex lock protected  by log_buff_mutex */
static pthread_mutex_t log_buff_mutex;
static FILE *egi_log_fp; /* log_buff_mutex lock */
static char **log_buff;  /* log_buff_mutex lock */
static int log_buff_count; /* count number, or number of the first available log buff,log_buffer_mutex lock */
static bool log_is_running;
static bool write_thread_running;

/*-----------------------------------------------------------------------
Push log string to log buff item
String chars that are out of log_buff[] item mem. space will be discarded.

TODO: check log string total length!!!

return:
	0	OK
	<0	Fails
	>0	Log buff is full.
----------------------------------------------------------------------*/
int egi_push_log(const char *fmt, ...)
{
	char strlog[EGI_LOG_MAX_ITEMLEN]={0}; /* for temp. use */

	/* get extended parameters */
	va_list arg;
	va_start(arg,fmt);/* ----- start of extracting extended parameters to arg ... */

	/* get time stamp */
	time_t t=time(NULL);
	struct tm *tm=localtime(&t);

	/* prepare time stamp string */
	sprintf(strlog, "[%d-%02d-%02d %02d:%02d:%02d]  ",
				tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour, tm->tm_min,tm->tm_sec);
	printf("time string for strlog: %s \n",strlog);

	int tmlen=strlen(strlog);

	/* push log string into temp. strlog */
	vsnprintf(strlog+tmlen, EGI_LOG_MAX_ITEMLEN-tmlen-1, fmt, arg); /* -1 for /0 */
	printf("strlog=%s\n",strlog);

	va_end(arg); /* ----- end of extracting extended parameters ... */

   	/* get mutex lock */
   	if(pthread_mutex_lock(&log_buff_mutex) != 0)
   	{
		printf("egi_quit_log():fail to get mutex lock.\n");
		return -1;
   	}

   /* -------------- entering critical zone ---------------- */
	/* check if log_is_running */
	if(!log_is_running)
	{
		printf("%s...%s: egi log is not running! try egi_init_log() first. \n",__FILE__,__FUNCTION__);
		pthread_mutex_unlock(&log_buff_mutex);
		return -1;
	}

	/* check if log_buff overflow */
	else if(log_buff_count>EGI_LOG_MAX_BUFFITEMS-1)
	{
		printf("egi_push_log(): log_buff[] is full, fail to push strlog, unlock mutex and return...\n");
		pthread_mutex_unlock(&log_buff_mutex);
		return 1;
	}

	/* copy log string to log_buf */
	printf("egi_push_log(): start strncpy...  log_buff_count=%d, strlen(strlog)=%d\n",
									log_buff_count, strlen(strlog) );

	memset(log_buff[log_buff_count],0,EGI_LOG_MAX_ITEMLEN); /* clear buff item */
	strncpy(log_buff[log_buff_count],strlog,strlen(strlog));

	/* increase count */
	log_buff_count++;

	/* put mutex lock */
   	pthread_mutex_unlock(&log_buff_mutex);

   /* -------------- exiting critical zone ---------------- */

   	return 0;
}

/*---------------------------------------
It's a thread function running detached.
Note
1. exits when log_is_running is false.
2. flush log file before exit.
---------------------------------------*/
static void egi_log_thread_write(void)
{
	int i;

	/* check log file */
  	if(egi_log_fp==NULL)
  	{
		printf("egi_log_thread_write(): Log file is not open, egi_log_fp is NULL.\n");
		return;
  	}
	/* check log_buff */
  	if(log_buff==NULL || log_buff[EGI_LOG_MAX_BUFFITEMS-1]==NULL)
  	{
		printf("egi_log_thread_write(): log_buff is NULL.\n");
		return;
  	}

	/* loop checking and write log buff to log file */
  	while(1)
  	{
		/* get buff lock */
		if(pthread_mutex_lock(&log_buff_mutex) != 0)
		{
			printf("egi_log_thread_write():fail to get mutex lock.\n");
			tm_delayms(25);
			continue;
			//return;
		}

	 /* -------------- entering critical zone ---------------- */

		/* write log buff items to log file */
		if(log_buff_count>0)
		{
			for(i=log_buff_count;i>0;i--)
			{
				printf("egi_log_thread_write(): start fprintf() log_buff[%d]: %s \n",
											i-1, log_buff[i-1] );
				if( fprintf(egi_log_fp,"%s",log_buff[i-1]) <0 )
					printf("egi_log_thread_write():fail to fprintf() log_buff \n");

			}
			/* reset count */
			log_buff_count=0;
		}

		/* check log_is_running token */
		if( !log_is_running ) /* !!! log_buff[] may already have been freed then */
		{
			printf("egi_log_thread_write(): log_is_running is false, flush log file and exit pthread now...\n");
			/* flush log file */
			//fflush(egi_log_fp); // NO use !!!!
			pthread_mutex_unlock(&log_buff_mutex);
			pthread_exit(0);
		}

		/* put buff lock */
		pthread_mutex_unlock(&log_buff_mutex);

      /* -------------- exiting critical zone ---------------- */

		/* sleep for a while, let other process to get locker */
		tm_delayms(EGI_LOG_WRITE_SLEEPGAP);
   	}

	//pthread_exit(0);
}

/*--------------------------------------------------------------
malloc 2 dimension buff.
char** buff to be regarded as char buff[items][item_len];

return:
	0	OK
	<0	Fails
----------------------------------------------------------------*/
static int egi_malloc_buff2D(char * **buff, int items, int item_len)
{
	int i,j;

	/* check data */
	if( items <= 0 || item_len <= 0 )
	{
		printf("egi_malloc_buff2(): itmes or item_len is illegal.\n");
		return -1;
	}

	/* malloc buff */
	*buff=malloc(items*sizeof(char *));
	if(*buff==NULL)
	{
		printf("egi_malloc_buff2(): fail to malloc buff.\n");
		return -2;
	}

	/* malloc buff items */
	for(i=0;i<items;i++)
	{
		(*buff)[i]=malloc((item_len)*sizeof(char)); /* +1 for string end */
		if((*buff)[i]==NULL)
		{
			printf("egi_malloc_buff2(): fail to malloc buff[%d], free buff and return.\n",i);
			/* free mallocated items */
			for(j=0;j<i;j++)
			{
				free((*buff)[j]);
				(*buff)[j]=NULL;
			}
			free(*buff);
			*buff=NULL;
			return -3;
		}

		/* clear data */
		memset((*buff)[i],0,item_len*sizeof(char));
	}

	return 0;
}

/*--------------------------------------------------------------
free 2 dimension buff.
char** buff to be regarded as char buff[items][item_len];
return:
	0	OK
	<0	Fails
	>0	buff is NULL
----------------------------------------------------------------*/
static int egi_free_buff2D(char **buff, int items, int item_len)
{
	int i;

	/* check data */
	if( items <= 0 || item_len <= 0 )
	{
		printf("egi_malloc_buff2(): itmes or item_len is illegal.\n");
		return -1;
	}

	/* free buff items and buff */
	if( buff == NULL)
	{
		return 1;
	}
	else
	{
		for(i=0;i<items;i++)
			free(buff[i]);
		free(buff);
		buff=NULL;
	}

	return 0;
}

/*-------------------------------------------
1. allocate log buff itmes
2. reset log_count;
3. start to run log_writting thread
4. set log_is_running.

return:
	0	OK
	<0	fails
-------------------------------------------*/
int egi_init_log(void)
{
	int ret=0;

	/* 1. init buff mutex */
	if(pthread_mutex_init(&log_buff_mutex,NULL) != 0)
	{
		printf("egi_init_log(): fail to initiate log_buff_mutex.\n");
		return -1;
	}

	/* 2. malloc log buff */
	if(egi_malloc_buff2D(&log_buff,EGI_LOG_MAX_BUFFITEMS,EGI_LOG_MAX_ITEMLEN)<0)
	{
		printf("egi_init_log(): fail to malloc log_buff.\n");
		goto init_fail;
		return -2;
	}

	printf("egi_init_log(): finish egi_malloc_buff2D().\n");

	/* 3. reset log_buff_count */
	log_buff_count=0;

	/* 4. open log file */
	egi_log_fp=fopen(EGI_LOGFILE_PATH,"a+");
	if(egi_log_fp==NULL)
	{
		printf("egi_init_log():fail to open log file %s\n",EGI_LOGFILE_PATH);
		ret=-3;
		goto init_fail;
	}

	/* 5. run log_writting thread */
	if( pthread_create(&log_write_thread, NULL, (void *)egi_log_thread_write, NULL) !=0 )
	{
		printf("egi_init_log():fail to create pthread for log_write_thread().\n");
		ret=-4;
		goto init_fail;
	}
	printf("egi_init_log(): finish creating pthread log_write_thread().\n");

	/* 6. set log_is_running */
	log_is_running=true;


#if 0 /* test log_buf */
	int i;

	printf("egi_init_log(): test log_buff[] ...\n");

	for(i=0;i<EGI_LOG_MAX_BUFFITEMS;i++)
	{
		/* test 256 bytes log string.. */
		snprintf(log_buff[i],EGI_LOG_MAX_ITEMLEN-1,"---1--------------------- TEST LOG_BUFF: %03d ---\
--2------------------------------------------------\
--3------------------------------------------------\
--4------------------------------------------------\
--5---------------------------------------------0123\n",i);
		//printf("%s\n",log_buff[i]);
		//printf("strlen(log_buff[%d])=%d\n",i,strlen(log_buff[i]) ); /* 254=256-1'\n'-1'\0' */
		fprintf(egi_log_fp,"%s",log_buff[i]);
		fflush(NULL);
	}

#endif


	return 0; /* OK */


init_fail:
	pthread_mutex_destroy(&log_buff_mutex); /* destroy mutex */

	egi_free_buff2D(log_buff,EGI_LOG_MAX_BUFFITEMS,EGI_LOG_MAX_ITEMLEN);

	if(egi_log_fp != NULL)/* close file */
		fclose(egi_log_fp);

	return ret;
}

/*-------------------------------------------------------------------
set log_is_running to false to invoke egi_log_thread_write() to end.

NOte:
1. let egi_log_thread_write() to flush log file.

Return:
	0	OK
	<0	Fails
-------------------------------------------------------------------*/
static int egi_stop_log(void)
{

	/* get mutex lock */
	if(pthread_mutex_lock(&log_buff_mutex) != 0)
	{
		printf("egi_quit_log():fail to get mutex lock.\n");
		return -1;
	}
      /* -------------- entering critical zone ---------------- */
		/* reset log_is_running, to let wirte thread stop
		   ignore all log items in the log_buff. */
		log_is_running=false;
	/* put mutex lock */
	pthread_mutex_unlock(&log_buff_mutex);
      /* -------------- exiting critical zone ---------------- */

	return 0;
}


/*-------------------------------------------------------------------
free all buff 

Return:
	0	OK
	<0	Fails
-------------------------------------------------------------------*/
static int egi_free_logbuff(void)
{

	/* get mutex lock */
	if(pthread_mutex_lock(&log_buff_mutex) != 0)
	{
		printf("egi_free_logbuff():fail to get mutex lock.\n");
		return -1;
	}
      /* -------------- entering critical zone ---------------- */

	/* free log buff */
	if( egi_free_buff2D(log_buff,EGI_LOG_MAX_BUFFITEMS,EGI_LOG_MAX_ITEMLEN) !=0 )
	{
		printf("egi_free_logbuff():fail to free log buff.\n");
		pthread_mutex_unlock(&log_buff_mutex);
		return -2;
	}

	/* put mutex lock */
	pthread_mutex_unlock(&log_buff_mutex);

      /* -------------- exiting critical zone ---------------- */

	return 0;

}


/*-------------------------------------------------------------------
quit log process...

!!!! join thread of egi_log_thread_write()

??? pthread_joint() return 

Return:
	0	OK
	<0	Fails
-------------------------------------------------------------------*/
int egi_quit_log(void)
{
	int ret;

	/* stop log to let egi_log_thread_write() end */
	if( egi_stop_log() !=0 )
	{
		printf("egi_quit_log():fail to stop log.\n");
		return -1;
	}

	/* wait log_write_thread to end writing buff to log file. */

	printf("egi_quit_log(): start pthread_join(log_write_thread)...\n" );
	ret=pthread_join(log_write_thread,NULL);
	if( ret !=0 )
	{
		perror("pthread_join");
		printf("egi_quit_log():fail to join log_write_thread. ret=%d\n",ret);
		return -2;
	}

	/* free log buff */
	if(egi_free_logbuff() !=0 )
	{
		printf("egi_quit_log():fail to free log_buff .\n");
		return -1;
	}

	/* close log file */
	fclose(egi_log_fp);

	return 0;
}
