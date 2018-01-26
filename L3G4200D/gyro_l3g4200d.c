
//------------------------L3G4200D Operation Function Definition------------------------------ 
#include "gyro_l3g4200d.h"

/*
void halSpiStrobe(uint8_t strobe)
{
	SPI_Write(&strobe,1);
}

// read Status byte from MISO 
uint8_t halSpiGetStatus(void)
{
	uint8_t status;
        SPI_Read(&status,1);
	return status;
}
*/

/* Write to a register */
void halSpiWriteReg(const uint8_t addr, const uint8_t value)
{
	uint8_t data[2];
	data[0]=addr; data[1]=value;
	SPI_Write(data,2);
}

/*  burst write to registers */
void halSpiWriteBurstReg(const uint8_t addr,  uint8_t *buffer, const uint8_t count)
{
	uint8_t tmp;
	tmp=addr|WRITE_BURST;
	SPI_Write_then_Write(&tmp,1,buffer,count);
}

/* read a register */
uint8_t halSpiReadReg(uint8_t addr)
{
	uint8_t tmp,value;
	tmp=addr|READ_SINGLE;
	SPI_Write_then_Read(&tmp,1,&value,1);
	return value;
}


/* burst read registers */
//--- Max. count=31 ---//
void halSpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count)
{
	uint8_t tmp;
        tmp=addr|READ_BURST;
        SPI_Write_then_Read(&tmp,1,buffer,count);
}

/* read a status register */
uint8_t halSpiReadStatus(uint8_t addr)
{
	uint8_t tmp,value;
	tmp=addr|READ_BURST; //-- To read a status-register with command header 11
	SPI_Write_then_Read(&tmp,1,&value,1);
	return value;
}


/*---- init L3G4200D -----*/
void Init_L3G4200D(void) {
        //0xcf: ODR=800Hz,Fc=30Hz, normal mode, XYZ all enabled,
        //0x0f: ODR=100Hz,Fc=12.5Hz, normal mode, XYZ all enabled,
	halSpiWriteReg(L3G_CTRL_REG1, 0xcf);//output data rate[7:6], bandwidth[5:4],power down mode[3], and Axis enable[2:0]
	// HPF normal mode,HPFcut off freq=56Hz when ODR=800Hz,
	halSpiWriteReg(L3G_CTRL_REG2, 0b00000000);//!!!High Pass filter mode[5:4], High Pass filter Cutoff freqency [3:0]
	// Interrupt disabled
	halSpiWriteReg(L3G_CTRL_REG3, 0x00);//interrupt configuration, disable
	// set measure range +-2000dps
	halSpiWriteReg(L3G_CTRL_REG4, 0b10100000);//+-2000dps, update method[7],big/little endian[6],full scale[5:4],self test[2:1],SPI mode[0] 
	//FIFO disabled,HPF disabled,
	halSpiWriteReg(L3G_CTRL_REG5, 0x00);//FIFO disabled, boot[7],FIFO_EN[6],HighPass filter enable[5],INI1 select[3:2],Out select[1:0]
	//FIFO disable
	halSpiWriteReg(L3G_FIFO_CTRL_REG,0x00);
}

/*   check if XYZ new data is available  */
bool status_XYZ_available(void)
{
	//---- if STATUS_REG[3]==1, XYZ data is available
	return halSpiReadReg(L3G_STATUS_REG)&0x08;
}


/*----------------------------------------------------
get RX RY RZ in int16_t *[3]
----------------------------------------------------- */
inline void gyro_read_int16RXYZ(int16_t *angRXYZ)
{
	//----- wait for new data
        while(!status_XYZ_available()) {
//        	fprintf(stdout," XYZ new data is not availbale now!\n");
                usleep(L3G_READ_WAITUS);
        }

        //----- read data from L3G4200
        halSpiReadBurstReg(L3G_OUT_X_L, (uint8_t *)angRXYZ, 6);
}

/*----------------------------------------------------
Get bias values of RX RY RZ in int16_t *[3]
Bias values will be used to set zero level for L3G4200D
----------------------------------------------------- */
inline  void gyro_get_int16BiasXYZ(int16_t* bias_xyz)
{
	int i;
	int16_t  xyz_val[3];
	int32_t  xyz_sums[3]={0};

	for(i=0;i<L3G_BIAS_SAMPLE_NUM;i++)
	{
        	gyro_read_int16RXYZ(xyz_val);
		xyz_sums[0] += xyz_val[0];
		xyz_sums[1] += xyz_val[1];
		xyz_sums[2] += xyz_val[2];
	}

	for(i=0;i<3;i++)
		bias_xyz[i]=xyz_sums[i]/L3G_BIAS_SAMPLE_NUM;

}


/*----------------------------------------------------
 thread function:
   In a loop to display gyro data on OLED
----------------------------------------------------- */
void  thread_gyroWriteOled(void)
{
	char strx[18]={0};
	while(1)
	{
		//----- quit if receive token
		if(gtok_QuitGyro)
			break;
		//----- write data string to OLED
		sprintf(strx," X: %7.2f", g_fangXYZ[0]);
		push_Oled_Ascii32x18_Buff(strx,0,0);
		sprintf(strx," Y: %7.2f", g_fangXYZ[1]);
		push_Oled_Ascii32x18_Buff(strx,1,0);
		sprintf(strx," Z: %7.2f", g_fangXYZ[2]);
		push_Oled_Ascii32x18_Buff(strx,2,0);

		refresh_Oled_Ascii32x18_Buff(false);
		usleep(100000);
	}
}


