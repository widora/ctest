/*----------------------- egi_obj.c ------------------------------

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <signal.h>
#include "color.h"
//#include "fblines.h"
#include "egi.h"
//#include "xpt2046.h"
//#include "bmpjpg.h"
//#include "egi_timer.h"
#include "symbol.h"

/*  ------------ MEMO: txt ebox  ----------- */
struct egi_data_txt memo_txt={0}; /* default: memo_txt.foff=0 */
struct egi_element_box ebox_memo=
{
	.movable=true,
	.type = type_txt,
	.height = 320, /*box height, one line, will be ajusted according to numb of lines */
	.width = 240,
	.prmcolor = WEGI_COLOR_ORANGE, //EGI_NOPRIM_COLOR, //WEGI_COLOR_ORANGE,
	.x0= 12,
	.y0= 0, // 25 - 320,
	.frame=-1, //no frame
	.tag="memo stick",
};

/*--------------------------------
return:
	0 	OK
	<0	fail
--------------------------------*/
int egi_obj_txtmemo_init(void)
{
	/* init txtbox data: txt offset(5,5) to box, 12_lines, 24bytes char per line, font, font_color */
	if( egi_init_data_txt(&memo_txt, 5, 5, 12, 24, &sympg_testfont, WEGI_COLOR_BLACK) == NULL ) {
		printf("init MEMO data txt fail!\n");
		return -1;
	}
	/* indicate a txt file */
	memo_txt.fpath="/home/memo.txt";

	ebox_memo.egi_data =(void *)&memo_txt; /* try &note_txt.....you may use other txt data  */

	return 0;
}
