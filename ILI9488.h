/*------------------------------------------------------------
 ILI9488 controlled LCD display  and  pin-control functions

-------------------------------------------------------------*/
#ifndef _ILI9488_H
#define _ILI9488_H

#include "mygpio.h"
#include "ft232.h"

#define DCX_GPIO_PIN  14  //pin number for DC/X (data and command selection)
#define RESX_GPIO_PIN 15   //pin number for RESX (hard reset)

//---- picture definition limits  ----
#define PIC_MAX_WIDTH 480
#define PIC_MAX_HEIGHT 320

//----- pixel format -----
#define PXLFMT_RGB888 0
#define PXLFMT_RGB565 1

//----- default format ----
int LCD_PxlFmt=PXLFMT_RGB888;

/* ---------  DCX pin:  0 as command, 1 as data --------- */
 #define DCXdata mt76x8_gpio_set_pin_value(DCX_GPIO_PIN,1)
 #define DCXcmd mt76x8_gpio_set_pin_value(DCX_GPIO_PIN,0)

/* --------- Hardware set and reset pin-------------- */
 #define RESX_SET mt76x8_gpio_set_pin_value(RESX_GPIO_PIN,1)
 #define RESX_RESET mt76x8_gpio_set_pin_value(RESX_GPIO_PIN,0)

//----- graphic buffer ------
uint8_t *g_pRGB565;//RGB565 data buff
uint8_t g_GBuffer[480*320][3];

//----- convert 24bit color to 18bit color -----
#define GET_RGB565(r,g,b)  (  ((g>>2)<<13) | ((b>>3)<<8) | ((r>>3)<<3) | (g>>5) ) 

//----- function declaration ----------
void LCD_Set_PxlFmt24bit(void);
void LCD_Set_PxlFmt18bit(void);
void GRAM_Block_Set(uint16_t Xstart,uint16_t Xend,uint16_t Ystart,uint16_t Yend);
void GBuffer_Write_Block(int Hs,int He, int Vs, int Ve, const uint8_t *data); //write block pic to g_GBuffer;
int LCD_Write_GBuffer(void);//write whole page of g_GBuffer to LCD
void LCD_ColorBox(uint16_t xStart,uint16_t yStart,uint16_t xLong,uint16_t yLong, const uint8_t *color_buf);
int LCD_Write_Block(int Hs,int He, int Vs, int Ve, const uint8_t *data, int nb);//write block pic to LCD

//----- function definition ---------
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
int LCD_Write_NData(const uint8_t *pdata, int n)
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


/*------------------------------------------------------------------
write a block of data to g_GBuffer[]

Hs---start of horizon,    He---end of horizon
Vs---start of vertical,    Ve---end of vertical
data -- point to RGB data
------------------------------------------------------------------------*/
void GBuffer_Write_Block(int Hs,int He, int Vs, int Ve, const uint8_t *data)
{
	int i;
	int lenH;
	const uint8_t *pt=data;//to data
        uint8_t *pb;// to g_GBuffer

	//------- push RGB data to GBuffer, Horizontal direction first, then Verital .....
	lenH=(He-Hs+1)*3;
	for(i=Vs;i<Ve+1;i++){
		pb=&g_GBuffer[0][0]+480*i*3+Hs*3;
		memcpy(pb,pt,lenH);
		pt+=lenH;
	}

}



/*------------------------------------------------------------------
write a block of data to LCD to refresh display
Hs---start of horizon,    He---end of horizon
Vs---start of vertical,    Ve---end of vertical
data -- point to RGB data
nb--- total bytes of RGB data

return
	>0 bytes written
	<0 fails
------------------------------------------------------------------------*/
int LCD_Write_Block(int Hs,int He, int Vs, int Ve, const uint8_t *data, int nb)
{
   int ret;
   int i;

   //------ set GRAM ZONE -------
   GRAM_Block_Set(Hs,He,Vs,Ve);

   //----- write data to GRAM -----
   //LCD_Write_Cmd(0x2c); //memory write
    LCD_Write_Cmd(0x3c); //continue memeory wirte

   DCXdata;
   //------ transfer data to ft232h  -------
   //   ret = ftdi_write_data(g_ftdi, g_usb_GBuffer, 480*320*3);
   ret = ftdi_write_data(g_ftdi, data, nb);
   if (ret < 0)
   {
          fprintf(stderr,"ftdi write failed!, error %d (%s)\n",ret, ftdi_get_error_string(g_ftdi));
   }
   else
	printf("ftdi succeed to write g_usb_GBuffer to ft232h \n");

   return ret;
}


/*-----------------------------------------------------------
write whole page of data in GBuffer to LCD to refresh display
return
	>0 bytes written
	<0 fails
------------------------------------------------------------*/
int LCD_Write_GBuffer(void)
{
   int ret;
   int i;

   //---full_area GRAM write,whole page refresh!!!!!!------
   GRAM_Block_Set(0,479,0,319);//column and page exchanged

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

 //----- set interface pixel format, default 24bit_data/18bit_color -----
 LCD_PxlFmt=PXLFMT_RGB888;
 LCD_Write_Cmd(0x3a);
 LCD_Write_Data(0b01100110);//[2:0]=110 24bit_data/18bit_color; [2:0]=101  16bit_data/16bit_color;
 //0b01010101-error,0b01110101-error,0b01110111-ok,0b01010111 ok,
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

 //----   backout  -----
 LCD_Write_GBuffer();

 printf("finish preparing ILI9488\n");
}


//----- set interface pixel format to 24bit_data/18bit_color  -----
void LCD_Set_PxlFmt24bit(void)
{
  LCD_PxlFmt=PXLFMT_RGB888;
  LCD_Write_Cmd(0x3a);
  LCD_Write_Data(0b01100110);//[2:0]=110 18bit color; [2:0]=101  16bit color;
//  delayms(50);
}

//----- set interface pixel format to 16bit_data/16bit_color -----
void LCD_Set_PxlFmt16bit(void)
{
  LCD_PxlFmt=PXLFMT_RGB565;
  LCD_Write_Cmd(0x3a);
  LCD_Write_Data(0b01100101);//[2:0]=110 18bit color; [2:0]=101  16bit color;
//  delayms(50);
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
void LCD_ColorBox(uint16_t xStart,uint16_t yStart,uint16_t xLong,uint16_t yLong, const uint8_t *color_buf)
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



#endif




