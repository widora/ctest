#ifndef __EGI_FILO_H__
#define __EGI_FILO_H__


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*  WARNING: No mutex lock applied, It is supposed there is only one pusher and one puller,
 *  and they will NOT write/read the buff at the same time.
 */
typedef struct
{
        int             item_size;      /* size of each item data, in byte.*/
        int             buff_size;      /* total item number that the buff is capable of holding */
        unsigned char   **buff;         /* data buffer [buff_size][item_size] */

	unsigned int	pt;		/* curretn data position,
					 * pt++ AFTER push; pt-- BEFORE pop
					 */

//        pthread_mutex_t lock;           /* thread mutex lock */
	int		auto_realloc;	/* when !=0 double buff size each time when it's full */
					 /* TODO:???? or release unused space when it's half empty ????*/
} EGI_FILO;


EGI_FILO * egi_malloc_filo(int buff_size, int item_size, int realloc);
void egi_free_filo(EGI_FILO *efilo );
int egi_filo_push(EGI_FILO *filo, const void* data);
int egi_filo_pop(EGI_FILO *filo, void* data);


#endif
