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
void IIR_Lowpass_int16Filter(int16_t *p_in_data, int16_t *p_out_data, int nmov)
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
void IIR_Lowpass_dblFilter(double *p_in_data, double *p_out_data, int nmov)
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
( State Dimension: n, Observation Dimension m.   m<=n??? )

(input/output)  Mat_Y[nx1]:  the state (optimized mean value)  matrix of concerning variables (updated)
                             with init. value.
(-)             Mat_Yp[nx1]:   predicted of Mat_Yp[]
(input)         Mat_F[nxn]:  the state-transition matrix
(input)         Mat_H[mxn]:  the observation transformation matrix
(input)         Mat_S[mx1]:  the observation matrix (observed var.)
(input/output)  Mat_Q[nxn]:  the covariance matrix of the process noise
                             with some estimated init value.
(input/output)  Mat_R[mxm]:  the covarinace matrix of the observation noise
                             with some estimated init value.
(input/output)  Mat_P[nxn]:  the state_estimation error covaraince matrix (updated)
                             init. with some estimated value.
(-)             Mat_Pp[nxn]: predicted of Mat_P[]
(-)             Mat_K[mxn]:  Kalman Gain matrix
(-)             Mat_I[nxn]:  Eye matrix with 1 on diagonal and zeros elesewhere
---------------------------------------------------------------------------------------------------*/
void  float_KalmanFilter( struct floatKalmanDB *fdb,    //filter data base
			  struct float_Matrix *pMat_S)  //input observation matrix
{




}

/*

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

*/


/*----------------------------------------------------------------
Initiliaze kalman filter data base
Return:
	0    OK
	<0   fails
----------------------------------------------------------------*/
int Init_floatKalman_FilterDB( struct floatKalmanDB *fdb,
				     int n, int m,  //n---state var. dimension,  m---observation dimension
				     struct float_Matrix *pMat_Y,  //[nx1] state var.
				     struct float_Matrix *pMat_F,  //[nxn] transition
				     struct float_Matrix *pMat_P,  //[nxn] state covariance
				     struct float_Matrix *pMat_H,  //[mxn] observation transformation
				     struct float_Matrix *pMat_Q,  //[nxn] system noise covariance
				     struct float_Matrix *pMat_R )  //[mxm] observation noise covariance
{
	int i,j;

	//----- check n and m -----
	if( n<1 || m < 1)
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): n or m illegal!\n");
		return -1;
	}

    //----- dedicate input matrxi to filter database matrix ------
	if(pMat_Y->nr != n || pMat_Y->nc != 1 )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_Y[nx1] dimension incorrect!\n");
		return -1;
	}
	fdb->pMY=pMat_Y;
	//-------------------------------------
	if(pMat_F->nr != n || pMat_F->nc != n )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_F[nxn] dimension incorrect!\n");
		return -1;
	}
	fdb->pMF=pMat_F;
	//-------------------------------------
	if(pMat_P->nr != n || pMat_P->nc != n )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_P[nxn] dimension incorrect!\n");
		return -1;
	}
	fdb->pMP=pMat_P;
	//-------------------------------------
	if(pMat_H->nr != m || pMat_H->nc != n )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_H[mxn] dimension incorrect!\n");
		return -1;
	}
	fdb->pMH=pMat_H;
	//-------------------------------------
	if(pMat_Q->nr != n || pMat_Q->nc != n )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_Q[nxn] dimension incorrect!\n");
		return -1;
	}
	fdb->pMQ=pMat_Q;
	//-------------------------------------
	if(pMat_R->nr != m || pMat_R->nc != m )
	{
		fprintf(stderr,"Init_floatKalman_FilterDB(): pMat_R[mxm] dimension incorrect!\n");
		return -1;
	}
	fdb->pMR=pMat_R;

   //--------------  init other kalman filter matrix  ----------------
   /* following pointers MUST be freed in filterDB release function */
	//----- pMYp: [nx1] predicted state var.
	fdb->pMYp = init_float_Matrix(n,1);
        //----- pMPp: [nxn] predicted state conv.
	fdb->pMPp = init_float_Matrix(n,n);
        //----- pMI: [nxn] eye matrix with 1 on diagonal and zeros elesewhere
	fdb->pMI = init_float_Matrix(n,n);
	for(i=0;i<n;i++) {  //---- put 1.o to diagonal element 
		for(j=0;j<n;j++) {
			if(i==j)
				(fdb->pMI->pmat)[i*i]=1.0;
		}
	}
        //----- pMK: [mxn] Kalman Gain Matrix
	fdb->pMK = init_float_Matrix(m,n);

   //--------------  init temp. buffer matrix  ----------------
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
 



	return 0;
}

/*----------------------------------------------------------------
 Release float type Kalman Filter Data Base
----------------------------------------------------------------*/
void Release_floatKalman_FilterDB( struct floatKalmanDB *fdb )
{


}
