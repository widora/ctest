/*------------------------------------------------------------
 ILI9488 controlled LCD display  and  pin-control functions

-------------------------------------------------------------*/
#ifndef _ILI9488_H
#define _ILI9488_H

#include "mygpio.h"
#include "ft232.h"

#define DCX_GPIO_PIN  14  //pin number for DC/X (data and command selection)
#define RESX_GPIO_PIN 15   //pin number for RESX (hard reset)

/* ---------  DCX pin:  0 as command, 1 as data --------- */
 #define DCXdata mt76x8_gpio_set_pin_value(DCX_GPIO_PIN,1)
 #define DCXcmd mt76x8_gpio_set_pin_value(DCX_GPIO_PIN,0)

/* --------- Hardware set and reset pin-------------- */
 #define RESX_SET mt76x8_gpio_set_pin_value(RESX_GPIO_PIN,1)
 #define RESX_RESET mt76x8_gpio_set_pin_value(RESX_GPIO_PIN,0)

//----- graphic buffer ------
uint8_t g_GBuffer[480*320][3];


void GRAM_Block_Set(uint16_t Xstart,uint16_t Xend,uint16_t Ystart,uint16_t Yend);
void LCD_ColorBox(uint16_t xStart,uint16_t yStart,uint16_t xLong,uint16_t yLong,uint8_t *color_buf);

void delayms(int s)
{
 int k,m;
 for(k=0;k<s;k++)
 {
    usleep(1000);
 }
}

void setPinMmap(void)
{
 /*------------ set DCX pin ------------ */
 if(gpio_mmap()){
    printf("gpio_mmap failed!");
    return;
  }
  mt76x8_gpio_set_pin_direction(DCX_GPIO_PIN,1); // 1 -out
  mt76x8_gpio_set_pin_direction(RESX_GPIO_PIN,1);

}

/*------ release PIN mmap -----------*/
void resPinMmap(void)
{
  close(gpio_mmap_fd);
 }

/* -----  hardware reset ----- */
void LCD_RESET(void)
{
	RESX_SET;
	delayms(1);
        RESX_RESET;
        delayms(20);
        RESX_SET;
        delayms(10);
}

/*------ write command to LCD -------------*/
int LCD_Write_Cmd(uint8_t data)
{
  int ret;

   DCXcmd;

   ret = ftdi_write_data(g_ftdi, &data, 1); //----------------------------
   if (ret < 0)
   {
          fprintf(stderr,"ftdi write failed!, error %d (%s)\n",ret, ftdi_get_error_string(g_ftdi));
   }
   else
	printf("ftdi succeed to write cmd: 0x%02x to ft232h \n",data);

   return ret;
}

/*------ write 1 Byte data to LCD -------------*/
int LCD_Write_Data(uint8_t data)
{
   int ret;

   DCXdata;

   ret = ftdi_write_data(g_ftdi, &data, 1); //----------------------------
   if (ret < 0)
   {
          fprintf(stderr,"ftdi write failed!, error %d (%s)\n",ret, ftdi_get_error_string(g_ftdi));
   }
   else
	printf("ftdi succeed to write data: 0x%02x to ft232h \n",data);

   return ret;
}

/*------ write n Bytes data to LCD -------------*/
int LCD_Write_NData(uint8_t *pdata, int n)
{
   int ret;

   DCXdata;

   ret = ftdi_write_data(g_ftdi, pdata, n);
   if (ret < 0)
   {
          fprintf(stderr,"ftdi write failed!, error %d (%s)\n",ret, ftdi_get_error_string(g_ftdi));
   }
   else
	printf("ftdi succeed to write %d bytes data to ft232h \n",n);

   return ret;

}



/*------ write g_GBuffer data to LCD -------------*/
int LCD_Write_GBuffer(void)
{
   int ret;

   //----- write data to GRAM -----
   LCD_Write_Cmd(0x2c); //memory write
   // LCD_Write_Cmd(0x3c); //continue memeory wirte

   DCXdata;
   ret=LCD_Write_NData(&g_GBuffer[0][0], 480*320*3); //write g_GBuffer to LCD

   return ret;
}


/*
//-------- write 2 Byte date to LCD ---------
void WriteDData(uint16_t data)
{
  uint8_t tmp[2];
  tmp[0]=data>>8;
  tmp[1]=data&0x00ff;

  DCXdata;
  SPI_Write(tmp,2);
}

//------ write N Bytes data to LCD -------------
void WriteNData(uint8_t* data,int N)
{
 DCXdata;
 SPI_Write(data,N);
}


//--------------- prepare for RAM direct write --------
void LCD_ramWR_Start()
{
  LCD_Write_Cmd(0x2c);
 }

*/

// ------------------ LCD Initialize Function ---------------
void LCD_INIT_ILI9488(void)
{
 delayms(20);
 LCD_RESET();
 delayms(120);

 LCD_Write_Cmd(0x11); // sleep out
 delayms(120); // must wait 120ms for sleep out

/*
 LCD_Write_Cmd(0xc0);
 LCD_Write_Cmd(0xB7); 	//t ver address
 LCD_Write_Data(0x07);


 LCD_Write_Cmd(0xF0); 	//Enter ENG
 LCD_Write_Data(0x36);
 LCD_Write_Data(0xA5);
 LCD_Write_Data(0xD3);


 LCD_Write_Cmd(0xE5); 	//Open gamma function
 LCD_Write_Data(0x80);

 LCD_Write_Cmd(0xF0); 	//	Exit ENG
 LCD_Write_Data(0x36);
 LCD_Write_Data(0xA5);
 LCD_Write_Data(0x53);
*/

 LCD_Write_Cmd(0xE0); 	//positive Gamma setting
 LCD_Write_Data(0x0F);
 LCD_Write_Data(0x1f);
 LCD_Write_Data(0x1c);
 LCD_Write_Data(0x0b);
 LCD_Write_Data(0x0e);
 LCD_Write_Data(0x09);
 LCD_Write_Data(0x48);
 LCD_Write_Data(0x99);
 LCD_Write_Data(0x38);
 LCD_Write_Data(0x0a);
 LCD_Write_Data(0x14);
 LCD_Write_Data(0x06);
 LCD_Write_Data(0x11);
 LCD_Write_Data(0x09);
 LCD_Write_Data(0x00);

 LCD_Write_Cmd(0xE1); 	//negative Gamma setting
 LCD_Write_Data(0x0F);
 LCD_Write_Data(0x36);
 LCD_Write_Data(0x2e);
 LCD_Write_Data(0x09);
 LCD_Write_Data(0x0a);
 LCD_Write_Data(0x04);
 LCD_Write_Data(0x46);
 LCD_Write_Data(0x66);
 LCD_Write_Data(0x37);
 LCD_Write_Data(0x06);
 LCD_Write_Data(0x10);
 LCD_Write_Data(0x04);
 LCD_Write_Data(0x24);
 LCD_Write_Data(0x20);
 LCD_Write_Data(0x00);

// --------- brightness control ----------
/*
 LCD_Write_Cmd(0x55);LCD_Write_Data(0x02); //-- adaptive brightness control Still Mode
 LCD_Write_Cmd(0x53);LCD_Write_Data(0x2c); //--- Display control
 LCD_Write_Cmd(0x5E);LCD_Write_Data(0x00); //--- Min Brightness
 LCD_Write_Cmd(0x51); LCD_Write_Data(0x00); //-- display brightness
 printf("\nBrightness set finished!\n");
*/

 LCD_Write_Cmd(0x29); 	//display ON
 delayms(10);

 //----- set interface pixel format -----
 LCD_Write_Cmd(0x3a);
 LCD_Write_Data(0b01100111);

 //----- adjust pic layout position here ------
 LCD_Write_Cmd(0x36); //memory data access control
 LCD_Write_Data(0x08); //Data[3]=0 RGB, Data[3]=1 BGR 

 //----- write data to GRAM -----
 LCD_Write_Cmd(0x2c); //memory write
// LCD_Write_Cmd(0x3c); //continue memeory wirte

 int i;
 uint8_t color[3]={
0x0f,
0xf0,
0x00,
};

//00,ff,00 Yellow
//ff,00,00 blue
//00,00,ff  light green
/*
color[0]=0xff;color[1]=0x00;color[2]=0x00;
LCD_ColorBox(0,0,30,300,color);
color[0]=0x00;color[1]=0xff;color[2]=0x00;
LCD_ColorBox(30,0,30,300,color);
color[0]=0x00;color[1]=0x00;color[2]=0xff;
LCD_ColorBox(60,0,30,300,color);
*/

//---- clear graphic buffer -----
memset(g_GBuffer,0,480*320*3);

/*
 for(i=0;i<480*320/3;i++){
	LCD_Write_NData(color,3);
 }
 LCD_Write_Cmd(0x00); //write a dummy cmd to end
*/

}



//------------- GRAM block address set ----------------
// x -HEIGHT,  y-LENGTH
void GRAM_Block_Set(uint16_t Xstart,uint16_t Xend,uint16_t Ystart,uint16_t Yend)
{
	LCD_Write_Cmd(0x2a);
	LCD_Write_Data(Xstart>>8);
	LCD_Write_Data(Xstart&0xff);
	LCD_Write_Data(Xend>>8);
	LCD_Write_Data(Xend&0xff);

	LCD_Write_Cmd(0x2b);
	LCD_Write_Data(Ystart>>8);
	LCD_Write_Data(Ystart&0xff);
	LCD_Write_Data(Yend>>8);
	LCD_Write_Data(Yend&0xff);
}


// ------------  draw a color box --------------
void LCD_ColorBox(uint16_t xStart,uint16_t yStart,uint16_t xLong,uint16_t yLong,uint8_t *color_buf)
{
	uint32_t temp;

	GRAM_Block_Set(xStart,xStart+xLong-1,yStart,yStart+yLong-1);
        LCD_Write_Cmd(0x2c);  // ----for continous GRAM write

	for (temp=0; temp<xLong*yLong; temp++)
	{
  	  LCD_Write_NData(color_buf,3);
	}
}

/*
// ------------------- show a picture stored in a char* array -----------

void LCD_Fill_Pic(uint16_t x, uint16_t y, uint16_t pic_H, uint16_t pic_V, const unsigned char* pic)
{
        uint32_t i;
	uint16_t j;

 	LCD_Write_Cmd(0x36); //Set_address_mode
 	LCD_Write_Data(0x08); // show vertically 
        GRAM_Block_Set(x,x+pic_H-1,y,y+pic_V-1);
        LCD_Write_Cmd(0x2c);  // ----for continous GRAM write
	for (i = 0; i < pic_H*pic_V*2; i+=2)
	{
           LCD_Write_Data(pic[i]);
           LCD_Write_Data(pic[i+1]);
 
	}
 	LCD_Write_Cmd(0x36); //Set_address_mode
 	LCD_Write_Data(0x68); //show horizontally
}
*/


#endif




