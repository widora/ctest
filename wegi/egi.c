#include <stdio.h>
#include <stdbool.h>
#include "egi.h"



/*----------obselete!!!, substitued by egi_getindex_ebox() now!!! ----------------
  check if (px,py) in the ebox
  return true or false
  !!!--- ebox local coordinate original is NOT sensitive in this function ---!!!
--------------------------------------------------------------------------------*/
bool egi_point_inbox(int px,int py, struct egi_element_box *ebox)
{
        int xl,xh,yl,yh;
        int x1=ebox->x0;
	int y1=ebox->y0;
        int x2=x1+ebox->width;
	int y2=y1+ebox->height;

        if(x1>=x2){
                xh=x1;xl=x2;
        }
        else {
                xl=x1;xh=x2;
        }

        if(y1>=y2){
                yh=y1;yl=y2;
        }
        else {
                yh=y2;yl=y1;
        }

        if( (px>=xl && px<=xh) && (py>=yl && py<=yh))
                return true;
        else
                return false;
}


/*------------------------------------------------------------------
find the ebox index according to given x,y
x,y: point at request
ebox:  ebox pointer
num: total number of eboxes referred by *ebox.

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


