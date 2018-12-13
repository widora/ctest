/*------------------------------------------------------------------------------
embedded graphic interface based on frame buffer.

Midas Zhou
-----------------------------------------------------------------------------*/
#ifndef __EGI_H__
#define __EGI_H__

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>


/* -----  structs  ----- */
struct egi_element_box
{
	/* start point coordinate, left top point */
	int x0;
	int y0;

	/* box size */
	int height;
	int width;

	/* prime box color */
	uint16_t color;

	/* box icon jpg file */
	char *filjpg;

	/* pointer to back image */
	uint16_t *pbkimg;

	/* pointer to icon image */
	uint16_t *picon;

};

struct egi_page
{
	/* back image */
	uint16_t *bkimg;

	/* eboxes */
	struct egi_element_box *boxlist;
};


/* -----  functions  ----- */
bool egi_point_inbox(int px,int py, struct egi_element_box *ebox);
int egi_get_boxindex(int x,int y, struct egi_element_box *ebox, int num);


#endif
