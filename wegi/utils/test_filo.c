#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "egi_timer.h"
#include "egi_filo.h"
#include "egi_log.h"


EGI_FILO *filo=NULL;

int main(void)
{
	int k;
	int data;

        /* --- start egi tick --- */
        tm_start_egitick();

	/* init logger */
  	if(egi_init_log("/mmc/log_filo") != 0)
	{
		printf("Fail to init logger,quit.\n");
		return -1;
	}

while(1)
{
	//egi_malloc_filo(int buff_size, int item_size, int realloc)
	/* init filo */
	filo=egi_malloc_filo( 1<<1, sizeof(int), 0b01|0b10 ); /* enable double/halve realloc */
	if(filo==NULL)
	{
		printf("Fail to init filo, quit.\n");
		return -1;
	}

	printf(" start push...\n");
	for(k=0;k<(1<<12);k++) {
		printf("push data=%d\n",k);
		egi_filo_push(filo,(void *)&k);
	}

	printf(" start pop...\n");
	for(k=0;k<(1<<12);k++) {
		egi_filo_pop(filo, (void *)&data);
		printf("pop data=%d\n",data);
		if(data!=(1<<12)-1-k)
			printf("data[%d]=%d, ----- ERROR! ----- \n", k, data);
	}
	printf(" --------- push and pop results OK ---------\n");
	/* free filo */
	egi_free_filo(filo);

	tm_delayms(300);
}


  	/* quit logger */
  	egi_quit_log();

	return 0;
}

