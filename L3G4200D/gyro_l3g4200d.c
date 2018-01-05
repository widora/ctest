/*----------------------------------------------------




----------------------------------------------------*/
#include <stdio.h>
#include <sys/time.h>
#include "gyro_spi.h"
#include "gyro_l3g4200d.h"
#include "data_server.h"

#define DATA_PACKET_SIZE 16 // for fbuff[], buff size, for each TCP send
#define SEND_PACKET_NUM 1000 //number of total packets for TCP transfer

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

   uint8_t addr,val;
   int j,k;
   uint8_t outbuff[6]; //Lxout[0] Hxout[1]
   int16_t angRX,angRY,angRZ; //int angular rate of X,Y,Z aixs.
   float fangRX,fangRY,fangRZ; //float angular rate of X,Y,Z aixs.
   float sensf=70/1000000.0; //sensitivity factor for FS=2000 dps.
   float fRXbuff[DATA_PACKET_SIZE];//for TCP transfer
   float fRYbuff[DATA_PACKET_SIZE];//for TCP transfer
   float fRZbuff[DATA_PACKET_SIZE];//for TCP transfer

   struct timeval tm_start,tm_end;

   SPI_Open(); //SP clock set to 10MHz OK, if set to 5MHz, then read value of WHO_AM_I is NOT correct !!!!???????
   Init_L3G4200D();

   //---- preare TCP data server
   if(prepare_data_server() < 0)
	printf(" fail to preapre data server!\n");


   val=halSpiReadReg(L3G_WHO_AM_I);
   printf("L3G_WHO_AM_I: 0x%02x\n",val);
   val=halSpiReadReg(L3G_FIFO_CTRL_REG);
   printf("FIFO_CTRL_REG: 0x%02x\n",val);
   val=halSpiReadReg(L3G_OUT_TEMP);
   printf("OUT_TEMP: %d\n",val);

   //----- wait to accept data client ----
   cltsock_desc=accept_data_client();
   if(cltsock_desc < 0){
	printf(" Fail to accept data client!\n");
	return -1;
   }

   j=0; // counter of while loop
   k=0; //fbuff[] slot
   while (1) {
	   //----- read STATUS_REG
	   //----- if STATUS_REG(7)=1 then

	   gettimeofday(&tm_start,NULL);
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
	   fRXbuff[k]=fangRX;
	   fRYbuff[k]=fangRY;
	   fRZbuff[k]=fangRZ;
	   k++;
	   //-----TCP send data to client by packets -----
	   if(k==DATA_PACKET_SIZE) {
		   //---- send data to client
		   if( send_client_data(fRXbuff,DATA_PACKET_SIZE*sizeof(float)) < 0)
			printf("-------- fail to send client data ------\n");
		   if( send_client_data(fRYbuff,DATA_PACKET_SIZE*sizeof(float)) < 0)
			printf("-------- fail to send client data ------\n");
		   if( send_client_data(fRZbuff,DATA_PACKET_SIZE*sizeof(float)) < 0)
			printf("-------- fail to send client data ------\n");
		   //--- reset k
		   k=0;
		   //--- renew j
		   j++;
		   if(j==SEND_PACKET_NUM-1) break;
	   }

//	   printf("angRX=%d  angRY=%d  angRZ=%d \n",angRX,angRY,angRZ);
	   printf("fangRX=%f  fangRY=%f  fangRZ=%f \n",fangRX,fangRY,fangRZ);

	   gettimeofday(&tm_end,NULL);
	   printf("time_span:%dus\n",get_costtimeus(tm_start,tm_end));
//	   usleep(200000);
   }

   SPI_Close();
   //----- close TCP server
   close_data_service();

   return 0;
}
