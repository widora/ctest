/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
-------------------------------------------------------------------*/


#ifndef __EGI_MATH_H__
#define __EGI_MATH_H__

#include "egi.h"
#include "egi_fbgeom.h"

#define MATH_PI 3.1415926535897932

void mat_create_fptrigontab(void);
/*
void mat_pointrotate_SQMap(int n, int angle, struct egi_point_coord centxy,
                                                        struct egi_point_coord *SQMat_XRYR);
*/

/* float point type */
void mat_pointrotate_SQMap(int n, double angle, struct egi_point_coord centxy,
                                                        struct egi_point_coord *SQMat_XRYR);
/* fixed point type */
void mat_pointrotate_fpSQMap(int n, int angle, struct egi_point_coord centxy,
                                                         struct egi_point_coord *SQMat_XRYR);

void mat_pointrotate_fpAnnulusMap(int n, int ni, int angle, struct egi_point_coord centxy,
                                                         struct egi_point_coord *ANMat_XRYR);



#endif
