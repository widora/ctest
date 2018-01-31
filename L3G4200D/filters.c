#include "filters.h"


/*----------------------------------------------------------------------
            ***  Moving Average Filter   ***
1. !!!Reset the filter data base struct every time before call the function, to clear old data in f_buff and f_sum.
Filter a data stream until it ends, then reset the filter context struct before filter another.
2.To filter data you must iterate nmov one by one:
         source[nmov] ----> filter ----> dest[nmov]
3. dest[] is 2^ng+2 points later thean source[], so all consider reactive time when you choose ng.

*fdb:     int16 MAFilter data base
*source:  raw data input
*dest:    where you put your filtered data
	  (source and dest may be the same)
nmov:     nmov_th step of moving average calculation

Return:
	The last 2^f_ng pionts average value.
	0  if fails
--------------------------------------------------------------------------*/
inline int16_t int16_MAfilter(struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov)
{
	int i;
	int np=1<<(fdb->f_ng); //2^np points average !!!!

	//----- verify filter data base
	if(fdb->f_buff == NULL)
	{
		fprintf(stderr,"int16_MA16P_filter(): int16MAFilter data base has not been initilized yet!\n");
		*dest=0;
		return 0;
	}

	//----- deduce old data from f_sum
	fdb->f_sum -= fdb->f_buff[0];

	//----- shift filter buff data, get rid of the oldest data
	for(i=0; i<np+1; i++) //totally np+2 elements !!!
	{
		fdb->f_buff[i]=fdb->f_buff[i+1];
	}
	//----- put new data to the appended slot
	fdb->f_buff[np+1]=source[nmov];

	//----- get new data to fdb->f_buff[np-1] --------
	//----- reassure limit first
	//fdb->f_limit = abs(fdb->f_limit);

#if 1	// Limit Pick Method : ---  Relative Limit --- , comparing with next fdb->f_buff[] 
	if( fdb->f_buff[np-1] > (fdb->f_buff[np-2]+fdb->f_limit) ) //check up_limit
	{
		//------- if 1+2 CONTINOUS up_limit detected, then do NOT trim ----
		if( fdb->f_buff[np] > (fdb->f_buff[np-2]+fdb->f_limit) 
		   && fdb->f_buff[np+1] > (fdb->f_buff[np-2]+fdb->f_limit) )
		{
			// keep fdb->f_buff[np-1] then;
		}
		else
			fdb->f_buff[np-1]=fdb->f_buff[np-2]; //use previous value then;
	}
	else if( fdb->f_buff[np-1] < (fdb->f_buff[np-2]-fdb->f_limit))  //check low_limit
	{
		//------- if 1+2 CONTINOUS low_limit detected, then do NOT trim ----
		if( fdb->f_buff[np] < (fdb->f_buff[np-2]-fdb->f_limit) 
		  && fdb->f_buff[np+1] < (fdb->f_buff[np-2]-fdb->f_limit) )
		{
			// keep fdb->f_buff[np-1] then;
		}
		else
			fdb->f_buff[np-1]=fdb->f_buff[np-2]; //use previous value then;
	}

        // ------ else if the value is within normal scope
	//else do nothing !
#endif

	fdb->f_sum += fdb->f_buff[np-1]; //add new data to f_sum

	//----- update dest with average value
	//----- dest[] is np+2 points later then source[]
	dest[nmov]=(fdb->f_sum)>>(fdb->f_ng);

	return dest[nmov];
}

/*----------------------------------------------------------------------------
Init. int16 MA filter strut with given value

*fdb --- filter data buffer base
ng  --- filter grade number,  2^ng points average.
limit  --- abs(limit) value for input data, above which the value will be trimmed .
Return:
	0    OK
	<0   fails
--------------------------------------------------------------------------*/
inline int Init_int16MAFilterDB(struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit)
{
//	int i;
	int np;

	//---- clear struct
	memset(fdb,0,sizeof(struct int16MAFilterDB)); 

	//---- set grade
	if(ng>INT16MAF_MAX_GRADE)
	{
		printf("Max. Filter grade is 10!\n");
		fdb->f_ng=INT16MAF_MAX_GRADE;
	}
	else
		fdb->f_ng=ng;

	//---- set np
	np=1<<(fdb->f_ng); //2^f_ng
	fprintf(stdout,"	%d points moving average filter initilizing...\n",np);

	//---- set limit
	fdb->f_limit=abs(limit);
	//---- clear sum
	fdb->f_sum=0;
	//---- allocate mem for buff data
	fdb->f_buff = (uint16_t *)malloc( (np+2)*sizeof(int16_t) ); //!!!! one more int16_t for temp. store, not for AMF buff !!!
	if(fdb->f_buff == NULL)
	{
		fprintf(stderr,"Init_int16MAFilterDB(): malloc f_buff failed!\n");
		return -1;
	}

	//---- clear buff data if f_buff 
	memset(fdb->f_buff,0,(np+2)*sizeof(int16_t));

	return 0;
}


/*----------------------------------------------------------------------
Release int16 MA filter struct
----------------------------------------------------------------------*/
inline void Release_int16MAFilterDB(struct int16MAFilterDB *fdb)
{
	if(fdb->f_buff != NULL)
		free(fdb->f_buff);

}


/*--------------------------------------------------------
<<<<<<<<    multi group moving average filter   >>>>

m:        total group number of filtering data
**fdb:     int16 MAFilter data base in [m] groups
**source:  raw data input in [m] groups
**dest:    where you put your filtered data in [m] groups
	  (source and dest may be the same)
nmov:     nmov_th step of moving average calculation

!!!!! To filter data you must iterate nmov one by one  !!!!!

---------------------------------------------------------*/
inline  void int16_MAfilter_NG(uint8_t m, struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov)
{
	int i;

	for(i=0; i<m; i++)
	{
		int16_MAfilter(fdb+i,source+i,dest+i,nmov);
	}

}

/*----------------------------------------------------------------------------
       <<<<<<<<    multi group moving average filter   >>>>

Init. int16 MA filter strut with given value
**fdb --- filter data buffer base in [m] groups
ng  --- filter grade number,  2^ng points average.
limit  --- limit value for input data, above which the value will be trimmed .
Return:
	0    OK
	<0   fails
--------------------------------------------------------------------------*/
inline int Init_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit)
{
	int i;
	for(i=0; i<m; i++)
	{
              if( Init_int16MAFilterDB(fdb+i, ng, limit)<0 )
			return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------
       <<<<<<<<    multi group moving average filter   >>>>

Release [m] groups of int16 MA filter struct
----------------------------------------------------------------------*/
inline void Release_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb)
{
	int i;
	for(i=0; i<m; i++)
	{
	   Release_int16MAFilterDB(fdb+i);
	}
}




/*---------------------- IIR_LOWPASS_FILTER -----------------------------------

                          4 order IIR filter

p_in_data:   pointer to start of input data
p_out_data:  pointer to start of output data
nmov:        processed slot number of the data, p_in_data[nmov] --> p_out_data[nmov]
!!!! p_ind_data and p_out_data maybe the same address !!!!!
----------------------------------------------------------------------*/
void IIR_Lowpass_int16Filter(const int16_t *p_in_data, int16_t *p_out_data, int nmov)
{
	//----factors for 4 order IIR filter ----
	//----Fs=400Hz Fc=199Hz--
//	static  double IIR_B[5]={ 0.9796854871904, 3.918741948762, 5.878112923142, 3.918741948762, 0.9796854871904};
//	static  double IIR_A[5]={ 1, 3.958953318647, 5.877700273536, 3.878530549052, 0.9597836538115};
	//----Fs=300Hz Fc=50Hz--
	static const double IIR_B[5]={ 0.02607772170109, 0.1043108868044, 0.1564663302066, 0.1043108868044, 0.02607772170109};
	static const double IIR_A[5]={ 1, -1.306605144101, 1.03045383542, -0.3623690447689, 0.0557639006678};
	static  double w[5]={0.0, 0.0, 0.0, 0.0, 0.0};
	//---- filter data ----
        w[0]=p_in_data[nmov]-IIR_A[1]*w[1]-IIR_A[2]*w[2]-IIR_A[3]*w[3]-IIR_A[4]*w[4];
        p_out_data[nmov]=IIR_B[0]*w[0]+IIR_B[1]*w[1]+IIR_B[2]*w[2]+IIR_B[3]*w[3]+IIR_B[4]*w[4];
	//---- renew w[] for next operation ----
        w[4]=w[3];
        w[3]=w[2];
        w[2]=w[1];
        w[1]=w[0];

}
/*------------------ double precision ------------------*/
void IIR_Lowpass_dblFilter(const double *p_in_data, double *p_out_data, int nmov)
{
	//----factors for 4 order IIR filter ----
	//----Fs=300Hz Fc=50Hz--
	static const double IIR_B[5]={ 0.02607772170109, 0.1043108868044, 0.1564663302066, 0.1043108868044, 0.02607772170109};
	static const double IIR_A[5]={ 1, -1.306605144101, 1.03045383542, -0.3623690447689, 0.0557639006678};
	static  double w[5]={0.0, 0.0, 0.0, 0.0, 0.0};
	//---- filter data ----
        w[0]=p_in_data[nmov]-IIR_A[1]*w[1]-IIR_A[2]*w[2]-IIR_A[3]*w[3]-IIR_A[4]*w[4];
        p_out_data[nmov]=IIR_B[0]*w[0]+IIR_B[1]*w[1]+IIR_B[2]*w[2]+IIR_B[3]*w[3]+IIR_B[4]*w[4];
	//---- renew w[] for next operation ----
        w[4]=w[3];
        w[3]=w[2];
        w[2]=w[1];
        w[1]=w[0];

}





/*-------------------------------------   System Matrix    ------------------------------------------
<<< 1.  State Dimension: n, Observation Dimension m.   m<=n???   >>>>
<<< 2.  Control matrix is omitted, So let Mat_Q=[0]???


(input/output)  Mat_Y[nx1]:  the state (optimized mean value)  matrix of concerning variables (updated)
                             with init. value.
(-)             Mat_Yp[nx1]:   predicted of Mat_Yp[]
(input)         Mat_F[nxn]:  the state-transition matrix
(-)  	        Mat_Ftp[nxn]:  the transpose of state-transition matrix
(input)         Mat_H[mxn]:  the observation transformation matrix
(-)	        Mat_Htp[nxm]:  the transpose of observation transformation matrix
(input)         Mat_S[mx1]:  the observation matrix (observed,reading var.)
(input/output)  Mat_Q[nxn]:  the covariance matrix of the untracked noise for outside
                             with some estimated init value.
(input/output)  Mat_R[mxm]:  the covarinace matrix of the observation(reading,sensor) noise
                             with some estimated init value.
(input/output)  Mat_P[nxn]:  the state_estimation error covaraince matrix (updated)
                             init. with some estimated value.
(-)             Mat_Pp[nxn]: predicted of Mat_P[]
(-)             Mat_K[nxm]:  Kalman Gain matrix
(-)             Mat_I[nxn]:  Eye matrix with 1 on diagonal and zeros elesewhere

struct floatKalmanDB {
        int n; // state var. dimension
        int m; // observation dimension
        struct float_Matrix *pMY;  //[nx1] state var. matrix
        struct float_Matrix *pMYp; //[nx1] predicted state var.
        struct float_Matrix *pMF;  //[nxn] transition matrix
        struct float_Matrix *pMFtp;  //[nxn] transpose of transition matrix
        struct float_Matrix *pMP;  //[nxn] state covariance
        struct float_Matrix *pMPp; //[nxn] predicted state conv. 
        struct float_Matrix *pMH;  //[mxn] observation transformation
        struct float_Matrix *pMHtp;  //[nxm] the transpose of observation transformation
        struct float_Matrix *pMQ;  //[nxn] system noise covariance
        struct float_Matrix *pMR;  //[mxm] observation noise covariance
        struct float_Matrix *pMI; //[nxn] eye matrix with 1 on diagonal and zeros elesewhere
        struct float_Matrix *pMK; //[nxm] Kalman Gain Matrix
---------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------
		KALMAN FILTER REALTIME CALCUALATOR

1. Initiate fdb before calling the function.
2. Realtime calculation, update pMS  and call float_KalmanFilter() constantly.
3. Release fdb after Kalman computation.

fdb: pointer of struct floatKalmanDB
pMS: [mx1] input observation matrix

---------------------------------------------------------------------------------*/
void  float_KalmanFilter( struct floatKalmanDB *fdb,    //filter data base
			 const struct float_Matrix *pMS )  //[mx1] input observation matrix
{

	//----- check observation matrix dimension -----
	if( fdb->m != pMS->nr || pMS->nc != 1 )
	{
		fprintf(stderr,"float_KalmanFilter(): fdb->m=%d,  pMS->nr=%d, pMS->nc=%d, \
				matrix dimension incorrect!\n", fdb->m, pMS->nr, pMS->nc);
		return;
	}

	//----- 1.Predict(priori) state:  Yp = F*Y -----(n,1) = (n,n)*(n,1)
//	printf("Yp = F*Y\n");
        Matrix_Multiply(fdb->pMF,fdb->pMY,fdb->pMYp);
//        printf("pMYp=\n");
//        Matrix_Print(*fdb->pMYp);

        //----- 2. Predict(priori) state covariance:  Pp = F*P*F'+Q ----- (n,n)=(n,n)*(n,n)*(n,n)'+(n,n)
//	printf("Pp = F*P*F'+Q \n");
        Matrix_Add (
                Matrix_Multiply( fdb->pMF,
                                 Matrix_Multiply( fdb->pMP,
                                                  Matrix_Transpose(fdb->pMF, fdb->pMFtp),
                                                  fdb->pMat_nXnA
                                                ),
                                 fdb->pMat_nXnB
                                ),
                fdb->pMQ,
                fdb->pMPp  //the result matrix
        );
//        printf("pMPp=\n");
//        Matrix_Print(*fdb->pMPp);

        //----- 3. Update Kalman Gain:  K = Pp*H'*inv(H*Pp*H'+R)  -----
        //                        (n,m) =(n,n)*(m,n)'*inv((m,n)*(n,n)*(m,n)'+(m,m))
//	printf("K = Pp*H'*inv(H*Pp*H'+R) \n");
        Matrix_Transpose( fdb->pMH, fdb->pMHtp);
        Matrix_Multiply( fdb->pMPp,
                         Matrix_Multiply( fdb->pMHtp,
                                          Matrix_Inverse( Matrix_Add(  Matrix_Multiply( fdb->pMH,
                                                                                        Matrix_Multiply( fdb->pMPp, fdb->pMHtp, fdb->pMat_nXmA),
                                                                                        fdb->pMat_mXmA
                                                                        ),
                                                                        fdb->pMR,
                                                                        fdb->pMat_mXmB
                                                           ),
                                                           fdb->pMat_mXmC
                                          ),
                                          fdb->pMat_nXmB
                        ),
                        fdb->pMK
        );
//        printf("Kalman Gain K=\n");
//        Matrix_Print(*fdb->pMK);

        //----- 4. Update(posteriori) state:  Y = Yp + K*(S-H*Yp)  ----- (n,1) = (n,1) + (n,m)*( (m,1)-(m,n)*(n,1) )
        // Matrix_CopyColumn(&Mat_S,k,&Mat_1X1A,0); //extract one column from Mat_S, to Mat_1X1A,
//	printf(" Y = Yp + K*(S-H*Yp) \n");
        Matrix_Add( fdb->pMYp,
                    Matrix_Multiply( fdb->pMK,
                                     Matrix_Sub( pMS, // ------- input obversation matrix here -------
                                                 Matrix_Multiply( fdb->pMH, fdb->pMYp, fdb->pMat_mX1A),
                                                 fdb->pMat_mX1B
                                     ),
                                     fdb->pMat_nX1A
                    ),
                    fdb->pMY
        );
//        printf("pMY=\n");
//        Matrix_Print(*fdb->pMY);

        //----- 5. Update(posteriori) state covariance:  P = (I-K*H)*Pp   ---- (n,n) = ( (n,n)-(n,m)*(m,n) )*(n,n)
//	printf("P = (I-K*H)*Pp \n");
        Matrix_Multiply( Matrix_Sub( fdb->pMI,
                                    Matrix_Multiply( fdb->pMK, fdb->pMH, fdb->pMat_nXnA),
                                    fdb->pMat_nXnB
                         ),
                         fdb->pMPp,
                         fdb->pMP
        );
//      printf("Mat_P=\n");
//      Matrix_Print(*fdb->pMP);

}





/*----------------------------------------------------------------
Initiliaze kalman filter data base
Return:
	fdb    OK
	NULL   fails

   !!!--- WARNING ---!!! pMat_Y and pMat_P will be changed
----------------------------------------------------------------*/
struct floatKalmanDB * Init_floatKalman_FilterDB(
				     int n, int m,  //n---state var. dimension,  m---observation dimension
				     struct float_Matrix *pMat_Y,  //[nx1] state var.
			     const   struct float_Matrix *pMat_F,  //[nxn] transition
				     struct float_Matrix *pMat_P,  //[nxn] state covariance
			     const   struct float_Matrix *pMat_H,  //[mxn] observation transformation
			     const   struct float_Matrix *pMat_Q,  //[nxn] system noise covariance
			     const   struct float_Matrix *pMat_R )  //[mxm] observation noise covariance
{
	struct floatKalmanDB *fdb;
	int i,j;

	//------ init struct floatKalmanDB ----
	fdb=malloc(sizeof(struct floatKalmanDB));
	if(fdb==NULL)
        {
		fprintf(stderr,"Init_floatKalman_FilterDB(): malloc fdb failed!\n");
		return NULL;
	}
	memset(fdb,0,sizeof(struct floatKalmanDB));

	//----- check n and m -----
	if( n<1 || m < 1)
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): n or m illegal!\n");
		goto FAIL_CALL;
	}
	//----- assign n,m ----
	fdb->n=n;
	fdb->m=m;


    //----- dedicate input matrxi to filter database matrix ------
	if(pMat_Y->nr != n || pMat_Y->nc != 1 )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_Y[nx1] dimension incorrect!\n");
		goto FAIL_CALL;
	}
	fdb->pMY=pMat_Y;
	//-------------------------------------
	if(pMat_F->nr != n || pMat_F->nc != n )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_F[nxn] dimension incorrect!\n");
		goto FAIL_CALL;
	}
	fdb->pMF=pMat_F;
	//-------------------------------------
	if(pMat_P->nr != n || pMat_P->nc != n )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_P[nxn] dimension incorrect!\n");
		goto FAIL_CALL;
	}
	fdb->pMP=pMat_P;
	//-------------------------------------
	if(pMat_H->nr != m || pMat_H->nc != n )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_H[mxn] dimension incorrect!\n");
		goto FAIL_CALL;
	}
	fdb->pMH=pMat_H;
	//-------------------------------------
	if(pMat_Q->nr != n || pMat_Q->nc != n )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_Q[nxn] dimension incorrect!\n");
		goto FAIL_CALL;
	}
	fdb->pMQ=pMat_Q;
	//-------------------------------------
	if(pMat_R->nr != m || pMat_R->nc != m )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_R[mxm] dimension incorrect!\n");
		goto FAIL_CALL;
	}
	fdb->pMR=pMat_R;

   //--------------  init other kalman filter matrix  ----------------
   /* following pointers MUST be freed in filterDB release function */
	//----- pMYp: [nx1] predicted state var.
	fdb->pMYp = init_float_Matrix(n,1);
        //----- pMFtp: [nxn] transpose of pMF(transition).
	fdb->pMFtp = init_float_Matrix(n,n);
        //----- pMPp: [nxn] predicted state conv.
	fdb->pMPp = init_float_Matrix(n,n);
        //----- pMHtp: [nxm] predicted state conv.
	fdb->pMHtp = init_float_Matrix(n,m);
        //----- pMI: [nxn] eye matrix with 1 on diagonal and zeros elesewhere
	fdb->pMI = init_float_Matrix(n,n);
	for(i=0;i<n;i++) ////---- put 1.o to diagonal eleme
		(fdb->pMI->pmat)[i*(n+1)]=1.0;

        //----- pMK: [mxn] Kalman Gain Matrix
	fdb->pMK = init_float_Matrix(n,m);

	if( fdb->pMYp==NULL || fdb->pMFtp==NULL || fdb->pMPp==NULL ||
	    fdb->pMHtp==NULL ||	fdb->pMI==NULL ||fdb->pMK==NULL )
	{
		goto FAIL_CALL;
	}
   //--------------  init temp. buffer matrix  ----------------
        //Mat [mx1]
        fdb->pMat_mX1A=init_float_Matrix(m,1);
        //Mat [mx1]
        fdb->pMat_mX1B=init_float_Matrix(m,1);
        //Mat [mxm]
        fdb->pMat_mXmA=init_float_Matrix(m,m);
        //Mat [mxm]
        fdb->pMat_mXmB=init_float_Matrix(m,m);
        //Mat [mxm]
        fdb->pMat_mXmC=init_float_Matrix(m,m);
        //----Mat [nx1]
        fdb->pMat_nX1A=init_float_Matrix(n,1);
        //----Mat [nxm]
        fdb->pMat_nXmA=init_float_Matrix(n,m);
        //----Mat [nxm]
        fdb->pMat_nXmB=init_float_Matrix(n,m);
        //----Mat [nxn]
        fdb->pMat_nXnA=init_float_Matrix(n,n);
        //----Mat [nxn]
        fdb->pMat_nXnB=init_float_Matrix(n,n);

	if( fdb->pMat_mXmA==NULL || fdb->pMat_mXmB==NULL || fdb->pMat_mXmC==NULL ||
	    fdb->pMat_nXmA==NULL || fdb->pMat_nXmB==NULL || fdb->pMat_nXnA==NULL ||
	    fdb->pMat_nXnB==NULL )
	{
		goto FAIL_CALL;
	}

	//----- if succeed -----
	return fdb;

FAIL_CALL:
	Release_floatKalman_FilterDB(fdb);
	return NULL;
}

/*----------------------------------------------------------------
 Release float type Kalman Filter Data Base
----------------------------------------------------------------*/
void Release_floatKalman_FilterDB( struct floatKalmanDB *fdb )
{
    //---- if not NULL ---
    if(fdb != NULL)
    {
	printf("realese fdb-> pMYp,pMPp,pMI,pMK...\n");
	release_float_Matrix(fdb->pMYp);
	release_float_Matrix(fdb->pMFtp);
	release_float_Matrix(fdb->pMPp);
	release_float_Matrix(fdb->pMHtp);
	release_float_Matrix(fdb->pMI);
	release_float_Matrix(fdb->pMK);

	printf("realese fdb-> tmp buff matrix...\n");
	release_float_Matrix(fdb->pMat_mX1A);
	release_float_Matrix(fdb->pMat_mX1B);
	release_float_Matrix(fdb->pMat_mXmA);
	release_float_Matrix(fdb->pMat_mXmB);
	release_float_Matrix(fdb->pMat_mXmC);
	release_float_Matrix(fdb->pMat_nX1A);
	release_float_Matrix(fdb->pMat_nXmA);
	release_float_Matrix(fdb->pMat_nXmB);
	release_float_Matrix(fdb->pMat_nXnA);
	release_float_Matrix(fdb->pMat_nXnB);

	printf("free fdb...\n");
    	free(fdb);
    }
}
