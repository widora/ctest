/*---------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas-Zhou
--------------------------------------------------------------------*/
#include "egi.h"
#include "egi_fbgeom.h"
#include "egi_math.h"
#include "egi_debug.h"
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


#define MATH_PI 3.1415926535897932

int fp16_sin[360]={0}; /* fixed point sin() value table, divisor 2<<16 */
int fp16_cos[360]={0}; /* fixed point cos() value table, divisor 2<<16 */

/*--------------------------------------------------------------
Create fixed point lookup table for
trigonometric functions: sin(),cos()
angle in degree

fb16 precision is abt.   0.0001, that means 1 deg rotation of
a 10000_pixel long radius causes only 1_pixel distance error.??

---------------------------------------------------------------*/
void mat_create_fptrigontab(void)
{
	double pi=3.1415926535897932;  /* double: 16 digital precision excluding '.' */
	int i;

	for(i=0;i<360;i++)
	{
		fp16_sin[i]=sin(1.0*i*pi/180.0)*(1<<16);
		fp16_cos[i]=cos(1.0*i*pi/180.0)*(1<<16);
		//printf("fp16_sin[%d]=%d\n",i,fp16_sin[i]);
	}
	//printf(" ----- sin(1)=%f\n",sin(1.0*pi/180.0));
}


/*----------------------------------------------------------------------------------
Fix point square root for uint32_t, the result is <<16 shifted, so you need to shift
>>16 back after call to get the right answer.

Original Source:
	http://read.pudn.com/downloads286/sourcecode/embedded/1291109/sqrt.c__.htm
------------------------------------------------------------------------------------*/
uint64_t mat_fp16_sqrtu32(uint32_t x)
{
	uint64_t x0=x;
	x0<<=32;
	uint64_t x1;
	uint32_t s=1;
	uint64_t g0,g1;

	if(x<=1)return x;

	x1=x0-1;
	if(x1>4294967296-1) {
		s+=16;
		x1>>=32;
	}
	if(x1>65535) {
		s+=8;
		x1>>=16;
	}
	if(x1>255) {
		s+=4;
		x1>>=8;
	}
	if(x1>15) {
		s+=2;
		x1>>=4;
	}
	if(x1>3) {
		s+=1;
	}

	g0=1<<s;
	g1=(g0+(x0>>s))>>1;

	while(g1<g0) {
		g0=g1;
		g1=(g0+x0/g0)>>1;
	}

	return g0; /* after call, right shift >>16 to get right answer */
}


/*---------------------------------------------------------------------
Get Min and Max value of a float array
data:	data array
num:	item size of the array
min,max:	Min and Max value of the array

---------------------------------------------------------------------*/
void egi_float_limits(float *data, int num, float *min, float *max)
{
	int i=num;
	float fmin=data[0];
	float fmax=data[0];

	if(data==NULL || min==NULL || max==NULL || num<=0 )
		return;

	for(i=0;i<num;i++) {
		if(data[i]>fmax)
			fmax=data[i];
		else if(data[i]<fmax)
			fmin=data[i];
	}

	*min=fmin;
	*max=fmax;
}

/*---------------------------------------------------------------------------------------
generate a rotation lookup map for a square image block

1. rotation center is the center of the square area.
2. The square side length must be in form of 2n+1, so the center is just at the middle of
   the symmetric axis.
3. Method 1 (BUG!!!!): from input coordinates (x,y) to get rotated coordinate(i,j) matrix,
   because of calculation precision limit, this is NOT a one to one mapping!!!  there are
   points that may map to the same point!!! and also there are points in result matrix that
   NOT have been mapped,they keep original coordinates.
4. Method 2: from rotated coordinate (i,j) to get input coordinates (x,y), this can ensure
   that the result points mastrix all be mapped.

n: 		pixel number for square side.
angle: 		rotation angle. clockwise is positive.
centxy 		the center point coordinate of the concerning square area of LCD(fb).
SQMat_XRYR: 	after rotation
	      square matrix after rotation mapping, coordinate origin is same as LCD(fb) origin.


Midas Zhou
-------------------------------------------------------------------------------------------*/
/*------------------------------- Method 1 --------------------------------------------*/

#if 0
void mat_pointrotate_SQMap(int n, double angle, struct egi_point_coord centxy,
                       					 struct egi_point_coord *SQMat_XRYR)
{
	int i,j;
	double sinang,cosang;
	double xr,yr;
	struct egi_point_coord  x0y0; /* the left top point of the square */
	double pi=3.1415926535897932;


	/* check if n can be resolved in form of 2*m+1 */
	if( (n-1)%2 != 0)
	{
		printf("mat_pointrotate_SQMap(): the number of pixels on the square side must be n=2*m+1.\n");
	 	return;
	}

/* 1. generate matrix of point_coordinates for the concerning square area, origin LCD(fb) */
	/* get left top coordinate */
	x0y0.x=centxy.x-((n-1)>>1);
	x0y0.y=centxy.y-((n-1)>>1);
	/* */
	for(i=0;i<n;i++) /* row index,Y */
	{
		for(j=0;j<n;j++) /* column index,X */
		{
			SQMat_XRYR[i*n+j].x=x0y0.x+j;
			SQMat_XRYR[i*n+j].y=x0y0.y+i;
			//printf("LCD origin: SQMat_XY[%d]=(%d,%d)\n",i*n+j,SQMat_XY[i*n+j].x, SQMat_XY[i*n+j].y);
		}
	}

/* 2. transform coordinate origin from LCD(fb) origin to the center of the square*/
	for(i=0;i<n*n;i++)
	{
		SQMat_XRYR[i].x -= x0y0.x+((n-1)>>1);
		SQMat_XRYR[i].y -= x0y0.y+((n-1)>>1);
	}

/* 3. map coordinates of all points to a new matrix by rotation transformation */
	sinang=sin(angle/180.0*pi);//MATH_PI);
	cosang=cos(angle/180.0*pi);//MATH_PI);
	//printf("sinang=%f,cosang=%f\n",sinang,cosang);

	for(i=0;i<n*n;i++)
	{
		/* point rotation transformation ..... */
		xr=(SQMat_XRYR[i].x)*cosang+(SQMat_XRYR[i].y)*sinang;
		yr=(SQMat_XRYR[i].x)*(-sinang)+(SQMat_XRYR[i].y)*cosang;
		//printf("i=%d,xr=%f,yr=%f,\n",i,xr,yr);

		/* check if new piont coordinate is within the square */
		if(  (xr >= -((n-1)/2)) && ( xr <= ((n-1)/2))
					 && (yr >= -((n-1)/2)) && (yr <= ((n-1)/2))  )
		{
			SQMat_XRYR[i].x=nearbyint(xr);//round(xr);
			SQMat_XRYR[i].y=nearbyint(yr);//round(yr);
		/*
			printf("JUST IN RANGE: xr=%f,yr=%f , origin point (%d,%d), new point (%d,%d)\n",
				xr,yr,SQMat_XY[i].x,SQMat_XY[i].y,SQMat_XRYR[i].x, SQMat_XRYR[i].y);
		*/
		}
		/*else:	just keep old coordinate */
		else
		{
			//printf("NOT IN RANGE: xr=%f,yr=%f , origin point (%d,%d)\n",xr,yr,SQMat_XY[i].x,SQMat_XY[i].y);
		}
	}


/* 4. transform coordinate origin back to X0Y0  */
	for(i=0;i<n*n;i++)
	{
                SQMat_XRYR[i].x += (n-1)>>1;
                SQMat_XRYR[i].y += (n-1)>>1;
		//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);
	}
	//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);
}
#endif

#if 1
/*----------------------- Method 2: revert rotation (float point)  -------------------------*/
void mat_pointrotate_SQMap(int n, double angle, struct egi_point_coord centxy,
                       					 struct egi_point_coord *SQMat_XRYR)
{
	int i,j;
	double sinang,cosang;
	double xr,yr;

	sinang=sin(1.0*angle/180.0*MATH_PI);
	cosang=cos(1.0*angle/180.0*MATH_PI);

	/* clear the result matrix  */
	memset(SQMat_XRYR,0,n*n*sizeof(struct egi_point_coord));

	/* check if n can be resolved in form of 2*m+1 */
	if( (n-1)%2 != 0)
	{
		printf("!!! WARNING !!! mat_pointrotate_SQMap(): the number of pixels on	\
							the square side is NOT in form of n=2*m+1.\n");
	}

/* 1. generate matrix of point_coordinates for the square, result Matrix is centered at square center. */
	/* */
	for(i=-n/2;i<=n/2;i++) /* row index,Y */
	{
		for(j=-n/2;j<=n/2;j++) /* column index,X */
		{
			/*   get XRYR revert rotation matrix centered at SQMat_XRYU's center
			this way can ensure all SQMat_XRYR[] points are filled!!!  */

			xr = j*cosang+i*sinang;
			yr = -j*sinang+i*cosang;

                	/* check if new piont coordinate is within the square */
                	if(  ( xr >= -((n-1)>>1)) && ( xr <= ((n-1)>>1))
                                         && ( yr >= -((n-1)>>1)) && ( yr <= ((n-1)>>1))  )
			{
				SQMat_XRYR[((2*i+n)/2)*n+(2*j+n)/2].x= round(xr);
				SQMat_XRYR[((2*i+n)/2)*n+(2*j+n)/2].y= round(yr);
			}
		}
	}


/* 2. transform coordinate origin back to X0Y0  */
	for(i=0;i<n*n;i++)
	{
//		if( SQMat_XRYR[i].x == 0 )
//			printf(" ---- SQMat_XRYR[%d].x == %d\n",i,SQMat_XRYR[i].x );

                SQMat_XRYR[i].x += ((n-1)>>1); //+centxy.x);
                SQMat_XRYR[i].y += ((n-1)>>1); //+centxy.y);
		//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);
	}
	//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);



//	free(Mat_tmp);
}
#endif

/*----------------------- Method 3: revert rotation (fixed point)  -------------------------*/
void mat_pointrotate_fpSQMap(int n, int angle, struct egi_point_coord centxy,
                       					 struct egi_point_coord *SQMat_XRYR)
{
	int i,j;
//	int sinang,cosang;
	int xr,yr;

	/* normalize angle to be within 0-360 */
	int ang=angle%360;
	int asign=ang >= 0 ? 1:-1; /* angle sign */
	ang=ang>=0 ? ang:-ang ;


	/* clear the result matrix */
	memset(SQMat_XRYR,0,n*n*sizeof(struct egi_point_coord));

	/* check if n can be resolved in form of 2*m+1 */
	if( (n-1)%2 != 0)
	{
		printf("!!! WARNING !!! mat_pointrotate_fpSQMap(): the number of pixels on	\
							the square side is NOT in form of n=2*m+1.\n");
	}


	/* check whether fp16_cos[] and fp16_sin[] is generated */
	if( fp16_sin[30] == 0)
		mat_create_fptrigontab();


/* 1. generate matrix of point_coordinates for the square, result Matrix is centered at square center. */
	/* */
	for(i=-n/2;i<=n/2;i++) /* row index,Y */
	{
		for(j=-n/2;j<=n/2;j++) /* column index,X */
		{
			/*   get XRYR revert rotation matrix centered at SQMat_XRYU's center
			this way can ensure all SQMat_XRYR[] points are filled!!!  */

			//xr = j*cosang+i*sinang;
			xr = (j*fp16_cos[ang]+i*asign*fp16_sin[ang])>>16;
			//yr = -j*sinang+i*cosang;
			yr = (-j*asign*fp16_sin[ang]+i*fp16_cos[ang])>>16;

                	/* check if new piont coordinate is within the square */
                	if(  ( xr >= -((n-1)>>1)) && ( xr <= ((n-1)>>1))
                                         && ( yr >= -((n-1)>>1)) && ( yr <= ((n-1)>>1))  )
			{
				SQMat_XRYR[((2*i+n)/2)*n+(2*j+n)/2].x= xr;
				SQMat_XRYR[((2*i+n)/2)*n+(2*j+n)/2].y= yr;
			}
		}
	}


/* 2. transform coordinate origin back to X0Y0  */
	for(i=0;i<n*n;i++)
	{
//		if( SQMat_XRYR[i].x == 0 )
//			printf(" ---- SQMat_XRYR[%d].x == %d\n",i,SQMat_XRYR[i].x );

                SQMat_XRYR[i].x += ((n-1)>>1); //+centxy.x);
                SQMat_XRYR[i].y += ((n-1)>>1); //+centxy.y);
		//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);
	}
	//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);


//	free(Mat_tmp);
}


/*----------------------- Annulus Mapping: revert rotation (fixed point)  ------------------
generate a rotation lookup map for a annulus shape image block

1. rotation center is the center of the square area.
2. The square side length must be in form of 2n+1, so the center is just at the middle of
   the symmetric axis.
3. Method 2: from rotated coordinate (i,j) to get input coordinates (x,y), this can ensure
   that the result points mastrix all be mapped.

n:              pixel number for ouside square side. also is the outer diameter for the annulus.
ni:		pixel number for inner square side, alos is the inner diameter for the annulus.
angle:          rotation angle. clockwise is positive.
centxy          the center point coordinate of the concerning square area of LCD(fb).
ANMat_XRYR:     after rotation
                square matrix after rotation mapping, coordinate origin is same as LCD(fb) origin.


Midas Zhou
-------------------------------------------------------------------------------------------*/

void mat_pointrotate_fpAnnulusMap(int n, int ni, int angle, struct egi_point_coord centxy,
                       					 struct egi_point_coord *ANMat_XRYR)
{
	int i,j,m,k;
//	int sinang,cosang;
	int xr,yr;

	/* normalize angle to be within 0-359 */
	int ang=angle%360;
	int asign=ang >= 0 ? 1:-1; /* angle sign */
	ang=(ang>=0 ? ang:-ang) ;

	/* clear the result maxtrix */
	memset(ANMat_XRYR,0,n*n*sizeof(struct egi_point_coord));

	/* check if n can be resolved in form of 2*m+1 */
	if( (n-1)%2 != 0)
	{
		printf("!!! WARNING !!! mat_pointrotate_fpAnnulusMap(): the number of pixels on	\
							the square side is NOT in form of n=2*m+1.\n");
	}


	/* check whether fp16_cos[] and fp16_sin[] is generated */
	//printf("prepare fixed point sin/cos ...\n");
	if( fp16_sin[30] == 0)
		mat_create_fptrigontab();


/* 1. generate matrix of point_coordinates for the square, result Matrix is centered at square center. */
	/* */
	for(j=0; j<=n/2;j++) /* row index,Y: 0->n/2 */
	{
		m=sqrt( (n/2)*(n/2)-j*j ); /* distance from Y to the point on outer circle */

		if(j<ni/2)
			k=sqrt( (ni/2)*(ni/2)-j*j); /* distance from Y to the point on inner circle */
		else
			k=0;

		for(i=-m;i<=-k;i++) /* colum index, X: -m->-n, */
		{
			/*   get XRYR revert rotation matrix centered at ANMat_XRYU's center
			this way can ensure all ANMat_XRYR[] points are filled!!!  */
			/* upper and left part of the annuls j: 0->n/2*/
			xr = (j*fp16_cos[ang]+i*asign*fp16_sin[ang])>>16;
			yr = (-j*asign*fp16_sin[ang]+i*fp16_cos[ang])>>16;
			ANMat_XRYR[(n/2-j)*n+(n/2+i)].x= xr;
			ANMat_XRYR[(n/2-j)*n+(n/2+i)].y= yr;
			/* lower and left part of the annulus -j: 0->-n/2*/
			xr = (-j*fp16_cos[ang]+i*asign*fp16_sin[ang])>>16;
			yr = (j*asign*fp16_sin[ang]+i*fp16_cos[ang])>>16;
			ANMat_XRYR[(n/2+j)*n+(n/2+i)].x= xr;
			ANMat_XRYR[(n/2+j)*n+(n/2+i)].y= yr;
			/* upper and right part of the annuls -i: m->n */
			xr = (j*fp16_cos[ang]-i*asign*fp16_sin[ang])>>16;
			yr = (-j*asign*fp16_sin[ang]-i*fp16_cos[ang])>>16;
			ANMat_XRYR[(n/2-j)*n+(n/2-i)].x= xr;
			ANMat_XRYR[(n/2-j)*n+(n/2-i)].y= yr;
			/* lower and right part of the annulus */
			xr = (-j*fp16_cos[ang]-i*asign*fp16_sin[ang])>>16;
			yr = (j*asign*fp16_sin[ang]-i*fp16_cos[ang])>>16;
			ANMat_XRYR[(n/2+j)*n+(n/2-i)].x= xr;
			ANMat_XRYR[(n/2+j)*n+(n/2-i)].y= yr;
		}
	}

/* 2. transform coordinate origin back to X0Y0  */
	for(i=0;i<n*n;i++)
	{
                ANMat_XRYR[i].x += ((n-1)>>1);
                ANMat_XRYR[i].y += ((n-1)>>1);
	}

}

