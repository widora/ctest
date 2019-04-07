/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Test EGI BMPJPG unctions

Midas Zhou
-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "egi_timer.h"
#include "egi_fbgeom.h"
#include "egi_color.h"
#include "egi_log.h"
#include "egi.h"
#include "egi_bmpjpg.h"

int main(int argc, char **argv)
{
	char path[128]={0};
	FBDEV fb_dev;

	/* --- init logger --- */
/*
  	if(egi_init_log("/mmc/log_color") != 0)
	{
		printf("Fail to init logger,quit.\n");
		return -1;
	}
*/
        /* --- start egi tick --- */
        tm_start_egitick();
        /* --- prepare fb device --- */
        fb_dev.fdfd=-1;
        init_dev(&fb_dev);

	/* >>>>>>>>>>>>>>>>>>>>>  START TEST  >>>>>>>>>>>>>>>>*/
#if 1

        show_bmp(argv[1],&fb_dev,0,0,0);/* 0-BALCK_ON, 1-BLACK_OFF, 0,0-x0y0 */
	return 0;
#endif

        /* get time stamp */
        time_t t=time(NULL);
        struct tm *tm=localtime(&t);

	if(argc>1)
	     sprintf(path,argv[1]);
	else {
		sprintf(path,"/tmp/FB%d-%02d-%02d_%02d:%02d:%02d.bmp",
        	                   tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour, tm->tm_min,tm->tm_sec);
	}

	egi_save_FBbmp(&fb_dev, path);

	/* <<<<<<<<<<<<<<<<<<<<<  END TEST  <<<<<<<<<<<<<<<<<<*/

  	/* quit logger */
//  	egi_quit_log();

        /* close fb dev */
        munmap(fb_dev.map_fb,fb_dev.screensize);
        close(fb_dev.fdfd);

	return 0;
}

