/*----------------------------------------------------
TEST L3G4200d

set ODR=800Hz

----------------------------------------------------*/
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include "gyro_spi.h"
#include "gyro_l3g4200d.h"
#include "data_server.h"
#include "filters.h"
#include "mathwork.h"

//#define TCP_TRANSFER


int main(void)
{
   int ret_val;
//   uint8_t val;
   int i,k;
   int send_count=0;
   int16_t angRXYZ[3];//angular rate of XYZ
   double fangRXYZ[3]; //float angular rate of X,Y,Z aixs. --- fangRXYZ[] = sensf * angRXYZ[]
   //double fs_dpus;global ar.

   //------ PID -----
   uint32_t dt_us; //delta time in us //U16: 0-65535 
   uint32_t sum_dt=0;//sum of dt //U32: 0-4294967295 ~4.3*10^9
   //in head file: double g_fangXYZ[3]={0};// angle value of X Y Z  ---- g_fangXYZ[]= integ{ dt_us*fangRXYZ[] }

   //----- filter db -----
   struct int16MAFilterDB fdb_RXYZ[3];

   //----- timer -----
   struct timeval tm_start,tm_end;
   struct timeval tmTestStart,tmTestEnd;

   //----- pthread -----
   pthread_t pthrd_WriteOled;
   int pret;

   //----- init L3G4200D
   printf("Init L3G4200D ...\n");
   if( Init_L3G4200D(L3G_DR800_BW35) !=0 )
   {
	printf("Init L3G4200D failed!\n");
	ret_val=-1;
	goto INIT_L3G4200D_FAIL;
   }

   //----- init i2c and oled
   printf("Init I2C OLED ...\n");
   init_OLED_128x64();

   //----- create thread for displaying data to OLED
   if(pthread_create(&pthrd_WriteOled,NULL, (void *)thread_gyroWriteOled, NULL) !=0)
   {
		printf("fail to create pthread for OLED displaying\n");
		ret_val=-1;
		goto INIT_PTHREAD_FAIL;
   }
   //---- init filter data base
   printf("Init int16MA filter data base ...\n");
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

   //================   loop: get data and record  ===============
   printf(" starting testing ...\n");
   gettimeofday(&tmTestStart,NULL);

   k=0;
   while (1) {
	   k++;
//   for(k=0;k<100000;k++) {

	   //------- read angular rate of XYZ
	   gyro_read_int16RXYZ(angRXYZ);
//	   printf("Raw data: angRX=%d angRY=%d angRZ=%d \n",angRXYZ[0],angRXYZ[1],angRXYZ[2]);

	   //------  deduce bias to adjust zero level
	   for(i=0; i<3; i++)
		   angRXYZ[i]=angRXYZ[i]-g_bias_int16RXYZ[i];
//	   printf("Zero_leveled data: angRX=%d angRY=%d angRZ=%d \n",angRXYZ[0],angRXYZ[1],angRXYZ[2]);

	   //---- Activate filter for  RX RY RZ
	   // first reset each filter context struct, then you must use the same data stream until end.
	   int16_MAfilter_NG(3,fdb_RXYZ, angRXYZ, angRXYZ, 0); // three group fitler 

	   //----- convert to real value
	   for(i=0; i<3; i++)
		   fangRXYZ[i]=fs_dpus*angRXYZ[i];


	   //<<<<<<<<<<<<      time integration of angluar rate RXYZ     >>>>>>>>>>>>>
	    sum_dt=math_tmIntegral_NG(3,fangRXYZ, g_fangXYZ, &dt_us); // one instance only!!!

	   //<<<<<<<<<< Every 500th count:  send integral XYZ angle to client Matlab    >>>>>>>>>>>
#ifdef TCP_TRANSFER
	   if(send_count==0)
	   {
//		if( send_client_data((uint8_t *)g_fangXYZ,3*sizeof(double)) < 0)
		if( send_client_data((uint8_t *)fangRXYZ,3*sizeof(double)) < 0)
			printf("-------- fail to send client data ------\n");
		send_count=10; //send everty 10th data to the client
	   }
	   else
		send_count-=1;
#endif
	   //------  print out
	   printf(" k=%d   fangX=%f  fangY=%f  fangZ=%f \r", k, g_fangXYZ[0], g_fangXYZ[1], g_fangXYZ[2]);
	   fflush(stdout);

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

INIT_MAFILTER_FAIL:
   //---- release filter data base
   Release_int16MAFilterDB_NG(3,fdb_RXYZ);

INIT_PTHREAD_FAIL:
   //----- close I2C and Oled
   close_OLED_128x64();

INIT_L3G4200D_FAIL:
   //----- close L3G4200D
    Close_L3G4200D();

   //----- close TCP server
   close_data_service();

   return ret_val;
}


