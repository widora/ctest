#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "egi_log.h"
#include "egi_utils.h"
#include "egi_filo.h"

/*-----------------------------------------------------------------
Calloc and init a FILO buffer
buff_size:	as for buff[buff_size][item_size]
item_size:
realloc:	if !=0, enable auto realloc for buff.

Return:
	0	Ok
	<0	fails
-------------------------------------------------------------------*/
EGI_FILO * egi_malloc_filo(int buff_size, int item_size, int realloc)
{
	EGI_FILO *efilo=NULL;

        if( buff_size<=0 || item_size<=0 ) {
                EGI_PLOG(LOGLV_ERROR, "%s: input buff_size or item_size is invalid.\n",__func__);
                return NULL;
        }
        /* calloc efilo */
        efilo=(EGI_FILO *)calloc(1, sizeof(EGI_FILO));
        if(efilo==NULL) {
                EGI_PLOG(LOGLV_ERROR,"%s: Fail to malloc efilo.\n",__func__);
                return NULL;
        }

        /* NOT LOCK APPLIED !!!! buff mutex lock */
//        if(pthread_mutex_init(&efilo->lock,NULL) != 0) {
//                printf("%s: fail to initiate EGI_FILO mutex lock.\n",__func__);
//                free(efilo);
//                return NULL;
//        }

        /* malloc buff, and memset inside... */
        efilo->buff=egi_malloc_buff2D(buff_size, item_size);
        if(efilo->buff==NULL) {
                EGI_PLOG(LOGLV_ERROR,"%s: Fail to malloc efilo->buff.\n", __func__);
                free(efilo);
                efilo=NULL;
                return NULL;
        }
	efilo->pt=0;
	efilo->auto_realloc=realloc;

	return efilo;
}


/*---------------------------------------
Free a EGI_FILO structure and its buff.
----------------------------------------*/
void egi_free_filo(EGI_FILO *efilo )
{
        if( efilo != NULL)
        {
                /* free buff */
                if( efilo->buff != NULL ) {
                        egi_free_buff2D(efilo->buff, efilo->buff_size);
                        //free(efifo->buff) and set efifo->buff=NULL in egi_free_buff2D() already
                }
                /* free itself */
                free(efilo);
                efilo=NULL;
        }
}



/*-----------------------------------------------
Push data into FILO buffer
The caller shall confirm data size as per filo.
filo:	FILO struct
data:	pointer to data for push

Return:
	>0	filo buff is full.
	0	OK
	<0	fails
-------------------------------------------------*/
int egi_filo_push(EGI_FILO *filo, const void* data)
{
	/* verifyi input data */
	if( filo==NULL || filo->buff==NULL ) {
                EGI_PLOG(LOGLV_ERROR, "%s: input filo is invalid.\n",__func__);
                return -1;
        }
	if( data==NULL ) {
                EGI_PLOG(LOGLV_ERROR, "%s: input data is invalid.\n",__func__);
                return -2;
        }
	/* check buff space */
	if(filo->pt==filo->buff_size) {
		/* auto reacllocate, increase buff to double size*/
		if(filo->auto_realloc) {
			if(realloc(filo->buff, (filo->buff_size)<<1) == NULL)	{
				EGI_PLOG(LOGLV_ERROR,"%s: fail to realloc filo buff.\n",__func__);
				return -3;
			}
			filo->buff_size <<= 1;
		}
		/* buff is full */
		else {
			EGI_PLOG(LOGLV_ERROR,"%s: FILO buff is full.\n",__func__);
			return 1;
		}
	}
	/* push data into buff */
	memcpy(filo->buff[filo->pt],(unsigned char *)data, filo->item_size);
	filo->pt++;

	return 0;
}


/*-------------------------------------------
Pop data from FILO buffer
filo:	FILO struct
data:	pointer to pass the data

Return:
	>0	filo buff is empty.
	0	OK
	<0	fails
-------------------------------------------*/
int egi_filo_pop(EGI_FILO *filo, void* data)
{
	/* verifyi input data */
	if( filo==NULL || filo->buff==NULL ) {
                EGI_PLOG(LOGLV_ERROR, "%s: input filo is invalid.\n",__func__);
                return -1;
        }
	if( data==NULL ) {
                EGI_PLOG(LOGLV_ERROR, "%s: input data is invalid.\n",__func__);
                return -2;
        }
	/* check buff point */
	if( filo->pt==0 ) {
		printf("egi_filo_pop(): buff is empty!\n");
		return 1;
	}
	/* shift pt and copy data */
	filo->pt--;
	memcpy((unsigned char *)data, filo->buff[filo->pt],filo->item_size);

	return 0;
}
