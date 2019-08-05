/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Test egi_math functions


Midas Zhou
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "egi_math.h"
#include <math.h>
#include <inttypes.h>
#include "egi_timer.h"

int mat_FFTu16(	uint16_t np, const EGI_FCOMPLEX *wang, const float *x, EGI_FCOMPLEX *ffx);


int main(void)
{
int i,j,s,k;

#if 0  /////////////  1.  TEST SQRTU32()  ///////////
int m;
for(m=0;m<32; m++)
{
  printf("sqrtu32( 1<<%d )=%"PRId64"\n", m, (mat_fp16_sqrtu32(1<<m)>>16));
}
#endif   ///////////////////////////////////////


#if 0 /////////////////// 2. TEST COMPLEX  FUNCTIION  //////////////////////////////
/*--------------- test fix point ----------------*/
float a=1<<15; //-3.12322;
float b=1<<15; //-34.39001;

EGI_FVAL fva=MAT_FVAL(a);
EGI_FVAL fvb=MAT_FVAL(b);
EGI_FVAL fvc,fvs,fvm,fvd;

printf("\n---------------- Test Fix Point Functions ------------\n");
printf("fva=");
mat_FixPrint(fva);
printf("\n");
printf("Float compare: 	a=%15.9f, fva=%15.9f\n", a, mat_floatFix(fva));

printf("fvb=");
mat_FixPrint(fvb);
printf("\n");
printf("Float compare: 	b=%15.9f, fvb=%15.9f\n", b, mat_floatFix(fvb));

fvc=mat_FixAdd(fva, fvb);	/* ADD */
printf("fvc=fva+fvc=");
mat_FixPrint(fvc);
printf("\n");
printf("Float compare: 	c=a+b=%15.9f, fvc=%15.9f\n", a+b, mat_floatFix(fvc));

fvs=mat_FixSub(fva, fvb);	/* Subtraction */
printf("fvs=fva-fvc=");
mat_FixPrint(fvs);
printf("\n");
printf("Float compare: 	s=a-b=%15.9f, fvs=%15.9f\n", a-b, mat_floatFix(fvs));

fvm=mat_FixMult(fva,fvb);	/* Multiply */
printf("Float compare: 	a*b=%15.9f, fvm=fva*fvb=%15.9f\n", a*b, mat_floatFix(fvm));

fvd=mat_FixDiv(fvb,fva);	/* Divide */
printf("Float compare: 	b/a=%15.9f, fvb/fva=%15.9f\n", b/a, mat_floatFix(fvd));


/*--------------- test fix complex ----------------*/
//float ar=0.707, aimg=-0.707;
float	ar=1<<16, aimg=1<<16; /* !!!taken 16, only you know it will not flow over. */
//float br=1.414, bimg=-1.414;
float	br=1<<15, bimg=1<<15;

EGI_FCOMPLEX cpa=MAT_FCPVAL(ar, aimg);
EGI_FCOMPLEX cpb=MAT_FCPVAL(br, bimg);
EGI_FCOMPLEX cpc,cps,cpm,cpd;

printf( "\n -------------- Test Fix Point Complex Functions ----------\n");
printf("Float compare: ca=%f%+fj	cpa=%f%+fj\n", ar,aimg, mat_floatFix(cpa.real), mat_floatFix(cpa.imag));
printf("Float compare: cb=%f%+fj	cpb=%f%+fj\n", br,bimg, mat_floatFix(cpb.real), mat_floatFix(cpb.imag));

cpc=mat_CompAdd(cpa, cpb);	/* ADD */
printf("Float compare: cc=ca+cb=%f%+fj      cpc=cpa+cpb=%f%+fj\n",
				ar+br,aimg+bimg, mat_floatFix(cpc.real), mat_floatFix(cpc.imag));

cps=mat_CompSub(cpa, cpb);	/* Subtraction */
printf("Float compare: cs=ca-cb=%f%+fj      cps=cpa-cpb=%f%+fj\n",
				ar-br,aimg-bimg, mat_floatFix(cps.real), mat_floatFix(cps.imag));

cpm=mat_CompMult(cpa, cpb);	/* Multiply */
printf("Float compare: cm=ca*cb=%f%+fj      cpm=cpa*cpb=%f%+fj\n",
                             ar*br-aimg*bimg,ar*bimg+br*aimg, mat_floatFix(cpm.real), mat_floatFix(cpm.imag));

cpd=mat_CompDiv(cpb, cpa);     /* Multiply */
printf("Float compare: cd=cb/ca=%f%+fj      cpd=cpb/cpa=%f%+fj\n",
//                             (br*ar+aimg*bimg)/(br*br+bimg*bimg),(-ar*bimg+br*aimg)/(br*br+bimg*bimg),
                             (br*ar+aimg*bimg)/(ar*ar+aimg*aimg),(ar*bimg-br*aimg)/(ar*ar+aimg*aimg),
			     mat_floatFix(cpd.real), mat_floatFix(cpd.imag));
printf("\n\n");

return 0;
#endif ///////////////////  END COMPLEX FUNCTION TEST    ///////////////////



#if 1 ///////////////////  3. TEST FIXED POINT FFT  //////////////////////////////
	struct timeval tm_start,tm_end;
	uint16_t np=1<<8; //10;
	double famp;

	float *x;	    /* input */
	EGI_FCOMPLEX *ffx; /* result */
	EGI_FCOMPLEX *wang;

	x=calloc(np, sizeof(float));
	if(x==NULL)
        	return -1;

	ffx=calloc(np, sizeof(EGI_FCOMPLEX));
	if(ffx==NULL)
		return -1;

	/* generate NN points samples */
	for(i=0; i<np; i++) {
		x[i]=10.5*sin(2.0*MATH_PI*1000*i/8000) +0.5*sin(2.0*MATH_PI*2000*i/8000+3.0*MATH_PI/4.0) \
		     +1.0*((1<<9))*sin(2.0*MATH_PI*3000*i/8000);  /* Amplitud Max. abt.(1<<12)+(1<<10) */
		printf(" x[%d]: %f\n", i, x[i]);
	}
	/* prepare phase angle */
	wang=mat_CompFFTAng(np);

k=0;
do { 	///// ------- LOOP TEST FFT ------- //////
k++;

	gettimeofday(&tm_start, NULL);

	/* call fix_FFT */
	mat_FFTu16(np, wang, x, ffx);

	gettimeofday(&tm_end, NULL);

	/* print result */
	#if 1
	if( (k&(64-1))==0 )
	  for(i=0; i<np; i++) {
		  famp=mat_floatCompAmp(ffx[i])/(np/2);
		 // if(i==704||i==768||i==832) {// famp>0.001 || famp <-0.001 ) {
			printf("ffx[%d] ",i);
			mat_CompPrint(ffx[i]);
		 	printf(" Amp=%f ", famp);
			printf("\n");
		//}
  	}
	#endif


	if( (k&(64-1))==0 )
	   printf(" --- K=%d  time cost: %d us\n", k, tm_diffus(tm_start,tm_end));

} while(1);  ///// --------  END LOOP TEST  -------- //////


free(wang);
free(x);
free(ffx);
#endif /////////////////// END  FFT TEST ////////////////////////

return 0;
}






/*-------------------------------------------------------------------------------------
U16 Fixed point Fast Fourier Transform

@np:	Total number of data for FFT, will be ajusted to a number of 2
	powers. Max=1<<15;
@wang	complex phase angle factor, according to normalized np.
@x:	Pointer to array of input data x[]
@ffx:	Pointer to array of FFT result ouput.

Note:
1. Input number of points will be ajusted to a number of 2 powers.
2. Actual amplitude is FFT_A/(NN/2), where FFT_A is FFT result amplitud.
3. !!!--- Cal overflow ---!!!
   Expected amplitude 1<<N, expected sampling point: 1<<M
   Taken: N+M-1=16  to incorporate with mat_floatCompAmp()
   Example: Amp_Max: 1<<12;  FFT point number: 1<<5:  so Amp_Max*(np/2) : 12+(5-1)=16;


Return:
	0	OK
	<0	Fail
---------------------------------------------------------------------------------------*/
int mat_FFTu16(	uint16_t np, const EGI_FCOMPLEX *wang, const float *x, EGI_FCOMPLEX *ffx)
{
	int i,j,s;
	int kx,kp;
	int exp=0;		/* 2^exp=nn */
	int nn;  		/* number of data for FFT analysis, a number of 2 powers */
	EGI_FCOMPLEX *ffodd;	/* for FFT STAGE 1,3,5.. , store intermediate result */
	EGI_FCOMPLEX *ffeven; 	/* for FFT STAGE 0,2,4.. , store intermediate result */
	int *ffnin;		/* normal order mapped index */

	/* check input data */
	if( np==0 || x==NULL || ffx==NULL) {
		printf("%s: Invalid input data!\n",__func__);
		return -1;
	}

	/* get exponent number of 2 */
	for(i=0; i<16; i++) {
		if( (np<<i) & (1<<15) ){
			exp=16-i-1;
			break;
		}
	}
//	if(exp==0)
//		return -1;

	/* reset nn to be powers of 2 */
	nn=1<<exp;

	/* allocate vars !!!!! This is time consuming !!!!!! */
	ffodd=calloc(nn, sizeof(EGI_FCOMPLEX));
	if(ffodd==NULL) {
		printf("%s: Fail to alloc ffodd[] \n",__func__);
		return -1;
	}
	ffeven=calloc(nn, sizeof(EGI_FCOMPLEX));
	if(ffeven==NULL) {
		printf("%s: Fail to alloc ffeven[] \n",__func__);
		free(ffodd);
		return -1;
	}
	ffnin=calloc(nn, sizeof(int));
	if(ffnin==NULL) {
		printf("%s: Fail to alloc ffnin[] \n",__func__);
		free(ffodd); free(ffeven);
		return -1;
	}


	//////////////////////  FFT  ///////////////////////////
	/* 1. map normal order index to  input x[] index */
	for(i=0; i<nn; i++)
	{
		ffnin[i]=0;
		for(j=0; j<exp; j++) {
			/*  move i(j) to i(0), then left shift (exp-j) bits and assign to ffnin[i](exp-j) */
			ffnin[i] += ((i>>j)&1) << (exp-j-1);
		}
		//printf("ffnin[%d]=%d\n", i, ffnin[i]);
	}

	/*  2. store x() to ffeven[], index as mapped according to ffnin[] */
	for(i=0; i<nn; i++)
		ffeven[i]=MAT_FCPVAL(x[ffnin[i]], 0.0);

	/* 3. stage 2^1 -> 2^2 -> 2^3 -> 2^4....->NN point DFT */
	for(s=1; s<exp+1; s++) {
	    for(i=0; i<nn; i++) {   /* i as normal order index */
        	/* get coupling x order index: ffeven order index -> x order index */
	        kx=((i+ (1<<(s-1))) & ((1<<s)-1)) + ((i>>s)<<s); /* (i+2^(s-1)) % (2^s) + (i/2^s)*2^s) */

	        /* get 8th complex phase angle index */
	        kp= (i<<(exp-s)) & ((1<<exp)-1); // k=(i*1)%8

		if( s & 1 ) { 	/* odd stage */
			if(i < kx )
				ffodd[i]=mat_CompAdd( ffeven[i], mat_CompMult(ffeven[kx], wang[kp]) );
			else	  /* i > kx */
				ffodd[i]=mat_CompAdd( ffeven[kx], mat_CompMult(ffeven[i], wang[kp]) );
		}
		else {		/* even stage */
			if(i < kx) {
				ffeven[i]=mat_CompAdd( ffodd[i], mat_CompMult(ffodd[kx], wang[kp]) );
				//printf(" stage 2: ffeven[%d]=ffodd[%d]+ffodd[%d]*wang[%d]\n", i,i,kx,kp);
			}
			else {  /* i > kx */
				ffeven[i]=mat_CompAdd( ffodd[kx], mat_CompMult(ffodd[i], wang[kp]) );
				//printf(" stage 2: ffeven[%d]=ffodd[%d]+ffodd[%d]*wang[%d]\n", i,kx,i,kp);
			}
		} /* end of odd and even stage cal. */
	    } /* end for(i) */
	} /* end for(s) */


	/* 4. pass result */
	if(exp&1) {  /* result in odd array */
		for(i=0; i<nn; i++) {
			ffx[i]=ffodd[i];
#if 0
		        printf("X(%d) ",i);
	                mat_CompPrint(ffodd[i]);
        	        printf(" Amp=%f ",mat_floatCompAmp(ffodd[i])/(nn/2) );
			printf("\n");
#endif
		}
	} else {    /* result in even array */
		for(i=0; i<nn; i++) {
			ffx[i]=ffeven[i];
#if 0
		        printf("X(%d) ",i);
	                mat_CompPrint(ffeven[i]);
        	        printf(" Amp=%f ",mat_floatCompAmp(ffeven[i])/(nn/2) );
			printf("\n");
#endif 
		}
	}


/* free resources */
free(ffodd);
free(ffeven);
free(ffnin);

return 0;

}
