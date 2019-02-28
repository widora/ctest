/* ------------------------------------------------------------------------
NOTE:
1. egi_touch_loopread() will wait until live_touch_data.updated is fals,
   so discard first egi_touch_getdata() before loop call it.


TODO:
1. The speed of updating live_touch_data depends on gi_touch_loopread()
   however, other threads may not be able to keep up with it.
   try adjusting tm_delayms()...


Midas
-----------------------------------------------------------------------*/

#include "egi.h"
#include "egi_debug.h"
#include "egi_timer.h"
#include "xpt2046.h"
#include "egi_touch.h"
#include <stdbool.h>

static EGI_TOUCH_DATA live_touch_data;

/*------------------------------------------
pass touch data to the caller

return:
	true	pass updated data
	false	ingore obsolet data
------------------------------------------*/
bool egi_touch_getdata(EGI_TOUCH_DATA *data)
{
	if(!live_touch_data.updated)
	{
		data->updated=false;
		return false;
	}

	/* pass data, directly assign */
	*data=live_touch_data;
	/* reset update flag */
	live_touch_data.updated=false;

	//printf("--------- touch get data -----------\n");

	return true;
}


/*-------------------------------------------------------------------
loop in reading touch data and updating live_touch_data

NOTE:
	1. tm_timer(): start tick timer before run this function.
--------------------------------------------------------------------*/
void egi_touch_loopread(void)
{
	int ret;
	uint16_t sx,sy; /* last x,y */
	struct egi_point_coord sxy;
	int last_x,last_y; /* last recorded x,y */

        /* for time struct */
        struct timeval t_start,t_end; /* record two pressing_down time */
        long tus; /* time	 in us */

	/* reset status */
	enum egi_touch_status last_status;

	last_status=released_hold;
	live_touch_data.coord.x=0;
	live_touch_data.coord.y=0;
	live_touch_data.dx=0;
	live_touch_data.dy=0;
	live_touch_data.status=released_hold;


	while(1)
 	{
/*
enum egi_touch_status
{
        unkown=-1,
        releasing=0,
        pressing=1,
        released_hold=2,
        pressed_hold=3,
        db_releasing=4,
        db_pressing=5,
};
*/
	        /* 1. necessary wait,just for XPT to prepare data */
//
        	tm_delayms(2);

		/* wait .... until read out */
		if(live_touch_data.updated==true)
			continue;

		/* 2. read XPT to get avg tft-LCD coordinate */
        	//printf("start xpt_getavt_xy() \n");
        	ret=xpt_getavg_xy(&sx,&sy); /* if fail to get touched tft-LCD xy */
		sxy.x=sx; sxy.y=sy;

        	/* 3. touch reading is going on... */
        	if(ret == XPT_READ_STATUS_GOING )
        	{
			//printf("XPT READ STATUS GOING ON....\n");
               	 	/* DO NOT assign last_status=unkown here!!! because it'll always happen!!!
                           and you will never get pressed_hold status if you do so. */

			continue; /* continue to loop to finish reading touch data */
		}

               	/* 4. put PEN-UP status events here */
                else if(ret == XPT_READ_STATUS_PENUP )
                {
                        if(last_status==pressing || last_status==db_pressing || last_status==pressed_hold)
                        {
                                last_status=releasing; /* or db_releasing */
                                EGI_PDEBUG(DBG_TOUCH,": ... ... ... pen releasing ... ... ...\n");

				/* update touch data */
				live_touch_data.coord=sxy; /* record the last point coord */
				live_touch_data.status=releasing;
				live_touch_data.updated=true;
				/* reset sliding deviation */
				live_touch_data.dx=0;
				last_x=sx;
				live_touch_data.dy=0;
				last_y=sy;

                        }
                        else /* last_status also released_hold */
			{
                                last_status=released_hold;
				/* reset last_x,y */
				last_x=0;
				last_y=0;
				/* update touch data */
				live_touch_data.updated=true;
				live_touch_data.status=released_hold;
			}
                        tm_delayms(100);/* hold on for a while to relive CPU load, or the screen will be ...heheheheheh... */
                }

		/* 5. get touch coordinates and trigger actions for the hit button if any */
		else if(ret == XPT_READ_STATUS_COMPLETE) /* touch action detected */
		{
                        /* CASE HOLD_ON: check if hold on */
                        if( last_status==pressing || last_status==db_pressing || last_status==pressed_hold )
                        {
                                last_status=pressed_hold;
                                EGI_PDEBUG(DBG_TOUCH," ... ... ... pen hold down ... ... ...\n");

				/* update touch data */
				live_touch_data.coord=sxy;
				live_touch_data.updated=true;
				live_touch_data.status=pressed_hold;
				/* sliding deviation */
				live_touch_data.dx += (sx-last_x);
				last_x=sx;
				live_touch_data.dy += (sy-last_y);
				last_y=sy;
				EGI_PDEBUG(DBG_TOUCH,"egi_touch_loopread(): ...... dx=%d, dy=%d ......\n",
								live_touch_data.dx,live_touch_data.dy );

                        }
                        else /* CASE PRESSING: it's a pressing action */
                        {
                                last_status=pressing;
				/* update touch data */
				live_touch_data.dx = 0;
				live_touch_data.dy = 0;
				live_touch_data.coord=sxy;
				live_touch_data.updated=true;
				live_touch_data.status=pressing;
				last_x=sx;
				last_y=sy;
                      		EGI_PDEBUG(DBG_TOUCH,"egi_touch_loopread(): ... ... ... pen pressing ... ... ...\n");

                                /* check if it's a double-click   */
                                t_start=t_end;
                                gettimeofday(&t_end,NULL);
                                tus=tm_diffus(t_end,t_start);
                                //printf("------- diff us=%ld  ---------\n",tus);
                                if( tus < TM_DBCLICK_INTERVAL )
                                {
                                        EGI_PDEBUG(DBG_TOUCH,"egi_touch_loopread(): ... ... ... double click,tus=%ld    \
											  ... ... ...\n",tus);
                                        live_touch_data.status=db_pressing;
                                }
                        }
                        //eig_pdebug(DBG_TOUCH,"egi_touch_loopread(): --- XPT_READ_STATUS_COMPLETE ---\n");

		}

	}/* while() end */
}


