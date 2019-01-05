/*-----------------------------------------
All EGI OBJ Initializations

------------------------------------------*/
#ifndef __EGI_OBJTXT_H__
#define __EGI_OBJTXT_H__

#include <stdint.h>


/*  ------------ MEMO: txt ebox  ----------- */
EGI_EBOX *create_ebox_memo(void);
EGI_EBOX *create_ebox_clock(void);
EGI_EBOX *create_ebox_note(void);
void egi_txtbox_demo(void);

/*  -------- egi pattern  ----- */
EGI_EBOX *create_ebox_titlebar(
        int x0, int y0,
        int offx, int offy,
        uint16_t bkcolor,
        char *title
);


#endif
