#ifndef __FILTER_H__
#define __FILTER_H__


#include <math.h>


#define PI 3.1415926535898

/*---------------------- IIR_BANDPASS_FILTER -----------------------------------
----- 4 order IIR filter Fl=300Hz,Fh=3400Hz,Fs=8kHz  -----
p_in_data:   pointer to start of input data
p_out_data:  pointer to start of output data
count:       count of data number in unit of int16_t
!!!! p_ind_data and p_out_data maybe the same address !!!!!
----------------------------------------------------------------------*/
static int IIR_bandpass_filter(int16_t *p_in_data, int16_t *p_out_data, int count)
{
int ret=0;
  //----factors for 4 order IIR filter Fl=300Hz,Fh=3400Hz,Fs=8kHz
static  double IIR_B[5]={ 0.6031972438993, 0, -1.206394487799, 0, 0.6031972438993 };
static  double IIR_A[5]={ 1,-0.325257157029, -1.004332872001, 0.1022259821442, 0.3705866844043 };
static  double w[5]={0.0, 0.0, 0.0, 0.0, 0.0};
int k;

for(k=0;k<count;k++)
 {
        w[0]=(*p_in_data)-IIR_A[1]*w[1]-IIR_A[2]*w[2]-IIR_A[3]*w[3]-IIR_A[4]*w[4];
        (*p_out_data)=IIR_B[0]*w[0]+IIR_B[1]*w[1]+IIR_B[2]*w[2]+IIR_B[3]*w[3]+IIR_B[4]*w[4];

        w[4]=w[3];
        w[3]=w[2];
        w[2]=w[1];
        w[1]=w[0];

        p_in_data++;
        p_out_data++;
 }

return ret;
}


/*---------------------- IIR_TRAPPER_FILTER -----------------------------------
----- 2 order IIR frequency trapper -----
p_in_data:   pointer to start of input data
p_out_data:  pointer to start of output data
count:       count of data number in unit of int16_t
int  freq:   frequency to trap
int  fs:     sample rate 
!!!! p_ind_data and p_out_data maybe the same address !!!!!
----------------------------------------------------------------------*/
static int IIR_freq_trapper(int16_t *p_in_data, int16_t *p_out_data, int count, int f0, int fs) 
{
 int ret=0;

 static double r=0.98; //0<r<1, to adjust the trapper(gap) band width,
 static double w0; // static init. requires constant value
 w0=2*PI*f0/fs;
 double IIR_B[3]={1.0, -2.0*cos(w0), 1.0};
 double IIR_A[3]={1.0, -2*r*cos(w0), r*r}; //{1,-a1,-a2,-a3...} as for Y(z)=B#/(1-SUM(A#)) in matlab FDAtool.
 static  double w[3]={0.0, 0.0, 0.0};
 int k;

for(k=0;k<count;k++)
 {
        w[0]=(*p_in_data)-IIR_A[1]*w[1]-IIR_A[2]*w[2];
        (*p_out_data)=IIR_B[0]*w[0]+IIR_B[1]*w[1]+IIR_B[2]*w[2];
	
	w[2]=w[1]; 
	w[1]=w[0];

        p_in_data++;
        p_out_data++;
  }

return ret;

}


#endif
