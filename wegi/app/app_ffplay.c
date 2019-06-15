#include "egi_common.h"
#include "spi.h"
#include "egi_pageffplay.h"
#include <signal.h>
#include <sys/types.h>

static char app_name[]="app_ffplay";
static EGI_PAGE *page_ffplay=NULL;

static struct sigaction sigact;

/*---------------------------------------------
		signal handler
----------------------------------------------*/
static void app_sighandler( int signum, siginfo_t *info, void *ucont )
{
	pid_t spid;

	/* Signal continue */
	if( signum==SIGCONT ) {
		spid=info->si_pid;/* get sender's pid */
	        EGI_PLOG(LOGLV_INFO,"%s:[%s] SIGCONT received from PID[%d] process.\n",
							app_name, __func__, spid);
//		kill(getpid(),SIGCONT);
		tm_delayms(10);
		egi_page_needrefresh(page_ffplay);
	}
}

#if 0  ////////////////////////////////////////////////////////////////////////
siginfo_t {
               int      si_signo;     /* Signal number */
               int      si_errno;     /* An errno value */
               int      si_code;      /* Signal code */
               int      si_trapno;    /* Trap number that caused
                                         hardware-generated signal
                                         (unused on most architectures) */
               pid_t    si_pid;       /* Sending process ID */
               uid_t    si_uid;       /* Real user ID of sending process */
               int      si_status;    /* Exit value or signal */
               clock_t  si_utime;     /* User time consumed */
               clock_t  si_stime;     /* System time consumed */
               sigval_t si_value;     /* Signal value */
               int      si_int;       /* POSIX.1b signal */
               void    *si_ptr;       /* POSIX.1b signal */
               int      si_overrun;   /* Timer overrun count;
                                         POSIX.1b timers */
               int      si_timerid;   /* Timer ID; POSIX.1b timers */
               void    *si_addr;      /* Memory location which caused fault */
               long     si_band;      /* Band event (was int in
                                         glibc 2.3.2 and earlier) */
               int      si_fd;        /* File descriptor */
               short    si_addr_lsb;  /* Least significant bit of address
                                         (since Linux 2.6.32) */
               void    *si_call_addr; /* Address of system call instruction
                                         (since Linux 3.5) */
               int      si_syscall;   /* Number of attempted system call
                                         (since Linux 3.5) */
               unsigned int si_arch;  /* Architecture of attempted system call
                                         (since Linux 3.5) */
           }
#endif////////////////////////////////////////////////////////////////////////////


/*----------------------------
	     MAIN
----------------------------*/
int main(int argc, char **argv)
{
	pthread_t thread_loopread;

        /* --- 0. set signal handler for SIGCONT --- */
        sigemptyset(&sigact.sa_mask);
        sigact.sa_flags=SA_SIGINFO; /* then use sa_sigaction instead of sa_handler */
	sigact.sa_flags|=SA_NODEFER; /* Do not block set signal */
        sigact.sa_sigaction=app_sighandler;
        if(sigaction(SIGCONT, &sigact, NULL) <0 ){
                perror("Set sigation error");
                return -1;
        }

        /*  ---  1. EGI General Init Jobs  --- */
        tm_start_egitick();
        if(egi_init_log("/mmc/egi_log") != 0) {
                printf("Fail to init logger,quit.\n");
                return -1;
        }
        if(symbol_load_allpages() !=0 ) {
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
        init_fbdev(&gv_fb_dev);
        /* start touch_read thread */
        SPI_Open();/* for touch_spi dev  */
        if( pthread_create(&thread_loopread, NULL, (void *)egi_touch_loopread, NULL) !=0 )
        {
                printf("%s: pthread_create(... egi_touch_loopread() ... ) fails!\n",app_name);
                return -3;
        }


	/*  ---  2. EGI PAGE creation  ---  */
        /* create page and load the page */
        page_ffplay=egi_create_ffplaypage();
        EGI_PLOG(LOGLV_INFO,"%s: [page '%s'] is created.\n", app_name, page_ffplay->ebox->tag);

	/* activate and display the page */
        egi_page_activate(page_ffplay);
        EGI_PLOG(LOGLV_INFO,"%s: [page '%s'] is activated.\n", app_name, page_ffplay->ebox->tag);

        /* trap into page routine loop */
        EGI_PLOG(LOGLV_INFO,"%s: Now trap into routine of [page '%s']...\n", app_name, page_ffplay->ebox->tag);
        page_ffplay->routine(page_ffplay);

        /* get out of routine loop */
        EGI_PLOG(LOGLV_INFO,"%s: Exit routine of [page '%s'], start to free the page...\n",
		                                                app_name,page_ffplay->ebox->tag);

	tm_delayms(200); /* let LOG finish */
        egi_page_free(page_ffplay);


	/* NOTE: APP ends, screen need refresh by the father process!! */
	return pgret_OK;

}
