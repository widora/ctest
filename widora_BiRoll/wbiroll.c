/*----------------------------------------------------

set GYRO L3G4200 ODR=800Hz BW=35Hz
set ACC. ADXL345 ODR=800Hz BW=400Hz

----------------------------------------------------*/
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include "gyro_spi.h"
#include "gyro_l3g4200d.h"
#include "i2c_adxl345.h" //--- use  read() write() to operate I2C
#include "data_server.h"
#include "filters.h"
#include "mathwork.h"
#include "kalman_n2m2.h"

//#define TCP_TRANSFER


int main(void)
{
   int ret_val;
//   uint8_t val;
   int i,k;
   int send_count=0;

   //------ for ADXL345  -----
   int16_t accXYZ[3]={0}; // acceleration value of XYZ
   double  faccXYZ[3]; //double //faccXYZ=fsmg_full*accXYZ
   float fangX; //atan(X/Z) angle of X axis
   struct int16MAFilterDB fdb_accXYZ[3]; // filter data base
   //Note: Big limit value smooths data better,but you have to trade off with reactive speed.
   uint16_t  relative_uint16limit=128;//256 //1.0*1000/3.9=256; relative difference limit between two fdb_faccXYZ[] data

   //----- for L3G4200D -----
   int16_t angRXYZ[3];//angular rate of XYZ
   double fangRXYZ[3]; //float angular rate of X,Y,Z aixs. --- fangRXYZ[] = sf * angRXYZ[]
   //double fs_dpus;global ar.
   struct int16MAFilterDB fdb_RXYZ[3]; //filter data base

   //------ PID time difference -----
   uint32_t dt_us; //delta time in us //U16: 0-65535 
   uint32_t sum_dt=0;//sum of dt //U32: 0-4294967295 ~4.3*10^9
   //in head file: double g_fangXYZ[3]={0};// angle value of X Y Z  ---- g_fangXYZ[]= integ{ dt_us*fangRXYZ[] }


   //----- timer -----
   struct timeval tm_start,tm_end;
   struct timeval tmTestStart,tmTestEnd;

   //----- pthread -----
   pthread_t pthrd_WriteOled;
   int pret;

   //------ init. ADXL345 -----
   //Note: adjust ADXL_DATAREADY_WAITUS accordingly in i2c_adxl345_2.h ...
   //Full resolution,OFSX,OFSY,
   //OFSZ also set in Init_ADXL345()
   printf("Init ADXL345 ...\n");
   if(Init_ADXL345(ADXL_DR800_BW400,ADXL_RANGE_4G) != 0 ) 
   {
        ret_val=-1;
        goto INIT_ADXL345_FAIL;
   }
   //----- init. L3G4200D -----
   printf("Init L3G4200D ...\n");
   if( Init_L3G4200D(L3G_DR800_BW35) !=0 )
   {
	printf("Init L3G4200D failed!\n");
	ret_val=-1;
	goto INIT_L3G4200D_FAIL;
   }
   //----- init i2c and oled -----
   printf("Init I2C OLED ...\n");
   init_OLED_128x64();

   //----- init N2M2 Kalman Filter ----
   if(init_N2M2_KalmanFilter() !=0)
   {
        ret_val=-1;
        goto INIT_KALMAN_FAIL;
   }

   //----- create thread for displaying data to OLED
   if(pthread_create(&pthrd_WriteOled,NULL, (void *)thread_gyroWriteOled, NULL) !=0)
   {
		printf("fail to create pthread for OLED displaying\n");
		ret_val=-1;
		goto INIT_PTHREAD_FAIL;
   }


   //---- init filter data base for ADXL345 -----
   printf("Init int16MA filter data base for ADXL345 ...\n");
   if( Init_int16MAFilterDB_NG(3, fdb_accXYZ, 4, relative_uint16limit)<0) //2^4=16 points average filter
   {
        ret_val=-2;
        goto INIT_MAFILTER_FAIL;
   }
   //---- init filter data base for L3G4200D -----
   printf("Init int16MA filter data base for L3G4200D...\n");
   if( Init_int16MAFilterDB_NG(3, fdb_RXYZ, 6, 0x7fff)<0) //2^6=64 points average filter
   {
	ret_val=-2;
	goto INIT_MAFILTER_FAIL;
   }



   //---- preare TCP data server
#ifdef TCP_TRANSFER
   printf("Prepare TCP data server ...\n");
   if(prepare_data_server() < 0)
   {
	printf(" fail to prepare data server!\n");
	ret_val=-3;
	goto CALL_FAIL;
   }
#endif

   //----- wait to accept data client ----
#ifdef TCP_TRANSFER
   printf("wait for Matlab to connect...\n");
   cltsock_desc=accept_data_client();
   if(cltsock_desc < 0){
	printf(" Fail to accept data client!\n");
	ret_val=-4;
	goto CALL_FAIL;
   }
#endif

   //================   LOOP: read data from sensors and proceed  ===============
   printf(" starting testing ...\n");
   gettimeofday(&tmTestStart,NULL);

   k=0;
 while (1) {
	   k++;
//   for(k=0;k<100000;k++) {

	    printf("Reading ADXL345 and L3G4200 ...\n");
	   //----- read acceleration value of XYZ
	   adxl_read_int16AXYZ(accXYZ); // OFSX,OFSY,OFSZ preset in Init_ADXL345()
	   //------- read angular rate of XYZ
	   gyro_read_int16RXYZ(angRXYZ);
//	   printf("Raw data: angRX=%d angRY=%d angRZ=%d \n",angRXYZ[0],angRXYZ[1],angRXYZ[2]);

	   printf("Deducing bias ...\n");
	   //------  ADXL345: set OFSX,OFSY,OFSZ in Init_ADXL345()
	   //------  L3G4200D: deduce bias to adjust zero level
	   for(i=0; i<3; i++)
		   angRXYZ[i]=angRXYZ[i]-g_bias_int16RXYZ[i];
//	   printf("Zero_leveled data: angRX=%d angRY=%d angRZ=%d \n",angRXYZ[0],angRXYZ[1],angRXYZ[2]);

	   printf("Passing through Moving Average filters...\n");
           //----- Passing through filter for acceleration accXYZ ----
           //( single and double spiking value will be trimmed. )
           int16_MAfilter_NG(3,fdb_accXYZ, accXYZ, accXYZ, 0); //int16, dest. and source is the same,three filter groups, only [0] data filt
	   //----- Passing through filter for angualr rate RX RY RZ  -------
	   int16_MAfilter_NG(3,fdb_RXYZ, angRXYZ, angRXYZ, 0); //int16, dest. and source is the same,three group fitler 

	   printf("Calculating fangX...\n");
	   //------ calculate fangX -------
	   if(accXYZ[2] == 0) accXYZ[2]=1;
	   fangX=atan( (accXYZ[0]*1.0)/(accXYZ[2]*1.0) ); //atan(x/z)
	   printf("fangX=%f \n",fangX*180.0/PI);

	   //----- accXYZ,angRXYZ convert to real value -------
	   for(i=0; i<3; i++)
	   {
           	faccXYZ[i]=fsmg_full/1000.0*accXYZ[i];
		fangRXYZ[i]=fs_dpus*angRXYZ[i];  //rad/us
 	   }
/*
           printf("\rK=%d, accX: %+f,  accY: %+f,  accZ:%+f \n", k, faccXYZ[0], faccXYZ[1], faccXYZ[2]);
	   printf("k=%d, fangX: %+f,  fangY: %+f,  fangZ:%+f \e[1A", k, g_fangXYZ[0], g_fangXYZ[1], g_fangXYZ[2]);
	   fflush(stdout);
*/

	   //<<<<<<<<<<<<      time integration of angluar rate RXYZ     >>>>>>>>>>>>>
	   sum_dt=math_tmIntegral_NG(3,fangRXYZ, g_fangXYZ, &dt_us); // one instance only!!!
	   printf("dt_us = %dus \n", dt_us);

	   //----- PASS dt_us to pMat_H
	   *(pMat_F->pmat+1)=dt_us;

           //----- KALMAN FILTER: get float type sensor readings(measurement) ----
	   // vector (angle,angular rate, 0)
	   *pMat_S->pmat = fangX;
	   *(pMat_S->pmat+1) = fangRXYZ[0];

           //----- KALMAN FILTER: Passing through Kalman filter -----
           float_KalmanFilter( fdb_kalman, pMat_S );   //[mx1] input observation matrix
	   printf("pMat_Y:  %f,   %f \n", *pMat_Y->pmat, *(pMat_Y->pmat+1));




#ifdef TCP_TRANSFER
	   if(send_count==0)
	   {
//		if( send_client_data((uint8_t *)&fangX, sizeof(float)) < 0)
		if( send_client_data((uint8_t *)(pMat_S->pmat), 2*sizeof(float)) < 0)
			printf("-------- fail to send client data ------\n");
		send_count=5; //send everty 10th data to the client
	   }
	   else
		send_count-=1;
#endif

   }  //---------------------------  end of loop  -----------------------------


   gettimeofday(&tmTestEnd,NULL);
   printf("\n k=%d\n",k);
   printf("Time elapsed: %fs \n",get_costtimeus(tmTestStart,tmTestEnd)/1000000.0);
   printf("Sum of dt: %fs \n", sum_dt/1000000.0);

   printf("-----  integral value of fangX=%f  fangY=%f  fangZ=%f  ------\n", g_fangXYZ[0], g_fangXYZ[1], g_fangXYZ[2]);

   //----- wait OLED display thread
   gtok_QuitGyro=true;
   pthread_join(pthrd_WriteOled,NULL);


CALL_FAIL:

INIT_KALMAN_FAIL:
   //----- release Kalman data base and filter -----
   release_N2M2_KalmanFilter();

INIT_MAFILTER_FAIL:
   //---- release filter data base
   Release_int16MAFilterDB_NG(3,fdb_accXYZ);
   Release_int16MAFilterDB_NG(3,fdb_RXYZ);

INIT_PTHREAD_FAIL:
   //----- close I2C and Oled
   close_OLED_128x64();

INIT_L3G4200D_FAIL:
   //----- close L3G4200D
    Close_L3G4200D();

INIT_ADXL345_FAIL:
   Close_ADXL345();

   //----- close TCP server
   close_data_service();

   return ret_val;
}


