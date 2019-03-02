/*--------------------------------------------------------------------------------------------
Utility functions, mem.


Midas Zhou
-----------------------------------------------------------------------------------------------*/
#ifndef __EGI_UTILS_H__
#define __EGI_UTILS_H__

#include <stdio.h>

char** egi_malloc_buff2D(int items, int item_len) __attribute__((__malloc__));

void egi_free_buff2D(char **buff, int items);



#endif
