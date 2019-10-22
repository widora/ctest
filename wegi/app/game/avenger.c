/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "avg_mvobj.h"
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

	/* path for icons collection */
	const char *fpath="/mmc/avenger/planes.png";

	/* Icons collection */
	EGI_IMGBOX *imboxes=NULL;
	EGI_IMGBUF *icon_collection=NULL;

	/* Plane icons */
	/* 	    plane_icon_index = 0 - 23 */
	AVG_MVOBJ  *planes[15];
	int	    px,py;

	/* Gun icons */
	//EGI_IMGBUF *gun_icon=NULL;
	int	    GunStation_icon_index=24;
	AVG_MVOBJ  *gun_station=NULL;

	/* Bullet icon */
	int	    bullet_icon_index=26;
	AVG_MVOBJ  *bullet=NULL;
	int 	    bltx, blty;

	/* Wall block icons */
	int	    wallblock_icon_index=25;
	//EGI_IMGBUF *wallblock_icon=NULL;
	int 	    wblk_width=60;
	int	    wblk_height=45;


	printf("Start GAME avenger...\n");

	/* ---  (1) Definie all icons  --- */

	/* 1.0 read in icon collection file */
	printf("read in icons...\n");
        icon_collection=egi_imgbuf_readfile(fpath);
        if(icon_collection==NULL) {
                printf("%s: Fail to read image file '%s'.\n", __func__, fpath);
		//egi_imgboxes_free(imboxes);
		return (void *)-2;
        }

	/* 1.1 Allocate subimage boxes for icon collection */
	imboxes=egi_imgboxes_alloc(28);

	/* 1.2 PLANE ICONS:  Subimage definition: 4 colums, 2 rows of subimages */
	if(imboxes==NULL) {
		printf("%s: Fail to alloc imgboxes!\n",__func__);
		return (void *)-1;
	}
	for(i=0; i<6; i++) {
		for(j=0; j<4; j++) {
			imboxes[i*4+j]=(EGI_IMGBOX){ j*50, i*40, 50, 40 };
		}
	}

	/* 1.3 GUN ICONS:  Subimage definition  */
	imboxes[GunStation_icon_index]=(EGI_IMGBOX){ 0, 6*40, 80, 80 };

	/* 1.4 WALL block ICONS: Subimage definition */
	imboxes[wallblock_icon_index]=(EGI_IMGBOX){ 110, 320-45, 60, 45 };

	/* 1.5 Bullet ICON: Subimage definition */
	imboxes[bullet_icon_index]=(EGI_IMGBOX){ 80, 6*40, 30, 80 };

	/* 1.6 Set subimage for icon collection */
	icon_collection->subimgs=imboxes; imboxes=NULL; /* Ownership transferred */
	icon_collection->submax=28-1;


	/* ---  (2) Create all objects --- */

	/*  Create planes*/
	for(i=0; i<15; i++)  {
		planes[i]=avg_create_mvobj( icon_collection, egi_random_max(8)-1,   /* EGI_IMGBUF *icons, icon_index */
       	                              (EGI_POINT){egi_random_max(240+50)-25, -25},	/* EGI_POINT pxy */
		    			    egi_random_max(37)-19,		/* heading, maybe out of sight */
					    avg_random_speed(),			/* int speed */
					    fly_trail	//upward_trail		/* int (*trail_mode)(AVG_MVOBJ *) */
                        	    	 );

		planes[i]->effect_index=20; /* Effect image index of icons */
		planes[i]->effect_stages=4; /* Effect images total */
		planes[i]->hit_effect=avg_effect_exploding;
	}

	/* Create A protective gun station */
	gun_station=avg_create_mvobj( icon_collection, GunStation_icon_index, /* EGI_IMGBUF *icons, icon_index */
					(EGI_POINT){ 120, 320-40 },	   /* pxy, center */
		    			    egi_random_max(120)-60,	  /* heading, maybe out of sight */
					    0,				  /* int speed */
					    turn_trail			  /* int (*trail_mode)(AVG_MVOBJ *) */
                                         );
	gun_station->vang=6;

	/* Create A bullet */
	bullet=avg_create_mvobj( icon_collection, bullet_icon_index,  	/* EGI_IMGBUF *icons, icon_index */
				    gun_station->pxy,		/* pxy, same as gun station */
	    			    gun_station->heading,	/* heading, same as gun station */
				    -15,			/* int speed */
				    bullet_trail		/* int (*trail_mode)(AVG_MVOBJ *) */
                                );
	/* It has a station */
	bullet->station=gun_station;
	/* assing fixed type pos from gun to bullet */
	bullet->fvpx.num=gun_station->fvpx.num;
	bullet->fvpx.div=gun_station->fvpx.div;
	bullet->fvpy.num=gun_station->fvpy.num;
	bullet->fvpy.div=gun_station->fvpy.div;

	/* Create wall blocks: just draw at the bottom of the PAGE */
	for(i=0; i<240/60; i++)
	        egi_subimg_writeFB( icon_collection, &gv_fb_dev, wallblock_icon_index, -1,
				    i*wblk_width, 320-wblk_height +10);


	/* ---  (3) Preparation before game  --- */

        /* Check whether lookup table fp16_cos[] and fp16_sin[] is generated */
        if( fp16_sin[30] == 0)
                mat_create_fpTrigonTab();

	/* reset tokens */
	k=0;
	secTick=0;


	/* ------ + ------ + ------ + ----- (4) GAME LOOP ----- + ----- + ----- + ----- */

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

		/*  --- 2. Update Game Objectst --- */

		/* 2.1 Refresh gun station FIRST!, as time passes. */
		refresh_mvobj(gun_station);

		/* 2.2 Refresh bullet */
		printf(" ----- bullet -----\n");
		refresh_mvobj(bullet);

		/* 2.3 Refresh planes, as time passes. */
		for(i=0; i<10; i++)
			refresh_mvobj(planes[i]);

		/* 2.4 TODO: Crash/Hit_target calculaiton  */
		bltx=bullet->fvpx.num>>MATH_DIVEXP;
		blty=bullet->fvpy.num>>MATH_DIVEXP;
		for(i=0; i<10; i++) {
			px=planes[i]->fvpx.num>>MATH_DIVEXP;
			py=planes[i]->fvpy.num>>MATH_DIVEXP;

			/* Hit gun station */
			if( py > 320-60
			    && px > 240/2-40
			    && px < 240/2+40 	) {
				planes[i]->is_hit=true;
			}
			/* Crash on fender wall */
			else if( py > 320-wblk_height ) {
				planes[i]->is_hit=true;
			}

			/* Hit by bullet */
			else if( abs(py-blty)<15 && abs(px-bltx)<15 )
				planes[i]->is_hit=true;
		}



#if 1		/* 2.5 Score calculation */
		/* TODO: 			--- Score Rules ---
			1. If a suicide plane hits the fender wall, the gamer loses 1 point.
			2. If a suicide plane hits the gun station, the gamer loses 3 points.
			3. If the gun hits a plane, the gamer gains 1 point.
			4. If the score is less than -3 points, the game is over.
			5. The gamer shall survive all attacks and keep score above -3 to win the game.
		 */

		if( (k&0x1) == 0 ) {
			j=egi_random_max(15)-1; 	planes[j]->is_hit=true;
			//j=egi_random_max(15)-1;		planes[j]->is_hit=true;
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
	egi_imgbuf_free(icon_collection);
	for(i=0; i<15; i++)
		avg_destroy_mvobj(&planes[i]);

	return (void *)0;
}
