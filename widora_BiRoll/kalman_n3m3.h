/*-------------------------------------------------------------------------


---------------------------------------------------------------------------*/
#ifndef __KALMAN_N3M3__
#define __KALMAN_N3M3__

#include "mathwork.h"
#include "filters.h"

/*-------------------------------------   System Matrix    ------------------------------------------
( State Dimension: n, Observation Dimension m.   m<=n )

(input/output)  Mat_Y[nx1]:    the state (optimized mean value)  matrix of concerning variables (updated)
	 		     with init. value.
(-) 		Mat_Yp[nx1]:   predicted of Mat_Yp[]
(input)  	Mat_F[nxn]:  the state-transition matrix
(input)  	Mat_H[mxn]:  the observation transformation matrix
(input)		Mat_S[mx1]:  the observation matrix
(input) 	Mat_Q[nxn]:  the covariance matrix of the process noise
	 		     with some estimated init value.
(input)		Mat_R[mxm]:  the covarinace matrix of the observation noise
	 		     with some estimated init value.
(input/output) 	Mat_P[nxn]:  the state_estimation error covaraince matrix (updated)
	 		     init. with some estimated value.
(-)		Mat_Pp[nxn]: predicted of Mat_P[]
(-)		Mat_K[mxn]:  Kalman Gain matrix
(-)		Mat_I[nxn]:  Eye matrix with 1 on diagonal and zeros elesewhere
---------------------------------------------------------------------------------------------------*/

//--------- init N3M3 Kalman Filter Param. Matrixces  -------
float MatY[3*1]=  //----- state of (angle, angular rate,0)
{
0,
0,
0
};

float MatF[3*3]= //----- state transition matrix
{
  1, 5000, 0,  //5000 dt_us  will be real value
  0, 1, 0,
  0, 0, 0
};

float MatH[3*3]= //----- observation matrix
{ 1, 0, 0,
  0, 1, 0,
  0, 0, 0
};

float MatQ[3*3]= //----- process outside noise covariance
{
 0,0,0,
 0,0,0,
 0,0,0,
};

float MatR[3*3]= //----- observatin(reading) noise covaraince
{
  0.1,0,0,
  0,0,0,
  0,0,0
};

float MatP[3*3]= //----- state convaraince matrix 
{
 0.3, 0, 0,
 0, 0, 0,
 0, 0, 0
};


struct float_Matrix * pMat_Y;
struct float_Matrix * pMat_F;
struct float_Matrix * pMat_H;
struct float_Matrix * pMat_Q;
struct float_Matrix * pMat_R;
struct float_Matrix * pMat_P;
struct float_Matrix * pMat_S;// input readings

struct floatKalmanDB * fdb_kalman; //Kalman Fitler data base;


/*------------ Init. N3M3  KALMAN FILTER ---------
 Return:
       0  --- OK
      <0 --- Fail
---------------------------------------------------*/
int init_N3M3_KalmanFilter(void)
{
   pMat_Y=init_float_Matrix(3,1);
   Matrix_FillArray(pMat_Y,MatY);

   pMat_F=init_float_Matrix(3,3);
   Matrix_FillArray(pMat_F,MatF);

   pMat_H=init_float_Matrix(3,3);
   Matrix_FillArray(pMat_H,MatH);

   pMat_Q=init_float_Matrix(3,3);
   Matrix_FillArray(pMat_Q,MatQ);

   pMat_R=init_float_Matrix(3,3);
   Matrix_FillArray(pMat_R,MatR);

   pMat_P=init_float_Matrix(3,3);
   Matrix_FillArray(pMat_P,MatP);

   pMat_S = init_float_Matrix(3,1);// input readings


   fdb_kalman = Init_floatKalman_FilterDB( 3, 3, pMat_Y, pMat_F, pMat_P, pMat_H, pMat_Q, pMat_R);
   if(fdb_kalman==NULL)
   {
         fprintf(stderr,"Init. Kalman filter data base failed!\n");
	 return -1;
    }

    return 0;
}

//----- Release N3M3  KALMAN FILTER -----
void release_N3M3_KalmanFilter(void)
{
  //----- release data base -----
  Release_floatKalman_FilterDB(fdb_kalman);

  //----- release input matrix ------
  release_float_Matrix(pMat_S);
  release_float_Matrix(pMat_Y);
  release_float_Matrix(pMat_F);
  release_float_Matrix(pMat_P);
  release_float_Matrix(pMat_H);
  release_float_Matrix(pMat_Q);
  release_float_Matrix(pMat_R);
}




#endif
