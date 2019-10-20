/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
-------------------------------------------------------------------*/
#ifndef __AVG_PLANE_H__
#define __AVG_PLANE_H__

#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "page_avenger.h"


typedef struct avg_plane_data AVG_PLANE;

struct avg_plane_data {
	/* Icons and images */
  const	EGI_IMGBUF	*icons;	 		/* Icons collection */
	int		icon_index;	 	/* Index of the icons for the plane image */
	EGI_IMGBUF	*refimg;		/* Loaded image as for refrence */
	EGI_IMGBUF	*actimg;		/* Image for displaying */

	/* Position, center of the plane */
	EGI_POINT	pxy;	 		/* Current position, image center hopefully.*/
	float		fpx;			/* Coord. in float, to improve calculation precision  */
	float		fpy;
	EGI_FVAL	fvpx;			/* Fixed point for pxy.x */
	EGI_FVAL	fvpy;

	/* Heading & Speed */
	int		heading;		/* Current heading, in Degree
						 * Constrain to [0, 360]
						 */
	int		speed;   		/* Current speed, in pixles per refresh */

	/* Trail mode */
	int (*trail_mode)(AVG_PLANE *);  	/* Method to refresh trail
						 * Jobs:
						 *	1. Update positon AND heading of the plane
						 *	2. Check if its out of visible region. then renew it.
						 */

	/* For hit effect */
	int (*hit_effect)(AVG_PLANE *);  	/* Method to display effect for an hit/damaged plane */
	bool		is_hit;			/* Whether it's hit */
	int		effect_index;		/* Effect image index of icons, starting index of a serial subimages */
	int 		effect_stages;		/* Total number of special effect subimages in icons,
						 * after it's hit/damaged.
						 */
	int		stage;			/* current exploding image index, from 0 to effect_stages-1 */
};

/*  FUNCTIONS  */
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
void 	game_readme(void);

#endif
