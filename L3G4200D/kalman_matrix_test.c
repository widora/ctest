/*-------------------------------------------------------------------------
KALMAN filter test program
  Example derived from:	blog.csdn.net/zhangwenyu111/article/details/17034813


---------------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/time.h>
#include "mathwork.h"


int main(void)
{

while(1)
{
  //----- test init and release ---
  struct float_Matrix *pmtest=init_float_Matrix(5,5);
  Matrix_Print(*pmtest);

  float fmat[5*5]=
 {
   1,2,3,4,5,
   6,7,8,9,0,
   1,2,3,4,5,
   6,7,8,9,0,
   1,2,3,4,5
 };
  Matrix_FillArray(pmtest,fmat); 

  Matrix_Print(*pmtest);
  release_float_Matrix(pmtest);
 
}
 return 0;


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

/*------------------    Measured Values    ---------------------
#--- you may adjust follow noise altitude ---
mat_s[]: s with noise 0.5*wt;
mat_v[]: v with noise 100*wt;
mat_a[]: a with noise 1.0*wt;
---------------------------------------------------------------*/
float MatS[1*100]=   //measured value, object distance
{
1059.5019,1138.0822,1240.1443,1360.3312,1498.6799,1661.3712,1841.3692,2039.9567,
2260.3768,2500.2011,2759.785,3040.8357,3339.3226,3662.5137,3999.843,4360.1312,4741.2283,
5140.0683,5559.8899,5999.0416,6460.339,6938.4615,7440.8225,7961.8694,8499.2035,9060.9879,
9641.4439,10238.165,10858.3409,11500.6576,12159.5396,12840.7945,13540.9391,14260.8197,
15001.4856,15760.7698,16541.3711,17338.6155,18159.9772,18999.8196,19858.153,20740.2963,
21638.7836,22561.6294,23499.073,24460.6088,25440.2525,26438.9385,27457.5007,28499.9319,
29558.8363,30640.7075,31740.5846,32861.9487,34000.6808,35159.259,36340.4379,37538.8381,
38759.9775,39999.9445,41260,42539.634,43841.2608,45157.8423,46500.493,47861.0312,49240.8416,
50640.6653,52060.0464,53500.7796,54960.655,56439.7056,57939.5654,59459.6593,60998.3015,
62559.7306,64140.1364,65740.3625,67361.6621,68999.5959,70660.7176,72340.92,74041.0833,
75758.8577,77500.2441,79260.2739,81038.8397,82839.1456,84661.2462,86499.8486,88360.4489,
90240.1013,92139.2683,94059.3557,96000.5108,97958.9063,99940.8995,101940.6551,103959.0539,
105999.6942
};
struct float_Matrix Mat_S;
Mat_S.nr=1; Mat_S.nc=100; Mat_S.pmat=MatS;


float matV[100]= //measured value, object speed
{
-29.6116,-293.5529,138.8617,196.2465,-114.0109,444.2456,463.8423,201.3338,305.3693,
290.2161,227.0045,457.136,174.5217,832.7469,318.5906,396.2363,635.6568,423.6514,
407.974,258.3255,537.7973,182.3023,674.4956,903.876,390.697,767.5806,878.7731,
242.9939,298.1728,781.5246,577.9138,848.8935,897.8226,893.9392,1047.1204,923.9662,
1064.2277,533.0966,825.4428,813.9109,500.6091,949.2524,666.714,1255.8806,764.6027,
1091.7596,1040.5055,797.7033,530.1342,1036.3702,837.27,1231.4994,1226.9232,1519.735,
1286.1613,1021.7921,1277.5846,977.6196,1225.5071,1238.8957,1270.0099,1216.8029,1562.1589,
898.4551,1448.6026,1576.2488,1558.3258,1543.0697,1439.2836,1605.921,1601.0071,1431.1296,
1423.076,1461.8627,1210.3041,1516.1132,1617.2756,1682.4946,1962.413,1569.1771,1813.5191,
1874.006,1926.6694,1501.5398,1798.8277,1824.7798,1557.9309,1639.121,2079.2323,1819.7181,
1959.7822,1910.2618,1763.6642,1801.1407,2052.1651,1751.2549,2169.8915,2141.021,1840.7746,
1988.8357
};

float matA[100]= //measured value, object acceleration.
{
19.0039,16.1645,20.2886,20.6625,17.3599,22.7425,22.7384,19.9133,20.7537,
20.4022,19.57,21.6714,18.6452,25.0275,19.6859,20.2624,22.4566,20.1365,
19.7797,18.0833,20.678,16.923,21.645,23.7388,18.407,21.9758,22.8877,16.3299,
16.6817,21.3152,19.0791,21.5889,21.8782,21.6394,22.9712,21.5397,22.7423,
17.231,19.9544,19.6391,16.3061,20.5925,17.5671,23.2588,18.146,21.2176,20.5051,
17.877,15.0013,19.8637,17.6727,21.415,21.1692,23.8973,21.3616,18.5179,20.8758,
17.6762,19.9551,19.889,20.0001,19.268,22.5216,15.6846,20.986,22.0625,21.6833,
21.3307,20.0928,21.5592,21.3101,19.4113,19.1308,19.3186,16.603,19.4611,20.2728,
20.7249,23.3241,19.1918,21.4352,21.8401,22.1667,17.7154,20.4883,20.5478,17.6793,
18.2912,22.4923,19.6972,20.8978,20.2026,18.5366,18.7114,21.0217,17.8125,21.7989,
21.3102,18.1077,19.3884
};




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
float MatY[3*1]=  //----- state of s,v,a
{
900,
0,
0
};
struct float_Matrix Mat_Y;
Mat_Y.nr=3; Mat_Y.nc=1; Mat_Y.pmat=MatY;

float MatYp[3*1]={0};  //----- predicted state of s,v,a
struct float_Matrix Mat_Yp;
Mat_Yp.nr=3; Mat_Yp.nc=1; Mat_Yp.pmat=MatYp;


float MatF[3*3]= //----- state transition matrix
{
  1, 1, 0,
  0, 1, 1,
  0, 0, 1
};
struct float_Matrix Mat_F;
Mat_F.nr=3; Mat_F.nc=3; Mat_F.pmat=MatF;

float MatF_trans[3*3]={0}; //----- state transition matrix transpose
struct float_Matrix Mat_F_trans; // for transpose operation buff
Mat_F_trans.nr=3; Mat_F_trans.nc=3; Mat_F_trans.pmat=MatF_trans;


float MatH[1*3]= //----- observation matrix
{ 1, 0, 0};
struct float_Matrix Mat_H;
Mat_H.nr=1; Mat_H.nc=3; Mat_H.pmat=MatH;

float MatH_trans[3*1]={0}; //----- observation matrix transpose
struct float_Matrix Mat_H_trans;
Mat_H_trans.nr=3; Mat_H_trans.nc=1; Mat_H_trans.pmat=MatH_trans;

float MatQ[3*3]= //----- process noise covariance
{
 0,0,0,
 0,0,0,
 0,0,20
};
struct float_Matrix Mat_Q;
Mat_Q.nr=3; Mat_Q.nc=3; Mat_Q.pmat=MatQ;

float MatR[1*1]={16.0}; //----- observatin nose covaraince
struct float_Matrix Mat_R;
Mat_R.nr=1; Mat_R.nc=1; Mat_R.pmat=MatR;

float MatP[3*3]= //----- state convaraince matrix 
{
 30, 0, 0,
 0, 10, 0,
 0, 0, 10
};
struct float_Matrix Mat_P;
Mat_P.nr=3; Mat_P.nc=3; Mat_P.pmat=MatP;

float MatPp[3*3]={0}; //----- predicted state convariance matrix
memcpy(MatPp,MatP,3*3*sizeof(float)); //init value
struct float_Matrix Mat_Pp;
Mat_Pp.nr=3; Mat_Pp.nc=3; Mat_Pp.pmat=MatPp;
printf("Mat_Pp=\n");
Matrix_Print(Mat_Pp);

float MatK[3*1]={0}; //----- Kalman Gain Matrix
struct float_Matrix Mat_K;
Mat_K.nr=3; Mat_K.nc=1; Mat_K.pmat=MatK;


//-------temp. buff matrxi ----------
//Mat [mxm]
float Mat1X1A[1*1]={0}; //for temp. buff
struct float_Matrix Mat_1X1A;
Mat_1X1A.nr=1; Mat_1X1A.nc=1; Mat_1X1A.pmat=Mat1X1A;

//Mat [mxm]
float Mat1X1B[1*1]={0}; //for temp. buff
struct float_Matrix Mat_1X1B;
Mat_1X1B.nr=1; Mat_1X1B.nc=1; Mat_1X1B.pmat=Mat1X1B;

//Mat [mxm]
float Mat1X1C[1*1]={0}; //for temp. buff
struct float_Matrix Mat_1X1C;
Mat_1X1C.nr=1; Mat_1X1C.nc=1; Mat_1X1C.pmat=Mat1X1C;

//----Mat [nxm]
float Mat3X1A[3*1]={0}; //for temp. buff
struct float_Matrix Mat_3X1A;
Mat_3X1A.nr=3; Mat_3X1A.nc=1; Mat_3X1A.pmat=Mat3X1A;

//----Mat [nxm]
float Mat3X1B[3*1]={0}; //for temp. buff
struct float_Matrix Mat_3X1B;
Mat_3X1B.nr=3; Mat_3X1B.nc=1; Mat_3X1B.pmat=Mat3X1B;

//----Mat [nxn]
float Mat3X3A[3*3]={0}; //for temp. buff
struct float_Matrix Mat_3X3A;
Mat_3X3A.nr=3; Mat_3X3A.nc=3; Mat_3X3A.pmat=Mat3X3A;

//----Mat [nxn]
float Mat3X3B[3*3]={0}; //for temp. buff
struct float_Matrix Mat_3X3B;
Mat_3X3B.nr=3; Mat_3X3B.nc=3; Mat_3X3B.pmat=Mat3X3B;

//----Mat [nxn]
float MatI[3*3]= //eye matrix with 1 on diagonal and zeros elesewhere
{
  1,0,0,
  0,1,0,
  0,0,1
 }; 
struct float_Matrix Mat_I;
Mat_I.nr=3; Mat_I.nc=3; Mat_I.pmat=MatI;


//-------------- test -------------------------

	int n=100; //samples, number of points to be filtered
	int i,j,k;

	gettimeofday(&tm_start,NULL);
        //------- Kalman Filter Processing --------
        for(k=0; k<n; k++)
	{
		printf("--------- k=%d ---------\n",k);

		//----- 1.Predict(priori) state:  Yp = F*Y -----(3,1) = (3,3)*(3,1)
		//----- 1.Predict(priori) state:  Yp = F*Y -----(n,1) = (n,n)*(n,1)
		Matrix_Multiply(&Mat_F,&Mat_Y,&Mat_Yp);
		printf("Mat_Yp=\n");
		Matrix_Print(Mat_Yp);

		//----- 2. Predict(priori) state covariance:  Pp = F*P*F'+Q ----- (3,3)=(3,3)*(3,3)*(3,3)'+(3,3)
		//----- 2. Predict(priori) state covariance:  Pp = F*P*F'+Q ----- (n,n)=(n,n)*(n,n)*(n,n)'+(n,n)
		Matrix_Add (
			Matrix_Multiply( &Mat_F,
					 Matrix_Multiply( &Mat_P,
							  Matrix_Transpose(&Mat_F,&Mat_F_trans),
							  &Mat_3X3A
							),
					 &Mat_3X3B
					),
			&Mat_Q,
			&Mat_Pp  //the result matrix
		);
		printf("Mat_Pp=\n");
		Matrix_Print(Mat_Pp);

		//----- 3. Update Kalman Gain:  K = Pp*H'*inv(H*Pp*H'+R)  -----
		//			  (3,1) =(3,3)*(1,3)'*inv((1,3)*(3,3)*(1,3)'+(1,1))
		//			  (n,m) =(n,n)*(m,n)'*inv((m,n)*(n,n)*(m,n)'+(m,m))
		Matrix_Transpose( &Mat_H, &Mat_H_trans);
//		printf("Mat_H_trans=\n");
//		Matrix_Print(Mat_H_trans);
		Matrix_Multiply( &Mat_Pp,
				 Matrix_Multiply( &Mat_H_trans,
						  Matrix_Inverse( Matrix_Add(  Matrix_Multiply( &Mat_H,
										                Matrix_Multiply( &Mat_Pp, &Mat_H_trans,&Mat_3X1A),
											        &Mat_1X1A
									        ),
										&Mat_R,
										&Mat_1X1B
								   ),
								   &Mat_1X1C
						  ),
						  &Mat_3X1B
				),
				&Mat_K
		);
//		printf("Inv(H*P2*H'+Rk)=\n");
//		Matrix_Print(Mat_1X1C);
//		printf("H'*Inv(H*P2*H'+Rk)=\n");
//		Matrix_Print(Mat_3X1B);
		printf("K=\n");
		Matrix_Print(Mat_K);

		//----- 4. Update(posteriori) state:  Y = Yp + K*(S-H*Yp)   ---- (3,1) = (3,1) + (3,1)*( (1,1)-(1,3)*(3,1) )
		//----- 4. Update(posteriori) state:  Y = Yp + K*(S-H*Yp)   ---- (n,1) = (n,1) + (n,m)*( (m,1)-(m,n)*(n,1) )
		Matrix_CopyColumn(&Mat_S,k,&Mat_1X1A,0); //extract one column from Mat_S, to Mat_1X1A,
//		printf("Mat_1X1A=Mat_S[%d]=\n",k);
//		Matrix_Print(Mat_1X1A);
		Matrix_Add( &Mat_Yp,
			    Matrix_Multiply( &Mat_K,
					     Matrix_Sub( &Mat_1X1A,
							 Matrix_Multiply( &Mat_H, &Mat_Yp, &Mat_1X1B),
					     		 &Mat_1X1C
					     ),
					     &Mat_3X1A
			    ),
			    &Mat_Y
		);
//		printf("Mat_Yp=\n");
//		Matrix_Print(Mat_Yp);
//		printf("Mat_H=\n");
//		Matrix_Print(Mat_H);
//		printf("H*Yp=\n");
//		Matrix_Print(Mat_1X1B);
//		printf("S-H*Yp=\n");
//		Matrix_Print(Mat_1X1C);
		printf("Mat_Y=\n");
		Matrix_Print(Mat_Y);

		//----- 5. Update(posteriori) state covariance:  P = (I-K*H)*Pp   ---- (3,1) = ( (3,3)-(3,1)*(1,3) )*(3,1)
		//----- 5. Update(posteriori) state covariance:  P = (I-K*H)*Pp   ---- (n,1) = ( (n,n)-(n,m)*(m,n) )*(n,1)
		Matrix_Multiply( Matrix_Sub( &Mat_I,
					     Matrix_Multiply( &Mat_K, &Mat_H, &Mat_3X3A),
					     &Mat_3X3B
				 ),
				 &Mat_Pp,
				 &Mat_P
		);
//		printf("K*H=\n");
//		Matrix_Print(Mat_3X3A);
//		printf("I-K*H=\n");
//		Matrix_Print(Mat_3X3B);
		printf("Mat_P=\n");
		Matrix_Print(Mat_P);

	} //end of for()

	gettimeofday(&tm_end,NULL);
	printf("----- recursion count: %d, time elapsed: %d(us) \n",k,get_costtimeus(tm_start, tm_end));
	printf("----- final Kalman Gain Matrix Mat_K= \n");
	Matrix_Print(Mat_K);
	printf("----- final State Covaraince Matrix  Mat_P= \n");
	Matrix_Print(Mat_P);


	return 0;
}
