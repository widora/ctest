#ifndef   ___MATHWORK__H__
#define   ___MATHWORK__H__

#include <stdint.h>
#include <sys/time.h>

/*----------------------------------------------
 calculate and return time difference in us

Return
  if tm_start > tm_end then return 0 !!!!
  us
----------------------------------------------*/
inline uint32_t get_costtimeus(struct timeval tm_start, struct timeval tm_end) {
        uint32_t time_cost;
        if(tm_end.tv_sec>tm_start.tv_sec)
                time_cost=(tm_end.tv_sec-tm_start.tv_sec)*1000000+(tm_end.tv_usec-tm_start.tv_usec);
        //--- also consider tm_sart > tm_end !!!!!!!
        else if( tm_end.tv_sec==tm_start.tv_sec )
        {
                time_cost=tm_end.tv_usec-tm_start.tv_usec;
                time_cost=time_cost>0 ? time_cost:0;
        }
        else
                time_cost=0;

        return time_cost;
}


/*-------------------------------------------------------------------------------
               Integral of time difference and fx[num]

1. Only ONE instance is allowed, since timeval and sum_dt is static var.
2. In order to get a regular time difference,you shall call this function
in a loop.
dt_us will be 0 at first call
3. parameters:
	*num ---- number of integral groups.
	*fx ---- [num] functions of time,whose unit is us(10^-6 s) !!!
	*sum ----[num] results integration(summation) of fx * dt_us in sum[num]

return:
	summation of dt_us
-------------------------------------------------------------------------------*/
inline uint32_t math_tmIntegral_NG(uint8_t num, const double *fx, double *sum)
{
	   uint32_t dt_us;//time in us
	   static struct timeval tm_end,tm_start;
	   static struct timeval tm_start_1, tm_start_2;
	   static double sum_dt; //summation of dt_us 
	   int i;

	   gettimeofday(&tm_end,NULL);// !!!--end of read !!!
           //------  to minimize error between two timer functions
           gettimeofday(&tm_start,NULL);// !!! --start time !!!
	   tm_start_2 = tm_start_1;
	   tm_start_1 = tm_start;
           //------ get time difference for integration calculation
	   if(tm_start_2.tv_sec != 0) 
	           dt_us=get_costtimeus(tm_start_2,tm_end); //return  0 if tm_start > tm_end
	   else // discard fisrt value
		   dt_us=0;

  	   sum_dt +=dt_us;

	   //------ integration calculation -----
	   for(i=0;i<num;i++)
	   {
         	  sum[i] += fx[i]*(double)dt_us;
	   }

	   return sum_dt;
}


/*------------  single group time integral  -------------------*/
inline uint32_t math_tmIntegral(const double fx, double *sum)
{
	   uint32_t dt_us;//time in us
	   static struct timeval tm_end,tm_start;
	   static struct timeval tm_start_1, tm_start_2;
	   static double sum_dt; //summation of dt_us 

	   gettimeofday(&tm_end,NULL);// !!!--end of read !!!
           //------  to minimize error between two timer functions
           gettimeofday(&tm_start,NULL);// !!! --start time !!!
	   tm_start_2 = tm_start_1;
	   tm_start_1 = tm_start;
           //------ get time difference for integration calculation
	   if(tm_start_2.tv_sec != 0) 
	           dt_us=get_costtimeus(tm_start_2,tm_end); //return  0 if tm_start > tm_end
	   else // discard fisrt value
		   dt_us=0;

	   sum_dt +=dt_us;

           (*sum) += fx*dt_us;

	   return sum_dt;
}


//<<<<<<<<<<<<<<<      MATRIX ---  OPERATION     >>>>>>>>>>>>>>>>>>

/*-------------------------------------------
Print matrix
nr:  number of row of *matA
nc:  number of column of *matA
*matA: point to the matrix
-------------------------------------------*/
void Matrix_Print(int nr, int nc, float *matA)
{
   int i,j;

   for(i=0; i<nr; i++)
   {
        printf("Row(%d):  ",i);
        for(j=0; j<nc; j++)
           printf("%f    ", matA[i*nc+j] );
        printf("\n");
   }
}


/*----------------     MATRIX ADD/SUB    ---------------------
Addup/subtraction operation of two matrices with same row and column number
int nr:  number of row
int cn:  number of column
*matA:   pointer to matrix A
*matB:   pointer to matrix B
*matC:   pointer to mastrix A+B or A-B
Return:
	NULL  --- fails
	pointer to matC --- OK
----------------------------------------------------------*/
float* Matrix_Add(int nr, int nc, float *matA, float *matB, float *matC)
{
   int i,j;

   //---- check pointer -----
   if(matA==NULL || matB==NULL || matC==NULL)
   {
	   fprintf(stderr,"Matrix_Add(): matrix pointer is NULL!\n");
	   return NULL;
   }

   for(i=0; i<nr; i++) //row count
   {
	for(j=0; j<nc; j++) //column count
	{
		matC[i*nc+j] = matA[i*nc+j]+matB[i*nc+j];
	}
   }

   return matC;
}

float* Matrix_Sub(int nr, int nc, float *matA, float *matB, float *matC)
{
   int i,j;

   //---- check pointer -----
   if(matA==NULL || matB==NULL || matC==NULL)
   {
	   fprintf(stderr,"Matrix_Sub(): matrix pointer is NULL!\n");
	   return NULL;
   }

   for(i=0; i<nr; i++) //row count
   {
        for(j=0; j<nc; j++) //column count
        {
                matC[i*nc+j] = matA[i*nc+j]-matB[i*nc+j];
        }
   }
   return matC;
}



/*----------------     MATRIX MULTIPLY    ---------------------
Multiply two matrices
   !!! ncA == nrB !!!!
int nrA,nrB:  number of row
int ncA,ncB:  number of column
matA[nrA,ncA]:   matrix A
matB[nrB,ncB]:   matrix B
matC[nrA,ncB]:   pointer to mastrix A*B 

Return:
	NULL ---  fails
	point to matC  --- OK
----------------------------------------------------------*/
float* Matrix_Multiply(int nrA, int ncA, float *matA, int nrB, int ncB, float *matB, float *matC)
{
	float *fret;
	int i,j,k;

	//---- check pointer -----
	if(matA==NULL || matB==NULL || matC==NULL)
	{
	   fprintf(stderr,"Matrix_Multiply(): matrix pointer is NULL!\n");
	   return NULL;
	}
	//----- verify matrix dimension -----
	if(ncA != nrB)
	{
	   fprintf(stderr,"Matrix_Multiply(): dimension not correct!\n");
	   return NULL;
	}

	//---- result matrix with row: nrA  column: ncB, recuse nrA and ncB then.
	for(i=0; i<nrA; i++) //rows of matC (matA)
		for(j=0; j<ncB; j++) // columns of matC  (matB)
			for(k=0; k<ncA; k++)//
			{
				matC[i*ncB+j] += matA[i*ncA+k]*matB[ncB*k+j]; // (row i of matA) .* (column j of matB)
				//matC[0] += matA[k]*matB[ncB*k];//i=0,j=0
			}
       return matC;
}




#endif
