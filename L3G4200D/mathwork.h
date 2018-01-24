
/*----------------------------------------------------------------------
For Widora WiFi_Robo

TODO:
	1. Float precision seems not enough ???
	   Determinat computation example: matlab 0.6    mathwork 0.599976
	2.

Midas
----------------------------  COPYLEFT  --------------------------------*/

#ifndef   ___MATHWORK__H__
#define   ___MATHWORK__H__

#include <stdint.h>
#include <sys/time.h>
#include <malloc.h>
#include <string.h>

struct float_Matrix
{
  int nc; //column n
  int nr; //row n
  float* pmat;
};


//------ function declaration -------
inline uint32_t get_costtimeus(struct timeval tm_start, struct timeval tm_end);
inline uint32_t math_tmIntegral_NG(uint8_t num, const double *fx, double *sum);
inline uint32_t math_tmIntegral(const double fx, double *sum);

/*<<<<<<<<<<<<<<<      MATRIX ---  OPERATION     >>>>>>>>>>>>>>>>>>
NOTE:
	1. All matrix data is stored from row to column.
	2. All indexes are starting from 0 !!!
------------------------------------------------------------------*/
void   Matrix_Print(struct float_Matrix matA);

float  Matrix3X3_Determ(float *pmat);
float  MatrixGT3X3_Determ(int nrc, float *pmat);

struct float_Matrix* Matrix_CopyColumn(struct float_Matrix *matA, int nclmA, struct float_Matrix *matB, int nclmB);

struct float_Matrix* Matrix_Add( struct float_Matrix *matA,
				 struct float_Matrix *matB,
				 struct float_Matrix *matC );

struct float_Matrix* Matrix_Sub( struct float_Matrix *matA,
				 struct float_Matrix *matB,
				 struct float_Matrix *matC  );

struct float_Matrix* Matrix_Multiply( const struct float_Matrix *matA,
				      const struct float_Matrix *matB,
				      struct float_Matrix *matC );

struct float_Matrix* Matrix_MultFactor(struct float_Matrix *matA, float fc);
struct float_Matrix* Matrix_DivFactor(struct float_Matrix *matA, float fc);
struct float_Matrix* Matrix_Transpose( struct float_Matrix *matA,
				       struct float_Matrix *matB );
float* Matrix_Determ( struct float_Matrix *matA,  float *determ );
struct float_Matrix* Matrix_Inverse( struct float_Matrix *matA,
				     struct float_Matrix *matAdj );


/*----------------------------------------------
 calculate and return time difference in us

Return
  if tm_start > tm_end then return 0 !!!!
  us
----------------------------------------------*/
inline uint32_t get_costtimeus(struct timeval tm_start, struct timeval tm_end)
{
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
//  !!!! NOTE:
//		1. All matrix data is stored from row to column.
//		2. All indexes are starting from 0 !!!

/*-------------------------------------------
              Print matrix
-------------------------------------------*/
void Matrix_Print(struct float_Matrix matA)
{
   int i,j;
   int nr=matA.nr;
   int nc=matA.nc;

   if(matA.pmat == NULL)
   {
	printf(" Matrix data is NULL!\n");
	return;
   }

   for(i=0; i<nc; i++)
	printf("       Column(%d)",i);
   printf("\n");

   for(i=0; i<nr; i++)
   {
        printf("Row(%d):  ",i);
        for(j=0; j<nc; j++)
           printf("%f    ", (matA.pmat)[nc*i+j] );
        printf("\n");
   }
   printf("\n");
}


/*-------------------------     MATRIX ADD/SUB    -------------------------
     matC = matA +/- matB
Addup/subtraction operation of two matrices with same row and column number
*matA:   pointer to matrix A
*matB:   pointer to matrix B
*matC:   pointer to mastrix A+B or A-B
Note: matA matB and matC may be the same!!
Return:
	NULL  --- fails
	matC --- OK
----------------------------------------------------------*/
struct float_Matrix* Matrix_Add( struct float_Matrix *matA,
		   struct float_Matrix *matB,
		   struct float_Matrix *matC )
{
   int i,j;
   int nr,nc;

   //---- check pointer -----
   if(matA==NULL || matB==NULL || matC==NULL)
   {
	   fprintf(stderr,"Matrix_Add(): matrix is NULL!\n");
	   return NULL;
   }
   //----- check pmat ------
   if(matA->pmat==NULL || matB->pmat==NULL || matC->pmat==NULL)
   {
	   fprintf(stderr,"Matrix_Add(): matrix pointer pmat is NULL!\n");
	   return NULL;
   } 
   //---- check dimension -----
   if(  matA->nr != matB->nr || matB->nr != matC->nr ||
        matA->nc != matB->nc || matB->nc != matC->nc )
   {
	  fprintf(stderr,"Matrix_Add(): matrix dimensions don't match!\n");
	  return NULL;
   }

   nr=matA->nr;
   nc=matA->nc;

   for(i=0; i<nr; i++) //row count
   {
	for(j=0; j<nc; j++) //column count
	{
		(matC->pmat)[i*nc+j] = (matA->pmat)[i*nc+j]+(matB->pmat)[i*nc+j];
	}
   }

   return matC;
}

struct float_Matrix* Matrix_Sub( struct float_Matrix *matA,
				 struct float_Matrix *matB,
				 struct float_Matrix *matC  )
{
   int i,j;
   int nr,nc;

   //---- check pointer -----
   if(matA==NULL || matB==NULL || matC==NULL)
   {
	   fprintf(stderr,"Matrix_Sub(): matrix is NULL!\n");
	   return NULL;
   }
   //----- check pmat ------
   if(matA->pmat==NULL || matB->pmat==NULL || matC->pmat==NULL)
   {
	   fprintf(stderr,"Matrix_Sub(): matrix pointer pmat is NULL!\n");
	   return NULL;
   }
   //---- check dimension -----
   if(  matA->nr != matB->nr || matB->nr != matC->nr ||
        matA->nc != matB->nc || matB->nc != matC->nc )
   {
	  fprintf(stderr,"Matrix_Sub(): matrix dimensions don't match!\n");
	  return NULL;
   }

   nr=matA->nr;
   nc=matA->nc;

   for(i=0; i<nr; i++) //row count
   {
        for(j=0; j<nc; j++) //column count
        {
                (matC->pmat)[i*nc+j] = (matA->pmat)[i*nc+j]-(matB->pmat)[i*nc+j];
        }
   }
   return matC;
}


/*----------------     MATRIX Get a Column  ---------------------
Copy a column of a matrix to another one dimension matrix,or to the first
column of another matrix.
matA:   pointer to matrix A
nclmA:  number of column to be copied
matB:   pointer to the result matrix.
nclmB:  number of column to copy to
Return:
	NULL  --- fails
	matB   --- OK
----------------------------------------------------------*/
struct float_Matrix* Matrix_CopyColumn(struct float_Matrix *matA, int nclmA, struct float_Matrix *matB, int nclmB)
{
    int i;
    int ncA,nrA,ncB;

   //---- check pointer -----
   if(matA==NULL || matB==NULL )
   {
	   fprintf(stderr,"Matrix_CopyColumn(): matrix is NULL!\n");
	   return NULL;
   }

   //----- check pmat ------
   if(matA->pmat==NULL || matB->pmat==NULL )
   {
	   fprintf(stderr,"Matrix_CopyColumn(): matrix pointer pmat is NULL!\n");
	   return NULL;
   }

    ncA=matA->nc;
    nrA=matA->nr;
    ncB=matB->nc;

   //----- check column number
   if( nclmA > ncA || nclmB > ncB)
   {
	   fprintf(stderr,"Matrix_CopyColumn():column number out of range!\n");
	   return NULL;
   }

   for(i=0; i<nrA; i++)
	(matB->pmat)[nclmB+i*ncB] = (matA->pmat)[nclmA+i*ncA];

   return   matB;

}


/*----------------     MATRIX MULTIPLY    ---------------------
Multiply two matrices
   !!! ncA == nrB !!!!
matA[nrA,ncA]:   matrix A
matB[nrB,ncB]:   matrix B
matC[nrA,ncB]:   mastrix c=A*B

Return:
	NULL ---  fails
	matC  --- OK
----------------------------------------------------------*/
struct float_Matrix* Matrix_Multiply( const struct float_Matrix *matA,
				      const struct float_Matrix *matB,
				      struct float_Matrix *matC )
{
	float *fret;
	int i,j,k;
	int nrA,ncA,nrB,ncB,nrC,ncC;

	//---- check pointer -----
	if(matA==NULL || matB==NULL || matC==NULL)
	{
	   fprintf(stderr,"Matrix_Multiply(): matrix is NULL!\n");
	   return NULL;
	}
   	//----- check pmat ------
	if(matA->pmat==NULL || matB->pmat==NULL || matC->pmat==NULL)
	{
	    fprintf(stderr,"Matrix_Multiply(): matrix pointer pmat is NULL!\n");
	    return NULL;
	}

	nrA=matA->nr;
	ncA=matA->nc;
	nrB=matB->nr;
	ncB=matB->nc;
	nrC=matC->nr;
	ncC=matC->nc;

	//----- match matrix dimension -----
	if(ncA != nrB || nrC != nrA || ncC != ncB)
	{
	   fprintf(stderr,"Matrix_Multiply(): dimension not match!\n");
	   return NULL;
	}

	//---- result matrix with row: nrA  column: ncB, recuse nrA and ncB then.
	for(i=0; i<nrA; i++) //rows of matC (matA)
		for(j=0; j<ncB; j++) // columns of matC  (matB)
		{
			//---- clear matC->pmat first before += !!!!!!
			(matC->pmat)[i*ncB+j] =0;
			for(k=0; k<ncA; k++)//
			{
				(matC->pmat)[i*ncB+j] += (matA->pmat)[i*ncA+k] * (matB->pmat)[ncB*k+j]; // (element j, row i of matA) .* (column j of matB)
				//matC[0] += matA[k]*matB[ncB*k];//i=0,j=0
			}
		}
       return matC;
}


/*-----------------------------------------------
	 matA=matA*fc
Multiply a matrix with a factor
matA:   pointer to matrix A
fc:      the  multiplicator
Return:
	NULL  --- fails
	matA --- OK
-----------------------------------------------*/
struct float_Matrix* Matrix_MultFactor(struct float_Matrix *matA, float fc)
{
	int i;
	int nr;
	int nc;

	//---- check pointer ----
	if(matA==NULL)
	{
	   fprintf(stderr,"Matrix_MultFactor(): matA is a NULL!\n");
	   return NULL;
	}
   	//----- check pmat ------
	if(matA->pmat==NULL)
	{
	    fprintf(stderr,"Matrix_MultFactor(): matA->pmat is a NULL!\n");
	    return NULL;
	}

	nr=matA->nr;
	nc=matA->nc;

	for(i=0; i<nr*nc; i++)
		(matA->pmat)[i] *= fc;

	return matA;
}


/*-----------------------------------------------
	matA=matA/factor
Divide a matrix with a factor 
matA:    pointer to matrix
fc:      the divider
Return:
	NULL  --- fails
	pointer to matA --- OK
-----------------------------------------------*/
struct float_Matrix* Matrix_DivFactor(struct float_Matrix *matA, float fc)
{
	int i;
	int nr;
	int nc;

	//----- check pointer ----
	if(matA==NULL)
	{
	   fprintf(stderr,"Matrix_DivFactor(): matA is a NULL pointer!\n");
	   return NULL;
	}
   	//----- check pmat ------
	if(matA->pmat==NULL)
	{
	    fprintf(stderr,"Matrix_DivFactor(): matA->pmat is a NULL!\n");
	    return NULL;
	}
	//---- check fc -----
	if(fc==0)
	{
	    fprintf(stderr,"Matrix_DivFactor(): divider factor is ZERO!\n");
	    return NULL;
	}

	nr=matA->nr;
	nc=matA->nc;

	for(i=0; i<nr*nc; i++)
		(matA->pmat)[i] /= fc;

	return matA;
}



/*----------------     MATRIX TRANSPOSE    ---------------------
	matA(i,j) -> matB(j,i)
Transpose a matrix and copy to another matrix
transpose means: swap element matA(i,j) and matA(j,i)
after operation row number will be cn, and column number will be nr.

matA:   pointer to matrix A
matB:   pointer to matrix B, who will receive the transposed matrix.

Note:   1.MatA and matB may be the same!
	2.Applicable ONLY if (nrA*ncA == nrB*ncB)

Return:
	NULL ---  fails
	point to matB  --- OK
----------------------------------------------------------*/
struct float_Matrix* Matrix_Transpose( struct float_Matrix *matA,
				       struct float_Matrix *matB )
{
	int i,j;
	int nr;
	int nc;
	int nbytes;
	float *mat; //for temp. buff

	//---- check pointer -----
	if(matA==NULL || matB==NULL)
	{
	   fprintf(stderr,"Matrix_Transpose(): matrix is NULL!\n");
	   return NULL;
	}
   	//----- check pmat ------
	if(matA->pmat==NULL || matB->pmat==NULL)
	{
	    fprintf(stderr,"Matrix_Transpose(): matrix data pmat is NULL!\n");
	    return NULL;
	}

	nr=matA->nr;
	nc=matA->nc;
	nbytes=nr*nc*sizeof(float);

	mat=malloc(nbytes); //for temp. buff
	if(mat==NULL)
	{
	   fprintf(stderr,"Matrix_Transpose(): malloc() mat fails!\n");
	   return NULL;
	}

	//----- match matrix dimension -----
	if( nr*nc != (matB->nr)*(matB->nc) )
	{
	   fprintf(stderr,"Matrix_Transpose(): dimensions do not match!\n");
	   return NULL;
	}


	//----- swap to matB ----
	for(i=0; i<nc; i++) //nc-> new nr
		for(j=0; j<nr; j++)// nr -> new nc
			mat[i*nr+j]=(matA->pmat)[j*nc+i];

	//----- recopy to matA -----
	memcpy(matB->pmat,mat,nbytes);

	//----- revise nc and nr for matB ----
	matB->nr = nc;
	matB->nc = nr;

	//----- release temp. mat
	free(mat);

	return matB; 
}


/*----------------     MATRIX DETERMINANT    ---------------------

	!!!! for matrix dimension NOT great than 3 ONLY !!!!

Calculate determinant of a SQUARE matrix
int nrc:  number of row and column
matA:   the square matrix
determ: determinant of the matrix
Return:
	NULL ---  fails
	float pointer to the result  --- OK
----------------------------------------------------------*/
float* Matrix_Determ( struct float_Matrix *matA,
				    float *determ )
{
     int i,j,k;
     int nrc;
     float pt=0.0,mt=0.0; //plus and minus multiplication
     float tmp=1.0;

     //----- preset determ
     *determ = 0.0;

     //----- check pointer ----
     if(matA==NULL || determ==NULL)
     {
	   fprintf(stderr,"Matrix_Determ(): matrix is NULL!\n");
	   return determ;
     }
     //----- check matrix dimension -----
     if(matA->nr != matA->nc)
     {
	   fprintf(stderr,"Matrix_Determ(): it's NOT a square matrix!\n");
	   return determ;
     }

     nrc=matA->nr;

     //--------------- CASE 1 ----------------------
     if(nrc==1)
     {
	*determ = (matA->pmat)[0];
	return determ;
     }
     //--------------- CASE 2 ----------------------
     else if(nrc==2)
     {
	*determ = (matA->pmat)[0]*(matA->pmat)[3]-(matA->pmat)[1]*(matA->pmat)[2];
	 return determ;
     }
     //--------------- CASE 3 ----------------------
     else if(nrc==3)
     {
	*determ=Matrix3X3_Determ(matA->pmat);
	return determ;
     }
     //--------------- CASE >3 ----------------------
     else // (nrc > 3)
     {

	*determ=MatrixGT3X3_Determ(nrc, matA->pmat);
	return determ;

     }

}



/*----------------     MATRIX INVERSE    -------------------
compute the inverse of a SQUARE matrix

int nrc:  number of row and column
matA: the sqaure matrix,
matAdj: for adjugate matrix, also for the final inversed square matrix!!!
Return:
	NULL ---  fails
	pointer to the result matAdj  --- OK
----------------------------------------------------------*/
struct float_Matrix* Matrix_Inverse(struct float_Matrix *matA,
				    struct float_Matrix *matAdj )
{
	int i,j,k;
	int nrc;
	float  det_matA; //determinant of matA
	struct float_Matrix matCof;

	//------ check pointer -----
	if(matA==NULL || matAdj==NULL)
	{
		fprintf(stderr,"Matrix_Inverse():  matA and/or matAdj is a NULL!\n");
		return NULL;
	}
	//----- check data -----
        if(matA->pmat==NULL || matAdj->pmat==NULL)
        {
	   	fprintf(stderr,"Matrix_Inverse():  matrix data is NULL!\n");
	   	return NULL;
        }

	//----- check dimension -----
	if( matA->nc != matA->nr || matAdj->nc != matAdj->nr )
	{
		fprintf(stderr,"Matrix_Inverse():  matrix is not square!\n");
		return NULL;
	}
	if( matA->nc != matAdj->nc )
	{
		fprintf(stderr,"Matrix_Inverse():  dimensions do NOT match!\n");
		return NULL;
	}

        nrc=matA->nr;
	matCof.nc=nrc-1;
	matCof.nr=nrc-1;

	//----- check nrc and malloc matCof ------
	if( nrc <= 0)
        {
		fprintf(stderr,"Matrix_Inverse():  nrc <= 0 !! \n");
		return NULL;
        }
	else if( nrc == 1 ) // if it's ONE dimentsion matrix !!!
        {
		matAdj->pmat[0]=1.0/(matA->pmat[0]);
		return matAdj;
        }
        else // nrc > 1
        {
		//---- malloc matCof ----
		matCof.pmat=malloc((nrc-1)*(nrc-1)*sizeof(float)); //for cofactor matrix data
		//------ check pointer -----
		if( matCof.pmat==NULL)
		{
			fprintf(stderr,"Matrix_Inverse():  malloc. matCof.pmat failed!\n");
			return NULL;
		}
   	}

	//----- check if matrix matA is invertible ----
	Matrix_Determ(matA,&det_matA); //comput determinant of matA
	printf("Matrix_Inverse(): determint of input matrix is %f\n",det_matA);
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
			matCof.pmat[j]=matA->pmat[k];
			k++;
			j++;
		}//end of while()
		//finish i_th matCof

		//---- compute determinant of the i_th cofactor matrix as i_th element of the adjugate matrix
		Matrix_Determ(&matCof, (matAdj->pmat+i));
		//----- decide the sign of the element
		if( (i/nrc)%2 != (i%nrc)%2 )
			(matAdj->pmat)[i] = -1.0*(matAdj->pmat)[i];
	}// end of for(i),  computing adjugate matrix NOT finished yet
	//----- transpose the matrix to finish computing adjugate matrix !!!
	Matrix_Transpose(matAdj,matAdj);

	//----- compute inverse matrix
	Matrix_DivFactor(matAdj,det_matA);//matAdj /= det_matA

	//----- free mem.
	free(matCof.pmat);

	return matAdj;
}


/*-------------------------------------------------
       3x3 matrix determinant computation

pmat: pointer to a 3x3 data array as input matrix
return:
	fails       0
	succeed     determinant value
--------------------------------------------------*/
float  Matrix3X3_Determ(float *pmat)
{
     int i,j,k;
     float pt=0.0,mt=0.0; //plus and minus multiplication
     float tmp=1.0;
     //----- preset determ
     float determ = 0.0;

     //----- check pointer -----
     if(pmat==NULL)
     {
	fprintf(stderr,"Matrix4X4_Determ(): pmat is NULL!\n");
	return 0;
     }
    //------plus multiplication
     for(i=0; i<3; i++)
     {
	   k=i;
  	   for(j=0; j<3; j++)
	   {
		tmp *= pmat[k];
		if( (k+1)%3 == 0)
			k+=1;
		else
			k+=(3+1);
     	   }
	   pt += tmp;
	   tmp=1.0;
     }

     //------minus multiplication
     for(i=0; i<3; i++)
     {
	   k=i;
	   for(j=0; j<3; j++)
	   {
		tmp *= pmat[k];
		if( k%3 == 0 )
			k+=(2*3-1);
		else
			k+=(3-1);
	   }
	   mt += tmp;
	   tmp=1.0;
      }

     determ = pt-mt;
     return determ;
}


/*-----------------------------------------------------------
       >3x3 matrix determinant computation
!!! Warning: THis is a self recursive function !!!!
The input Must be a square matrix, number of rows and columns are the same
ncr: dimension number, number of columns or rows
pmat: pointer to a >=4x4 data array as input matrix
return:
	fails       0
	succeed     determinant value
-------------------------------------------------------------*/
float  MatrixGT3X3_Determ(int nrc, float *pmat)
{
   int i,j,k;
   float *pmatCof; //pointer to cofactor matrix
   float determ=0;
   float sign; //sign of element of the adjugate matrix

   //---------------  CASE   nrc<3  -----------------
   if (nrc < 3)
   {
	printf("MatrixGT3X3_Determ(): matrix dimension is LESS than 3x3!\n");
	return 0;
   }
   //---------------  CASE   nrc=3  !!!! this case is a MUST for recursive call !!!!  -----------------
   if (nrc == 3)
	return Matrix3X3_Determ(pmat);

   //------------     CASE   nrc >3    --------------
   //-----malloc for pmatCof---
   pmatCof=malloc((nrc-1)*(nrc-1)*sizeof(float));
   if(pmatCof == NULL)
   {
	fprintf(stderr,"MatrixGT3X3_Determ(): malloc pmatConf failed!\n");
	return 0;
   }

   //----- the terminant of a matrix equals summation of (any) one row of (element value)*sign*(determinant of its cofactor matrix)  ----
   for(i=0; i<nrc; i++)  //summation first row,  i- also cross center element natural number
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
		pmatCof[j]=pmat[k];
		k++;
		j++;
	}//end of while()
	//finish i_th matCof

	//----- decide the sign of the element
	if( (i/nrc)%2 != (i%nrc)%2 )
		sign=-1.0;
	else
		sign=1.0;

	//----- recursive call -----
        //summation of first row of (element value)*sign*(determinant of its cofactor;
	determ += pmat[i]*sign*MatrixGT3X3_Determ(nrc-1, pmatCof);

//	printf("determ[%d]=%f\n",i,determ);

   }// end of for(i),  computing adjugate matrix NOT finished yet

  return determ;

}

#endif
