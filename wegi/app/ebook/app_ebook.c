/*-------------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An EGI APP program for EBOOK reading.

   [  EGI_UI HOME_PAGE ]
	|
	|____ [ BUTTON ]
		  |
		  |
           <<<  signal  >>>
		  |
		  |
	      [ SUBPROCESS ] app_ebook.c
				|
	        		|____ [ UI_PAGE ] page_ebook.c
							|
							|_____ [ extern OBJ_FUNC ] xxxxx.c


Midas Zhou
midaszhou@yahoo.com
---------------------------------------------------------------------------------------*/
#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "page_ebook.h"
#include <signal.h>
#include <sys/types.h>

static char app_name[]="app_ebook";
static EGI_PAGE *page_ebook=NULL;

static struct sigaction sigact_cont;	/* SIG continue process */
static struct sigaction osigact_cont;

static struct sigaction sigact_usr;	/* SIG hang up process */
static struct sigaction osigact_usr;


/*------------------------------------------------------------
	   <<<  Signal handler for SIGSTOP  >>>

Use default signal handling action for SIGSTOP.

Note: A stopped process/task will NOT react to signal TERM, only
      after receiving a CONT signal to activate the it first,
      then it will terminate itself. However it will appear as
      a zombie after such two signals.
      It's suggested to terminat a task with a TERM signal
      only when the task is active.

------------------------------------------------------------*/


/*----------------------------------------------------
  	   <<<  Signal handler for SIGCONT  >>>

To activate/continue a hang_up process.

SIGCONT:	To continue the process.
		SIGCONT can't not be blocked.
SIGUSR1:
SIGUSR2:
SIGTERM:	To terminate the process.
-----------------------------------------------------*/
static void sigcont_handler( int signum, siginfo_t *info, void *ucont )
{
	pid_t spid=info->si_pid;/* get sender's pid */
	sigval_t   sval=info->si_value; /* Signal value */

   	if(signum==SIGCONT)  {
        	EGI_PLOG(LOGLV_INFO,"%s:[%s] SIGCONT received from process [PID:%d].\n",
								app_name, __func__, spid);
	/* SIGCONT can't be be blocked and will pass to handler */

	/* set page refresh flag */
//	egi_page_needrefresh(page_ffplay);
	//page_ffplay->ebox->need_refresh=false; /* Do not refresh page bkcolor */

	/* restore FBDEV buffer[0] to FB, do not clear buffer */
//	fb_restore_FBimg(&gv_fb_dev, 0, false);

  	}
}


/*------------------------------------------------------------------
	<<<  Signal handler for SIGUSR1 (to raise SIGSTOP)  >>>
If receive signal SIGUSR1, then self raise SIGSTOP to hang up the
process.
------------------------------------------------------------------*/
static void sigusr_handler( int signum, siginfo_t *info, void *ucont )
{
	pid_t 	   spid=info->si_pid;	/* get sender's pid */
	sigval_t   sval=info->si_value; /* Signal value */

  if(signum==SIGUSR1) {
	/* restore FBDEV buffer[0] to FB, do not clear buffer */
        EGI_PLOG(LOGLV_INFO,"%s:[%s] SIGSUR1 received from process [PID:%d].\n", app_name, __func__, spid);

	/* buffer FB image */
//	tm_delayms(1500);
                         /* Delay to let touch_effect disappear before buffering the page image,
			 * It seems need rather long time!
			 */
//	fb_buffer_FBimg(&gv_fb_dev, 0);

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
        /* 1. set signal action for SIGCONT: continue process */
        sigemptyset(&sigact_cont.sa_mask); /* No signal will be blocked during sighandler processing */
        sigact_cont.sa_flags=SA_SIGINFO;   /*  use sa_sigaction instead of sa_handler */
	sigact_cont.sa_flags|=SA_NODEFER;  /*  Do  not  prevent  the  signal from being received from
					    *  within its own signal handler.
					    */
        sigact_cont.sa_sigaction=sigcont_handler;
        if(sigaction(SIGCONT, &sigact_cont, &osigact_cont) <0 ){
	        EGI_PLOG(LOGLV_ERROR,"%s:[%s] fail to call sigaction() for SIGCONT.\n", app_name, __func__);
                return -1;
        }

        /* 2. set signal handler for SIGUSR1: hang up process */
        sigemptyset(&sigact_usr.sa_mask); /* No signal will be blocked during sighandler processing */
        sigact_usr.sa_flags=SA_SIGINFO;   /*  use sa_sigaction instead of sa_handler */
	sigact_usr.sa_flags|=SA_NODEFER;  /*  Do  not  prevent  the  signal from being received from
					   *  within its own signal handler.
					   */
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
	pthread_t thread_loopread;

        /*  ---  0. assign signal actions  --- */
	printf("APP_EBOOK:  -------- assign signal action --------\n");
	assign_signal_actions();

        /*  ---  1. EGI General Init Jobs  --- */
        tm_start_egitick();
        if(egi_init_log("/mmc/ebook_log") != 0 ) {
                EGI_PLOG(LOGLV_ERROR,"Fail to init logger,quit.\n");
                return -1;
        }
        if(symbol_load_allpages() !=0 ) {
                EGI_PLOG(LOGLV_ERROR,"Fail to load sym pages,quit.\n");
                ret=-2;
		goto FF_FAIL;
        }
	/* FTsymbol needs more memory, so just disable it if not necessary */
        if(FTsymbol_load_allpages() !=0 ) {
                EGI_PLOG(LOGLV_ERROR,"Fail to load FTsymbol pages,quit.\n");
                ret=-2;
		goto FF_FAIL;
        }
	/* FTsymbol needs more memory, so just disable it if not necessary */
        if(FTsymbol_load_appfonts() !=0 ) {
                EGI_PLOG(LOGLV_ERROR,"Fail to load FT sys fonts,quit.\n");
                ret=-2;
		goto FF_FAIL;
        }


	/* Init FB device */
        init_fbdev(&gv_fb_dev);
        /* start touch_read thread */
        SPI_Open();/* for touch_spi dev  */
        if( pthread_create(&thread_loopread, NULL, (void *)egi_touch_loopread, NULL) !=0 )
        {
                EGI_PLOG(LOGLV_ERROR, "%s: pthread_create(... egi_touch_loopread() ... ) fails!\n",app_name);
                ret=-3;
		goto FF_FAIL;
        }

	/*  ---  2. EGI PAGE creation  ---  */
	printf(" start page ebook creation....\n");
        /* create page and load the page */
        page_ebook=create_ebook_page();
        EGI_PLOG(LOGLV_INFO,"%s: [page '%s'] is created.\n", app_name, page_ebook->ebox->tag);

	/* activate and display the page */
        egi_page_activate(page_ebook);
        EGI_PLOG(LOGLV_INFO,"%s: [page '%s'] is activated.\n", app_name, page_ebook->ebox->tag);

        /* trap into page routine loop */
        EGI_PLOG(LOGLV_INFO,"%s: Now trap into routine of [page '%s']...\n", app_name, page_ebook->ebox->tag);
        page_ebook->routine(page_ebook);
        /* get out of routine loop */
        EGI_PLOG(LOGLV_INFO,"%s: Exit routine of [page '%s'], start to free the page...\n",
		                                                app_name,page_ebook->ebox->tag);

	tm_delayms(200); /* let page log_calling finish */

	/* free PAGE and other resources */
        egi_page_free(page_ebook);
	free_ebook_page();

	ret=pgret_OK;

FF_FAIL:
       	release_fbdev(&gv_fb_dev);
	FTsymbol_release_allfonts();
        FTsymbol_release_allpages();
        symbol_release_allpages();
	SPI_Close();
        egi_quit_log();

	/* NOTE: APP ends, screen to be refreshed by the parent process!! */
	return ret;

}
