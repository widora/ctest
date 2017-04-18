/************************************************************************************
*  Copyright (c), 2017, IMIO co.,LTD.
*            All rights reserved.
* Author : zyc
* Version: 1.00
* Date   : 2017.4.10
*************************************************************************************/
#include "./include/mraa.h"
#include <stdio.h>
#include <unistd.h>

// ------------------------------------------------------------
// IO IIC
// SCL  GPIO41
// SDA GPIO42
// ------------------------------------------------------------
#define SCL  41
#define SDA 42

#define high 1
#define low 0

#define Brightness 0xCF 
#define X_WIDTH 128
#define Y_WIDTH  64

mraa_gpio_context gpio0;
mraa_gpio_context gpio1;

void GPIO_Init()
{
    gpio0 = mraa_gpio_init(SCL);
    gpio1 = mraa_gpio_init(SDA);
    mraa_gpio_edge_mode(gpio0, MRAA_GPIO_EDGE_NONE);
    mraa_gpio_edge_mode(gpio1, MRAA_GPIO_EDGE_NONE);
    mraa_gpio_mode(gpio0, MRAA_GPIO_STRONG);
    mraa_gpio_mode(gpio1, MRAA_GPIO_STRONG);
    mraa_gpio_dir(gpio0, MRAA_GPIO_OUT);
    mraa_gpio_dir(gpio1, MRAA_GPIO_OUT);
}

/**********************************************
//IIC Start
**********************************************/
void IIC_Start()
{
    mraa_gpio_write(gpio0, 1);
    mraa_gpio_write(gpio1, 1);
    mraa_gpio_write(gpio1, 0);
    mraa_gpio_write(gpio0, 0);
}

/**********************************************
//IIC Stop
**********************************************/
void IIC_Stop()
{
    mraa_gpio_write(gpio0, 0);
    mraa_gpio_write(gpio1, 0);
    mraa_gpio_write(gpio0, 1);
    mraa_gpio_write(gpio1, 1);
}

/**********************************************
// ¨ª¡§1yI2C¡Á¨¹??D¡ä¨°???¡Á??¨²
**********************************************/
void Write_IIC_Byte(unsigned char IIC_Byte)
{
    unsigned char i;
    for(i=0;i<8;i++)
    {
        if(IIC_Byte & 0x80)
            mraa_gpio_write(gpio1, 1);
        else
            mraa_gpio_write(gpio1, 0);
        mraa_gpio_write(gpio0, 1);
        mraa_gpio_write(gpio0, 0);
        IIC_Byte<<=1;
    }
    mraa_gpio_write(gpio1, 1);
    mraa_gpio_write(gpio0, 1);
    mraa_gpio_write(gpio0, 0);
}

/*********************OLEDD¡ä¨ºy?Y************************************/ 
void OLED_WrDat(unsigned char IIC_Data)
{
    IIC_Start();
    Write_IIC_Byte(0x78);
    Write_IIC_Byte(0x40);            //write data
    Write_IIC_Byte(IIC_Data);
    IIC_Stop();
}
/*********************OLEDD¡ä?¨¹¨¢?************************************/
void OLED_WrCmd(unsigned char IIC_Command)
{
    IIC_Start();
    Write_IIC_Byte(0x78);            //Slave address,SA0=0
    Write_IIC_Byte(0x00);            //write command
    Write_IIC_Byte(IIC_Command);
    IIC_Stop();
}

int main()
{
    mraa_init();
    printf("1\n");
    gpio0 = mraa_gpio_init(SCL);
    gpio1 = mraa_gpio_init(SDA);
    printf("2\n");
    mraa_gpio_edge_mode(gpio0, MRAA_GPIO_EDGE_NONE);
    mraa_gpio_edge_mode(gpio1, MRAA_GPIO_EDGE_NONE);
    printf("3\n");
    mraa_gpio_mode(gpio0, MRAA_GPIO_STRONG);
    mraa_gpio_mode(gpio1, MRAA_GPIO_STRONG);
    printf("4\n");
    mraa_gpio_dir(gpio0, MRAA_GPIO_OUT);
    mraa_gpio_dir(gpio1, MRAA_GPIO_OUT);
    printf("5\n");

    while(1)
    {
        mraa_gpio_write(gpio0, 1);
        mraa_gpio_write(gpio1, 1);
        sleep(3);
        mraa_gpio_write(gpio1, 0);
        mraa_gpio_write(gpio0, 0);
    }
    
    return 0;
}

