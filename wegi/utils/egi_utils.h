/*--------------------------------------------------------------------------------------------
Utility functions, mem.


Midas Zhou
-----------------------------------------------------------------------------------------------*/
#ifndef __EGI_UTILS_H__
#define __EGI_UTILS_H__

#include <stdio.h>

unsigned char** egi_malloc_buff2D(int items, int item_size) __attribute__((__malloc__));
int egi_realloc_buff2D(unsigned char **buff, int old_items, int new_items, int item_size);
void egi_free_buff2D(unsigned char **buff, int items);
char* egi_alloc_search_files(const char* path, const char* fext,  int *pcount );

#endif
