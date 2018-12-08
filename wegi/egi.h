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
	int x0; /* start point coordinate, top left point */
	int y0;
	int height;
	int width;
};


/* -----  functions  ----- */
int egi_get_boxindex(int x,int y, struct egi_element_box *ebox, int num);


#endif
