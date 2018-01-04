/*---------------     ----------------
TODOs and BUGs:
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

//---- SPI Read and Write BITs set -----
#define WRITE_SINGLE 0x00
#define WRITE_BURST 0x40
#define READ_SINGLE 0x80
#define READ_BURST 0xc0


//----- define registers -----
#define L3G_WHO_AM_I 0x0f
#define L3G_CTRL_REG1 0x20
#define L3G_CTRL_REG2 0x21
#define L3G_CTRL_REG3 0x22
#define L3G_CTRL_REG4 0x23
#define L3G_CTRL_REG5 0x24
#define L3G_REFERENCE 0x25
#define L3G_OUT_TEMP 0x26
#define L3G_STATUS_REG 0x27
#define L3G_OUT_X_L 0x28
#define L3G_OUT_X_H 0x29
#define L3G_OUT_Y_L 0x2a
#define L3G_OUT_Y_H 0x2b
#define L3G_OUT_Z_L 0x2c
#define L3G_OUT_Z_H 0x2d
#define L3G_FIFO_CTRL_REG 0x2e
#define L3G_FIFO_SRC_REG 0x2f
#define L3G_INT1_CFG 0x30
#define L3G_INT1_SRC 0x31
#define L3G_INT1_TSH_XH 0x32
#define L3G_INT1_TSH_XL 0x33
#define L3G_INT1_TSH_YH 0x34
#define L3G_INT1_TSH_YL 0x35
#define L3G_INT1_TSH_ZH 0x36
#define L3G_INT1_TSH_ZL 0x37
#define L3G_INT1_DURATION 0x38


//------------------------function definition------------------------------ 
void halSpiStrobe(uint8_t strobe)
{
	SPI_Write(&strobe,1);
}


/* read Status byte from MISO */
uint8_t halSpiGetStatus(void)
{
	uint8_t status;
        SPI_Read(&status,1);
	return status;
}

/* Write to a register */
void halSpiWriteReg(uint8_t addr, uint8_t value)
{
	uint8_t data[2];
	data[0]=addr; data[1]=value;
	SPI_Write(data,2);
}

/*  burst write to registers */
void halSpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count)
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
	uint8_t tmp,value;
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

void Init_L3G4200D(void) {
	halSpiWriteReg(L3G_CTRL_REG1, 0x0f);
	halSpiWriteReg(L3G_CTRL_REG2, 0x00);
	halSpiWriteReg(L3G_CTRL_REG3, 0x08);
	halSpiWriteReg(L3G_CTRL_REG4, 0x30); //+-2000dps
	halSpiWriteReg(L3G_CTRL_REG5, 0x00);
}
