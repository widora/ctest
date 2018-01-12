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
#include "i2c_oled_128x64.h"

//#define MATLAB_TCP
#define RECORD_DATA_SIZE 4096 //


int main(void)
{
   int ret_val;
   uint8_t val;
   int i,k;
   int send_count=0;
   int16_t bias_RXYZ[3];//bias value of RX RY RZ
   int16_t angRXYZ[3];//angular rate of XYZ
   double fangRXYZ[3]; //float angular rate of X,Y,Z aixs. --- fangRXYZ[] = sensf * angRXYZ[]
   double sensf=70/1000000000.0;//dpus //70/1000.0-dps, 70/1000000.0-dpms //sensitivity factor for FS=2000 dps.
   //------ PID -----
   uint32_t dt_us; //delta time in us //U16: 0-65535 
   uint32_t sum_dt=0;//sum of dt //U32: 0-4294967295 ~4.3*10^9
   //in head file: double g_fangXYZ[3]={0};// angle value of X Y Z  ---- g_fangXYZ[]= integ{ dt_us*fangRXYZ[] }

   //----- integral -----
   struct int16MAFilterDB fdb_RXYZ[3];

   struct timeval tm_start,tm_end;
   struct timeval tmTestStart,tmTestEnd;
   //----- pthread -----
   pthread_t pthrd_WriteOled;
   int pret;


   //----- init spi
   printf("Open SPI ...\n");
   SPI_Open(); //SP clock set to 10MHz OK, if set to 5MHz, then read value of WHO_AM_I is NOT correct !!!!???????

   //----- init L3G4200D
   printf("Init L3G4200D ...\n");
   Init_L3G4200D();

   //----- init i2c and oled
   printf("Init I2C OLED ...\n");
   init_I2C_Slave();
   initOledDefault();
   clearOledV();
   push_Oled_Ascii32x18_Buff("-- Widora-NEO --",3,0);
   refresh_Oled_Ascii32x18_Buff(false);

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
	goto INIT_MAFILTER_FAIL;

   //---- preare TCP data server
#ifdef MATLAB_TCP
   printf("Prepare TCP data server ...\n");
   if(prepare_data_server() < 0)
   {
	printf(" fail to prepare data server!\n");
	ret_val=-3;
	goto CALL_FAIL;
   }
#endif

   val=halSpiReadReg(L3G_WHO_AM_I);
   printf("L3G_WHO_AM_I: 0x%02x\n",val);
   val=halSpiReadReg(L3G_OUT_TEMP);
   printf("OUT_TEMP: %d\n",val);

   //----- to get bias value for RXYZ
   printf("---  Read and calculate the bias value now, keep the L3G4200D even and static !!!  ---\n");
   sleep(1);
   gyro_get_int16BiasXYZ(bias_RXYZ);
   printf("bias_RX: %f,  bias_RY: %f, bias_RZ: %f \n",sensf*bias_RXYZ[0],sensf*bias_RXYZ[1],sensf*bias_RXYZ[2]);

   //----- wait to accept data client ----
#ifdef MATLAB_TCP
   printf("wait for Matlab to connect...\n");
   cltsock_desc=accept_data_client();
   if(cltsock_desc < 0){
	printf(" Fail to accept data client!\n");
	goto CALL_FAIL;
   }
#endif

   //================   loop: get data and record  ===============
   printf(" starting testing ...\n");
   gettimeofday(&tmTestStart,NULL);

   k=0;
//   while (1) {
//	   k++;
   for(k=0;k<100000;k++) {

	   //------- read angular rate of XYZ
	   gyro_read_int16RXYZ(angRXYZ);
//	   printf("Raw data: angRX=%d angRY=%d angRZ=%d \n",angRXYZ[0],angRXYZ[1],angRXYZ[2]);

	   //------  deduce bias to adjust zero level
	   for(i=0; i<3; i++)
		   angRXYZ[i]=angRXYZ[i]-bias_RXYZ[i];
//	   printf("Zero_leveled data: angRX=%d angRY=%d angRZ=%d \n",angRXYZ[0],angRXYZ[1],angRXYZ[2]);

	   //---- Activate filter for  RX RY RZ
	   // first reset each filter context struct, then you must use the same data stream until end.
	   int16_MAfilter_NG(3,fdb_RXYZ, angRXYZ, angRXYZ, 0); // three group fitler 

	   //----- convert to real value
	   for(i=0; i<3; i++)
		   fangRXYZ[i]=sensf*angRXYZ[i];


	   //<<<<<<<<<<<<      time integration of angluar rate RXYZ     >>>>>>>>>>>>>
	    sum_dt=math_tmIntegral_NG(3,fangRXYZ, g_fangXYZ); // one instance only!!!

	   //<<<<<<<<<< Every 500th count:  send integral XYZ angle to client Matlab    >>>>>>>>>>>
#ifdef MATLAB_TCP
	   if(send_count==0)
	   {
//		if( send_client_data((uint8_t *)g_fangXYZ,3*sizeof(double)) < 0)
		if( send_client_data((uint8_t *)fangRXYZ,3*sizeof(double)) < 0)
			printf("-------- fail to send client data ------\n");
		send_count=10;
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
    free_I2C_IOdata();
    intFcntlOp(g_fdOled,F_SETLK,F_UNLCK,0,SEEK_SET,10);
   //---- close spi
   SPI_Close();
   //----- close TCP server
   close_data_service();

   return ret_val;
}


