/*--------------------------------------------------------------------------------------------
Utility functions, mem.


Midas Zhou
-----------------------------------------------------------------------------------------------*/
#include "egi_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*---------------------------------------------------------------------
malloc 2 dimension buff
char** buff to be regarded as char buff[items][item_len];

total 'items' memory blocks allocated, each block owns 'item_len' bytes.

return:
	!NULL	 	OK
	NULL		Fails
---------------------------------------------------------------------*/
char** egi_malloc_buff2D(int items, int item_len)
{
	int i,j;
	char **buff=NULL;

	/* check data */
	if( items <= 0 || item_len <= 0 )
	{
		printf("egi_malloc_buff2(): itmes or item_len is illegal.\n");
		return NULL;
	}

	buff=malloc(items*sizeof(char *));
	if(buff==NULL)
	{
		printf("egi_malloc_buff2(): fail to malloc buff.\n");
		return NULL;
	}

	/* malloc buff items */
	for(i=0;i<items;i++)
	{
		buff[i]=malloc((item_len)*sizeof(char)); /* +1 for string end */
		if(buff[i]==NULL)
		{
			printf("egi_malloc_buff2(): fail to malloc buff[%d], free buff and return.\n",i);
			/* free mallocated items */
			for(j=0;j<i;j++)
			{
				free(buff[j]);
				buff[j]=NULL;
			}
			free(buff);
			buff=NULL;
			return NULL;
		}

		/* clear data */
		memset(buff[i],0,item_len*sizeof(char));
	}

	return buff;
}

/*--------------------------------------------------------------
free 2 dimension buff.
----------------------------------------------------------------*/
void egi_free_buff2D(char **buff, int items)
{
	int i;

	/* check data */
	if( items <= 0  )
	{
		printf("egi_free_buff2D(): itmes or item_len is illegal.\n");
		return;
	}

	/* free buff items and buff */
	if( buff == NULL)
	{
		printf("egi_free_buff2D(): input buff is NULL!\n");
		return;
	}
	else
	{
		for(i=0;i<items;i++)
			free(buff[i]);
		free(buff);
		buff=NULL;
	}

}

