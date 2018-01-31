/*-------------------------------------------------------------------------
KALMAN filter test program
  Example derived from:	blog.csdn.net/zhangwenyu111/article/details/17034813


---------------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/time.h>
#include "mathwork.h"
#include "filters.h"

int main(void)
{
   int k,count;
   struct timeval tm_start, tm_end;

/*------------------ Parameter Description ---------------------
T: sampling period, T=1s.
n: samples, also as total sampling time(t) for T=1s.
s: distance of the object, init value 1000m
	s[n]=1000+v0*n+0.5*a[n]*n*n;
v: speed of the object,init value v0=50 m/s
	v[n]=v0+a[n]*n;
a: acceleration of the object, init value a0=20 m/s^2
	a[n]=a0=20;
wt: system noise
vt: measurement noise
---------------------------------------------------------------*/

/*------------------    Reading Values(Measurement)    ---------------------
#--- you may adjust follow noise altitude ---
mat_s[]: s with noise 0.5*wt;
mat_v[]: v with noise 100*wt;
mat_a[]: a with noise 1.0*wt;

MatS[]={(s,v,a) (s,v,a) ...}
---------------------------------------------------------------*/
#include "matrixS.h"
//float MatS[3*100]=   //measured value, object distance


/*-------------------------------------   System Matrix    ------------------------------------------
( State Dimension: n, Observation Dimension m.   m<=n )

(input/output)  Mat_Y[nx1]:    the state (optimized mean value)  matrix of concerning variables (updated)
	 		     with init. value.
(-) 		Mat_Yp[nx1]:   predicted of Mat_Yp[]
(input)  	Mat_F[nxn]:  the state-transition matrix
(input)  	Mat_H[mxn]:  the observation transformation matrix
(input)		Mat_S[mx1]:  the observation matrix
(input/output) 	Mat_Q[nxn]:  the covariance matrix of the process noise
	 		     with some estimated init value.
(input/output)	Mat_R[mxm]:  the covarinace matrix of the observation noise
	 		     with some estimated init value.
(input/output) 	Mat_P[nxn]:  the state_estimation error covaraince matrix (updated)
	 		     init. with some estimated value.
(-)		Mat_Pp[nxn]: predicted of Mat_P[]
(-)		Mat_K[mxn]:  Kalman Gain matrix
(-)		Mat_I[nxn]:  Eye matrix with 1 on diagonal and zeros elesewhere
---------------------------------------------------------------------------------------------------*/
//------------- init parameter matrixces  -------------
float MatY[3*1]=  //----- state of s,v,a
{
900,
0,
0
};
struct float_Matrix * pMat_Y=init_float_Matrix(3,1);
Matrix_FillArray(pMat_Y,MatY);

float MatF[3*3]= //----- state transition matrix
{
  1, 1, 0,
  0, 1, 1,
  0, 0, 1
};
struct float_Matrix * pMat_F=init_float_Matrix(3,3);
Matrix_FillArray(pMat_F,MatF);


float MatH[3*3]= //----- observation matrix
{ 1, 0, 0,
  0, 1, 0,
  0, 0, 1
};
struct float_Matrix * pMat_H=init_float_Matrix(3,3);
Matrix_FillArray(pMat_H,MatH);

float MatQ[3*3]= //----- process outside noise covariance
{
 1,0,0,
 0,0,0,
 0,0,0,
};
struct float_Matrix * pMat_Q=init_float_Matrix(3,3);
Matrix_FillArray(pMat_Q,MatQ);

float MatR[3*3]=
{
  5,0,0,
  0,2,0,
  0,0,1
}; //----- observatin(reading) noise covaraince
struct float_Matrix * pMat_R=init_float_Matrix(3,3);
Matrix_FillArray(pMat_R,MatR);

float MatP[3*3]= //----- state convaraince matrix 
{
 30, 0, 0,
 0, 20, 0,
 0, 0, 10
};
struct float_Matrix * pMat_P=init_float_Matrix(3,3);
Matrix_FillArray(pMat_P,MatP);

//----- init. input readings(measuremetn) and Kalman filter data base -----
struct float_Matrix *pMat_S = init_float_Matrix(3,1);// input readings (s,v,a)

//-----------   KALMAN FILTER TEST  ------------
struct floatKalmanDB *fdb_kalman;

k=0;

while(1)
{
	k++;


	// ------ !!!! since pMat_Y has been changed, refill init. data at the begin  ------
	Matrix_FillArray(pMat_Y,MatY);
	Matrix_FillArray(pMat_P,MatP);


	/* -----------------------------------------------------------------------------
	struct floatKalmanDB * Init_floatKalman_FilterDB(
                 int n, int m,  //n---state var. dimension,  m---observation dimension
                 struct float_Matrix *pMat_Y,  //[nx1] state var.
                 struct float_Matrix *pMat_F,  //[nxn] transition
                 struct float_Matrix *pMat_P,  //[nxn] state covariance
                 struct float_Matrix *pMat_H,  //[mxn] observation transformation
                 struct float_Matrix *pMat_Q,  //[nxn] outside noise covariance
                 struct float_Matrix *pMat_R );  //[mxm] observation noise covariance
	------------------------------------------------------------------------------- */
	fdb_kalman = Init_floatKalman_FilterDB( 3, 3, pMat_Y, pMat_F, pMat_P, pMat_H, pMat_Q, pMat_R);
	if(fdb_kalman ==NULL)
	{
		printf("Init. Kalman filter data base failed!\n");
		goto CALL_FAIL;
	}

/*
   printf("fdb->pMY:\n");
   Matrix_Print(*(fdb_kalman->pMY));
   printf("fdb->pMP:\n");
   Matrix_Print(*(fdb_kalman->pMP));
   printf("fdb->pMF:\n");
   Matrix_Print(*(fdb_kalman->pMF));
   printf("fdb->pMH:\n");
   Matrix_Print(*(fdb_kalman->pMH));
   printf("fdb->pMQ:\n");
   Matrix_Print(*(fdb_kalman->pMQ));
   printf("fdb->pMR:\n");
   Matrix_Print(*(fdb_kalman->pMR));
*/
 
	gettimeofday(&tm_start,NULL);
	for(count=0; count<100; count++)
	{
		//----- get input readings(measuremetn) ----
		Matrix_FillArray(pMat_S,MatS+3*count);
		//----- applay Kalman filter -----
		float_KalmanFilter( fdb_kalman, pMat_S );   //[mx1] input observation matrix

	} //end for()
	gettimeofday(&tm_end,NULL);
	printf("----- recursion count: %d, time elapsed: %d(ms) \n",k,get_costtimeus(tm_start, tm_end)/1000);
	printf("----- final Kalman Gain Matrix K= \n");
	Matrix_Print(*fdb_kalman->pMK);
	printf("----- final State Covaraince Matrix  P= \n");
	Matrix_Print(*fdb_kalman->pMP);


CALL_FAIL:
	//---- relase data base after filtering ----
	Release_floatKalman_FilterDB(fdb_kalman);
	usleep(800000);

 }//end while()

  release_float_Matrix(pMat_S);
  release_float_Matrix(pMat_Y);
  release_float_Matrix(pMat_F);
  release_float_Matrix(pMat_P);
  release_float_Matrix(pMat_H);
  release_float_Matrix(pMat_Q);
  release_float_Matrix(pMat_R);

 return 0;
//--------------   KALMAN FILTER TEST END   ---------------


}
