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


struct egi_point_coord
{
	unsigned int x;
	unsigned int y;
};

struct egi_box_coords
{
	struct egi_point_coord startxy;
	struct egi_point_coord endxy;
};

/* ----------element box type */
enum egi_ebox_type
{
	type_txt,
	type_picture,
	type_button,
	type_chart,
	type_motion,
};

/* element box status */
enum egi_ebox_status
{
	status_nobody=0,
	status_sleep,
	status_active,
};

/* -----------button shape type */
enum egi_btn_type
{
	square=0, /* default */
	circle,
};
/* button status */
enum egi_btn_status
{
	released=0,
	pressed,
};



/*
	-----  structs ebox, is basic element of egi. ----- 
	1. An abstract model of all egi objects.
	2. Common characteristic and parameters for all types of egi objects.
*/
struct egi_element_box
{
	/* ebox type */
	enum egi_ebox_type type;

	/* ebox tag */
	char *tag;	/* a simple description of the ebox for later debug */

	/* ebox statu */
	enum egi_ebox_status status;

	/* start point coordinate, left top point */
	unsigned int x0;
	unsigned int y0;

	/* box size, fit to different type */
	int height;
	int width;

	/*  frame type
	 <0: no frame
	 =0: simple type
	 >0: TBD
	*/
	int  frame;

	/* prime box color
	   >=0 normal 16bits color;
	   <0 transparent,will not draw the color. fonts floating on a page,for example.
	  Usually it shall be transparet, except for an egi page, or an floating txtbox?
	 */
	int prmcolor;

	/* pointer to back image
	   for a page, it is a wallpaper.
	   for other eboxes, it's an image backup for its holder(page)*/
	uint16_t *bkimg;
	/* tracking box coordinates for image backup */
	struct egi_box_coords bkbox;

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
	char tag[32]; /* short description of the button */
	int id; /* unique id number for btn */
	int offx; /* offset from ebox */
	int offy;
	enum egi_btn_type type; /* button shape type, square or circle */
	struct symbol_page *icon; /* button icon */
	int icon_code; /* code number of the symbol in the symbol_page */ 
	enum egi_btn_status status; /* button status, pressed or released */
	void (* action)(enum egi_btn_status status); /* triggered action */
};


/* egi data for a picture type ebox */
struct egi_data_pic
{

};


/* an egi_page takes hold of whole tft-LCD screen */
struct egi_data_page
{
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

int egi_txtbox_activate(struct egi_element_box *ebox);
void egi_txtbox_refresh(struct egi_element_box *ebox);
void egi_refresh(struct egi_element_box *ebox);



#endif
