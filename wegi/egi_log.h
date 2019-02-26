#ifndef __EGI_LOG__
#define __EGI_LOG__

#include <stdio.h>
#include <stdlib.h>

#define EGI_LOGFILE_PATH "/tmp/egi_log"

#define ENABLE_LOGBUFF_PRINT 	1	/* enable to print log buff content */
#define EGI_LOG_MAX_BUFFITEMS	128 	/* MAX. number of log buff items */
#define EGI_LOG_MAX_ITEMLEN	256 	/* Max length for each log string item */
#define EGI_LOG_WRITE_SLEEPGAP	10  	/* in ms, sleep gap between two buff write session in egi_log_thread_write() */
#define EGI_LOG_QUITWAIT 	55 	/* in ms, wait for other thread to finish pushing inhand log string,
				     	 * before quit the log process */


int egi_push_log(const char *fmt, ...);
//static void egi_log_thread_write(void);
//static int egi_malloc_buff2D(char ***buff, int items, int item_len);
//static int egi_free_buff2D(char ***buff, int items, int item_len);
int egi_init_log(void);
//static int egi_stop_log(void);
//static int egi_free_logbuff(void)
int egi_quit_log(void);



/* LOG flags */
#define LOG_NONE        (1<<0)
#define LOG_INFO        (1<<1)
#define LOG_WARN        (1<<2)
#define LOG_ERROR       (1<<3)
#define LOG_CRITICAL	(1<<4)

#define LOG_TEST        (1<<15)

/* default LOG flags */
#define DEFAULT_LOG_LEVELS   (LOG_NONE|LOG_TEST)

/* define egi_plog(), push to log_buff
 * Let the caller to put FILE and FUNCTION, we can not ensure that two egi_push_log()
 * will push string to the log buff exactly one after the other,because of concurrency
 * race condition.
 *  egi_push_log(" From file %s, %s(): \n",__FILE__,__FUNCTION__);
*/

#define EGI_PLOG(level, fmt, args...)                 \
        do {                                            \
                if(level & DEFAULT_LOG_LEVELS)          \
                {                                       \
			egi_push_log(fmt, ## args);	\
                }                                       \
        } while(0)





#endif
