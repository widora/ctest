/*------------------------------------------------------------------------------
embedded graphic interface based on frame buffer.

Midas
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
	 /* start point coordinate, top left point */
	int x0;
	int y0;
	/* box size */
	int height;
	int width;
	/* box color */
	uint16_t color;
	/* box icon jpg file */
	char *filjpg;
};


/* -----  functions  ----- */
bool egi_point_inbox(int px,int py, struct egi_element_box *ebox);
int egi_get_boxindex(int x,int y, struct egi_element_box *ebox, int num);


#endif
