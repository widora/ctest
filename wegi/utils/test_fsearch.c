#include <stdio.h>
#include <stdlib.h>
#include "egi_utils.h"
#include "egi_timer.h"

int main(void)
{
	int i;
	char (*path)[EGI_PATH_MAX+EGI_NAME_MAX];
	int total;

        /* --- start egi tick --- */
        tm_start_egitick();

while(1)
{
	path=egi_alloc_search_files("/mmc/","mp3", &total);
	printf("Find %d files.\n", total);
  	for(i=0;i<total;i++)
		printf("%s\n",path+i);

  	free(path);
  	tm_delayms(500);
}

	return 0;
}
