/*----------------------------------------------------
TEST L3G4200d



----------------------------------------------------*/
#include <stdio.h>
#include <sys/time.h>
#include "gyro_spi.h"
#include "gyro_l3g4200d.h"
#include "data_server.h"
#include "filters.h"

#define DATA_PACKET_SIZE 16 // for fbuff[], buff size, for each TCP send
#define SEND_PACKET_NUM 1000 //number of total packets for TCP transfer
#define RECORD_DATA_SIZE 4096 //

/*----------------------------------------------
   calculate and return time diff. in us
----------------------------------------------*/
int get_costtimeus(struct timeval tm_start, struct timeval tm_end) {
        int time_cost;
	if(tm_end.tv_sec>tm_start.tv_sec)
	        time_cost=(tm_end.tv_sec-tm_start.tv_sec)*1000000+(tm_end.tv_usec-tm_start.tv_usec);
	else // tm_end.tv_sec==tm_start.tv_sec
	        time_cost=tm_end.tv_usec-tm_start.tv_usec;

        return time_cost;
}



int main(void)
{

   uint8_t val;
   int k;
   int16_t bias_RXYZ[3];//bias value of RX RY RZ
   int16_t angRXYZ[3];
   int16_t angRX,angRY,angRZ; //int angular rate of X,Y,Z aixs.
   float fangRX,fangRY,fangRZ; //float angular rate of X,Y,Z aixs.
   float sensf=70/1000000.0; //sensitivity factor for FS=2000 dps.
//   int16_t RXYZbuff[RECORD_DATA_SIZE*3]; //for INT16 raw data from G3L4200D
   float fRXYZbuff[RECORD_DATA_SIZE*3];//for TCP transfer

   struct MA16_int16_FilterCtx fctx_RX,fctx_RY,fctx_RZ; // filter contexts for RX RY RZ

   struct timeval tm_start,tm_end;
   printf("Open SPI ...\n");
   SPI_Open(); //SP clock set to 10MHz OK, if set to 5MHz, then read value of WHO_AM_I is NOT correct !!!!???????
   printf("Init L3G4200D ...\n");
   Init_L3G4200D();

   //---- init filter context
   reset_MA16_filterCtx(&fctx_RX,0x7fff);
   reset_MA16_filterCtx(&fctx_RY,0x7fff);
   reset_MA16_filterCtx(&fctx_RZ,0x7fff);

   //---- preare TCP data server
   printf("Prepare TCP data server ...\n");
   if(prepare_data_server() < 0)
	printf(" fail to preapre data server!\n");


   val=halSpiReadReg(L3G_WHO_AM_I);
   printf("L3G_WHO_AM_I: 0x%02x\n",val);
   val=halSpiReadReg(L3G_FIFO_CTRL_REG);
   printf("FIFO_CTRL_REG: 0x%02x\n",val);
   val=halSpiReadReg(L3G_OUT_TEMP);
   printf("OUT_TEMP: %d\n",val);

   //----- to get bias value for RXYZ
   printf("---  Read and calculate the bias value now, keep the L3G4200D even and static !!!  ---\n");
   sleep(1);
   gyro_get_int16BiasXYZ(bias_RXYZ);
   printf("bias_RX: %f,  bias_RY: %f, bias_RZ: %f \n",sensf*bias_RXYZ[0],sensf*bias_RXYZ[1],sensf*bias_RXYZ[2]);

   //----- wait to accept data client ----
   printf("wait for Matlab to connect...\n");
   cltsock_desc=accept_data_client();
   if(cltsock_desc < 0){
	printf(" Fail to accept data client!\n");
	return -1;
   }

   //----- loop: get data and record -----
   for(k=0;k<RECORD_DATA_SIZE;k++) {


	   gettimeofday(&tm_start,NULL);

	   //------- read angular rate of XYZ
	   gyro_get_int16RXYZ(angRXYZ);
	   //------  deduce bias to adjust zero level
	   angRX=angRXYZ[0]-bias_RXYZ[0];
           angRY=angRXYZ[1]-bias_RXYZ[1];
	   angRZ=angRXYZ[2]-bias_RXYZ[2];

	   //---- Activate filter for  RX RY RZ
	   // first reset each filter context struct, then you must use the same data stream until end.
	   int16_MA16P_filter(&fctx_RX, &angRX, &angRX, 0); //No.0 fitler
	   int16_MA16P_filter(&fctx_RY, &angRY, &angRY, 0); //No.1 fitler
	   int16_MA16P_filter(&fctx_RZ, &angRZ, &angRZ, 0); //No.2 fitler

	   //----- convert to real value
	   fangRX=sensf*angRX;
	   fangRY=sensf*angRY;
	   fangRZ=sensf*angRZ;

	   //----- put data to buffer ----
	   fRXYZbuff[3*k]=fangRX;
	   fRXYZbuff[3*k+1]=fangRY;
	   fRXYZbuff[3*k+2]=fangRZ;

	   //---- filter  RX  data
//	   int16_MA16P_filter(fRXYZbuff, fRXYZbuff, 3k);

	   printf("angRX=%d angRY=%d angRZ=%d \n",angRX,angRY,angRZ);
	   printf("fangRX=%f  fangRY=%f  fangRZ=%f \n",fangRX,fangRY,fangRZ);

	   gettimeofday(&tm_end,NULL);
	   printf("time_span:%dus\n",get_costtimeus(tm_start,tm_end));
//	   usleep(200000);
   }

   //---- send data to client Matlab
   if( send_client_data((uint8_t *)fRXYZbuff,RECORD_DATA_SIZE*3*sizeof(float)) < 0)
	printf("-------- fail to send client data ------\n");



   SPI_Close();
   //----- close TCP server
   close_data_service();

   return 0;
}
