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
#include "egi_common.h"
#include "he_weather.h"
#include "egi_FTsymbol.h"

char *city_names[2]=
{
	"shanghai",
	"zhoushan"
};

int main(int argc, char **argv) {
  	int i=0;
	int index;
        int n=0;
	int pixlen;
	int icon_width;
	char strtemp[16];
	int  temp;
	char strhum[16];
	int  hum;
	char strbuff[256]={0};
	int  subindex;

        FBDEV vfb;
        EGI_IMGBUF *host_eimg=NULL;
        EGI_IMGBUF *icon_eimg=NULL;


        /* --- init log --- */
        if(egi_init_log("/mmc/log_weather") !=0 )
        {
           printf("egi_init_log() fails! \n");
           return -1;
        }

	/* display png cond image */
   	init_fbdev(&gv_fb_dev); /* init FB dev */

        /* --- load all symbol pages --- */
        if(symbol_load_allpages() !=0 ) {
		return -1;
	}
        if(FTsymbol_load_appfonts() !=0 ) {  /* and FT fonts LIBS */
                printf("Fail to load FT appfonts, quit.\n");
                return -2;
        }


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

	/* prepare host_eimg and virt FB */
        host_eimg=egi_imgbuf_new();
        egi_imgbuf_init(host_eimg, 120, 240); /* 2 rows */
        init_virt_fbdev(&vfb,host_eimg);


	subindex=1;
///////////////////////   LOOP TEST  //////////////////////
while(1) {
	n++;

	/* reset subindex 0,1 */
	subindex++;
	subindex=subindex%2;

	/* get NOW weather data */
	if(heweather_httpget_data(data_now, city_names[subindex]) !=0 )
		goto SLEEP_WAIT;

	/* put icon file in the dedicated directory for the EGI to fetch */
#if 0  ////////////////////////////////////////////////////////////////////
	printf("HeWeather NOW icon: '%s'\n", weather_data[data_now].icon_path);
	if(egi_copy_file(weather_data[data_now].icon_path, "/tmp/.egi/heweather/now.png")) {
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to copy weather icon: '%s'!\n",__func__,
								weather_data[data_now].icon_path);
		goto SLEEP_WAIT;
	}
#else  /////////////////////////////////////////////////////////////////


	/* clear subimg in host_img */
	egi_imgbuf_reset(host_eimg, subindex, 0, 0);

	/* Write city name to VIRT FB */
        FTsymbol_uft8strings_writeFB(&vfb, egi_appfonts.regular,   	    /* FBdev, fontface */
                                           20, 20, weather_data[0].city,    /* fw,fh, pstr */
                                           240, 1, 0,           	    /* pixpl, lines, gap */
                                           0, 20+subindex*60,                /* x0,y0, */
                                           WEGI_COLOR_WHITE, -1, -1);   /* fontcolor, stranscolor,opaque */

	/* Load icon to VIRT FB */
	pixlen=FTsymbol_uft8strings_pixlen(egi_appfonts.regular,20, 20, weather_data[0].city);
	printf("---------- pxilen=%d ----------\n",pixlen);
	egi_imgbuf_windisplay(weather_data[0].eimg, &vfb, SYM_NOSUB_COLOR, 0, 0, pixlen, subindex*60, 60, 60);

        /* Write cond_txt to VIRT FB */
        sprintf(strbuff,"%s",weather_data[0].cond_txt);
        FTsymbol_uft8strings_writeFB(&vfb, egi_appfonts.regular,   	/* FBdev, fontface */
                                           16, 16, strbuff,    		/* fw,fh, pstr */
                                           240, 1, 0,           	/* pixpl, lines, gap */
                                           pixlen+60, 5+subindex*60,     /* x0,y0, */
                                           WEGI_COLOR_GRAYB, -1, -1);   /* fontcolor, stranscolor,opaque */

        /* write temp & hum to VIRT FB */
        sprintf(strbuff, "温度%dC  湿度%d%%",weather_data[0].temp, weather_data[0].hum);
        FTsymbol_uft8strings_writeFB(&vfb, egi_appfonts.regular,   	/* FBdev, fontface */
                                           16, 16, strbuff,    		/* fw,fh, pstr */
                                           240, 1, 0,           	/* pixpl, lines, gap */
                                           pixlen+60, 30+subindex*60,   /* x0,y0, */
                                           WEGI_COLOR_WHITE, -1, -1);   /* fontcolor, stranscolor,opaque */
        /* save to PNG file */
        if(egi_imgbuf_savepng("/tmp/.egi/heweather/now.png", host_eimg)) {
                printf("Fail to save imgbuf to PNG file!\n");
        }
#endif


SLEEP_WAIT:
	printf(" --------------------- N:%d ---------------\n", n);
	sleep(90); //90); /* Limit 1000 per day, 90s per call */

} ///////////////////// LOOP TEST END ///////////////////////


	printf("release host_eimg and virt FB...\n");
        egi_imgbuf_free(host_eimg);
        release_virt_fbdev(&vfb);

        printf("FTsymbol_release_allfonts()...\n");
        FTsymbol_release_allfonts();

        printf("symbol_release_allpages()...\n");
        symbol_release_allpages();

        printf("release_fbdev()...\n");
   	release_fbdev(&gv_fb_dev);

        printf("egi_quit_log()...\n");
        egi_quit_log();
        printf("---------- END -----------\n");


  return 0;
}
