#include <stdio.h>
#include "mathwork.h"


void main(void)
{
 int i,j;
 float matA[2*3]=  // row 2, column 3
 {
   1,2,3,
   4,5,6
 };

 float matB[2*3]=  // row 2, column 3
 {
   6,5,4,
   3,2,1
 };

 float matX[3*4]=
{
   6,3,0.6,0.3,
   5,2,0.5,0.2,
   4,1,0.4,0.1
};

 float matM[3*3]=
{
   6,1,1,
   4,-2,5,
   2,8,7,
};


 float matN[3*3]=
{
  3,3.2,5,
  3.5,3.6,5,
  6,6,6
};

 float invmatN[2*2]={0};

  float determ;

  float matC[2*3]={0};
  float matD[2*3]={0};
  float matY[2*4]={0};
 


   //----- Matrix ADD_SUB Operation ----
   Matrix_Add(2,3,matA,matB,matC);
   Matrix_Sub(2,3,matA,Matrix_Add(2,3,matA,matB,matD),matD);
   //-----  print Matrix C ------
   printf("matA = \n");
   Matrix_Print(2,3,matA);
   printf("matB = \n");
   Matrix_Print(2,3,matB);
   printf("matD= matA-(matA+matB) = \n");
   Matrix_Print(2,3,matD);

   //----- Matrix Multiply Operation ----
   printf("\nmatB=\n");
   Matrix_Print(2,3,matB);
   printf("\nmatX=\n");
   Matrix_Print(3,4,matX);
  
   Matrix_Multiply(2,3,matB, 3, 4, matX, matY);

   printf("\nMatrix matY = matB*matX = \n");
   Matrix_Print(2,4,matY);

   //----- Matrix MultFactor and DivFactor ----
   Matrix_MultFactor(2,3,matA,5.0);
   printf("\nMatrix matA = matA*5.0 = \n");
   Matrix_Print(2,3,matA);
   Matrix_DivFactor(2,3,matB,5.0);
   printf("\nMatrix matB = matB/5.0 = \n");
   Matrix_Print(2,3,matB);

   //----- Matrix Transpose Operation ----
   Matrix_Transpose(2, 4, matY);
   printf("\nMatrix matY = Transpose(matY) = \n");
   Matrix_Print(4,2,matY);

   //----- Matrix Determinant Calculation ----
   Matrix_Determ(3, matM, &determ);
   printf("\nmatM = \n");
   Matrix_Print(3,3,matM);
   printf("Matrix_Determ(matM)=%f \n",determ);

   //---- Matrix Inverse Computatin -----
   printf("\nmatN = \n");
   Matrix_Print(3,3,matN);
   Matrix_Determ(3,matN, &determ);
   printf(" Matrix_Determ(matN) = %f \n",determ);
   Matrix_Inverse(3, matN, invmatN);
   printf("\ninvmatN = \n");
   Matrix_Print(3,3,invmatN);
}
