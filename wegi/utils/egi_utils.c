/*-------------------------------------------------------
Utility functions, mem.


Midas Zhou
-------------------------------------------------------*/
#include "egi_utils.h"
#include "egi_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>



/*---------------------------------------------------------------------
malloc 2 dimension buff.
char** buff to be regarded as char buff[items][item_size];

Allocates memory for an array of 'itmes' elements with 'item_size' bytes
each and returns a  pointer to the allocated memory.

Total 'items' memory blocks allocated, each block owns 'item_size' bytes.

return:
	!NULL	 	OK
	NULL		Fails
---------------------------------------------------------------------*/
unsigned char** egi_malloc_buff2D(int items, int item_size)
{
	int i,j;
	unsigned char **buff=NULL;

	/* check data */
	if( items <= 0 || item_size <= 0 )
	{
		printf("egi_malloc_buff2(): itmes or item_size is invalid.\n");
		return NULL;
	}

	buff=calloc(items, sizeof(unsigned char *));
	if(buff==NULL)
	{
		printf("egi_malloc_buff2(): fail to malloc buff.\n");
		return NULL;
	}

	/* malloc buff items */
	for(i=0;i<items;i++)
	{
		buff[i]=calloc(item_size, sizeof(unsigned char));
		if(buff[i]==NULL)
		{
			printf("egi_malloc_buff2(): fail to malloc buff[%d], free buff and return.\n",i);
			/* free mallocated items */
			for(j=0;j<i;j++)
			{
				free(buff[j]);
				//buff[j]=NULL;
			}
			free(buff);
			buff=NULL;
			return NULL;
		}
		/* clear data */
		//memset(buff[i],0,item_size*sizeof(unsigned char));
	}

	return buff;
}


/*---------------------------------------------------------------------------------------
reallocate 2 dimension buff.
NOTE:
  1. WARNING: Caller must ensure buff dimension size, old data will be lost after reallocation!!!!
     !!!! After realloc, the Caller is responsible to revise dimension size of buff accordingly.
  2. Be carefull to use when new_items < old_itmes, if realloc fails, then dimension of buff
     may NOT be able to reverted to the old_items.!!!!

buff:		buffer to be reallocated
old_items:	original item number as of buff[items][item_size]
new_items:	new item number as of buff[items][item_size]
item_size:	item size in byte.

return:
	0	 	OK
	<0		Fails
----------------------------------------------------------------------------------------*/
int egi_realloc_buff2D(unsigned char ***buff, int old_items, int new_items, int item_size)
{
	int i,j;
	unsigned char **tmp;

	/* check input data */
	if( buff==NULL || *buff==NULL) {
		printf("egi_realloc_buff2(): input buff is NULL.\n");
		return -1;
	}
	if( old_items <= 0 || new_items <= 0 || item_size <= 0 ) {
		printf("egi_realloc_buff2(): itmes or item_size is invalid.\n");
		return -2;
	}
	/* free redundant buff[] before realloc buff */
	if(new_items < old_items) {
		for(i=new_items; i<old_items; i++)
			free((*buff)[i]);
	}
	/* if items changed */
	if( new_items != old_items ) {
		tmp=realloc(*buff, sizeof(unsigned char *)*new_items);
		if(tmp==NULL) {
			printf("egi_realloc_buff2D(): fail to realloc buff.\n");
			return -3;
		}
		*buff=tmp;
	}
	/* need to calloc buff[] */
	if( new_items > old_items ) {
		for(i=old_items; i<new_items; i++) {
			(*buff)[i]=calloc(item_size, sizeof(unsigned char));

			if((*buff)[i]==NULL) {
				printf("egi_realloc_buff2D(): fail to calloc buff[%d], free buff and return.\n",i);
				for(j=i-1; j>old_items-1; j--)
					free( (*buff)[j] );

				/* revert item number to old_items !!! MAY BE TRAPED HERE!!! */
				tmp=NULL;
				while(tmp==NULL) {
					printf("egi_realloc_buff2D(): realloc fails,revert item number to old_items.\n");
					tmp=realloc( *buff, sizeof(unsigned char *)*old_items );
				}

				*buff=tmp;
				return -4;
			}
		}
	}

	return 0;
}



/*----------------------------------------------------
free 2 dimension buff.
----------------------------------------------------*/
void egi_free_buff2D(unsigned char **buff, int items)
{
	int i;

	/* check data */
	if( items <= 0  )
	{
		printf("egi_free_buff2D(): Param 'items' is invalid.\n");
		return;
	}

	/* free buff items and buff */
	if( buff == NULL)
	{
		printf("egi_free_buff2D(): Input buff is already a NULL!\n");
		return;
	}
	else
	{
		for(i=0; i<items; i++)
		{
			free(buff[i]);
			buff[i]=NULL;
		}

		free(buff);
		buff=NULL;
	}

}


/*------------------------------------------------------------------------------------------
1. Find out all files with specified type in a specified directory and push them into allocated fpbuff,
then return the fpbuff pointer.
2. The fpbuff will expend its memory double each time when no space left for more file paths.
3. Remember to free() the fpbuff at last.

path:           Sear path without extension name.
fext:		File extension name, MUST exclude ".", Example: "wav" or "mp3"...
pcount:         Total number of files found, NULL to ignore.
		-1, search fails.

return value:
         pointer to char (*)[EGI_PATH_MAX+FPLAY_NAME_MAX]   	OK
         NULL && pcount=-1;					Fails
--------------------------------------------------------------------------------------------*/
char* egi_alloc_search_files(const char* path, const char* fext,  int *pcount )
{
        DIR *dir;
        struct dirent *file;
        int num=0; /* file numbers */
	char *pt=NULL; /* pointer to '.', as for extension name */
	char (*fpbuff)[EGI_PATH_MAX+EGI_NAME_MAX]=NULL; /* pointer to final full_path buff */
	int km=0; /* doubling memory times */
	char *ptmp;

	/* 1. check input data */
	if( path==NULL || fext==NULL )
	{
		EGI_PLOG(LOGLV_ERROR, "ff_find_files(): Input path or extension name is NULL. \n");

		if(pcount!=NULL)*pcount=-1;
		return NULL;
	}

	/* 2. check input path leng */
	if( strlen(path) > EGI_PATH_MAX-1 )
	{
		EGI_PLOG(LOGLV_ERROR, "egi_alloc_search_files(): Input path length > EGI_PATH_MAX(%d). \n",
											EGI_PATH_MAX);
		if(pcount!=NULL)*pcount=-1;
		return NULL;
	}

        /* 3. open dir */
        if(!(dir=opendir(path)))
        {
                EGI_PLOG(LOGLV_ERROR,"egi_alloc_search_files(): %s, error open dir: %s!\n",strerror(errno),path);

		if(pcount!=NULL)*pcount=-1;
                return NULL;
        }

	/* 4. calloc one slot buff first */
	fpbuff= calloc(1,EGI_PATH_MAX+EGI_NAME_MAX);
	if(fpbuff==NULL)
	{
		EGI_PLOG(LOGLV_ERROR,"egi_alloc_search_files(): Fail to callo fpbuff!.\n");

		if(pcount!=NULL)*pcount=-1;
		return NULL;
	}
	km=0; /* 2^0, first double */

        /* 5. get file paths */
        while((file=readdir(dir))!=NULL)
        {
		/* 5.1 check number of files first, necessary to set limit????  */
                if(num >= EGI_SEARCH_FILE_MAX)/* break if fpaths buffer is full */
		{
			EGI_PLOG(LOGLV_WARN,"egi_alloc_search_files(): File fpath buffer is full! try to increase FFIND_MAX_FILENUM.\n");
                        break;
		}

		else if( num == (1<<km) )/* get limit of buff size */
		{
			/* double memory */
			km++;
			ptmp=(char *)realloc((char *)fpbuff,(1<<km)*(EGI_PATH_MAX+EGI_NAME_MAX) );

			/* if doubling mem fails */
			if(ptmp==NULL)
			{
				EGI_PLOG(LOGLV_ERROR,"egi_alloc_search_files(): Fail to realloc mem for fpbuff.\n");
				/* break, and return old fpbuff data though*/
				break;
			}

			/* get new pointer to the buff */
			fpbuff=( char(*)[EGI_PATH_MAX+EGI_NAME_MAX])ptmp;
			ptmp=NULL;

			EGI_PLOG(LOGLV_INFO,"egi_alloc_search_files(): fpbuff[] is reallocated with capacity to buffer totally %d items of full_path.\n",
													1<<km );
		}

                /* 5.2 check name string length */
                if( strlen(file->d_name) > EGI_NAME_MAX-1 )
		{
			EGI_PLOG(LOGLV_WARN,"egi_alloc_search_files(): %s.\n	File path is too long, fail to store.\n",
									file->d_name);
                        continue;
		}

		pt=strstr(file->d_name,"."); /* get '.' pointer */
		if(pt==NULL)
		{
			//printf("ff_find_files(): no extension '.' for %s\n",file->d_name);
			continue;
		}
		/* compare file extension name */
                if( strncmp(pt+1, fext, EGI_FEXTNAME_MAX)!=0 )
                         continue;

		/* Clear buff and save full path of the matched file */
		memset((char *)(fpbuff+num), 0, (EGI_PATH_MAX+EGI_NAME_MAX)*sizeof(char) );
		sprintf((char *)(fpbuff+num), "%s/%s", path, file->d_name);
		//printf("egi_alloc_search_files(): push %s ...OK\n",fpbuff+num);

                num++;
        }

	/* 6. feed back count to the caller */
	if(pcount != NULL)
	        *pcount=num; /* return count */

	/* 7. free fpbuff if no file found */
	if( num==0 && fpbuff != NULL )
	{
		free(fpbuff);
		fpbuff=NULL;
	}

	/* 8. close dir */
         closedir(dir);

	/* 9. return pointer to the buff */
         return (char *)fpbuff;
}



