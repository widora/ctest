#ifndef __EGI_TOUCH_H__
#define __EGI_TOUCH_H__

#include "egi.h"
#include "egi_fbgeom.h"
#include <stdbool.h>

#if 0
typedef struct egi_touch_data EGI_TOUCH_DATA;
struct egi_touch_data
{
	/* need semaphore lock ?????
	TODO:         */

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
#endif

bool egi_touch_getdata(EGI_TOUCH_DATA *data);
void egi_touch_loopread(void); /* for thread func */


#endif
