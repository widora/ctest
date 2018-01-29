#ifndef __FILTER__H__
#define __FILTER__H__

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include "mathwork.h"

#define PI 3.1415926535898
#define INT16MAF_MAX_GRADE 10  //max grade limit for int16MA filter, 2^10 points moving average.


//---- int16_t  moving average filter Data Base ----
struct  int16MAFilterDB {
	uint16_t f_ng; //grade number,   point number = 2^ng
	//----allocate 2^ng+2 elements actually, 2 more expanded slot fors limit filter check !!!
	int16_t* f_buff;//buffer for averaging data,size=(2^ng+2) !!!
        int32_t f_sum; //sum of the buff data
        int16_t f_limit;//=0x7fff, //limit value for each data of *source.
};

//----- float Kalman filter Data Base -----
struct floatKalmanDB {
	int n; // state var. dimension
	int m; // observation dimension
	struct float_Matrix *pMY;  //[nx1] state var. matrix
        struct float_Matrix *pMYp; //[nx1] predicted state var.
        struct float_Matrix *pMF;  //[nxn] transition matrix
        struct float_Matrix *pMP;  //[nxn] state covariance
	struct float_Matrix *pMPp; //[nxn] predicted state conv. 
        struct float_Matrix *pMH;  //[mxn] observation transformation
        struct float_Matrix *pMQ;  //[nxn] system noise covariance
        struct float_Matrix *pMR;  //[mxm] observation noise covariance
	struct float_Matrix *pMI; //[nxn] eye matrix with 1 on diagonal and zeros elesewhere
	struct float_Matrix *pMK; //[mxn] Kalman Gain Matrix

	//-----  temp. buff matrxi for matrix computation operation ------
	//Mat [mxm]
	struct float_Matrix *pMat_mXmA;
	//Mat [mxm]
	struct float_Matrix *pMat_mXmB;
	//Mat [mxm]
	struct float_Matrix *pMat_mXmC;
	//----Mat [nxm]
	struct float_Matrix *pMat_nXmA;
	//----Mat [nxm]
	struct float_Matrix *pMat_nXmB;
	//----Mat [nxn]
	struct float_Matrix *pMat_nXnA;
	//----Mat [nxn]
	struct float_Matrix *pMat_nXnB;

};



//------------------------     FUNCTION DECLARATION     ---------------------------
inline int16_t int16_MAfilter(struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov);
inline int Init_int16MAFilterDB(struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit);
inline void Release_int16MAFilterDB(struct int16MAFilterDB *fdb);
inline void int16_MAfilter_NG(uint8_t m, struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov);
inline int Init_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit);
inline void Release_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb);
void IIR_Lowpass_int16Filter(int16_t *p_in_data, int16_t *p_out_data, int nmov);
void IIR_Lowpass_dblFilter(double *p_in_data, double *p_out_data, int nmov);
//-------- Kalman Filter ----


#endif
