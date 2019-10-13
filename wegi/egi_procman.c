/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas_Zhou
------------------------------------------------------------------*/
#include <stdio.h>
#include <signal.h>
#include "egi_log.h"
#include "egi_procman.h"

static struct sigaction sigact_cont;
static struct sigaction osigact_cont;  /* Old one */

static struct sigaction sigact_usr;
static struct sigaction osigact_usr;   /* Old one */


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



/*----------------  For APP  -------------------------
                Signal handler for SIGCONT

SIGCONT:        To continue the process.
                SIGCONT can't not be blocked.
SIGUSR1:
SIGUSR2:
SIGTERM:        To terminate the process.

-----------------------------------------------------*/
static void app_sigcont_handler( int signum, siginfo_t *info, void *ucont )
{
        pid_t spid=info->si_pid;/* get sender's pid */


        if(signum==SIGCONT) {
                EGI_PLOG(LOGLV_INFO,"%s:[%s] SIGCONT received from process [PID:%d].\n",
                                                                app_name, __func__, spid);

  }
}



/*--------------------  For APP  ------------------------------
           Signal handler for SIGUSR1
SIGUSR1		To stop the process
-------------------------------------------------------------*/
static void app_sigusr_handler( int signum, siginfo_t *info, void *ucont )
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

/*-------------  For APP  -----------------
    assign signal actions for APP
------------------------------------------*/
int egi_assign_AppSigActions(void)
{
        /* 1. set signal action for SIGCONT */
        sigemptyset(&sigact_cont.sa_mask);
        sigact_cont.sa_flags=SA_SIGINFO; /*  use sa_sigaction instead of sa_handler */
        sigact_cont.sa_flags|=SA_NODEFER; /* Do  not  prevent  the  signal from being received from within its own signal handler */
        sigact_cont.sa_sigaction=app_sigcont_handler;
        if(sigaction(SIGCONT, &sigact_cont, &osigact_cont) <0 ){
                EGI_PLOG(LOGLV_ERROR,"%s:[%s] fail to call sigaction() for SIGCONT.\n", app_name, __func__);
                return -1;
        }

        /* 2. set signal handler for SIGUSR1 */
        sigemptyset(&sigact_usr.sa_mask);
        sigact_usr.sa_flags=SA_SIGINFO; /*  use sa_sigaction instead of sa_handler */
        sigact_usr.sa_flags|=SA_NODEFER; /* Do  not  prevent  the  signal from being received from within its own signal handler */
        sigact_usr.sa_sigaction=app_sigusr_handler;
        if(sigaction(SIGUSR1, &sigact_usr, &osigact_usr) <0 ){
                EGI_PLOG(LOGLV_ERROR,"%s:[%s] fail to call sigaction() for SIGUSR1.\n", app_name, __func__);
                return -2;
        }

        return 0;
}

