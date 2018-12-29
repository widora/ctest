/*-----------------------------------------
All EGI OBJ Initializations

------------------------------------------*/


#ifndef __EGI_OBJTXT_H__
#define __EGI_OBJTXT_H__

/*  ------------ MEMO: txt ebox  ----------- */
int egi_random_max(int max);
struct egi_element_box *create_ebox_memo(void);
struct egi_element_box *create_ebox_clock(void);
struct egi_element_box *create_ebox_note(void);

void egi_txtbox_demo(void);

#endif
