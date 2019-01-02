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
EGI_DATA_TXT *egi_init_data_txt(EGI_DATA_TXT *data_txt, /* ----- OBSOLETE ----- */
                 int offx, int offy, int nl, int llen, struct symbol_page *font, uint16_t color);


EGI_DATA_TXT *egi_txtdata_new(int offx, int offy, /* create new txt data */
        int nl,
        int llen,
        struct symbol_page *font,
        uint16_t color );

EGI_EBOX * egi_txtbox_new( char *tag,/* create new txt ebox */
        EGI_DATA_TXT *egi_data,
//        EGI_METHOD method,
        bool movable,
        int x0, int y0,
        int width, int height,
        int frame,
        int prmcolor );


int egi_txtbox_activate(EGI_EBOX *ebox);
int egi_txtbox_refresh(EGI_EBOX *ebox);
int egi_txtbox_decorate(EGI_EBOX *ebox);
int egi_txtbox_sleep(EGI_EBOX *ebox);
int egi_txtbox_readfile(EGI_EBOX *ebox, char *path);
void egi_free_data_txt(EGI_DATA_TXT *data_txt);


#endif
