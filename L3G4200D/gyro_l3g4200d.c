/*----------------------------------------------------
TEST L3G4200d



----------------------------------------------------*/
#include <stdio.h>
#include <sys/time.h>
#include "gyro_spi.h"
#include "gyro_l3g4200d.h"
#include "data_server.h"

#define DATA_PACKET_SIZE 16 // for fbuff[], buff size, for each TCP send
#define SEND_PACKET_NUM 1000 //number of total packets for TCP transfer
#define RECORD_DATA_SIZE 3000 //

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
   uint8_t outbuff[6]; //Lxout[0] Hxout[1]
   int16_t angRX,angRY,angRZ; //int angular rate of X,Y,Z aixs.
   float fangRX,fangRY,fangRZ; //float angular rate of X,Y,Z aixs.
   float sensf=70/1000000.0; //sensitivity factor for FS=2000 dps.
   float fRXYZbuff[RECORD_DATA_SIZE*3];//for TCP transfer


   struct timeval tm_start,tm_end;
   printf("Open SPI ...\n");
   SPI_Open(); //SP clock set to 10MHz OK, if set to 5MHz, then read value of WHO_AM_I is NOT correct !!!!???????
   printf("Init L3G4200D ...\n");
   Init_L3G4200D();

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
	   //----- wait for new data
	   while(!status_XYZ_available()){
		printf(" XYZ new data is not availbale now!\n");
		usleep(500);
	   }

	   halSpiReadBurstReg(L3G_OUT_X_L, outbuff, 6);
//	   printf("XL:0x%02x,XH:0x%02x   YL:0x%02x,YH:0x%02x  ZL:0x%02x,ZH:0x%02x  \n",outbuff[0],outbuff[1],outbuff[2],outbuff[3],outbuff[4],outbuff[5]);
//	   outbuff[0]=halSpiReadReg(L3G_OUT_X_L);
//	   outbuff[1]=halSpiReadReg(L3G_OUT_X_H);
//	   printf("XL:0x%02x, XH:0x%02x \n",outbuff[0],outbuff[1]);
	   angRX=*(int16_t *)outbuff;
	   angRY=*(int16_t *)(outbuff+2);
	   angRZ=*(int16_t *)(outbuff+4);
	   fangRX=sensf*angRX;
	   fangRY=sensf*angRY;
	   fangRZ=sensf*angRZ;

	   //----- put data to buffer ----
	   fRXYZbuff[3*k]=fangRX;
	   fRXYZbuff[3*k+1]=fangRY;
	   fRXYZbuff[3*k+2]=fangRZ;

	   printf("angRX=%d  angRY=%d  angRZ=%d \n",angRX,angRY,angRZ);
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
