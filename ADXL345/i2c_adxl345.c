#include <stdio.h>
#include "i2c_adxl345.h"

int main(void)
{
   uint8_t dat;
   int16_t accXYZ[3]={0}; // acceleration value of XYZ	
   float fs=3.9/1000.0;//scale factor 3.9mg/LSB for all g-ranges, full resolutioin

   //----- open and setup i2c slave
   printf("init i2c slave...\n");
   init_I2C_Slave();
   init_ADXL345();  //ODR=100Hz

   printf("read register...\n");
   Single_Read_ADXL345(0x00, &dat);
   printf("dat=0x%02x\n",dat);

   Single_Write_ADXL345(0x2c,0x0e);
   Single_Read_ADXL345(0x2c, &dat);
   printf("dat=0x%02x\n",dat);

   while(1) 
   {
   	Multi_Read_ADXL345(2,0x32,(uint8_t *)accXYZ);
   	printf("accX=%d, accY=%d, accZ=%d \n",accXYZ[0],accXYZ[1],accXYZ[2]);
   	printf("accX=%f, accY=%f, accZ=%f \n",accXYZ[0]*fs,accXYZ[1]*fs,accXYZ[2]*fs);
        usleep(20000);//for ODR=100HZ, sleep 5ms
   }
   //----- close i2c dev
   close(g_I2Cfd);

   return 0;
}
