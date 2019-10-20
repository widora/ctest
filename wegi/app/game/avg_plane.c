/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
-------------------------------------------------------------------*/
#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "page_avenger.h"
#include "avg_plane.h"


#define AVG_FIXED_POINT

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

	/* Assign methods and actions, May re_assign later... */
	plane->trail_mode=trail_mode;
	plane->hit_effect=avg_effect_exploding;

	/* Assign other memebers */
	plane->icons=icons;
	plane->icon_index=icon_index;
	plane->refimg=refimg;
	plane->actimg=actimg;
	plane->pxy=pxy;		/* Position: Integer type */

	plane->fpx=pxy.x;	/* Position: Float type */
	plane->fpy=pxy.y;

	plane->fvpx=MAT_FVAL(plane->pxy.x); /* Position: Fixed point value */
	plane->fvpy=MAT_FVAL(plane->pxy.y);

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
		egi_imgbuf_free((*plane)->refimg);
		(*plane)->refimg=NULL;
	}

	if( (*plane)->actimg != NULL) {
		egi_imgbuf_free((*plane)->actimg);
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

	/* display image */
     #ifdef AVG_FIXED_POINT  /* use plane->fvpx/fvpy */
	egi_subimg_writeFB( plane->icons, &gv_fb_dev, plane->effect_index + plane->stage, -1,
			    (plane->fvpx.num>>MATH_DIVEXP) - plane->actimg->width/2,
			    (plane->fvpy.num>>MATH_DIVEXP) - plane->actimg->height/2 );
     #else /* <<<<<  use plane->pxy */
	egi_subimg_writeFB( plane->icons, &gv_fb_dev, plane->effect_index + plane->stage,
			    -1, plane->pxy.x + plane->actimg->width/2, plane->pxy.y + plane->actimg->height/2);
     #endif


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
	int endx,endy;

	if(plane==NULL)
		return -1;

	/* Start point,  from to of the screen, screen Y=0  -25  */
	plane->pxy.x=egi_random_max(240+50)-25;
	plane->pxy.y=0-25;

	/* End point,  to the bottom of the screen, screen Y=320 +25  */
	endx=egi_random_max(240+50)-25;
	endy=320+25;

	/* Position: Float type */
	plane->fpx=plane->pxy.x;
	plane->fpy=plane->pxy.y;

	/* Position: Fixed point type */
	plane->fvpx.num=(plane->pxy.x)<<MATH_DIVEXP;
	plane->fvpx.div=MATH_DIVEXP;
	plane->fvpy.num=(plane->pxy.y)<<MATH_DIVEXP;
	plane->fvpy.div=MATH_DIVEXP;

	printf(" plane->fvpx: %d;      plane->fvpy: %d \n",
			(int)(plane->fvpx.num>>MATH_DIVEXP), (int)(plane->fvpy.num>>MATH_DIVEXP) );

	/* Set speed */
	plane->speed=egi_random_max(4)+2;

	/* Reste effect params */
	plane->is_hit=false;
	plane->stage=0;


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
    //fb_filo_off(&gv_fb_dev);
	#ifdef AVG_FIXED_POINT  /* use plane->fvpx */
	draw_wline(&gv_fb_dev, (plane->fvpx.num)>>MATH_DIVEXP, (plane->fvpy.num)>>MATH_DIVEXP,
								endx, endy, 1+egi_random_max(5));
	#else  /* use plane->pxy instead */
	draw_wline(&gv_fb_dev, plane->pxy.x, plane->pxy.y, endx, endy, 1+egi_random_max(5));
	#endif
    //fb_filo_on(&gv_fb_dev);
#endif

	/* ---------- Set heading angle ------------
	 * screenY as heading 0 Degree line.
	 * Counter_Clockwise as positive direction.
	 * ----------------------------------------*/
#ifdef AVG_FIXED_POINT /*  use plane->fvpx/fvpy */
	plane->heading=atan( 1.0*(endx-(plane->fvpx.num>>MATH_DIVEXP))/(endy-(plane->fvpy.num>>MATH_DIVEXP)) )/MATH_PI*180.0;
	printf("---  Pxy=(%d, %d),  Endxy=(%d, %d),  speed=%d  Heading=%d Deg. ---\n",
			(int)(plane->fvpx.num>>MATH_DIVEXP), (int)(plane->fvpy.num>>MATH_DIVEXP), \
			endx, endy, 	plane->speed, plane->heading );
#else 	/* use plane->pxy or instead */
	plane->heading=atan( 1.0*(endx-plane->pxy.x)/(endy-plane->pxy.y) )/MATH_PI*180.0;
	printf("---  Pxy=(%d, %d),  Endxy=(%d, %d),  speed=%d  Heading=%d Deg. ---\n",
			plane->pxy.x, plane->pxy.y, endx, endy, plane->speed, plane->heading);
#endif

	/* Change icon, randomly pick an incon in collection */
	egi_imgbuf_free(plane->refimg);
	plane->refimg=egi_imgbuf_subImgCopy( plane->icons, egi_random_max(16)-1 );

        /* Create actimg according to heading */
	egi_imgbuf_free(plane->actimg); plane->actimg=NULL;
       	plane->actimg=egi_imgbuf_rotate(plane->refimg, -plane->heading); /* Here clockwise is positive */

	return 0;
}


/*-----------------------------------------------------------
		A heading line trail

1. A slow plane may fly ahead while slipping aside, because
   of pxy+= ... incremental precision, as pxy is an integer.
   Use float type otherwise to improve the precision.

------------------------------------------------------------*/
int line_trail(AVG_PLANE *plane)
{
	if(plane==NULL)
		return -1;

        /* 0. Normalize heading angle to be within [0-360] */
        int ang=(plane->heading)%360;      /* !!! WARING !!!  The result is depended on the Compiler */
        int asign= ang >= 0 ? 1 : -1; /* angle sign for sin */

        ang= ang>=0 ? ang : -ang ;

	/* 1. Update plane position */
#ifdef AVG_FIXED_POINT	/* 1.3 Fixed point for sin()/cons() AND plane->fvxy */
	plane->fvpx.num += plane->speed*asign*fp16_sin[ang]>>(16-MATH_DIVEXP);
	plane->fvpy.num += plane->speed*fp16_cos[ang]>>(16-MATH_DIVEXP);

#else 	/*  Float type for plane->fpx AND sin()/cos() */
	plane->fpx += plane->speed*sin(plane->heading/180.0*MATH_PI);
	plane->pxy.x = plane->fpx;

	plane->fpy -= plane->speed*cos(plane->heading/180.0*MATH_PI);
	plane->pxy.y = plane->fpy;
#endif

	/* 2. Update heading */

	/* 3. Check if it's out of visible region, then renew it to starting position.
	 *    active zone { (-55,-55), (295,375) }
	 */
#ifdef AVG_FIXED_POINT  /* use plane->fvpx/fvpy */
	if( (plane->fvpx.num>>MATH_DIVEXP) < 0-55 ||  (plane->fvpx.num>>MATH_DIVEXP) > 240+55
			|| (plane->fvpy.num>>MATH_DIVEXP) < 0-55 || (plane->fvpy.num>>MATH_DIVEXP) > 320 + 55 )
	{
		avg_renew_plane(plane);

        }
#else  /* use plane->pxy instead */
	if( plane->pxy.x < 0-55 || plane->pxy.x > 240+55
				|| plane->pxy.y < 0-55 || plane->pxy.y > 320 + 55 )
	{
		avg_renew_plane(plane);

        }
#endif

	return 0;
}


/* ----------------------------------
	A Circle trail
-----------------------------------*/
int circle_trail(AVG_PLANE *plane)
{

	if(plane==NULL)
		return -1;

	/* 1. Update position */
	plane->fpx += plane->speed*sin(plane->heading/180.0*MATH_PI);
	plane->pxy.x = plane->fpx;
	plane->fpy -= plane->speed*cos(plane->heading/180.0*MATH_PI);
	plane->pxy.y = plane->fpy;

	/* 2. Update heading */


	/* 2. Disappearing point check.  active zone { (-55,-55), (295,375) }
         * and loop back to starting line
	 */
	if( plane->pxy.x < 0-55 || plane->pxy.x > 240+55
				|| plane->pxy.y < 0-55 || plane->pxy.y > 320 + 55 )
	{
		avg_renew_plane(plane);

        }

	return 0;
}


/*----------------------------------------------
	Refresh a plane image on screen.
----------------------------------------------*/
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

#ifdef AVG_FIXED_POINT /* Use plane->fvpx/fvpy */
        egi_imgbuf_windisplay( 	plane->actimg, &gv_fb_dev, -1,           /* img, fb, subcolor */
       	                       	0, 0,					 /* xp, yp */
				(plane->fvpx.num>>MATH_DIVEXP) - plane->actimg->width/2,   /* xw */
				(plane->fvpy.num>>MATH_DIVEXP) - plane->actimg->height/2,  /* yw */
                      		plane->actimg->width, plane->actimg->height );   /* winw, winh */
#else /* Use plane->pxy */
        egi_imgbuf_windisplay( 	plane->actimg, &gv_fb_dev, -1,           /* img, fb, subcolor */
       	                       	0, 0,					 /* xp, yp */
				plane->pxy.x - plane->actimg->width/2,		 /* xw */
				plane->pxy.y - plane->actimg->height/2,          /* yw */
                      		plane->actimg->width, plane->actimg->height );   /* winw, winh */
#endif

	return 0;
}


/*--------------------------
	Game README
--------------------------*/
void game_readme(void)
{
	const wchar_t *title=L"AVENGER V1.0";

	/* Following will produce 4 RETURE codes */
	const wchar_t *readme=L"    复仇者 made by EGI\n \
  更猛烈的一波攻击来袭! \n \n \
	           READY?!!";

        /*  title  */
        FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.bold,     /* FBdev, fontface */
                                          24, 24, title,                /* fw,fh, pstr */
                                          240, 1,  6,                   /* pixpl, lines, gap */
                                          30, 120,                      /* x0,y0, */
                                          WEGI_COLOR_GRAYC, -1, -1 );     /* fontcolor, stranscolor,opaque */

	/* README */
        FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.bold,   /* FBdev, fontface */
                                          18, 18, readme,                /* fw,fh, pstr */
                                          240, 5,  5,                    /* pixpl, lines, gap */
                                          0, 160,                        /* x0,y0, */
                                          WEGI_COLOR_GRAYC, -1, -1 );      /* fontcolor, stranscolor,opaque */

}



