/*---------------- egi_btn.h--------------------------
egi btn tyep ebox functions

Midas Zhou
------------------------------------------------------*/
#include "egi.h"


EGI_DATA_BTN *egi_btndata_new(int id, enum egi_btn_type shape,
                                        struct symbol_page *icon, int icon_code);
int egi_btnbox_activate(EGI_EBOX *ebox);
int egi_btnbox_refresh(EGI_EBOX *ebox);
void egi_free_data_btn(EGI_DATA_BTN *data_btn);

