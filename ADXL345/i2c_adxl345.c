/*--------------------------------------------
Operate I2C slave with  read() write()

Midas
---------------------------------------------*/
#include "i2c_adxl345.h"



/*-----------------------------------------
         initiate i2c ioctl msg data
-----------------------------------------*/
void init_I2C_IOmsg(void)
{
  printf("	hello");
}


/*----- open i2c slave and init ioctl IOsmg -------------
Return:
	0  ok
	<0 fails
--------------------------------------------------------*/
static int init_ADXL_I2C_Slave(void)
{
  struct flock lock;

  if((g_I2Cfd=open(g_I2Cfdev,O_RDWR))<0)
  {
	perror("	fail to open i2c device!");
        return -1;
  }
  else
   	printf("	Open %s successfully!\n",g_I2Cfdev);

  if(ioctl(g_I2Cfd, I2C_SLAVE_FORCE,I2C_Addr_ADXL345)<0)
  {
       perror("		fail to set slave address!");
       return -1;
  }
  else
	printf("	set i2c slave address successfully!\n");

  //----- set g_I2Cfd
  if(ioctl(g_I2Cfd,I2C_TIMEOUT,3)<0) 
  {
       perror("		fail to set I2C_TIMEOUT!");
       return -1;
  }

  if( ioctl(g_I2Cfd,I2C_RETRIES,1) <0 ) // !!!!!!!!
  {
       perror("	fail to set I2C_RETRIES!");
       return -1;
  }

  //---- init i2c ioctl msg data -----
  init_I2C_IOmsg();

  //-----  set I2C speed ------
/*
  if(ioctl(g_fdOled,I2C_SPEED,200000)<0)
	printf("Set I2C speed fails!\n");
  else
	printf("Set I2C speed to 200KHz successfully!\n");
*/
  return 0;
}


/*------------------------------------------
Write Register data

Return
     <0  fails
     >0  OK
-------------------------------------------*/
int I2C_Single_Write(uint8_t reg_addr, uint8_t reg_dat)
{
    int ret;
    uint8_t buff[2];

    buff[0]=reg_addr; buff[1]=reg_dat;
    ret=write(g_I2Cfd,buff,2);
    if(ret !=2 )
    {
	printf(" 	I2C Single Read failed\n");
	return -1;
    }

    return ret;
}


/*------------------------------------------
Read single Register data

Return
     <0  fails
     >0  OK
-------------------------------------------*/
int I2C_Single_Read(uint8_t reg_addr, uint8_t *reg_dat)
{
    int ret;
    uint8_t buff;

    ret=write(g_I2Cfd,&reg_addr,1);
    if(ret !=1 )
    {
	printf("	 I2C Single Read(write addr) failed\n");
	return -1;
    }
    ret=read(g_I2Cfd,&buff,1);
    if(ret !=1 )
    {
	printf(" 	I2C Single Read(read data) failed\n");
	return -1;
    }

    *reg_dat = buff;

    return ret;
}

/*------------------------------------------
Read multiple Register data

nbytes: bytes to be read
reg_addr:  starting register address
reg_dat[]: data returned

Return
     <0  fails
     >0  OK
-------------------------------------------*/
int I2C_Multi_Read(uint8_t nbytes, uint8_t reg_addr, uint8_t *reg_dat)
{
    int ret;
    uint8_t buff;

    ret=write(g_I2Cfd,&reg_addr,1);
    if(ret !=1 )
    {
	printf(" 	I2C Mutli Read():  write addr failed\n");
	return -1;
    }
    ret=read(g_I2Cfd,reg_dat,nbytes);
    if(ret !=nbytes )
    {
	printf(" 	I2C Multi Read():   read data failed\n");
	return -1;
    }

    return ret;
}


/*----------------------------------------------------
check ADXL345 register 0x30 to see if DATA_READY is 1
----------------------------------------------------*/
bool DataReady_ADXL345(void)
{
   uint8_t dat;
   I2C_Single_Read(ADXL345REG_INT_SOURCE, &dat);
//   printf("INT_SOURCE: 0x%02x \n",dat);
   return (dat>>7);
}



 /*---------------------------------------------------------------
              init ADXL345  with Full resolution
1. Init in full resolution mode
2. Max. offset adjustment value OFX,OFY,OFZ, is 2g!!!
Return:
	0   --- OK
	<0  --- Fail
 ---------------------------------------------------------------*/
int Init_ADXL345(enum ADXL_ODRBW_setval ODRBW_setval, uint8_t g_range)
{

   int8_t  int8_OFSX,int8_OFSY,int8_OFSZ;
   int16_t bias_AXYZ[3];

   usleep(500000);

 //----- open and setup i2c slave
   printf("	init i2c slave...\n");
   if( init_ADXL_I2C_Slave() <0 )
   {
        printf("	init i2c slave failed!\n");
        return -1;
   }

   I2C_Single_Write(ADXL345REG_DATA_FORMAT,ADXL_FULL_MODE|g_range);//Full resolutioin, +-16g,right-justified mode
   //----- for BW_RATE ODR:
   // 0x0f-3200Hz,0x0e-1600Hz,0x0d-800Hz,0x0c-400Hz,0x0b-200Hz, 0x0a-100Hz, 0x09-50Hz
   I2C_Single_Write(ADXL345REG_BW_RATE, ADXL_NORMAL_POWER|ODRBW_setval);//operation,0x1- reduced power mode, 0x0e 1600Hz,  0x08 ODR 25Hz, 0x1c ODR=400Hz,0x1d 800Hz,  0x1A ODR=100HZ; 0x0 normaol operation,0x1 reduced power mode
   I2C_Single_Write(ADXL345REG_POWER_CTL, 0x08);//measurement mode, no sleep
   I2C_Single_Write(ADXL345REG_INT_ENABLE, 0x00);//no interrupt

   //--- re-confirm setup here ///

   //----- to get bias value for AXYZ
   // !!! before that you need to clear OFSX,OFSY,OFSZ !!!
   I2C_Single_Write(ADXL345REG_OFSX, 0);
   I2C_Single_Write(ADXL345REG_OFSY, 0);
   I2C_Single_Write(ADXL345REG_OFSZ, 0);
   usleep(20000);
   printf("	!!! Read and calculate the bias value now, keep the Z-axis of ADXL345 upright and static !!!\n");
   sleep(1);
   adxl_get_int16BiasXYZ(bias_AXYZ);
   printf("	bias_AccX: %fmg,  bias_AccY: %fmg, bias_AccZ(Gravity): %fmg \n",fsmg_full*bias_AXYZ[0],fsmg_full*bias_AXYZ[1],fsmg_full*bias_AXYZ[2]-1000.0);

   //------ get eight bits OFSX,OFSY,OFSZ for offset adjustments
   //------ they are in tows complement format with a scale facotr of 15.6mg/LSB(0x7F=2g)
   //------ the value stored in the offset registers (!!!is automatically added) to the acceleration data
   int8_OFSX = -fsmg_full/15.6*bias_AXYZ[0]; //
   int8_OFSY = -fsmg_full/15.6*bias_AXYZ[1]; //
   int8_OFSZ = -(fsmg_full/15.6*bias_AXYZ[2]-1000.0/15.6); // 1000/3.9~=250 !!!! Z-axis as gravity vertical !!!!
   printf("	int8_OFSX: %d,  int8_OFSY: %d, int8_OFSZ: %d \n",int8_OFSX,int8_OFSY,int8_OFSZ);
   //user-set offset adjustments in twos complement format with a scale factor of 15.6mg/LSB
   I2C_Single_Write(ADXL345REG_OFSX, int8_OFSX);
   I2C_Single_Write(ADXL345REG_OFSY, int8_OFSY);
   I2C_Single_Write(ADXL345REG_OFSZ, int8_OFSZ);

   return 0;
}


/*--------------------------------------------
   close ADXL345
--------------------------------------------*/
void Close_ADXL345(void)
{
   //----- close i2c dev
   if(!g_I2Cfd)
	   close(g_I2Cfd);
}

/*--------------------------------------------
 Read DATAX0-DATAZ1 
 Retrun int16_t [3]
--------------------------------------------*/
void adxl_read_int16AXYZ(int16_t  *accXYZ)
{
    uint8_t *xyz = (uint8_t *)accXYZ;

    //----- read XYZ data one byte by one byte,!!!!!!!!!!!!!!!
    //------  continous reading will crash Openwrt !!!!

    //----- wait for data to be read
    while(!DataReady_ADXL345())
    {
//	  printf("wait for ADXL data ready...\n");
          usleep(ADXL_DATAREADY_WAITUS);
    }
    I2C_Single_Read(0x32,xyz);
    I2C_Single_Read(0x33,xyz+1);
    I2C_Single_Read(0x34,xyz+2);
    I2C_Single_Read(0x35,xyz+3);
    I2C_Single_Read(0x36,xyz+4);
    I2C_Single_Read(0x37,xyz+5);

}


/*----------------------------------------------------
Get bias values of AX AY AZ in int16_t *[3]
Bias values will be used to set zero level for ADXL345
----------------------------------------------------- */
inline void adxl_get_int16BiasXYZ(int16_t* bias_xyz)
{
        int i;
        int16_t  xyz_val[3];
        int32_t  xyz_sums[3]={0};

        for(i=0;i<ADXL_BIAS_SAMPLE_NUM;i++)
        {
                adxl_read_int16AXYZ(xyz_val);
                xyz_sums[0] += xyz_val[0];
                xyz_sums[1] += xyz_val[1];
                xyz_sums[2] += xyz_val[2];
        }

        for(i=0;i<3;i++)
                bias_xyz[i]=xyz_sums[i]/ADXL_BIAS_SAMPLE_NUM;

}
