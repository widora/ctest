/*---------------------------- eig_txt.h --------------------------------------

Midas Zhou
-----------------------------------------------------------------------------*/
#ifndef __EGI_TXT_H__
#define __EGI_TXT_H__

#include "egi.h"
#include <stdio.h>
//#include <stdint.h>
//#include <math.h>
#include <stdbool.h>


/* for txt ebox */
struct egi_data_txt *egi_init_data_txt(struct egi_data_txt *data_txt, /* ----- OBSOLETE ----- */
                 int offx, int offy, int nl, int llen, struct symbol_page *font, uint16_t color);


struct egi_data_txt *egi_txtdata_new(int offx, int offy, /* create new txt data */
        int nl,
        int llen,
        struct symbol_page *font,
        uint16_t color );

struct egi_element_box * egi_txtbox_new( char *tag,  enum egi_ebox_type type, /* create new txt ebox */
        struct egi_data_txt *egi_data,
        struct egi_ebox_method method,
        bool movable,
        int x0, int y0,
        int width, int height,
        int frame,
        int prmcolor );

int egi_txtbox_activate(struct egi_element_box *ebox);
int egi_txtbox_refresh(struct egi_element_box *ebox);
int egi_txtbox_sleep(struct egi_element_box *ebox);
int egi_txtbox_readfile(struct egi_element_box *ebox, char *path);
void egi_free_data_txt(struct egi_data_txt *data_txt);


#endif
