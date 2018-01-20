/*------------------------------------------------
For Widora WiFi_Robo

TODO:

	1. Matrix_Determ() function for matrix dimension > 3.
	2. Float precision seems not enough ???
	   Determinat computation example: matlab 0.6    mathwork 0.599976

Midas
------------------  COPYLEFT  --------------------*/

#ifndef   ___MATHWORK__H__
#define   ___MATHWORK__H__

#include <stdint.h>
#include <sys/time.h>
#include <malloc.h>

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
//  !!!!NOTE: all matrix data is stored from row to column.


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


/*-----------------------------------------------
Multiply a matrix with a factor
int nr:  number of row
int nc:  number of column
*matA:   pointer to matrix A
fc:      the  multiplicator
Return:
	NULL  --- fails
	pointer to matA --- OK
-----------------------------------------------*/
float* Matrix_MultFactor(int nr, int nc, float *matA, float fc)
{
	int i;

	if(matA==NULL)
	{
	   fprintf(stderr,"Matrix_MultFactor(): matA is a NULL pointer!\n");
	   return NULL;
	}

	for(i=0; i<nr*nc; i++)
		matA[i] *= fc;

	return matA;
}


/*-----------------------------------------------
Divide a matrix with a factor
int nr:  number of row
int nc:  number of column
*matA:   pointer to matrix A
fc:      the divider
Return:
	NULL  --- fails
	pointer to matA --- OK
-----------------------------------------------*/
float* Matrix_DivFactor(int nr, int nc, float *matA, float fc)
{
	int i;

	if(matA==NULL)
	{
	   fprintf(stderr,"Matrix_DivFactor(): matA is a NULL pointer!\n");
	   return NULL;
	}

	for(i=0; i<nr*nc; i++)
		matA[i] /= fc;

	return matA;
}



/*----------------     MATRIX TRANSPOSE    ---------------------
Transpose a matrix,swap element matA(i,j) and matA(j,i)

int nr:  number of row
int cn:  number of column
*matA:   pointer to matrix A
After operation row number will be cn, and column number will be nr.

Return:
	NULL ---  fails
	point to matA  --- OK
----------------------------------------------------------*/
float* Matrix_Transpose(int nr, int nc, float *matA)
{
	int i,j;
	int nbytes=nr*nc*sizeof(float);
	float *matB=malloc(nbytes);

	if(matB==NULL)
	{
	   fprintf(stderr,"Matrix_Transpose(): malloc() fails!\n");
	   return NULL;
	}
	//----- swap to matB ----
	for(i=0; i<nc; i++) //nc-> new nr
		for(j=0; j<nr; j++)// nr -> new nc
			matB[i*nr+j]=matA[j*nc+i];
	//----- recopy to matA -----
	memcpy(matA,matB,nbytes);

	//----- release matB
	free(matB);

	return matA; 
}


/*----------------     MATRIX DETERMINANT    ---------------------

	!!!! for matrix dimension NOT great than 3 !!!!

Calculate determinant of a SQUARE matrix
int nrc:  number of row and column
*matA: the square matrix
*determ: determinant of the matrix
Return:
	NULL ---  fails
	pointer to the result  --- OK
----------------------------------------------------------*/
float* Matrix_Determ(int nrc, float *matA, float *determ)
{
     int i,j,k;
     float pt=0.0,mt=0.0; //plus and minus multiplication
     float tmp=1.0;

     if(matA==NULL || determ==NULL)
     {
	   fprintf(stderr,"Matrix_Determ(): matrix pointer is NULL!\n");
	   return NULL;
     }

     //--------------- CASE 1 ----------------------
     if(nrc==1)
     {
	*determ = matA[0];
	return determ;
     }

     //--------------- CASE 2 ----------------------
     else if(nrc==2)
     {
	*determ = matA[0]*matA[3]-matA[1]*matA[2];
	 return determ;
     }

     //--------------- CASE 3 ----------------------
     else if(nrc==3) 
     {
     	//------plus multiplication
	     for(i=0; i<nrc; i++)
	     {
		   k=i;
	  	   for(j=0; j<nrc; j++)
		   {
			tmp *= matA[k];
			if( (k+1)%nrc == 0)
				k+=1;
			else
				k+=(nrc+1);
	     	   }
		   pt += tmp;
		   tmp=1.0;
	     }

	     //------minus multiplication
	     for(i=0; i<nrc; i++)
	     {
		   k=i;
  	  	 for(j=0; j<nrc; j++)
		   {
			tmp *= matA[k];
			if( k%nrc == 0 )
				k+=(2*nrc-1);
			else
				k+=(nrc-1);
	     	   }

		   mt += tmp;
		   tmp=1.0;
 	    }

	     *determ = pt-mt;
	     return determ;
     }

     //--------------- CASE 4, dimension great than 3 !!! ----------------------
     else
     {
	    fprintf(stderr, " Matrix dimension is great than 3, NO SOLUTION at present !!!!!\n");
	    return NULL;
     }
}


/*----------------     MATRIX INVERSE    -------------------
compute the inverse of a SQUARE matrix

int nrc:  number of row and column
*matA: the sqaure matrix,
*matAdj: for adjugate matrix, also for the final inversed square matrix!!!
Return:
	NULL ---  fails
	pointer to the result matAdj  --- OK
----------------------------------------------------------*/
float* Matrix_Inverse(uint32_t nrc, float *matA, float *matAdj)
{

	int i,j,k;
	float  det_matA; //determinant of matA
//	float* matAdj=malloc(nrc*nrc*sizeof(float)); // for adjugate matrix
	float* matCof=malloc((nrc-1)*(nrc-1)*sizeof(float)); //for cofactor matrix

	if(matAdj==NULL || matCof==NULL)
	{
		fprintf(stderr,"Matrix_Inverse(): malloc matAdj or/and matCof failed!\n");
		return NULL;
	}

	//----- check if matrix matA is invertible ----
	Matrix_Determ(nrc,matA,&det_matA); //comput determinant of matA
	printf("determint of input matrix is %f\n",det_matA);
	if(det_matA == 0)
	{
		fprintf(stderr,"Matrix_Inverse(): matrix is NOT invertible!\n");
		return NULL;
	}

  	//-----compute adjugate matrix
	for(i=0; i<nrc*nrc; i++)  // i, also cross center element natural number
	{
            //-------compute cofactor matrix matCof(i)[]
		j=0;// element number of matCof
		k=0; // element number of original matA
		while(k<nrc*nrc) // traverse all elements of the original matrix
		{
			//------ skip elements accroding to i ------
			if( k/nrc == i/nrc ) //skip row  elements 
			{
				k+=nrc;  //skip one row
				continue;
			}
		 	if( k%nrc == i%nrc ) //skip column  elements
			{
				k+=1; // skip one element
				continue;
			}
			//--- copy an element of matA to matCof
			matCof[j]=matA[k];
			k++;
			j++;
		}//end of while()

		//finish i_th matCof
//		Matrix_Print(nrc-1,nrc-1,matCof);

		//---- compute determinant of the i_th cofactor matrix as i_th element of the adjugate matrix
		Matrix_Determ(nrc-1,matCof, matAdj+i);
		// if(i%2 == 0) matAdj[i] = 1.0*matAdj[i];
		if( (i/nrc)%2 != (i%nrc)%2 )
			matAdj[i] = -1.0*matAdj[i];

	}// end of for(i),  computing adjugate matrix NOT finished yet

	//----- transpose the matrix to finish computing adjugate matrix
	Matrix_Transpose(nrc, nrc, matAdj);

	//----- compute inverse matrix
	Matrix_DivFactor(nrc,nrc,matAdj,det_matA);//matAdj /= det_matA


	//----- free mem.
	free(matCof);

	return matAdj;

}

#endif
