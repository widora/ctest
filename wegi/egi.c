

#include <stdio.h>
#include "egi.h"


/*------------------------------------------------------------------
find the ebox according to given x,y
x,y: point at request
ebox:  ebox pointer
num: total number of eboxes

return:
	>=0   Ok, as ebox pointer index
	<0    not in eboxes
-------------------------------------------------------------------*/
int egi_get_boxindex(int x,int y, struct egi_element_box *ebox, int num)
{
	int i=num;

	for(i=0;i<num;i++)
	{
		if( x>=ebox[i].x0 && x<=ebox[i].x0+ebox[i].width \
			&& y>=ebox[i].y0 && y<=ebox[i].y0+ebox[i].height )
			return i;
	}

	return -1;
}


