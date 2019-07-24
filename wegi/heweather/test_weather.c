/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example for www.heweahter.com https interface.

Midas Zhou
-------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "egi_log.h"
#include "egi_image.h"
#include "egi_fbgeom.h"
#include "egi_symbol.h"
#include "egi_https.h"
#include "egi_utils.h"
#include "he_weather.h"



int main(int argc, char **argv) {
  	int i=0;
	int index;
        int n=0;
	char strtemp[16];
	int  temp;
	char strhum[16];
	int  hum;

        /* --- init log --- */
        if(egi_init_log("/mmc/log_weather") !=0 )
        {
           printf("egi_init_log() fails! \n");
           exit(0);
        }
	/* display png cond image */
   	init_fbdev(&gv_fb_dev); /* init FB dev */
        /* --- load all symbol pages --- */
        symbol_load_allpages();

	/* get request index */
	if(argc>1) {
		index=atoi(argv[1]);
		if(index<0 || index>3)
			index=0;
	}
	else	index=0;


	/* create dir for heweather */
	egi_util_mkdir("/tmp/.egi/heweather",0755);
#if 0
	if(access("/tmp/.egi/heweather",F_OK)==-1) {
		printf("create dir for heweather...\n");
		mkdir("/tmp/.egi",0755);
		if(mkdir("/tmp/.egi/heweather",0755)) {
			printf("%s: Fail to create dir for heweather!\n");
			return -1;
		}
	}
#endif



///////////////////////   LOOP TEST  //////////////////////
while(1) {
	n++;

	/* get NOW weather data */
	if(heweather_httpget_data(data_now) !=0)
		goto SLEEP_WAIT;

	/* put icon file in the dedicated directory for the EGI to fetch */
	printf("HeWeather NOW icon: '%s'\n", weather_data[data_now].icon_path);
	if(egi_copy_file(weather_data[data_now].icon_path, "/tmp/.egi/heweather/now.png")) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to copy weather icon: '%s'!\n",__func__,
								weather_data[data_now].icon_path);
		goto SLEEP_WAIT;
	}

#if 0
        /* <<< Flush FB and Turn on FILO before wirteFB >>>*/
        printf("Flush pixel data in FILO, start  ---> ");
        fb_filo_flush(&gv_fb_dev); /* flush and restore old FB pixel data */
        printf(" <--- finish!\n");
        fb_filo_on(&gv_fb_dev); /* start collecting old FB pixel data */

   	egi_subimg_writeFB(weather_data[0].eimg, &gv_fb_dev, 0, WEGI_COLOR_WHITE, 70,250); //70, 220);

	sprintf(strtemp,"%dC", weather_data[0].temp);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_WHITE,
                                  		1, 130, 255, strtemp, 240);// 	170, 235, strtemp );
	sprintf(strhum,"%%%d", weather_data[0].hum);
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_WHITE,
                                  		1, 130, 280, strhum, 240);

        /* <<<  Turn off FILO after writeFB  >>> */
        fb_filo_off(&gv_fb_dev);
#endif

SLEEP_WAIT:
	printf(" --------------------- N:%d ---------------\n", n);
	sleep(90); //90); /* Limit 1000 per day, 90s per call */

} ///////////////////// LOOP TEST END ///////////////////////


   	release_fbdev(&gv_fb_dev);

  return 0;
}
