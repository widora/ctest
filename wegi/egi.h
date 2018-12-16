/*------------------------------------------------------------------------------------
embedded graphic interface based on frame buffer, 16bit color tft-LCD.

Midas Zhou
------------------------------------------------------------------------------------*/
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
	   >=0 normal 16bits color;
	   <0 transparent,will not draw the color. fonts floating on a page,for example.
	  Usually it shall be transparet, except for an egi page.
	 */
	int prmcolor;

	/* pointer to back image
	   for a page, it is a wallpaper.
	   for other eboxes, it's an image backup for its holder(page)*/
	uint16_t *bkimg;

	/* data pointer to different types of struct egi_data_xxx */
	void *egi_data;

	/* pointer to icon image */
//	uint16_t *picon;

	/*  method */
	void (*activate)(struct egi_element_box *);
	void (*refresh)(struct egi_element_box *);
	void (*sleep)(struct egi_element_box *);
	void (*destroy)(struct egi_element_box *);

	/* child list */
	struct egi_element_box *child;
};



/* egi data for a txt type ebox */
struct egi_data_txt
{
	int offx; /* offset from ebox x0,y0 */
	int offy;
	int nl;  /* number of txt lines */
	int llen; /* in byte, data length for each line */
	struct symbol_page *font;
	uint16_t color; /* txt color */
	char **txt; /*multiline txt data */
};

/* egi data for a botton type ebox */
struct egi_data_btn
{

};

/* egi data for a picture type ebox */
struct egi_data_pic
{

};


/* an egi_page takes hold of whole tft-LCD screen */
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


struct egi_data_txt *egi_init_data_txt(struct egi_data_txt *data_txt,
                 int offx, int offy, int nl, int llen, struct symbol_page *font, uint16_t color);
void egi_free_data_txt(struct egi_data_txt *data_txt);
struct egi_element_box *egi_init_ebox(struct egi_element_box *ebox);
void egi_free_data_txt(struct egi_data_txt *data_txt);
void egi_free_ebox(struct egi_element_box *ebox);

void egi_txtbox_refresh(struct egi_element_box *ebox);
void egi_refresh(struct egi_element_box *ebox);


#endif
