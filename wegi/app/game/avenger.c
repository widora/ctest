/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
-------------------------------------------------------------------*/
#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "avg_plane.h"
#include "page_avenger.h"


/*----------------------------------------
	     A thread function
-----------------------------------------*/
void *thread_game_avenger(EGI_PAGE *page)
{
	int i,j,k;
	int score=0;
	int secTick=0;
	int nget=0;
	const char *fpath="/mmc/avenger/planes.png";
	EGI_IMGBOX *imboxes=NULL;
	EGI_IMGBUF *plane_icons=NULL;

	AVG_PLANE  *planes[15];

	printf("Start GAME avenger...\n");

	/* Subimage definition: 4 colums, 2 rows of subimages */
	imboxes=egi_imgboxes_alloc(24);
	if(imboxes==NULL) {
		printf("%s: Fail to alloc imgboxes!\n",__func__);
		return (void *)-1;
	}
	for(i=0; i<6; i++) {
		for(j=0; j<4; j++) {
			imboxes[i*4+j]=(EGI_IMGBOX){ j*50, i*40, 50, 40 };
		}
	}

	/* read in icons file */
	printf("read in icons...\n");
        plane_icons=egi_imgbuf_readfile(fpath);
        if(plane_icons==NULL) {
                printf("%s: Fail to read image file '%s'.\n", __func__, fpath);
		egi_imgboxes_free(imboxes);
		return (void *)-2;
        }

	/* Set subimage */
	plane_icons->subimgs=imboxes; imboxes=NULL; /* Ownership transferred */
	plane_icons->submax=24-1;

	/* Create planes */
	for(i=0; i<15; i++)  {
		planes[i]=avg_create_plane( plane_icons, egi_random_max(8)-1,   /* EGI_IMGBUF *icons, icon_index */
       	                              //(EGI_POINT){egi_random_max(240+50)-25, 320+25},	/* EGI_POINT pxy */
       	                              (EGI_POINT){egi_random_max(240+50)-25, -25},	/* EGI_POINT pxy */
		    			    egi_random_max(37)-19,		/* heading, maybe out of sight */
					    egi_random_max(10)+4,		/* (12)+1 int speed */
					    line_trail	//upward_trail		/* int (*trail_mode)(AVG_PLANE *) */
                        	    	 );

		planes[i]->effect_index=20; /* Effect image index of icons */
		planes[i]->effect_stages=4; /* Effect images total */
		planes[i]->hit_effect=avg_effect_exploding;
	}

        /* Check whether lookup table fp16_cos[] and fp16_sin[] is generated */
        if( fp16_sin[30] == 0)
                mat_create_fpTrigonTab();


	/* ------ + ------ + ------ + ----- GAME LOOP ----- + ----- + ----- + ----- */
	k=0;
	secTick=0;

	while(1) {
          /* <<<<< Flush FB and Turn on FILO  >>>>> */
       	  fb_filo_flush(&gv_fb_dev); /* flush and restore old FB pixel data */
          fb_filo_on(&gv_fb_dev);    /* start collecting old FB pixel data */

		k++;

		/*  --- 1. Update PAGE BK now --- */

		/* 1.1 Update credit and time */
		if(tm_pulseus(1000000, 0))
			secTick++;
		avenpage_update_creditTxt(score, secTick);

		/*  --- 2. Update Game --- */
		/* 2.1 Refresh planes, as time passes. */
		for(i=0; i<10; i++)
			refresh_plane(planes[i]);

#if 1		/* 2.2 Score calculation */
		if( (k&0x1) == 0 ) {
			j=egi_random_max(15)-1; 	planes[j]->is_hit=true;
			j=egi_random_max(15)-1;		planes[j]->is_hit=true;
			//j=egi_random_max(15)-1;		planes[j]->is_hit=true;

			score++;
			printf("score=%d\n",score);
		}
#endif


#if 1		/*  --- 3. Demo. and Advertise --- */
		if( (secTick&(8-1)) == 0 && (nget !=secTick>>3) ) {
			nget=secTick>>3;
			game_readme(); sleep(3);
		}
#endif

          /* <<<<< Turn off FILO  >>>>> */
       	  fb_filo_off(&gv_fb_dev);  /* Stop filo */
	  tm_delayms(75);
	}


	/* ---  END: Destroy facility and free resources --- */
	egi_imgbuf_free(plane_icons);
	for(i=0; i<15; i++)
		avg_destroy_plane(&planes[i]);

	return (void *)0;
}
