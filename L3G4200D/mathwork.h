
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
  int nr; //row n
  int nc; //column n
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
struct float_Matrix * init_float_Matrix(int nr, int nc);
void release_float_Matrix(struct float_Matrix * pMat);


void   Matrix_Print(struct float_Matrix matA);
struct float_Matrix*  Matrix_FillArray(struct float_Matrix * pMat, const float * array); //fill pMat->pmat with data from array
float  Matrix3X3_Determ(float *pmat); // determinnat of a 3X3 matrix
float  MatrixGT3X3_Determ(int nrc, float *pmat); // determinant of a matrix whose dimension is great than 3x3

struct float_Matrix* Matrix_CopyColumn(struct float_Matrix *matA, int nclmA, struct float_Matrix *matB, int nclmB);

struct float_Matrix* Matrix_Add( const struct float_Matrix *matA,
				 const struct float_Matrix *matB,
				 struct float_Matrix *matC );

struct float_Matrix* Matrix_Sub( const struct float_Matrix *matA,
				 const struct float_Matrix *matB,
				 struct float_Matrix *matC  );

struct float_Matrix* Matrix_Multiply( const struct float_Matrix *matA,
				      const struct float_Matrix *matB,
				      struct float_Matrix *matC );

struct float_Matrix* Matrix_MultFactor(struct float_Matrix *matA, float fc);
struct float_Matrix* Matrix_DivFactor(struct float_Matrix *matA, float fc);
struct float_Matrix* Matrix_Transpose(const struct float_Matrix *matA,
				       struct float_Matrix *matB );
float* Matrix_Determ( const struct float_Matrix *matA,  float *determ );
struct float_Matrix* Matrix_Inverse( const struct float_Matrix *matA,
				     struct float_Matrix *matAdj );


#endif
