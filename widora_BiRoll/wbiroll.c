/*----------------------------------------------------------------------------------------
Note:
1. set GYRO L3G4200 ODR=800Hz BW=35Hz  (SPI interface)
2. set ACC. ADXL345 ODR=800Hz BW=400Hz (I2C interface)
3. set angle and angular rate with the same sign(positive or negative)
4. Since period of one circle of data reading(NOT interruptive reading) and processing is not fixed,
   that introduces untraced noise which may be unlinear.
5. Kalman matrix Q and R shall not both be zero !!! that will make computing matrix invertible!
6. You have to trade off between smoothness and agility when deceide
   filtering grade.
7. Sensors' sampling frequency shall be high enough to cover main noise frequency ??
8. When either Q or R is a zero matrix, matrix P will finally evolved to a zero matrix!  Why ???!
    You MUST adjust matrix Q and R deliberately, assign with reasonable value to make Kalman work.
9. Multi_threads for polling_sensor_data works ugly unless you set usleep(5000) in main while() loop,
    however it still produce more burr signals for acceleration reading than non_threads pollinging method.
10. If there are other devices connected to the I2C interface, that will affect acceleration result considerably??.
11. Power supply for motor driver and Widora_NEO must be effectively decoupled, or provided separately.

Midas
-----------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include "gyro_spi.h"
#include "gyro_l3g4200d.h"
#include "i2c_adxl345.h" //--- use  read() write() to operate I2C
#include "data_server.h"
#include "filters.h"
#include "mathwork.h"
#include "kalman_n2m2.h"
#include "pwm_ctl.c" //motor control

//#define TCP_TRANSFER
//#define PTHREAD_READ_SENSORS


#ifdef PTHREAD_READ_SENSORS
/*----- data struct for acceleration -----*/
struct struct_int16accXYZ
{
   int16_t accXYZ[3];
   pthread_mutex_t  accmLock;
}  int16_accXYZ_data;

/*----- data struct for gyro, angular rate -----*/
struct struct_int16angRXYZ
{
  int16_t angRXYZ[3];
  pthread_mutex_t  angmLock;
} int16_angRXYZ_data;


/*---------------- pthread function ------------------
get acceleration rate from ADXL345
---------------------------------------------------*/
void read_int16AccXYZ(void *arg)
{
    int16_t accXYZ[3];
    struct struct_int16accXYZ *accXYZ_data=(struct struct_int16accXYZ *)arg; // acceleration value of XYZ

    //-------- init mutex lock --------
    pthread_mutex_init(&accXYZ_data->accmLock,NULL);
    while(1)
    {
	    if(gtok_QuitGyro==true)
		break;
	    // read sensor
	    adxl_read_int16AXYZ(accXYZ); // OFSX,OFSY,OFSZ preset in Init_ADXL345()
	    // lock and copy data
	    printf(" mutex lock in read_int16AccXYZ...\n");
	    pthread_mutex_lock(&accXYZ_data->accmLock);
	    memcpy(accXYZ_data->accXYZ,accXYZ,3*sizeof(int16_t)); 
	    pthread_mutex_unlock(&accXYZ_data->accmLock);
    }
}

/*---------------- pthread function ------------------
get XYZ angluar rage from L3G4200D
----------------------------------------------------*/
void read_int16angRXYZ(void *arg)
{
    int16_t angRXYZ[3];
    struct struct_int16angRXYZ *angRXYZ_data=(struct struct_int16angRXYZ *)arg; // acceleration value of XYZ

    //-------- init mutex lock --------
    pthread_mutex_init(&angRXYZ_data->angmLock,NULL);
    while(1)
    {
	    if(gtok_QuitGyro==true)
		break;
	    // read sensor
	    gyro_read_int16RXYZ(angRXYZ); // OFSX,OFSY,OFSZ preset in Init_ADXL345()
	    // lock and copy data
	    printf(" mutex lock in read_int16angRXYZ...\n");
	    pthread_mutex_lock(&angRXYZ_data->angmLock);
	    memcpy(angRXYZ_data->angRXYZ,angRXYZ,3*sizeof(int16_t)); // OFSX,OFSY,OFSZ preset in Init_ADXL345()
	    pthread_mutex_unlock(&angRXYZ_data->angmLock);
    }
}

#endif //PTHREAD_READ_SENSORS end


/*=========================   MAIN  ===============================
    WiBi Roll Main Function
-----------------------------------------------------------------*/
int main(void)
{
   int ret_val;
//   uint8_t val;
   int i,k;
   int send_count=0;

   //------ for ADXL345  -----
   int16_t accXYZ[3]={0}; // acceleration value of XYZ
   double  faccXYZ[3]; //double //faccXYZ=fsmg_full*accXYZ
   float fangX; //atan(X/Z) angle of X axis, to be filtered.
   struct int16MAFilterDB fdb_accXYZ[3]; // filter data base
   struct floatMAFilterDB fdb_fangX; //float type filter data base for ANGLE X
   //Note: Big limit value smooths data better,but you have to trade off with reactive speed.
   uint16_t  relative_uint16limit=128;//256 //1.0*1000/3.9=256; relative difference limit between two fdb_faccXYZ[] data
   float     relative_floatAXlimit=0.3;//MAX. incremental of angle_X chang in ~5ms (one circle)

   //----- for L3G4200D -----
   int16_t angRXYZ[3];//angular rate of XYZ
   double fangRXYZ[3]; //float angular rate of X,Y,Z aixs. --- fangRXYZ[] = sf * angRXYZ[]
   //double fs_dpus;global ar.
   struct int16MAFilterDB fdb_RXYZ[3]; //filter data base

   //------ PID time difference -----
   int pwmval; //pwm value and direction for Motor control
   float pwmdir;
   uint32_t dt_us; //delta time in us //U16: 0-65535 
   uint32_t sum_dt=0;//sum of dt //U32: 0-4294967295 ~4.3*10^9
   //in head file: double g_fangXYZ[3]={0};// angle value of X Y Z  ---- g_fangXYZ[]= integ{ dt_us*fangRXYZ[] }

   //----- TCP buffer -----
   float tcp_buff[2];

   //----- timer -----
   struct timeval tm_start,tm_end;
   struct timeval tmTestStart,tmTestEnd;

   //----- pthread -----
   pthread_t pthrd_WriteOled;
   pthread_t pthrd_TCPsendData;
   pthread_t pthrd_ReadADXL345;
   pthread_t pthrd_ReadL3G4200D;
   int pret;
   int mret;
   //------ init. ADXL345 -----
   //Note: adjust ADXL_DATAREADY_WAITUS accordingly in i2c_adxl345_2.h ...
   //Full resolution,OFSX,OFSY,
   //OFSZ also set in Init_ADXL345()
   printf("Init ADXL345 ...\n");
   if(Init_ADXL345(ADXL_DR800_BW400,ADXL_RANGE_4G) != 0 ) //OFX,OFY,OFZ register set in Init_ADXL345()
   {
        ret_val=-1;
        goto INIT_ADXL345_FAIL;
   }
   //----- init. L3G4200D -----
   printf("Init L3G4200D ...\n");
   if( Init_L3G4200D(L3G_DR800_BW35) !=0 )  //g_bias_int16RXYZ[] get in Init_L3G4200D()
   {
	printf("Init L3G4200D failed!\n");
	ret_val=-1;
	goto INIT_L3G4200D_FAIL;
   }
   //----- init i2c and oled -----
/*
   printf("Init I2C OLED ...\n");
   init_OLED_128x64();
*/
   //----- init N2M2 Kalman Filter ----
   if(init_N2M2_KalmanFilter() !=0)
   {
        ret_val=-1;
        goto INIT_KALMAN_FAIL;
   }

   //----- create thread for displaying data to OLED
/*
   if(pthread_create(&pthrd_WriteOled,NULL, (void *)thread_gyroWriteOled, NULL) !=0)

   {
	printf("fail to create pthread for OLED displaying\n");
	ret_val=-1;
	goto INIT_PTHREAD_FAIL;
   }
*/

   //---- init filter data base for ADXL345 -----
   printf("Init int16MA filter data base for ADXL345 ...\n");
   if( Init_int16MAFilterDB_NG(3, fdb_accXYZ, 1, relative_uint16limit)<0) //2^4=16 points average filter
   {
        ret_val=-2;
        goto INIT_MAFILTER_FAIL;
   }
   //---- init filter data base for L3G4200D -----
   printf("Init int16MA filter data base for L3G4200D...\n");
   if( Init_int16MAFilterDB_NG(3, fdb_RXYZ, 1, 0x7fff)<0) // 2^6=64 points average filter
   {
	ret_val=-2;
	goto INIT_MAFILTER_FAIL;
   }
   //----- init filter data base for fangX ----
   printf("Init float type MA filter data base for fangX...\n");
   if( Init_floatMAFilterDB( &fdb_fangX, 1, relative_floatAXlimit) <0 ) //  2^6=64 points MAF,0.2 rad/5ms !!! MAX. incremental of angle chang in ~5ms (one data gap)
   {
	ret_val=-2;
	goto INIT_MAFILTER_FAIL;
   }


   //----- prepare Motor control system  ----
   printf("Init Motro control system ...\n");
   if( init_Motor_Control() != 0)
   {
	ret_val=-2;
	goto INIT_MOTOR_CONTROL_FAIL;
   }

//-------------- Prepare TCP Data Server ----------------
#ifdef TCP_TRANSFER
   printf("Prepare TCP data server ...\n");
   if(prepare_data_server() < 0)
   {
	printf(" fail to prepare data server!\n");
	ret_val=-3;
	goto CALL_FAIL;
   }

   //----- wait to accept data client ----
   printf("wait for Matlab to connect...\n");
   cltsock_desc=accept_data_client();
   if(cltsock_desc < 0){
	printf(" Fail to accept data client!\n");
	ret_val=-4;
	goto CALL_FAIL;
   }

   //----- create thread for TCP data sending
   if(pthread_create(&pthrd_TCPsendData,NULL,(void *)loop_send_data, (void *)fdb_kalman) !=0)
   {
	printf("fail to create pthread for TCP data transfering\n");
	ret_val=-1;
	goto CALL_FAIL;
   }
#endif  //------------ end TCP_TRANSFER PREPARATION -----------

   //================   LOOP: read data from sensors and proceed  ===============
   printf(" starting testing ...\n");
   gettimeofday(&tmTestStart,NULL);

#ifdef PTHREAD_READ_SENSORS
   //-------- init mutex lock in pthread functioin--------

   //-------- start Pthread of ADXL345 Data Reading ---------
   printf("start pthread_create() ReadADXL345...\n");
   //-------- start Pthread of L3G4200D Data Reading ---------
   if(pthread_create( &pthrd_ReadADXL345, NULL, (void *)read_int16AccXYZ, (void *)(&int16_accXYZ_data)) !=0 )
   {
	printf("fail to create pthread for ADXL345 data reading!\n");
	ret_val=-1;
	goto CALL_FAIL;
   }
   printf("start pthread_create() ReadL3G4200D...\n");
   //-------- start Pthread of L3G4200D Data Reading ---------
   if(pthread_create( &pthrd_ReadL3G4200D, NULL, (void *)read_int16angRXYZ, (void *)(&int16_angRXYZ_data)) !=0 )
   {
	printf("fail to create pthread for L3G4200D data reading!\n");
	ret_val=-1;
	goto CALL_FAIL;
   }
   printf(" finishing creating pthread reading sensors....\n");
#endif  //PTHREAD_READ_SENSORS ends


   k=0;
while (1) {

#ifdef PTHREAD_READ_SENSORS
   	   usleep(5000); //-------  !!! IMPORTANT !!! -------

	   //-------- memcpy pthread accXYZ data  ----------
	   printf(" mret = pthread_mutex_lock()..\n");
	   mret=pthread_mutex_lock(&int16_accXYZ_data.accmLock);
	   if(mret != 0) printf(" accXYZ mutex lock fail!\n");
	   printf(" memcpy ..\n");
	   memcpy(accXYZ,int16_accXYZ_data.accXYZ,3*sizeof(int16_t));
	   pthread_mutex_unlock(&int16_accXYZ_data.accmLock);
	   //-------- memcpy pthread angRXYZ data  ----------
	   mret=pthread_mutex_lock(&int16_angRXYZ_data.angmLock);
	   if(mret != 0) printf(" angRXYZ mutex lock fail!\n");
	   memcpy(angRXYZ,int16_angRXYZ_data.angRXYZ,3*sizeof(int16_t));
	   pthread_mutex_unlock(&int16_angRXYZ_data.angmLock);

#else  // ------ <<<<<<  No pthread applied  >>>>>> -------- Prefered!!!
	   //printf("start adxl reading..\n");
	   //----- read acceleration value of XYZ
	   adxl_read_int16AXYZ(accXYZ); // OFSX,OFSY,OFSZ preset in Init_ADXL345()
	   //------- read angular rate of XYZ
	   //printf(" start gyro reading...\n");
	   gyro_read_int16RXYZ(angRXYZ);
	   //printf("Raw data: angRX=%d angRY=%d angRZ=%d \n",angRXYZ[0],angRXYZ[1],angRXYZ[2]);

#endif  //PTHREAD_READ_SENSORS end

	   //=================(( apply BIAs  ))=================
	   //------  ADXL345: bias ajusted already with register OFSX,OFSY,OFSZ set in Init_ADXL345(), 
	   //------  L3G4200D: deduce bias to adjust zero level, g_bias_int16RXYZ[] get in Init_L3G4200D()
	   for(i=0; i<3; i++)
		   angRXYZ[i]=angRXYZ[i]-g_bias_int16RXYZ[i];
	   //printf("Zero_leveled data: angRX=%d angRY=%d angRZ=%d \n",angRXYZ[0],angRXYZ[1],angRXYZ[2]);

           //----- Passing through filter for acceleration accXYZ ----
           //( single and double spiking value will be trimmed. )
           int16_MAfilter_NG(3,fdb_accXYZ, accXYZ, accXYZ, 0); //int16, dest. and source is the same,three filter groups, only [0] data filt
	   //----- Passing through filter for angualr rate RX RY RZ  -------
	   int16_MAfilter_NG(3,fdb_RXYZ, angRXYZ, angRXYZ, 0); //int16, dest. and source is the same,three group fitler 
	   //------ calculate fangX -------
	   if(accXYZ[2] == 0)  // !!!!! for initial zero data in filter buffer !!!!!!
	   {
		printf("accXYZ[2] == %d !\n",accXYZ[2]);
	   	accXYZ[2]=1;  // !!!!!--- TO AVOID ZERO CASE,IMPORTANT. or it will corrupt Kalman --- !!!!
	   }
	   fangX=-1.0*atan( (accXYZ[0]*1.0)/(accXYZ[2]*1.0) ); //atan(x/z)
//	   printf("fangX=%f \n",fangX*180.0/PI);
	   //----- Passing through filter for fangX ------
	   float_MAfilter(&fdb_fangX,&fangX,&fangX,0);

	   //----- accXYZ,angRXYZ convert to real value -------
	   for(i=0; i<3; i++)
	   {
           	faccXYZ[i]=fsmg_full/1000.0*accXYZ[i]; // final unit: (g)
		fangRXYZ[i]=fs_dpus*angRXYZ[i]; // final unit: (rad/us)
 	   }
//         printf("\rK=%d, accX: %+f,  accY: %+f,  accZ:%+f \n", k, faccXYZ[0], faccXYZ[1], faccXYZ[2]);
//	   printf("k=%d, fangX: %+f,  fangY: %+f,  fangZ:%+f \e[1A", k, g_fangXYZ[0], g_fangXYZ[1], g_fangXYZ[2]);
//	   fflush(stdout);

	   //<<<<<<<<<<<<      time integration of angluar rate RXYZ     >>>>>>>>>>>>>
	   sum_dt=math_tmIntegral_NG(3,fangRXYZ, g_fangXYZ, &dt_us); // one instance only!!!
	   printf("dt_us = %dus \n", dt_us);

	   //----- PASS dt_us to pMat_H ------
	   *(pMat_F->pmat+1)=dt_us; // !!! -- This will introduce unlinear factors -- !!!

           //----- KALMAN FILTER: get float type sensor readings(measurement) ----
	   //----- Avoid ZERO! -----
//not necessry???	   if(fabs(fangX) < 1.0e-10 )fangX=1.0e-10; if( fabs(fangRXYZ[0]) < 1.0e-10)fangRXYZ[0]=1.0e-10;
	   // assign observation vector (angle,angular rate)
	   *pMat_S->pmat=fangX; //angle
	   *(pMat_S->pmat+1) = fangRXYZ[1]; //angular rate

           //----- KALMAN FILTER: Passing through Kalman filter -----
	   pthread_mutex_lock(&fdb_kalman->kmlock);
           float_KalmanFilter( fdb_kalman, pMat_S );   //[mx1] input observation matrix
	   pthread_mutex_unlock(&fdb_kalman->kmlock);
//	   printf("pMat_Y:  %e,   %e \n", *pMat_Y->pmat, *(pMat_Y->pmat+1));
//	   printf("pMat_P:  %e,   %e \n", *fdb_kalman->pMP->pmat, *(fdb_kalman->pMP->pmat+3));
//	   printf("pMat_Pp:  %e,  %e \n", *fdb_kalman->pMPp->pmat, *(fdb_kalman->pMPp->pmat+3));
//	   printf("pMat_Q:  %e,   %e \n", *pMat_Q->pmat, *(pMat_Q->pmat+2));

	   //------ control motor to counter inclination -------
	   //  void set_Motor_Speed(int pwmval,  float dirval)
	   // 1e-2 angle, 1e-7 angular rate
	   // pwmval=Kap*Angle+Kad*Ang_rate  (PID -- use PD only)
	   pwmval=4000*fabs(*pMat_Y->pmat)+1.0e8*fabs(*(pMat_Y->pmat+1));
	   pwmdir=*pMat_Y->pmat;
	   printf(" pwmval=%d pwmdir=%f\n",pwmval, pwmdir);
	   set_Motor_Speed(pwmval, pwmdir);
//	   set_Motor_Speed(50+10*100*fabs(*pMat_Y->pmat)+3e+7*fabs(*(pMat_Y->pmat+1)), *pMat_Y->pmat);
//	   set_Motor_Speed(300+5*100*fabs(*pMat_Y->pmat), *pMat_Y->pmat);
//	   set_Motor_Speed(300+50*1e+7*fabs(*(pMat_Y->pmat+1)), *(pMat_Y->pmat+1));

   }  //---------------------------  end of loop  -----------------------------


   gettimeofday(&tmTestEnd,NULL);
   printf("\n k=%d\n",k);
   printf("Time elapsed: %fs \n",get_costtimeus(tmTestStart,tmTestEnd)/1000000.0);
   printf("Sum of dt: %fs \n", sum_dt/1000000.0);

   printf("-----  integral value of fangX=%f  fangY=%f  fangZ=%f  ------\n", g_fangXYZ[0], g_fangXYZ[1], g_fangXYZ[2]);

   //----- thread quit signal -----
   gtok_QuitGyro=true;
/*
   //----- wait OLED display thread
   pthread_join(pthrd_WriteOled,NULL);
   pthread_join(pthrd_TCPsendData,NULL);
*/
#ifdef PTHREAD_READ_SENSORS
   pthread_join(pthrd_ReadADXL345,NULL);
   pthread_join(pthrd_ReadL3G4200D,NULL);
#endif //PTHREAD_READ_SENSORS end


CALL_FAIL:


#ifdef PTHREAD_READ_SENSORS
   //------ release mutext locker -----
   pthread_mutex_destroy(&int16_accXYZ_data.accmLock);
   pthread_mutex_destroy(&int16_angRXYZ_data.angmLock);
#endif //PTHREAD_READ_SENSORS end

INIT_MOTOR_CONTROL_FAIL:
   release_Motor_Control();

INIT_KALMAN_FAIL:
   //----- release Kalman data base and filter -----
   release_N2M2_KalmanFilter();

INIT_MAFILTER_FAIL:
   //---- release filter data base
   Release_int16MAFilterDB_NG(3,fdb_accXYZ);
   Release_int16MAFilterDB_NG(3,fdb_RXYZ);
   Release_floatMAFilterDB(&fdb_fangX);

INIT_PTHREAD_FAIL:
   //----- close I2C and Oled
/*
   close_OLED_128x64();
*/
INIT_L3G4200D_FAIL:
   //----- close L3G4200D
   Close_L3G4200D();

INIT_ADXL345_FAIL:
   Close_ADXL345();

   //----- close TCP server
   close_data_service();

   return ret_val;
}


