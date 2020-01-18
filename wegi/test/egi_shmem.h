/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
------------------------------------------------------------------*/
#ifndef __EGI_SHMEM__
#define __EGI_SHMEM__

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

typedef struct {
	const char* shm_name;
	char* shm_map;
	size_t shm_size;

	struct {
		//pthread_mutex_t shm_mutex;	/* Ensure no race in first mutex create */
        	bool    active;			/* status to indicate that data is being processed.
						 * The process occupying the data may quit abnormally, so other processes shall confirm
						 * this status by other means.
						 */
		bool 	sigstop;		/* siganl to stop data processing */
        	int     signum;			/* signal number */
		char    msg[64];
		void*	data;			/* for other type of data */
	}* msg_data;

} EGI_SHM_MSG;


/*------------------------------------------------------------
Open shared memory and get msg_data with mmap.
If the named memory dosen't exist, then create it first.

@shm_msg:	A struct pointer to EGI_SHM_MSG.

Return:
	0	OK
	<0	Fails
-------------------------------------------------------------*/
int egi_shmmsg_open(EGI_SHM_MSG *shm_msg)
{
        int shmfd;

	/* check input */
	if(shm_msg==NULL||shm_msg->shm_name==NULL)
		return -1;

	if(shm_msg->shm_map >0 ) {   /* MAP_FAILED==(void *)-1 */
		printf("%s: shm_msg already mapped!\n",__func__);
		return -2;
	}

	/* open shm */
	shmfd=shm_open(shm_msg->shm_name, O_CREAT|O_RDWR, 0666);
        if(shmfd<0) {
                printf("%s: fail shm_open(): %s",__func__, strerror(errno));
		return -3;
        }

        /* resize */
        if( ftruncate(shmfd, shm_msg->shm_size) <0 ) { /* page size */
                printf("%s: fail ftruncate(): %s",__func__, strerror(errno));
		return -4;
	}

        /* mmap */
        shm_msg->shm_map=mmap(NULL, shm_msg->shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);
        if( shm_msg->shm_map==MAP_FAILED ) {
                printf("%s: fail mmap(): %s",__func__, strerror(errno));
		return -5;
        }

	/* get msg_data pointer */
	shm_msg->msg_data=(typeof(shm_msg->msg_data))shm_msg->shm_map;


	return 0;
}


/*------------------------------------------------------------
munmap msg_data in a EGI_SHM_MSG. but DO NOT unlink the shm_name.

@shm_msg:	A struct pointer to EGI_SHM_MSG.

Return:
	0	OK
	<0	Fails
-------------------------------------------------------------*/
int egi_shmmsg_close(EGI_SHM_MSG *shm_msg)
{
        /* check input */
        if(shm_msg==NULL || shm_msg->shm_name==NULL)
                return -1;

        if(shm_msg->shm_map <=0 ) {   /* MAP_FAILED==(void *)-1 */
                printf("%s: shm_msg already unmapped!\n",__func__);
                return -2;
        }

	/* reset msg_data */
	shm_msg->msg_data=NULL;

	/* unmap shm_map */
        if( munmap(shm_msg->shm_map, shm_msg->shm_size)<0 ) {
                printf("%s: fail munmap(): %s",__func__, strerror(errno));
		return -3;
	}

	return 0;
}


#endif
