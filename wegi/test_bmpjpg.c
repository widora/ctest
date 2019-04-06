/*-------------------------------
Test EGI BMPJPG unctions

Midas Zhou
-------------------------------*/
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

	/* --- init logger --- */
  	if(egi_init_log("/mmc/log_color") != 0)
	{
		printf("Fail to init logger,quit.\n");
		return -1;
	}
        /* --- start egi tick --- */
        tm_start_egitick();
        /* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

	/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

        /* get time stamp */
        time_t t=time(NULL);
        struct tm *tm=localtime(&t);

	if(argc>1)
	     sprintf(path,argv[1]);
	else {
		sprintf(path,"/tmp/FB%d-%02d-%02d_%02d:%02d:%02d.bmp",
        	                   tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour, tm->tm_min,tm->tm_sec);
	}

	egi_save_FBbmp(&gv_fb_dev, path);

	/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

  	/* quit logger */
  	egi_quit_log();
        /* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);

	return 0;
}

