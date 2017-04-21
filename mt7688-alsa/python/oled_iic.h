/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#ifndef _OLED_IIC_H_
#define _OLED_IIC_H_


/*********************************************
*GPIO simulation IIC
**********************************************/
#define SCL 41
#define SDA 42
#define high 1
#define low 0

#define  Brightness    0xCF
#define  X_WIDTH   128
#define  Y_WIDTH   64


void init_gpio();
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_Fill(unsigned char bmp_dat);
void OLED_CLS(void);
void OLED_Init(void);
void OLED_P6x8Str(unsigned char x, unsigned char y,unsigned char ch[]);
void OLED_P8x16Str(unsigned char x, unsigned char y,unsigned char ch[]);
void OLED_P16x16Ch(unsigned char x, unsigned char y, unsigned char N);
void Draw_BMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1,unsigned char BMP[]);



#endif
