/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include <stdio.h>
#include "oled_iic.h"
#include "mem_gpio.h"
#include "codetab.h"


//init GPIO port
void init_gpio()
{
    if (gpio_mmap())
        printf("gpio_mmap() error!\n");

    printf("get pin SCL input %d\n", mt76x8_gpio_get_pin(SCL));
    printf("get pin SDA input %d\n", mt76x8_gpio_get_pin(SDA));

    printf("set pin SCL output 0\n");
    mt76x8_gpio_set_pin_direction(SCL, 1);
    mt76x8_gpio_set_pin_value(SCL, 0);
    printf("set pin SDA output 0\n");
    mt76x8_gpio_set_pin_direction(SDA, 1);
    mt76x8_gpio_set_pin_value(SDA, 0);
}


/**********************************************
//IIC Start
**********************************************/
void IIC_Start()
{
    mt76x8_gpio_set_pin_value(SCL, 1);
    mt76x8_gpio_set_pin_value(SDA, 1);
    mt76x8_gpio_set_pin_value(SDA, 0);
    mt76x8_gpio_set_pin_value(SCL, 0);
}

/**********************************************
//IIC Stop
**********************************************/
void IIC_Stop()
{
    mt76x8_gpio_set_pin_value(SCL, 0);
    mt76x8_gpio_set_pin_value(SDA, 0);
    mt76x8_gpio_set_pin_value(SCL, 1);
    mt76x8_gpio_set_pin_value(SDA, 1);
}

void IIC_Ack()
{
    mt76x8_gpio_set_pin_value(SCL, 0);
    mt76x8_gpio_set_pin_value(SDA, 0);
    mt76x8_gpio_set_pin_value(SCL, 1);
    mt76x8_gpio_set_pin_value(SCL, 0);
}

/**********************************************
// iic bus write a byte
**********************************************/
void Write_IIC_Byte(unsigned char IIC_Byte)
{
    unsigned char t;
    mt76x8_gpio_set_pin_value(SCL, 0);

    for (t = 0; t < 8; t++)
    {
        mt76x8_gpio_set_pin_value(SDA, (IIC_Byte&0x80)>>7);
        IIC_Byte<<=1;
        mt76x8_gpio_set_pin_value(SCL, 1);
        mt76x8_gpio_set_pin_value(SCL, 1);//!!! Import Here
        //usleep(1);
        mt76x8_gpio_set_pin_value(SCL, 0);
    }
}

/*********************OLED write date************************************/ 
void OLED_WrDat(unsigned char IIC_Data)
{
    IIC_Start();
    Write_IIC_Byte(0x78);
    IIC_Ack();
    Write_IIC_Byte(0x40);            //write data
    IIC_Ack();
    Write_IIC_Byte(IIC_Data);
    IIC_Ack();
    IIC_Stop();
}
/*********************OLED write command*********************************/
void OLED_WrCmd(unsigned char IIC_Command)
{
    IIC_Start();
    Write_IIC_Byte(0x78);            //Slave address,SA0=0
    IIC_Ack();
    Write_IIC_Byte(0x00);            //write command
    IIC_Ack();
    Write_IIC_Byte(IIC_Command);
    IIC_Ack();
    IIC_Stop();
}

/*********************OLED set postion************************************/
void OLED_Set_Pos(unsigned char x, unsigned char y) 
{ 
    OLED_WrCmd(0xb0+y);
    OLED_WrCmd(((x&0xf0)>>4)|0x10);
    OLED_WrCmd((x&0x0f)|0x02); //!!! Note Here 01 cannot dispaly perfact !!!
} 

/*********************OLED fill screen************************************/
void OLED_Fill(unsigned char bmp_dat) 
{
    unsigned char y,x;
    for(y=0;y<8;y++)
    {
        OLED_WrCmd(0xb0+y);
        OLED_WrCmd(0x02);//low column start address //!!! Note Here 01 cannot dispaly perfact !!!
        OLED_WrCmd(0x10);//high column start address
        for(x=0;x<X_WIDTH;x++)
        {
            OLED_WrDat(bmp_dat);
        }
    }
}

/*********************OLED clear************************************/
void OLED_CLS(void)
{
    unsigned char y,x;
    for(y=0;y<8;y++)
    {
        OLED_WrCmd(0xb0+y);
        OLED_WrCmd(0x02); //!!! Note Here 01 cannot dispaly perfact !!!
        OLED_WrCmd(0x10);
        for(x=0;x<X_WIDTH;x++)
        {
            OLED_WrDat(0);
        }
    }
}

/*********************OLED init¡¥************************************/
void OLED_Init(void)
{
    usleep(100000);//it's import for oled for delay a while
    OLED_WrCmd(0xae);//--turn off oled panel
    OLED_WrCmd(0x00);//---set low column address
    OLED_WrCmd(0x10);//---set high column address
    OLED_WrCmd(0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
    OLED_WrCmd(0x81);//--set contrast control register
    OLED_WrCmd(Brightness); // Set SEG Output Current Brightness
    OLED_WrCmd(0xa1);//--Set SEG/Column Mapping     0xa0 <> 0xa1 normal
    OLED_WrCmd(0xc8);//Set COM/Row Scan Direction   0xc0  0xc8 normal
    OLED_WrCmd(0xa6);//--set normal display
    OLED_WrCmd(0xa8);//--set multiplex ratio(1 to 64)
    OLED_WrCmd(0x3f);//--1/64 duty
    OLED_WrCmd(0xd3);//-set display offset    Shift Mapping RAM Counter (0x00~0x3F)
    OLED_WrCmd(0x00);//-not offset
    OLED_WrCmd(0xd5);//--set display clock divide ratio/oscillator frequency
    OLED_WrCmd(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
    OLED_WrCmd(0xd9);//--set pre-charge period
    OLED_WrCmd(0xf1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    OLED_WrCmd(0xda);//--set com pins hardware configuration
    OLED_WrCmd(0x12);
    OLED_WrCmd(0xdb);//--set vcomh
    OLED_WrCmd(0x40);//Set VCOM Deselect Level
    OLED_WrCmd(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
    OLED_WrCmd(0x02);//
    OLED_WrCmd(0x8d);//--set Charge Pump enable/disable
    OLED_WrCmd(0x14);//--set(0x10) disable
    OLED_WrCmd(0xa4);// Disable Entire Display On (0xa4/0xa5)
    OLED_WrCmd(0xa6);// Disable Inverse Display On (0xa6/a7) 
    OLED_WrCmd(0xaf);//--turn on oled panel
    OLED_Fill(0x00); //clear 
    OLED_Set_Pos(0,0);
} 

/***************display a 6*8 ASCII charactertic    postion x,y y 0-7****************/
void OLED_P6x8Str(unsigned char x, unsigned char y,unsigned char ch[])
{
    unsigned char c=0,i=0,j=0;

    while (ch[j]!='\0')
    {
        c =ch[j]-32;
        if(x>126) {x=0;y++;}
        OLED_Set_Pos(x,y);
        for(i=0;i<6;i++)
        {
            OLED_WrDat(F6x8[c][i]);
        }
        x+=6;
        j++;
    }
}

/*******************display a 8*16 ASCII charactertic    postion x,y y 0-7****************/
void OLED_P8x16Str(unsigned char x, unsigned char y,unsigned char ch[])
{
    unsigned char c=0,i=0,j=0;

    while (ch[j]!='\0')
    {
        c =ch[j]-32;
        if(x>120){x=0;y++;}
        OLED_Set_Pos(x,y);
        for(i=0;i<8;i++)
        {
            OLED_WrDat(F8X16[c*16+i]);
        }
        OLED_Set_Pos(x,y+1);
        for(i=0;i<8;i++)
        {
            OLED_WrDat(F8X16[c*16+i+8]);
        }
        x+=8;
        j++;
    }
}
/*****************display a 16*16 ASCII charactertic    postion x,y y 0-7****************************/
void OLED_P16x16Ch(unsigned char x, unsigned char y, unsigned char N)
{
    unsigned char wm=0;
    unsigned int adder=32*N;
    OLED_Set_Pos(x , y);
    for(wm = 0;wm < 16;wm++)
    {
        OLED_WrDat(F16x16[adder]);
        adder += 1;
    }
    OLED_Set_Pos(x,y + 1);
    for(wm = 0;wm < 16;wm++)
    {
        OLED_WrDat(F16x16[adder]);
        adder += 1;
    }           
}

/***********display a BMP 128*64 (x,y), x 0-127 y 0-7*****************/
void Draw_BMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1,unsigned char BMP[])
{
    unsigned int j=0;
    unsigned char x,y;

    if(y1%8==0) 
        y=y1/8;      
    else 
        y=y1/8+1;
    for(y=y0;y<y1;y++)
    {
        OLED_Set_Pos(x0,y);
        for(x=x0;x<x1;x++)
        {      
            OLED_WrDat(BMP[j++]);
        }
    }
}


