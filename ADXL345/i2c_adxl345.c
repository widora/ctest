#include <stdio.h>
//#include "i2c_adxl345_1.h"
#include "i2c_adxl345_2.h"
#include "mathwork.h"

int main(void)
{
   int i;
   uint8_t dat;
   uint8_t addr;
   int16_t accXYZ[3]={0}; // acceleration value of XYZ
   uint8_t xyz[6]={0};
   int16_t *tmp=(int16_t *)xyz;
   float fs=3.9/1000.0;//scale factor 3.9mg or 4mg?? /LSB for all g-ranges, full resolutioin
   //------ time value ----
   struct timeval tm_start,tm_end;


   //----- open and setup i2c slave
   printf("init i2c slave...\n");
   init_I2C_Slave();
   //------ set up ADXL345
   init_ADXL345();  //ODR=100Hz


#if 1  //------------ I2C operation with read() /write() -------------
i=0;
gettimeofday(&tm_start,NULL);
while(i<100000)
{
    i++;
    get_int16XYZ_ADXL345(accXYZ);
    printf("I=%d, accX: %f,  accY: %f,  accZ:%f \r", i, accXYZ[0]*fs, accXYZ[1]*fs, accXYZ[2]*fs);
    fflush(stdout);
}
gettimeofday(&tm_end,NULL);
printf("\n Time elapsed: %fs \n",get_costtimeus(tm_start,tm_end)/1000000.0);
#endif


#if 0 //------------  I2C OPERATION with IOCTL() -----------------

   printf("read register...\n");
   addr=0x00;
   Single_Read_ADXL345(addr, &dat);
   printf("dat=0x%02x\n",dat);

   Single_Write_ADXL345(0x2c,0x0e);
   addr=0x2c;
   Single_Read_ADXL345(addr, &dat);
   printf("dat=0x%02x\n",dat);

   i=0;
   addr=0x32;

   for(i=0x1D; i<0x39; i++)
   {
	Single_Read_ADXL345(i, &dat);
        printf("ADDR 0x%02x    VAL 0x%02x \n",i,dat);
   }

   while(1) 
   {
	i++;
	//----- !!!!! wait for data, if data is NOT ready, Multi_Read() will crash Openwrt !!!!!
	while(!IS_DataReady_ADXL345()) {
		usleep(100000);
		printf(" data not ready! \n");
	}
	//----- read data
   	Multi_Read_ADXL345(6,addr,(uint8_t *)accXYZ);
//   	printf("accX=%d, accY=%d, accZ=%d \n",accXYZ[0],accXYZ[1],accXYZ[2]);
   	printf("I=%d   accX=%f, accY=%f, accZ=%f \r",i,accXYZ[0]*fs,accXYZ[1]*fs,accXYZ[2]*fs);
	fflush(stdout);
        usleep(10000);//for ODR=100HZ, sleep 5ms
   }

#endif



   //----- close i2c dev
   close(g_I2Cfd);

   return 0;
}
