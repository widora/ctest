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
// else
//	printf("ftdi succeed to write cmd: 0x%02x to ft232h \n",data);

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
//   else
//	printf("ftdi succeed to write data: 0x%02x to ft232h \n",data);

   return ret;
}

/*------ write n Bytes data to LCD -------------*/
int LCD_Write_NData(uint8_t *pdata, int n)
{
   int ret=0;
   int i;

/*------chunck write, beware of baudrate speed -----*/
   DCXdata;

   ret = ftdi_write_data(g_ftdi, pdata, n);
   if (ret < 0)
   {
          fprintf(stderr,"ftdi write failed!, error %d (%s)\n",ret, ftdi_get_error_string(g_ftdi));
   }
//   else
//	printf("ftdi succeed to write %d bytes data to ft232h \n",n);

   return ret;

}


/*------ reorder and write GBuffer data to LCD -------------
return
	>0 bytes written
	<0 fails
-------------------------------------------------*/
int LCD_Write_GBuffer(void)
{
   int ret;
   int i;

   //----- write data to GRAM -----
   LCD_Write_Cmd(0x2c); //memory write
   // LCD_Write_Cmd(0x3c); //continue memeory wirte

   DCXdata;
   //------ transfer data to ft232h  -------
  //   ret = ftdi_write_data(g_ftdi, g_usb_GBuffer, 480*320*3);
   ret = ftdi_write_data(g_ftdi, &g_GBuffer[0][0], 480*320*3);
   if (ret < 0)
   {
          fprintf(stderr,"ftdi write failed!, error %d (%s)\n",ret, ftdi_get_error_string(g_ftdi));
   }
   else
	printf("ftdi succeed to write g_usb_GBuffer to ft232h \n");

   return ret;
}


// ------------------ LCD Initialize Function ---------------
void LCD_INIT_ILI9488(void)
{
 delayms(20);
 LCD_RESET();
 delayms(120);

 //---software reset ------
 LCD_Write_Cmd(0x01);
 delayms(10);

 LCD_Write_Cmd(0x11); // sleep out
 delayms(120); // must wait 120ms for sleep out

 //------ set image function,enable 24bits data bus  -----
 LCD_Write_Cmd(0xe9);
 LCD_Write_Data(0x00);//1-24bit bus ;0-18bit bus

 //----- set interface pixel format -----
 LCD_Write_Cmd(0x3a);
 LCD_Write_Data(0b01100110); //0b01010101-error,0b01110101-error,0b01110111-ok,0b01010111 ok,
//0b01110110-ok, 0b01100110-ok, 0b01100111-ok

/*
 //------ gama function ------
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
*/


 // --------- brightness control ----------

 //------- IDLE MODE OFF -----
 LCD_Write_Cmd(0x38);

 //------- DISPLAY ON ------
 LCD_Write_Cmd(0x29); 	//display ON
 delayms(10);

 //-------- set column and page address area  --------
 // GRAM_Block_Set(0,319,0,479);//full area GRAM ,column and page address normal

 //--- exchagne X and Y ------
 GRAM_Block_Set(0,479,0,319);//full area GRAM for column and page exchanged

 //----- adjust pic layout position here ------
 LCD_Write_Cmd(0x36); //memory data access control
 LCD_Write_Data(0x60); // exchange X and Y


 //----- write data to GRAM -----
 // LCD_Write_Cmd(0x2c); //memory write
 // LCD_Write_Cmd(0x3c); //continue memeory wirte

 //---- clear graphic buffer -----
 memset(g_GBuffer,0,480*320*3);

 printf("finish preparing ILI9488\n");
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


/* ------------  draw a color box --------------
uint8_t color_buf[3]:  RGB 3*8bits color
-----------------------------------------------*/
void LCD_ColorBox(uint16_t xStart,uint16_t yStart,uint16_t xLong,uint16_t yLong,uint8_t *color_buf)
{
	int i;
	uint32_t temp;
	uint8_t *block_buf;
	uint32_t byte_count;


	byte_count=xLong*yLong*3;
	//------ allocate mem. -------
	block_buf=malloc( byte_count );
	if(block_buf==NULL){
		printf("malloc bloc_buf failed!\n");
		return;
	}
	//----- fill color to mem -----
	for(i=0;i<byte_count/3;i++)
		memcpy(block_buf+3*i,color_buf,3);

	//------ set GRAME AREA -----
	GRAM_Block_Set(xStart,xStart+xLong-1,yStart,yStart+yLong-1);
        LCD_Write_Cmd(0x2c);  // ----for continous GRAM write
	LCD_Write_NData(block_buf,byte_count);

	//------ free mem. -------
	free(block_buf);
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




