/*-------------------------------------------------
  RM68140 controlled LCD display  and  pin-control functions

------------------------------------------------*/
#ifndef _RM68140_H
#define _RM68140_H

#include "./mygpio.h"
#include "./spi.h"


/* ---------  DCX pin:  0 as  command, 1 as data --------- */
 #define DCXdata mt76x8_gpio_set_pin_value(14,1)
 #define DCXcmd mt76x8_gpio_set_pin_value(14,0)
/* --------- Hardware set and reset pin-------------- */
 #define HD_SET mt76x8_gpio_set_pin_value(16,1)
 #define HD_RESET mt76x8_gpio_set_pin_value(16,0)


void delayms(int s)
{
 int k;
 for(k=0;k<s;k++)
      usleep(1000);
}

void setPinMmap()
{
 /*------------ set DCX pin ------------ */
 if(gpio_mmap())
    printf("gpio_mmap failed!");
  mt76x8_gpio_set_pin_direction(14,1);
  mt76x8_gpio_set_pin_direction(16,1);

}

/*------ release PIN mmap -----------*/
void resPinMmap()
{
  close(gpio_mmap_fd);
 }

/* -----  hardware reset ----- */
void LCD_HD_reset()
{
        HD_RESET;
        delayms(30);
        HD_SET;
        delayms(10);
}

/*------ write command to LCD -------------*/
void WriteComm(uint8_t cmd)
{
 DCXcmd;
 SPI_Write(&cmd,1);
}

/*------ write 1 Byte data to LCD -------------*/
void WriteData(uint8_t data)
{
 DCXdata;
 SPI_Write(&data,1);
}
/*-------- write 2 Byte date to LCD ---------*/
void WriteDData(uint16_t data)
{
  uint8_t tmp[2];
  tmp[0]=data>>8;
  tmp[1]=data&0x00ff;
 
  DCXdata;
  SPI_Write(tmp,2);
}

/*------ write N Bytes data to LCD -------------*/
void WriteNData(uint8_t* data,int N)
{
 DCXdata;
 SPI_Write(data,N);
}


/*--------------- prepare for RAM direct write --------*/
void LCD_ramWR_Start()
{
  WriteComm(0x2c);  
 } 



/* ------------------ LCD Initialize Function ---------------*/
void LCD_INIT_RM68140(void)
{
 delayms(20);
 LCD_HD_reset();

 WriteComm(0x11); // sleep out
 delayms(10); // must wait 5ms for sleep out

 WriteComm(0xc0);
 WriteComm(0xB7); 	//t ver address	
 WriteData(0x07);


 WriteComm(0xF0); 	//Enter ENG	
 WriteData(0x36);
 WriteData(0xA5);
 WriteData(0xD3);


 WriteComm(0xE5); 	//Open gamma function	
 WriteData(0x80);

 WriteComm(0xF0); 	//	Exit ENG	
 WriteData(0x36);
 WriteData(0xA5);
 WriteData(0x53);

 WriteComm(0xE0); 	//Gamma setting	
 WriteData(0x00);
 WriteData(0x35);
 WriteData(0x33);
 WriteData(0x00);
 WriteData(0x00);
 WriteData(0x00);
 WriteData(0x00);
 WriteData(0x35);
 WriteData(0x33);
 WriteData(0x00);
 WriteData(0x00);
 WriteData(0x00);

 WriteComm(0x3a); 	//interface pixel format	
 WriteData(0x55);      //16bits pixel 

/* --------- brightness control ----------*/

 WriteComm(0x55);WriteData(0x02); //-- adaptive brightness control Still Mode
 WriteComm(0x53);WriteData(0x2c); //--- Display control 
 WriteComm(0x5E);WriteData(0x00); //--- Min Brightness 
 WriteComm(0x51); WriteData(0x00); //-- display brightness
 printf("\nBrightness set finished!\n");


 WriteComm(0x29); 	//display ON
 delayms(10);

 WriteComm(0x36); //memory data access control
 WriteData(0x08); // BGR order ????? 
  //how to turn on off  back light

}

/* ------------- GRAM block address set ---------------- */
void GRAM_Block_Set(uint16_t Xstart,uint16_t Xend,uint16_t Ystart,uint16_t Yend)
{
	WriteComm(0x2a);   
	WriteData(Xstart>>8);
	WriteData(Xstart&0xff);
	WriteData(Xend>>8);
	WriteData(Xend&0xff);

	WriteComm(0x2b);   
	WriteData(Ystart>>8);
	WriteData(Ystart&0xff);
	WriteData(Yend>>8);
	WriteData(Yend&0xff);
}


/* ------------  draw a color box --------------*/
void LCD_ColorBox(uint16_t xStart,uint16_t yStart,uint16_t xLong,uint16_t yLong,uint16_t Color)
{
	uint32_t temp;

	GRAM_Block_Set(xStart,xStart+xLong-1,yStart,yStart+yLong-1);
        WriteComm(0x2c);  // ----for continous GRAM write

	for (temp=0; temp<xLong*yLong; temp++)
	{
          WriteDData(Color);
  	 //WriteData(Color>>8);
	 //WriteData(Color&0xff);
	}
}

/* ------------------- show a picture stored in a char* array ------------*/

void LCD_Fill_Pic(uint16_t x, uint16_t y, uint16_t pic_H, uint16_t pic_V, const unsigned char* pic)
{
        uint32_t i;
	uint16_t j;

 	WriteComm(0x36); //Set_address_mode
 	WriteData(0x08); // show vertically 
        GRAM_Block_Set(x,x+pic_H-1,y,y+pic_V-1);
        WriteComm(0x2c);  // ----for continous GRAM write
	for (i = 0; i < pic_H*pic_V*2; i+=2)
	{
           WriteData(pic[i]);
           WriteData(pic[i+1]);             	  
 
	}
 	WriteComm(0x36); //Set_address_mode
 	WriteData(0x68); //show horizontally
}



#endif




