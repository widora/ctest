#include "egi.h"


#define  LIST_ITEM_MAXLINES  10 /* MAX number of txt line for each list item */
#define  LIST_DEFAULT_BKCOLOR	WEGI_COLOR_GRAY /*default list item bkcolor */

typedef struct egi_list EGI_LIST;

struct egi_list
{
	/* left point coordinates */
	int x0;
	int y0;

	/* total number of items in a list */
	int inum;

	/*
	  in pixel,
	  width and height for each list item
	  the height also may be adjusted according to number of txt lines and icon later
	*/
	int width;
	int height;

	/*in byte,  length of each txt line */
	int llen;

	/* a list of type_txt ebox */
	EGI_EBOX **txt_boxes;

	/*
	 frame type, to see egi_ebox.c
	 -1  no frame
	  0  simple
	 ....
	*/
	int frame;

	/* sympg font for the txt ebox */
	struct symbol_page *font;

	/*  offset of txt from the ebox */
	int txtoffx;
	int txtoffy;

	/*
	  sympg icon for each list item
	  NULL means no icon
	*/
	struct symbol_page **icons;

	/*  offset of icon from the ebox */
	int iconoffx;
	int iconoffy;

	/*
	   back color for each list item,
	   or use first bkcolor as default color
	  init default color in egi_list_new()
	*/
	uint16_t *bkcolor;

	/*
	   color for each txt line in a list item.
	   if null, use default as black 0.
	   ALL BLACK now !!!!!
	*/
	//uint16_t (*txtcolor)[LIST_ITEM_MAXLINES];


};


EGI_LIST *egi_list_new (
        int x0, int y0, /* left top point */
        int inum,  /* item number of a list */
        int width, /* h/w for each list item */
        int height,
        int frame, /* -1 no frame for ebox, 0 simple .. */
        int llen, /* in byte, length for each txt line */
        struct symbol_page *font, /* txt font */
        int txtoffx, /* offset of txt from the ebox */
        int txtoffy,
        int iconoffx,
        int iconoffy
);

