//////////////////////////////////////////////////////////////////////////////////     
//Copyright(C) IMIO
//All rights reserved
//Author: zyc 
//Data   : 2017/04/07
//******************************************************************************/
#ifndef _OLED_H_
#define _OLED_H_

#include <stdio.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <sys/ioctl.h>

//#define I2C_SCL I2C_SCLK;
//#define I2C_SDA I2C_SD;

#define  u8 unsigned char 
#define  u32 unsigned int 
#define OLED_CMD  0
#define OLED_DATA 1

#define SIZE 16
#define XLevelL        0x02
#define XLevelH        0x10
#define Max_Column    128
#define Max_Row        64
#define    Brightness    0xFF 
#define X_WIDTH     128
#define Y_WIDTH     64            

//OLED
void OLED_WR_Byte(unsigned dat,unsigned cmd);  
void OLED_Display_On(void);
void OLED_Display_Off(void);  
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 Char_Size);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size);
void OLED_ShowString(u8 x,u8 y, u8 *p,u8 Char_Size);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowCHinese(u8 x,u8 y,u8 no);
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);
void fill_picture(unsigned char fill_Data);
void Picture();
//void IIC_Start();
//void IIC_Stop();
void Write_IIC_Command(unsigned char IIC_Command);
void Write_IIC_Data(unsigned char IIC_Data);
void Write_IIC_Byte(unsigned char IIC_Byte);
void IIC_Wait_Ack();

#endif
