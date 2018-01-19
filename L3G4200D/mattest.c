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
   printf("matB=\n");
   Matrix_Print(2,3,matB);
   printf("matX=\n");
   Matrix_Print(3,4,matX);
  
   Matrix_Multiply(2,3,matB, 3, 4, matX, matY);

   printf("Matrix matY = matB*matX = \n");
   Matrix_Print(2,4,matY);

}
