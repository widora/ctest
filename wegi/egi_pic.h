/*---------------- egi_pic.h--------------------------
egi pic tyep ebox functions

Midas Zhou
------------------------------------------------------*/
#ifndef __EGI_PIC_H__
#define __EGI_PIC_H__

#include "egi.h"
#include "egi_symbol.h"


EGI_DATA_PIC *egi_picdata_new( int offx, int offy,
                               int height, int width,
                               int imgpx, int imgpy,
                               struct symbol_page *font
                             );


EGI_EBOX * egi_picbox_new( char *tag, /* or NULL to ignore */
        EGI_DATA_PIC *egi_data,
        bool movable,
        int x0, int y0,
        int frame,
        int prmcolor /* applys only if prmcolor>=0 and egi_data->icon != NULL */
);


int egi_picbox_activate(EGI_EBOX *ebox);
int egi_picbox_refresh(EGI_EBOX *ebox);
int egi_picbox_sleep(EGI_EBOX *ebox);
void egi_free_data_pic(EGI_DATA_PIC *data_pic);


#endif
