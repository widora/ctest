
/*-------------------------------
Test EGI COLOR functions

Midas Zhou
-------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "egi_timer.h"
#include "egi_fbgeom.h"
#include "egi_symbol.h"
#include "egi_color.h"
#include "egi_log.h"
#include "egi.h"

int main(void)
{
	int i;
	int ln; /* line of subtitle */
	int subln=4; /* lines for subtitle */

	FILE *fil;
	char *subpath="/mmc/ross.srt";
	char *pt=NULL;
	int  len; /* length of fgets string */
	char strtm[32]={0}; /* time stamp */
	int  nsect; /* section number */
	int  start_secs; /* sub start time, in seconds */
	int  end_secs; /* sub end time, in seconds */
	char strsub[32*4]={0}; /* 64_chars x 4_lines, for subtitle content */


	EGI_BOX subbox={{0,150},{240-1, 260}};
	EGI_16BIT_COLOR color[3],subcolor[3];

	/* --- init logger --- */
  	if(egi_init_log("/mmc/log_color") != 0) {
		printf("Fail to init logger,quit.\n");
		return -1;
	}
        /* --- start egi tick --- */
        tm_start_egitick();

	/* --- load sym pages --- */
	 if(symbol_load_allpages() !=0 ) {
		printf("Fail to load sym pages,quit.\n");
		return -2;
	}
        /* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

	/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<  test draw_wline <<<<<<<<<<<<<<<<<<<<<<*/
#if 0 /////////////////////
	char *strtest="- and he has certain parcels...	\
you are from God and have overcome them,	\
for he who is in you is greater than \
he who is in the world.";

//   for(i=0;i<strlen(strtest);i++)
   {
	fbset_color(WEGI_COLOR_BLACK);
	draw_filled_rect(&gv_fb_dev,subbox.startxy.x,subbox.startxy.y,subbox.endxy.x,subbox.endxy.y);
 	symbol_strings_writeFB(&gv_fb_dev, &sympg_testfont, 240,subln, -9, WEGI_COLOR_ORANGE,
										1, 0,170,strtest+i);
	tm_delayms(100);
   };
   exit(0);
#endif ////////////////////////////////////////

while(1)
{
	/* open subtitle file */
	fil=fopen(subpath,"r");
	if(fil==NULL) {
		printf("Fail to open subtitle:%s\n",strerror(errno));
		return -1;
	}
	/* read subtitle section by section */
	while(!(feof(fil)))
	{
		/* section number or a return code, ignore it. */
		memset(strtm,0,sizeof(strtm));
		fgets(strtm,32,fil);
		if(*strtm=='\n' || *strtm=='\t')
			continue;  /* or continue if its a return */
		else {
			nsect=atoi(strtm);
			printf("Section: %d\n",nsect);
		}

		/* get time stamp */
		memset(strtm,0,sizeof(strtm));
		fgets(strtm,sizeof(strtm),fil);/* time stamp */
		//if(strcmp(strtm,"-->")==0) {
			printf("time stamp: %s\n",strtm);
			start_secs=atoi(strtm)*3600+atoi(strtm+3)*60+atoi(strtm+6);
			printf("Start(sec): %d\n",start_secs);
			end_secs=atoi(strtm+17)*3600+atoi(strtm+20)*60+atoi(strtm+23);
			printf("End(sec): %d\n",end_secs);
		//}
		/* read a section of sub and display */
		fbset_color(WEGI_COLOR_BLACK);
		draw_filled_rect(&gv_fb_dev,subbox.startxy.x,subbox.startxy.y,subbox.endxy.x,subbox.endxy.y);
		ln=0;
		len=0;
		memset(strsub,0,sizeof(strsub));
		do {   /* read a section of subtitle */
			pt=fgets(strsub+len,subln*32-len-1,fil);
			if(pt==NULL)break;
			printf("fgets pt:%s\n",pt);
			len+=strlen(pt); /* fgets also add a '\0\ */
			if(len>=subln*32-1)
				break;
		}while( *pt!='\n' && *pt!='\r' && *pt!='\0' ); /* return code, section end token */

	 	symbol_strings_writeFB(&gv_fb_dev, &sympg_testfont, 240, subln, -5, WEGI_COLOR_ORANGE,
										1, 0, 170, strsub);
		tm_delayms(1500);
	}
	fclose(fil);

}


	/* clear areana */
	fbset_color(WEGI_COLOR_BLACK);
	draw_filled_rect(&gv_fb_dev,subbox.startxy.x,subbox.startxy.y,subbox.endxy.x,subbox.endxy.y);

  	/* quit logger */
	printf("quit log...\n");
  	egi_quit_log();

        /* close fb dev */
	printf("unmap fb...\n");
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);

	printf("clse fb...\n");
        close(gv_fb_dev.fdfd);

	return 0;
}

