#include <stdio.h>
#include "mathwork.h"


void main(void)
{
 int i,j;



 float matA[3*4]=  // row 2, column 3
 {
   1,2,3,8,
   4,5,6,10,
   3,0,0.3,4
 };
 struct float_Matrix mat_A;
 mat_A.nr =3; mat_A.nc=4; mat_A.pmat=matA;


 float matB[3*4]=  // row 2, column 3
 {
   43,4,6,0.5,
   6,5,4,5,
   3,2,1,5
 };
 struct float_Matrix mat_B;
 mat_B.nr =3; mat_B.nc=4; mat_B.pmat=matB;

 float matX[4*4]=
{
   6,3,0.6,0.3,
   5,2,0.5,0.2,
   4,1,0.4,0.1,
   4,1,0.4,0.1
};
 struct float_Matrix mat_X;
 mat_X.nr =4; mat_X.nc=4; mat_X.pmat=matX;


float matClm[3*1]={0};


 float matM[2*2]=
{
   14.23,-2.5,
   2.10,80.445877
};
struct float_Matrix mat_M;
mat_M.nr=2; mat_M.nc=2; mat_M.pmat=matM;

 float matN[2*2]=
{
  3,23.2,
  3.5,13.6
};
struct float_Matrix mat_N;
mat_N.nr=2; mat_N.nc=2; mat_N.pmat=matN;

 float invmatN[2*2]={0};
 struct float_Matrix invmat_N; invmat_N.nr=2; invmat_N.nc=2; invmat_N.pmat=invmatN;


 float determ;

  float matC[3*4]={0};
  struct float_Matrix mat_C; mat_C.nr=3; mat_C.nc=4; mat_C.pmat=matC;

  float matD[2*4]={0};

  float matY[3*4]={0};
  struct float_Matrix mat_Y; mat_Y.nr=3; mat_Y.nc=4; mat_Y.pmat=matY;


  //----- Matrix ADD_SUB Operation ----
  printf("mat_A = \n");
  Matrix_Print(mat_A);
  printf("mat_B = \n");
  Matrix_Print(mat_B);
  Matrix_Add(&mat_A,&mat_B,&mat_C);
  printf("mat_C = \n");
  Matrix_Print(mat_C);
  //----- copy a column from a matrix ---
  Matrix_CopyColumn(&mat_A,2,&mat_A,1);
  printf("Matrix_CopyColumn(mat_A,2,mat_A,1), mat_A=\n");
  Matrix_Print(mat_A);
  Matrix_Add(&mat_A, &mat_A,&mat_A);
  printf("mat_A = mat_A + mat_A =\n");
  Matrix_Print(mat_A);
  Matrix_Sub(&mat_A,&mat_B,&mat_A);
  printf("matA = matA-matB =\n");
  Matrix_Print(mat_A);

  //----- Matrix Multiply Operation ----
   printf("\nmatB=\n");
   Matrix_Print(mat_B);
   printf("\nmatX=\n");
   Matrix_Print(mat_X);
   Matrix_Multiply(&mat_B, &mat_X, &mat_Y);
   printf("\n matY = matB*matX = \n");
   Matrix_Print(mat_Y);

   //----- Matrix MultFactor and DivFactor ----
   Matrix_MultFactor(&mat_X,5.0);
   printf("matX = matX*5.0 = \n");
   Matrix_Print(mat_X);
   Matrix_DivFactor(&mat_Y,5.0);
   printf("matY = matY/5.0 = \n");
   Matrix_Print(mat_Y);

   //----- Matrix Transpose Operation ----
   Matrix_Transpose(&mat_Y, &mat_Y);
   printf("\nmatY = Transpose(matY,matY) = \n");
   Matrix_Print(mat_Y);

   //----- Matrix Determinant Calculation ----
   Matrix_Determ(&mat_M, &determ);
   printf("\nmatM = \n");
   Matrix_Print(mat_M);
   printf("Matrix_Determ(mat_M)=%f \n",determ);

   //---- Matrix Inverse Computatin -----
   printf("\nmatN = \n");
   Matrix_Print(mat_N);
   Matrix_Determ(&mat_N, &determ);
   printf(" Matrix_Determ(matN) = %f \n",determ);
   Matrix_Inverse(&mat_N, &invmat_N);
   printf("\ninvmatN = \n");
   Matrix_Print(invmat_N);


}
