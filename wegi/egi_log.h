#ifndef __EGI_LOG__
#define __EGI_LOG__

#include <stdio.h>
#include <stdlib.h>

#define EGI_LOGFILE_PATH "/tmp/egi_log"

#define EGI_LOG_MAX_BUFFITEMS	128 	/* MAX. number of log buff items */
#define EGI_LOG_MAX_ITEMLEN	256 	/* Max length for each log string item */
#define EGI_LOG_WRITE_SLEEPGAP	10  /* in ms, sleep gap between two buff write session in  egi_log_thread_write() */
#define EGI_LOG_PUSH_WAITTM	0  //1000 /*in ms,  in egi_quit_log(), wait to let egi_push_log() finish */

int egi_push_log(const char *fmt, ...);
//static void egi_log_thread_write(void);
//static int egi_malloc_buff2D(char ***buff, int items, int item_len);
//static int egi_free_buff2D(char ***buff, int items, int item_len);
int egi_init_log(void);
//static int egi_stop_log(void);
//static int egi_free_logbuff(void)
int egi_quit_log(void);

#endif
