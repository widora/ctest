/*---------------- egi_btn.h--------------------------
egi btn tyep ebox functions

Midas Zhou
------------------------------------------------------*/
#ifndef __EGI_BTN_H__
#define __EGI_BTN_H__


#include "egi.h"
#include "egi_symbol.h"

EGI_DATA_BTN *egi_btndata_new(int id, enum egi_btn_type shape,
                                        struct symbol_page *icon, int icon_code);

EGI_EBOX * egi_btnbox_new( char *tag,
        EGI_DATA_BTN *egi_data,
        bool movable,
        int x0, int y0,
        int width, int height,
        int frame,
        int prmcolor
);

int egi_btnbox_activate(EGI_EBOX *ebox);
int egi_btnbox_refresh(EGI_EBOX *ebox);
void egi_free_data_btn(EGI_DATA_BTN *data_btn);


#endif
