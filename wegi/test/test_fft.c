/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example to test  EGI Fixed Point Fast Fourier Transform function.

Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "egi_math.h"
#include <math.h>
#include <inttypes.h>
#include "egi_timer.h"

int main(void)
{
	int i,k;


#if 0  /////////////  TEST SQRTU32()  ///////////
int m;
for(m=0;m<32; m++)
{
  printf("sqrtu32( 1<<%d )=%"PRId64"\n", m, (mat_fp16_sqrtu32(1<<m)>>16));
}
#endif   ///////////////////////////////////////


	struct timeval tm_start,tm_end;
	uint16_t np=1<<11; //<<9;
	double famp;

	float *x;	    /* input */
	EGI_FCOMPLEX *ffx; /* result */
	EGI_FCOMPLEX *wang;

	/* allocate vars */
	x=calloc(np, sizeof(float));
	if(x==NULL)
        	return -1;
	ffx=calloc(np, sizeof(EGI_FCOMPLEX));
	if(ffx==NULL)
		return -1;

	/* generate NN points samples */
	for(i=0; i<np; i++) {
		x[i]=10.5*sin(2.0*MATH_PI*1000*i/8000) +0.5*sin(2.0*MATH_PI*2000*i/8000+3.0*MATH_PI/4.0) \
		     +1.0*((1<<10))*sin(2.0*MATH_PI*3000*i/8000+MATH_PI/4.0);  /* Amplitud Max. abt.(1<<12)+(1<<10) */
		printf(" x[%d]: %f\n", i, x[i]);
	}

	/* prepare phase angle */
	wang=mat_CompFFTAng(np);

k=0;
do { 	///// --------------- LOOP TEST FFT ----------------- //////
k++;

	gettimeofday(&tm_start, NULL);

	/* call fix_FFT */
	mat_egiFFFT(np, wang, x, ffx);

	gettimeofday(&tm_end, NULL);

	/* print result */
	#if 1
	if( (k&(64-1))==0 )
	  for(i=0; i<np; i++) {
		  famp=mat_floatCompAmp(ffx[i])/(np/2);
		 if( famp > 0.001 || famp < -0.001 ) {
			printf("ffx[%d] ",i);
			mat_CompPrint(ffx[i]);
		 	printf(" Amp=%f ", famp);
			printf("\n");
		}
  	}
	#endif

	if( (k&(64-1))==0 )
	   printf(" --- K=%d  time cost: %d us\n", k, tm_diffus(tm_start,tm_end));

} while(1);  ///// --------  END LOOP TEST  -------- //////

free(wang);
free(x);
free(ffx);

return 0;
}


