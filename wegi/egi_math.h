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
#include <inttypes.h>

#define MATH_PI 3.1415926535897932

extern int fp16_sin[360];
extern int fp16_cos[360];

/* EGI fixed point / complex number */
typedef struct {
int64_t         num;	/* divident */
int	        div;	/* divisor, exponent of 2, take as 32-1 NOW??? */
} EGI_FVAL;

typedef struct {
EGI_FVAL       real;	/* real part */
EGI_FVAL       imag;	/* imaginery part */
} EGI_FCOMPLEX;

/* TODO: adjustable exponent value  MAT_FVAL(a,exp) */
#define MAT_FVAL(a) ( (EGI_FVAL){ (a)*(1U<<15), 15} ) /* shitf 15bit each time, 64-15-15-15-15-1 */

/* r--real, i--imagnery */
#define MAT_FCPVAL(r,i)	 ( (EGI_FCOMPLEX){ MAT_FVAL(r), MAT_FVAL(i) } )

/* complex phase angle factor
 @n:	m*(2*PI)/N  nth phase angle
 @N:	Total parts of a circle(2*PI).
*/
#define MAT_CPANGLE(n, N)  ( MAT_FCPVAL( cos(2.0*n*MATH_PI/N), -sin(2.0*n*MATH_PI/N) ) )

void mat_FixPrint(EGI_FVAL a);
void mat_CompPrint(EGI_FCOMPLEX a);
inline float mat_floatFix(EGI_FVAL a);
inline EGI_FVAL mat_FixAdd(EGI_FVAL a, EGI_FVAL b);
inline EGI_FVAL mat_FixSub(EGI_FVAL a, EGI_FVAL b);
inline EGI_FVAL mat_FixMult(EGI_FVAL a, EGI_FVAL b);
inline EGI_FVAL mat_FixDiv(EGI_FVAL a, EGI_FVAL b);
inline EGI_FCOMPLEX mat_CompAdd(EGI_FCOMPLEX a, EGI_FCOMPLEX b);
inline EGI_FCOMPLEX mat_CompSub(EGI_FCOMPLEX a, EGI_FCOMPLEX b);
inline EGI_FCOMPLEX mat_CompMult(EGI_FCOMPLEX a, EGI_FCOMPLEX b);
inline EGI_FCOMPLEX mat_CompDiv(EGI_FCOMPLEX a, EGI_FCOMPLEX b);
float mat_floatCompAmp( EGI_FCOMPLEX a );

void mat_create_fptrigontab(void);
uint64_t mat_fp16_sqrtu32(uint32_t x);
void egi_float_limits(float *data, int num, float *min, float *max);

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
