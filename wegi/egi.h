/*------------------------------------------------------------------------------
embedded graphic interface based on frame buffer.

Very simple concept:
0. The basic elements of egi objects are egi_element boxes.
1. Only one egi_page is active on the screen.
2. An active egi_page occupys the whole screen.
3. An egi_page hosts several type of egi_ebox, such as type_txt,type_button,...etc.


Midas Zhou
-----------------------------------------------------------------------------*/
#ifndef __EGI_H__
#define __EGI_H__

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>


struct egi_coordinate
{
	unsigned int x;
	unsigned int y;
};


/* element box type */
enum egi_ebox_type
{
	type_txt,
	type_picture,
	type_button,
	type_chart,
	type_motion,
};


/* -----  structs ebox, is basic element of egi. ----- */
struct egi_element_box
{
	/* ebox type */
	enum egi_ebox_type type;

	/* start point coordinate, left top point */
	unsigned int x0;
	unsigned int y0;

	/* box size, fit to different type */
	int height;
	int width;

	/* prime box color
	   <0 transparent; 
	 */
	int color;

	/* pointer to back image */
	uint16_t *pbkimg;

	/* data for different types of ebox */
	void *egi_data;

	/* pointer to icon image */
	uint16_t *picon;

	/* action in refresh, reload */
	void (*refresh)(void);

	/* child list */
	struct egi_element_box *child;
};

/* egi data for a txt type ebox */
struct egi_data_txt
{
	int nl;  /* number of txt lines */
	char **txt; /*multiline txt*/
	struct symbol_page *font;
};


struct egi_page
{
	/* back image */
	uint16_t *bkimg;

	/* eboxes */
	struct egi_element_box *boxlist;
};



/* ----------------  functions  ---------------- */
bool egi_point_inbox(int px,int py, struct egi_element_box *ebox);
int egi_get_boxindex(int x,int y, struct egi_element_box *ebox, int num);
struct egi_element_box *egi_init_ebox(struct egi_element_box *ebox);
void egi_free_ebox(struct egi_element_box *ebox);


#endif
