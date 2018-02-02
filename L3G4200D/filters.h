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
#define FLOAT_MAF_MAX_GRADE 10 //max grade limit for floatMA filter, 2^10 points moving average.

//---- int16_t  moving average filter Data Base ----
struct  int16MAFilterDB {
	uint16_t f_ng; //grade number,   point number = 2^ng
	//----allocate 2^ng+2 elements actually, 2 more expanded slot fors limit filter check !!!
	int16_t* f_buff;//buffer for averaging data,size=(2^ng+2) !!!
        int32_t f_sum; //sum of the buff data
        int16_t f_limit;//=0x7fff, //limit value for each data of *source.
};
//---- float_type  moving average filter Data Base ----
struct  floatMAFilterDB {
	uint16_t f_ng; //grade number,   point number = 2^ng
	//----allocate 2^ng+2 elements actually, 2 more expanded slot fors limit filter check !!!
	float* f_buff;//buffer for averaging data,size=(2^ng+2) !!!
        float f_sum; //sum of the buff data
        float f_limit;//=0x7fff, //limit value for each data of *source.
};

//----- float Kalman filter Data Base -----
struct floatKalmanDB {
	int n; // state var. dimension
	int m; // observation dimension
	struct float_Matrix *pMY;  //[nx1] state var. matrix
        struct float_Matrix *pMYp; //[nx1] predicted state var.
const	struct float_Matrix *pMF;  //[nxn] transition matrix
        struct float_Matrix *pMFtp;  //[nxn] transpose matrix of pMF
        struct float_Matrix *pMP;  //[nxn] state covariance
	struct float_Matrix *pMPp; //[nxn] predicted state conv. 
const   struct float_Matrix *pMH;  //[mxn] observation transformation
        struct float_Matrix *pMHtp;  //[mxn] transpose matrix of observation transformation
const   struct float_Matrix *pMQ;  //[nxn] system noise covariance
const	struct float_Matrix *pMR;  //[mxm] observation noise covariance
	struct float_Matrix *pMI; //[nxn] eye matrix with 1 on diagonal and zeros elesewhere
	struct float_Matrix *pMK; //[nxm] Kalman Gain Matrix

	//-----  temp. buff matrxi for matrix computation operation ------
	//Mat [mx1]
	struct float_Matrix *pMat_mX1A;
	//Mat [mx1]
	struct float_Matrix *pMat_mX1B;
	//Mat [mxm]
	struct float_Matrix *pMat_mXmA;
	//Mat [mxm]
	struct float_Matrix *pMat_mXmB;
	//Mat [mxm]
	struct float_Matrix *pMat_mXmC;
	//Mat [nx1]
	struct float_Matrix *pMat_nX1A;
	//Mat [nxm]
	struct float_Matrix *pMat_nXmA;
	//Mat [nxm]
	struct float_Matrix *pMat_nXmB;
	//Mat [nxn]
	struct float_Matrix *pMat_nXnA;
	//Mat [nxn]
	struct float_Matrix *pMat_nXnB;

};



//------------------------     FUNCTION DECLARATION     ---------------------------

//---------------  Int16 Type MAF  ---------------
inline int16_t int16_MAfilter(struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov);
inline int Init_int16MAFilterDB(struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit);
inline void Release_int16MAFilterDB(struct int16MAFilterDB *fdb);

inline void int16_MAfilter_NG(uint8_t m, struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov);
inline int Init_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit);
inline void Release_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb);

//<<<<<<<<<<<<<<<<  Float Type MAF  >>>>>>>>>>>>>
inline float float_MAfilter(struct floatMAFilterDB *fdb, const float *source, float *dest, int nmov);
inline int Init_floatMAFilterDB(struct floatMAFilterDB *fdb, uint16_t ng, float limit);
inline void Release_floatMAFilterDB(struct floatMAFilterDB *fd);

//----------------  IIR Filter  -------------
void IIR_Lowpass_int16Filter(const int16_t *p_in_data, int16_t *p_out_data, int nmov);
void IIR_Lowpass_dblFilter(const double *p_in_data, double *p_out_data, int nmov);


//-------------------------   Kalman Filter  ------------------------
/*  !!!WARNING!!! pMat_Y and pMat_P will be changed */
struct floatKalmanDB * Init_floatKalman_FilterDB(
                                     int n, int m,  //n---state var. dimension,  m---observation dimension
                                     struct float_Matrix *pMat_Y,  //[nx1] state var.
                             const   struct float_Matrix *pMat_F,  //[nxn] transition
                                     struct float_Matrix *pMat_P,  //[nxn] state covariance
                             const   struct float_Matrix *pMat_H,  //[mxn] observation transformation
                             const   struct float_Matrix *pMat_Q,  //[nxn] system noise covariance
                             const   struct float_Matrix *pMat_R );  //[mxm] observation noise covariance

void Release_floatKalman_FilterDB( struct floatKalmanDB *fdb );
void  float_KalmanFilter( struct floatKalmanDB *fdb,    //filter data base
                          const struct float_Matrix *pMS );   //[mx1] input observation matrix


#endif
