/*-------------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
midaszhou@yahoo.com
---------------------------------------------------------------------------------------*/
#include "egi_common.h"
#include "egi_utils.h"
#include "egi_cstring.h"
#include "egi_FTsymbol.h"
#include "page_avenger.h"
#include <signal.h>
#include <sys/types.h>
#include <malloc.h>

static char app_name[]="app_avenger";
static EGI_PAGE *page_avenger=NULL;

static struct sigaction sigact_cont;
static struct sigaction osigact_cont;

static struct sigaction sigact_usr;
static struct sigaction osigact_usr;


/*----------------------------------------------------
		Signal handler for SIGCONT

SIGCONT:	To continue the process.
		SIGCONT can't not be blocked.
SIGUSR1:
SIGUSR2:
SIGTERM:	To terminate the process.

-----------------------------------------------------*/
static void sigcont_handler( int signum, siginfo_t *info, void *ucont )
{
	pid_t spid=info->si_pid;/* get sender's pid */


   	if(signum==SIGCONT) {
        	EGI_PLOG(LOGLV_INFO,"%s:[%s] SIGCONT received from process [PID:%d].\n",
								app_name, __func__, spid);

  }
}


/*-------------------------------------------------------------------
		   Signal handler for SIGUSR1
-------------------------------------------------------------------*/
static void sigusr_handler( int signum, siginfo_t *info, void *ucont )
{
	pid_t spid=info->si_pid;/* get sender's pid */

  if(signum==SIGUSR1) {
	/* restore FBDEV buffer[0] to FB, do not clear buffer */
        EGI_PLOG(LOGLV_INFO,"%s:[%s] SIGSUR1 received from process [PID:%d].\n", app_name, __func__, spid);

	/* raise SIGSTOP */
        if(raise(SIGSTOP) !=0 ) {
                EGI_PLOG(LOGLV_ERROR,"%s:[%s] Fail to raise(SIGSTOP) to itself.\n",app_name, __func__);
        }
 }

}


/*------------------------------------
	assign signal actions
------------------------------------*/
static int assign_signal_actions(void)
{
        /* 1. set signal action for SIGCONT */
        sigemptyset(&sigact_cont.sa_mask);
        sigact_cont.sa_flags=SA_SIGINFO; /*  use sa_sigaction instead of sa_handler */
	sigact_cont.sa_flags|=SA_NODEFER; /* Do  not  prevent  the  signal from being received from within its own signal handler. */
        sigact_cont.sa_sigaction=sigcont_handler;
        if(sigaction(SIGCONT, &sigact_cont, &osigact_cont) <0 ){
	        EGI_PLOG(LOGLV_ERROR,"%s:[%s] fail to call sigaction() for SIGCONT.\n", app_name, __func__);
                return -1;
        }

        /* 2. set signal handler for SIGUSR1 */
        sigemptyset(&sigact_usr.sa_mask);
        sigact_usr.sa_flags=SA_SIGINFO; /*  use sa_sigaction instead of sa_handler */
	sigact_usr.sa_flags|=SA_NODEFER; /* Do  not  prevent  the  signal from being received from within its own signal handler. */
        sigact_usr.sa_sigaction=sigusr_handler;
        if(sigaction(SIGUSR1, &sigact_usr, &osigact_usr) <0 ){
	        EGI_PLOG(LOGLV_ERROR,"%s:[%s] fail to call sigaction() for SIGUSR1.\n", app_name, __func__);
                return -2;
        }

	return 0;
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
	int ret=0;

        /* Set memory allocation option */
        mallopt(M_MMAP_MAX,0);          /* forbid calling mmap to allocate mem */
        mallopt(M_TRIM_THRESHOLD,-1);   /* forbid memory trimming */

        /*  ---  0. assign signal actions  --- */
	assign_signal_actions();

        /*  ---  1. EGI General Init Jobs  --- */
        tm_start_egitick();
        if(egi_init_log("/mmc/log_ffmotion") != 0) {
                printf("Fail to init logger for ffmotion,  quit.\n");
                return -1;
        }
        if(symbol_load_allpages() !=0 ) {
                printf("Fail to load sym pages,quit.\n");
                ret=-2;
		goto FF_FAIL;
        }
        if(FTsymbol_load_allpages() !=0 ) {
                printf("Fail to load sym pages,quit.\n");
                ret=-2;
		goto FF_FAIL;
        }
	/* FT fonts needs more memory, disable it if not necessary */
	FTsymbol_load_appfonts();

	/* Init FB device,for PAGE displaying */
        init_fbdev(&gv_fb_dev);

        /* start touch_read thread */
	if( egi_start_touchread() != 0 ) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to start touch loopread thread!\n", __func__);
		goto FF_FAIL;
	}

	/*  --- 1.1 set FFPLAY Context --- */

	/*  ---  2. EGI PAGE creation  ---  */
	printf(" Start creating PAGE for FFmotion ....\n");
        /* create page and load the page */
        page_avenger=create_avengerPage();
        EGI_PLOG(LOGLV_INFO,"%s: [page '%s'] is created.\n", app_name, page_avenger->ebox->tag);

	/* activate and display the page */
        egi_page_activate(page_avenger);
        EGI_PLOG(LOGLV_INFO,"%s: [page '%s'] is activated.\n", app_name, page_avenger->ebox->tag);

        /* trap into page routine loop */
        EGI_PLOG(LOGLV_INFO,"%s: Now trap into routine of [page '%s']...\n", app_name, page_avenger->ebox->tag);
        page_avenger->routine(page_avenger);

        /* get out of routine loop */
        EGI_PLOG(LOGLV_INFO,"%s: Exit routine of [page '%s'], start to free the page...\n",
		                                                app_name,page_avenger->ebox->tag);

	/* free page */
	tm_delayms(2000); /* let page log_calling finish */
        egi_page_free(page_avenger);


	ret=pgret_OK;

FF_FAIL:
       	release_fbdev(&gv_fb_dev);
        FTsymbol_release_allpages();
        symbol_release_allpages();
	egi_end_touchread();
        egi_quit_log();

	/* NOTE: APP ends, screen to be refreshed by the parent process!! */
	return ret;

}
