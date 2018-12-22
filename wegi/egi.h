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


#define EGI_NOPRIM_COLOR -1 /* Do not draw primer color for an egi object */


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

/* element box type */
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

/*
 	button shape type
	Not so necessary, you may use bkcolor of symbol to cut
	out the shape you want. unless you want a frame.
*/
enum egi_btn_type
{
	square=0, /* default */
	circle,
};

/* button status */
enum egi_btn_status
{
	releasing=0,   /* status transforming from pressed_hold to released_hold */
	pressing,      /* status transforming from released_hold to pressed_hold */
	released_hold,
	pressed_hold,
};



/*
	-----  structs ebox, is basic element of egi. -----
	1. An abstract model of all egi objects.
	2. Common characteristic and parameters for all types of egi objects.
	3. or a holder for all egi objects.
*/
struct egi_element_box
{
	/* ebox type */
	enum egi_ebox_type type;

	/*
		--- movable or stationary ---
	If an ebox is immovale, it cann't move or resize. if it does so, then its
	image will be messed up after refeshing.
	*/
	bool movable;

	/* ebox tag */
	char tag[32];	/* a simple description of the ebox for later debug */

	/* ebox statu */
	enum egi_ebox_status status;

	/* start point coordinate, left top point */
	unsigned int x0;
	unsigned int y0;

	/* box size, fit to different type
	1. for txt box, H&W define the holding pad size.
	2. for button, H&W is same as symbol H&W.
	*/
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
	   <0 no prime color, will not draw color. fonts floating on a page,for example.
	  Usually it shall be transparet, except for an egi page, or an floating txtbox?
	 */
	int prmcolor;

	/* pointer to back image
	   1.For a page, it is a wallpaper.??
	   2.For other eboxes, it's an image backup for its holder(page)
           3.For MOVABLE icons/txt boxes, the size should not be too large,!!!
	   It will restore bkimg to fb and copy bkimg from fb everytime before you
	   draw the moving ebox. It's sure not efficient when bkimg is very large, you would
	   rather refresh the whole FB memory.
	   4.Not applicable for an immovable ebox.
      */
	uint16_t *bkimg;

	/* tracking box coordinates for image backup */
	struct egi_box_coords bkbox;

	/* data pointer to different types of struct egi_data_xxx */
	void *egi_data;

	/* --------- method ------- */
	/*  --- activate:
	   A._for a status_sleep ebox:
	   	1. wake it up, refresh to  display it on screen.
		2. reset status.
	   B._for a status_no-body ebox:
	   	1. initialization job for the ebox and ebox->egi_data.
	   	2. malloc bkimg for an movable ebox,and save the backgroud img.
	   	3. refresh to display the ebox on screen.
	   	4. reset the status as active.
	*/
	void (*activate)(struct egi_element_box *);

	/* --- refresh:
		0. a sleep ebox will not be refreshed.
		1. restore backgroud from bkimg and store new position backgroud to bkimg.
		2. update ebox->egi_data and do some job here ---------.
		3. redraw the ebox according to updated data.

	*/
	void (*refresh)(struct egi_element_box *);

	/* --- sleep:
	   1. Remove the ebox from the screen and restore the bkimg.
	   2. and set status as sleep.
	   3. if an immovale ebox sleeps, it will not dispear!
	   4. sleeping ebox will not be targed for touching.
	*/
	void (*sleep)(struct egi_element_box *);

	/* --- destroy:
 
	*/
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
	//char tag[32]; /* short description of the button */
	int id; /* unique id number for btn */
	enum egi_btn_type shape; /* button shape type, square or circle */
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
void *egi_alloc_bkimg(struct egi_element_box *ebox, int width, int height);
bool egi_point_inbox(int px,int py, struct egi_element_box *ebox);
int egi_get_boxindex(int x,int y, struct egi_element_box *ebox, int num);

struct egi_data_txt *egi_init_data_txt(struct egi_data_txt *data_txt,
                 int offx, int offy, int nl, int llen, struct symbol_page *font, uint16_t color);
void egi_free_data_txt(struct egi_data_txt *data_txt);
struct egi_element_box *egi_init_ebox(struct egi_element_box *ebox);


void egi_free_ebox(struct egi_element_box *ebox);

/* for txt ebox */
int egi_txtbox_activate(struct egi_element_box *ebox);
int egi_txtbox_refresh(struct egi_element_box *ebox);
int egi_txtbox_sleep(struct egi_element_box *ebox);
void egi_free_data_txt(struct egi_data_txt *data_txt);
int egi_txtbox_readfile(struct egi_element_box *ebox, char *path);

/* for button ebox */
int egi_btnbox_activate(struct egi_element_box *ebox);
int egi_btnbox_refresh(struct egi_element_box *ebox);



void egi_refresh(struct egi_element_box *ebox);


#endif
