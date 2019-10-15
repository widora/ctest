/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
-------------------------------------------------------------------*/
#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "page_avenger.h"

typedef struct avg_plane_data AVG_PLANE;
struct avg_plane_data {
  const	EGI_IMGBUF	*icons;	 		/* Icons collection */
	int		icon_index;	 	/* Index of the icons for the plane image */
	EGI_IMGBUF	*refimg;		/* Loaded image as for refrence */
	EGI_IMGBUF	*actimg;		/* Image for displaying */
	EGI_POINT	pxy;	 		/* Current position, image center hopefully.*/
	float		fpx;			/* Coord. in float, to improve calculation precision  */
	float		fpy;
	int		heading;		/* Current heading, in Degree */
	int		speed;   		/* Current speed, in pixles per refresh */
	int (*trail_mode)(AVG_PLANE *);  	/* Method to refresh trail */

	/* For exploding effect */
	bool		is_hit;			/* Whether it's hit */
	int		effect_index;		/* Effect image index of icons, starting index of a serial subimages */
	int 		effect_stages;		/* Total number of special effect subimages in icons,
						 * after it's hit/damaged.
						 */
	int		stage;			/* current exploding image index, from 0 to effect_stages-1 */
	int (*hit_effect)(AVG_PLANE *);  	/* Method to display effect for an hit/damaged plane */

};


/* --------------- FUNCTIONS --------------- */
AVG_PLANE* avg_create_plane(    EGI_IMGBUF *icons,  int icon_index,
				EGI_POINT pxy, int heading, int speed,
				int (*trail_mode)(AVG_PLANE *)
			    );

void 	avg_destroy_plane(AVG_PLANE **plane);
int 	avg_effect_exploding(AVG_PLANE *plane);
int 	avg_renew_plane(AVG_PLANE *plane);
int 	upward_trail(AVG_PLANE *plane);
int 	line_trail(AVG_PLANE *plane);
int 	refresh_plane(AVG_PLANE *plane);


/* --------------------------------------------------------------
		Create a new plane
Return:
	Pointer to an AVG_PLANE		OK
	NULL				Fails
-----------------------------------------------------------------*/
AVG_PLANE* avg_create_plane(    EGI_IMGBUF *icons,  int icon_index,
				EGI_POINT pxy, int heading, int speed,
				int (*trail_mode)(AVG_PLANE *)
			    )
{
	AVG_PLANE   *plane=NULL;
	EGI_IMGBUF  *refimg=NULL;
	EGI_IMGBUF  *actimg=NULL;

	/* Check input data */
	if( icons==NULL || icons->subimgs==NULL || icon_index<0 || icon_index > icons->submax ) {
		printf("%s:Input icons is NULL or icon_index invalid!\n",__func__);
		return NULL;
	}

	/* Copy icons->subimg[] to refimg */
	refimg=egi_imgbuf_subImgCopy( icons, icon_index );
	if(refimg==NULL)
		return NULL;

	/* Create actimg according to heading */
	actimg=egi_imgbuf_rotate(refimg, heading);
	if(actimg==NULL) {
		egi_imgbuf_free(refimg);
		return NULL;
	}

	/* Calloc AVG_PLANE */
	plane=calloc(1, sizeof(AVG_PLANE));
	if(plane==NULL) {
		printf("%s:Fail to call calloc() !\n",__func__);
		egi_imgbuf_free(refimg);
		egi_imgbuf_free(actimg);
		return NULL;
	}

	/* Assign methods and actions */
	plane->trail_mode=trail_mode;
	plane->hit_effect=avg_effect_exploding;

	/* Assign other memebers */
	plane->icons=icons;
	plane->icon_index=icon_index;
	plane->refimg=refimg;
	plane->actimg=actimg;
	plane->pxy=pxy;
	plane->fpx=pxy.x;
	plane->fpy=pxy.y;
	plane->heading=heading;
	plane->speed=speed;

	return plane;
}


/*--------------------------------------
	Destroy a plane
---------------------------------------*/
void avg_destroy_plane(AVG_PLANE **plane)
{
	if(*plane==NULL)
		return;

	if( (*plane)->refimg != NULL) {
		free((*plane)->refimg);
		(*plane)->refimg=NULL;
	}

	if( (*plane)->actimg != NULL) {
		free((*plane)->actimg);
		(*plane)->actimg=NULL;
	}

	free(*plane);
	*plane=NULL;
}


/* -------------------------------------
Special effects for an exploding plane.
---------------------------------------*/
int avg_effect_exploding(AVG_PLANE *plane)
{
	if(plane==NULL)
		return -1;

	/* !!! NOW !!! Effect image also in plane->icons */

	/* End of stage */
	if( plane->stage > plane->effect_stages-1 )
		return -1;

//	printf(" --- stage %d --- \n", plane->stage);
	/* (const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subindex, int subcolor, int x0,   int y0) */
	egi_subimg_writeFB( plane->icons, &gv_fb_dev, plane->effect_index + plane->stage,
			    -1, plane->pxy.x + plane->actimg->width/2, plane->pxy.y + plane->actimg->height/2);

	/* increase stage */
	plane->stage++;

	return 0;
}


/*----------------------------------------------
		Renew a plane
Chane param and start trail from starting line.
---------------------------------------------*/
int avg_renew_plane(AVG_PLANE *plane)
{
	int endx;

	if(plane==NULL)
		return -1;

	/* Start point,  from bottom of the screen, screen Y=320  +25  */
	plane->pxy.x=egi_random_max(240+50)-25;
	plane->pxy.y=320+25;
	plane->fpx=plane->pxy.x;
	plane->fpy=plane->pxy.y;

	/* End point,  to the top of then screen, screen Y=0  -25  */
	endx=egi_random_max(240+50)-25;
	/* while endy=0-25 */

#if 1	/* Draw xxx course line */
	switch(egi_random_max(3))
	{
		case 1:
			fbset_color(WEGI_COLOR_WHITE); break;
		case 2:
			fbset_color(WEGI_COLOR_ORANGE); break;
		case 3:
			fbset_color(WEGI_COLOR_RED); break;
		default:
			fbset_color(WEGI_COLOR_WHITE); break;
	}
       //	fb_filo_off(&gv_fb_dev);
	draw_wline(&gv_fb_dev, plane->pxy.x, plane->pxy.y, endx, -25, 1+egi_random_max(5));
       //	fb_filo_on(&gv_fb_dev);
#endif

	/* ----- Set heading angle -----
	 * Anti_screenY as heading 0 line.
	 * Clockwise as positive.
	 */
	plane->heading=atan( (endx-plane->pxy.x)/370.0 )/MATH_PI*180.0;
	//plane->heading=-30;
	printf("---  Pxy=(%d, %d),  Endxy=(%d, %d),  Heading=%d Deg. ---\n",
			plane->pxy.x, plane->pxy.y, endx, -25, plane->heading);

	/* Set speed */
	plane->speed=egi_random_max(12)+2;

	/* Change icon */
	egi_imgbuf_free(plane->refimg);
	plane->refimg=egi_imgbuf_subImgCopy( plane->icons, egi_random_max(16)-1 );

        /* Create actimg according to heading */
	egi_imgbuf_free(plane->actimg); plane->actimg=NULL;
       	plane->actimg=egi_imgbuf_rotate(plane->refimg, plane->heading);

	/* Reste effect params */
	plane->is_hit=false;
	plane->stage=0;

	return 0;
}



/*--------------------------------
A straight upward trail.
--------------------------------*/
int upward_trail(AVG_PLANE *plane)
{
	if(plane==NULL)
		return -1;

	plane->pxy.y -= plane->speed;

	/* Disappearing point check, and loop back to starting line */
	if(plane->pxy.y<-50) {
		plane->pxy.x=egi_random_max(240+50)-25;
		plane->pxy.y=320;
		plane->speed=egi_random_max(12)+2;
	}

	return 0;
}


/* ----------------------------------
	A heading line trail
-----------------------------------*/
int line_trail(AVG_PLANE *plane)
{

	if(plane==NULL)
		return -1;

	/* Update position */
	plane->fpx += plane->speed*sin(plane->heading/180.0*MATH_PI);
	plane->pxy.x = plane->fpx;
	plane->fpy -= plane->speed*cos(plane->heading/180.0*MATH_PI);
	plane->pxy.y = plane->fpy;

	/* Disappearing point check.  active zone { (-55,-55), (295,375) }
         * and loop back to starting line
	 */
	if( plane->pxy.x < 0-55 || plane->pxy.x > 240+55
				|| plane->pxy.y < 0-55 || plane->pxy.y > 320 + 55 )
	{
		avg_renew_plane(plane);

        }

	return 0;
}


/* ------------------------------
Refresh a plane image on screen.
--------------------------------*/
int refresh_plane(AVG_PLANE *plane)
{
	if(plane==NULL || plane->icons==NULL)
		return -1;

	/* If hit, show special effect */
	if(plane->is_hit) {
		/* Show hit effect stage by stage */
		if(plane->hit_effect != NULL)
			plane->hit_effect(plane);

		/* End of stages, renew the plane then */
		if( plane->stage > plane->effect_stages-1 ) {
			avg_renew_plane(plane);
		}

		return 0;
	}

	/* Else, update trail */
	if(plane->trail_mode != NULL)
		plane->trail_mode(plane);

	/* Refresh active image one the trail */
        egi_imgbuf_windisplay( 	plane->actimg, &gv_fb_dev, -1,           /* img, fb, subcolor */
       	                       	0, 0,					 /* xp, yp */
				plane->pxy.x + plane->actimg->width/2,		 /* xw */
				plane->pxy.y + plane->actimg->height/2,          /* yw */
                      		plane->actimg->width, plane->actimg->height );   /* winw, winh */

	return 0;
}

/* ------------------
Game README
--------------------*/
int game_readme(void)
{
	const wchar_t *title=L"AVENGER V1.0";
	const wchar_t *readme=L" GMAE 复仇者 Demo made by EGI. \
   --- END ---";

        /*  title  */
        FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.bold,     /* FBdev, fontface */
                                          24, 24, title,                /* fw,fh, pstr */
                                          240, 1,  6,                 /* pixpl, lines, gap */
                                          30, 120,                      /* x0,y0, */
                                          WEGI_COLOR_RED, -1, -1 );   /* fontcolor, stranscolor,opaque */

	/* README */
        FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.regular,     /* FBdev, fontface */
                                          18, 18, readme,                /* fw,fh, pstr */
                                          240, 4,  4,                 /* pixpl, lines, gap */
                                          0, 170,                      /* x0,y0, */
                                          WEGI_COLOR_RED, -1, -1 );   /* fontcolor, stranscolor,opaque */

}



void *thread_game_avenger(EGI_PAGE *page)
{

	int i,j,k;
	int score=0;
	int secTick=0;
	int nget;
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
       	                              (EGI_POINT){egi_random_max(240+50)-25, 320+25},	/* EGI_POINT pxy */
		    			    egi_random_max(120)-60,		/* int heading */
					    egi_random_max(12)+1,		/* int speed */
					    line_trail	//upward_trail		/* int (*trail_mode)(AVG_PLANE *) */
                        	    	 );

		planes[i]->effect_index=20; /* Effect image index of icons */
		planes[i]->effect_stages=4; /* Effect images total */
		planes[i]->hit_effect=avg_effect_exploding;
	}


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
		for(i=0; i<15; i++)
			refresh_plane(planes[i]);

		/* 2.2 Score calculation */
		if( (k&0x1) == 0 ) {
			j=egi_random_max(15)-1; 	planes[j]->is_hit=true;
			j=egi_random_max(15)-1;		planes[j]->is_hit=true;
			j=egi_random_max(15)-1;		planes[j]->is_hit=true;

			score++;
			printf("score=%d\n",score);
		}

		/*  --- 3. Demo. and Advertise --- */
		if( (secTick&(8-1)) == 0 && (nget !=secTick>>3) ) {
			nget=secTick>>3;
			game_readme(); sleep(3);
		}

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
