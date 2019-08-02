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

int main(void)
{



/*--------------- test fix point ----------------*/
float a=-3.12322;
float b=-34.39001;

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
float ar=0.707, aimg=-0.707;
float br=1.414, bimg=-1.414;

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


/*----------------------- test fix point FFT --------------------------
8_points FFT test example is from:
	<< Understanding Digital Signal Processing, Second Edition >>
			    by Richard G.Lyons
	p118-120
----------------------------------------------------------------------*/
int i;
int kx,kp;

#define	NN 8
EGI_FCOMPLEX  wang[NN];

printf( "\n -------------- Test Complex phase angle factor ----------\n");

/* generate complex phase angle factor */
for(i=0;i<NN;i++) {
	wang[i]=MAT_CPANGLE(i, NN);
	mat_CompPrint(wang[i]);
	printf("\n");
}

/* sample data */
float x[NN]={ 0.3535, 0.3535, 0.6464, 1.0607, 0.3535, -1.0607, -1.3535, -0.3535 };

EGI_FCOMPLEX ffodd[NN];		/* FFT STAGE 1, 3 */
EGI_FCOMPLEX ffeven[NN];	/* FFT STAGE 2 */


/*** For 8 points FFT
   Order index: 	0,1,2,3, ...
   input x[] index:     0,4,2,6, ...
*/
/* order index -> input x[] index */
unsigned int  ffnin[NN]={ 0,4,2,6, 1,5,3,7 };

/* stage 1 */
for(i=0; i<NN; i++) {  /* order index: i */
	if( (i&(2-1)) == 0 ) { /* i%2 */
		ffodd[i]=MAT_FCPVAL( x[ffnin[i]]+x[ffnin[i+1]], 0.0 );
		printf(" stage 1: ffodd[%d]=x(%d)+x(%d)\n", i,ffnin[i],ffnin[i+1]);
	}
	else {
		ffodd[i]=MAT_FCPVAL( x[ffnin[i-1]]-x[ffnin[i]], 0.0);
		printf(" stage 1: ffodd[%d]=x(%d)-x(%d)\n", i,ffnin[i-1],ffnin[i]);
	}

}
/* print result */
for(i=0; i<8; i++) {
        printf("ffodd(%d) ",i);
        mat_CompPrint(ffodd[i]);
        printf("\n");
}

/* stage 2 */
for(i=0; i<NN; i++) { 	/* order index: i */

	/* get coupling x order index: ffeven order index -> x order index */
	//kx=(i+2)%4+(i&(~(4-1))); /* (i+2)%4+i/4 */
	kx=(i+2)%4+((i>>2)<<2); /* (i+2)%4+i/4 */

	/* get 8th complex phase angle index */
	kp=(i<<1)%8;

	/* cal. ffeven */
	if(i < kx) {
		ffeven[i]=mat_CompAdd( ffodd[i], mat_CompMult(ffodd[kx], wang[kp]) );
		printf(" stage 2: ffeven[%d]=ffodd[%d]+ffodd[%d]*wang[%d]\n", i,i,kx,kp);
	}
	else {  /* i > kx */
		ffeven[i]=mat_CompAdd( ffodd[kx], mat_CompMult(ffodd[i], wang[kp]) );
		printf(" stage 2: ffeven[%d]=ffodd[%d]+ffodd[%d]*wang[%d]\n", i,kx,i,kp);
	}
}
/* print result */
for(i=0; i<8; i++) {
       	printf("ffeven(%d) ",i);
        mat_CompPrint(ffeven[i]);
       	printf("\n");
}

/* stage 3 */
for(i=0; i<NN; i++) {   /* order index: i */
        /* get coupling x order index: ffeven order index -> x order index */
        kx=(i+4)%8; /* (i+4)%8 */

        /* get 8th complex phase angle index */
        kp=i;

	/* cal. ffodd */
	if(i < kx ) {
		ffodd[i]=mat_CompAdd( ffeven[i], mat_CompMult(ffeven[kx], wang[kp]) );
	}
	else {  /* i > kx */
		ffodd[i]=mat_CompAdd( ffeven[kx], mat_CompMult(ffeven[i], wang[kp]) );
	}
}
/* print result */
for(i=0; i<8; i++) {
	printf("X(%d) ",i);
	mat_CompPrint(ffodd[i]);
	printf("\n");
}

return 0;

}
