/*-----------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Test FBDEV functions


Midas Zhou
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "egi_common.h"
#include "utils/egi_utils.h"

int main(int argc, char **argv)
{

do {
        /* EGI general init */
	printf("tm_start_egitick()...\n");
        tm_start_egitick();
	printf("egi_init_log()...\n");
        if(egi_init_log("/mmc/log_fb") != 0) {
                printf("Fail to init logger,quit.\n");
                return -1;
        }

	printf("symbol_load_allpages()...\n");
        if(symbol_load_allpages() !=0 ) {
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }

	printf("init_fbdev()...\n");
        init_fbdev(&gv_fb_dev);


#if 1 /* <<<<<<<<<<< TEST: fb_buffer_FBimg() and fb_restore_FBimg() <<<<<<<<<<<<< */

	printf("fb_buffer_FBimg(NB=0)...\n");
	fb_buffer_FBimg(&gv_fb_dev,0);

	tm_delayms(500);

	printf("fb_restore_FBimg(NB=0, clear=TRUE)...\n");
	fb_restore_FBimg(&gv_fb_dev,0,true);


#endif /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

	printf("release_fbdev()...\n");
	release_fbdev(&gv_fb_dev);

	printf("symbol_free_allpages()...\n");
	symbol_release_allpages();

	printf("egi_quit_log()...\n");
	egi_quit_log();
	printf("---------- END -----------\n");

}while(1);


	return 0;
}
