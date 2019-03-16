/*------------------------------------------
A EGI FIFO buffer

Midas Zhou
-------------------------------------------*/
#ifndef __EGI_FIFO__
#define __EGI_FIFO__

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>


/* WARNING!!!! DO NOT REFER EGI_FIFO MEMBER DIRECTLY, IT MAY CASE MUTEX LOCK DISFUNCTION/CRASH !!! */
typedef struct
{
	int	  	item_size;	/* size of each item data, in byte. */
	int   		buff_size;	/* itme number that the buff is capable of holding */
	unsigned char 	**buff;		/* data buffer [buff_size][item_size] */
	uint32_t 	pin;  		/* data pusher's position, as buff[pin].
					 * after push_in, pin++ to move to next slot position !!!!!!  */
	uint32_t 	pout; 		/* data puller's position!!!, as buff[pout]
					 * after pull_out, pout++ to move to next slot position !!!!!! */
	int		ahead; 		/* +1 when pin runs ahead of pout and && cross start line
					 * one more time then pout, -1 when pout cross start line
					 *  one more time. Normally ahead will be 0 or 1.
 					 */
	pthread_mutex_t lock;		/* thread mutex lock */
	int		pin_wait;	/* if ==0, keep pushing in data, otherwise stop and wait for pout
					 * to catch up. */

}EGI_FIFO;


EGI_FIFO * egi_malloc_fifo(int buff_size, int item_size);
void egi_free_fifo(EGI_FIFO *efifo );
int egi_push_fifo(EGI_FIFO *fifo, unsigned char *data, int size, int *in, int *out, int *ahd );
int egi_pull_fifo(EGI_FIFO *fifo, unsigned char *data, int size, int *in, int *out, int *ahd );


#endif
