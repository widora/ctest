#ifndef __EGI_TOUCH_H__
#define __EGI_TOUCH_H__

#include "egi.h"
#include "egi_fbgeom.h"
#include <stdbool.h>


/*
enum egi_touch_status
{
        unkown=-1,   during reading or fails
        releasing=0,   status transforming from pressed_hold to released_hold
        pressing=1,     status transforming from released_hold to pressed_hold
        released_hold=2,
        pressed_hold=3,
        db_releasing=4, double click, the last releasing
        db_pressing=5,  double click, the last pressing
};
*/


typedef struct egi_touch_data EGI_TOUCH_DATA;
struct egi_touch_data
{
	/* need semaphore lock ????? */


	/* flag, whether the data is updated after read out */
	bool updated;

	/* the last touch status */
	enum egi_touch_status	status;

	/* the latest touched point coordinate */
	struct egi_point_coord coord;

	/* the sliding deviation of coordXY from the beginnig touch point,
	in LCD coordinate */
	int	delx;
	int	dely;
};


bool egi_touch_getdata(EGI_TOUCH_DATA *data);
void egi_touch_loopread(void);



#endif
