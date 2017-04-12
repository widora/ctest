/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "codetab.h"

#define MMAP_PATH    "/dev/mem"


#define RALINK_GPIO_DIR_IN        0
#define RALINK_GPIO_DIR_OUT        1

#define RALINK_REG_PIOINT        0x690
#define RALINK_REG_PIOEDGE        0x6A0
#define RALINK_REG_PIORENA        0x650
#define RALINK_REG_PIOFENA        0x660
#define RALINK_REG_PIODATA        0x620
#define RALINK_REG_PIODIR        0x600
#define RALINK_REG_PIOSET        0x630
#define RALINK_REG_PIORESET        0x640

#define RALINK_REG_PIO6332INT        0x694
#define RALINK_REG_PIO6332EDGE        0x6A4
#define RALINK_REG_PIO6332RENA        0x654
#define RALINK_REG_PIO6332FENA        0x664
#define RALINK_REG_PIO6332DATA        0x624
#define RALINK_REG_PIO6332DIR        0x604
#define RALINK_REG_PIO6332SET        0x634
#define RALINK_REG_PIO6332RESET        0x644

#define RALINK_REG_PIO9564INT        0x698
#define RALINK_REG_PIO9564EDGE        0x6A8
#define RALINK_REG_PIO9564RENA        0x658
#define RALINK_REG_PIO9564FENA        0x668
#define RALINK_REG_PIO9564DATA        0x628
#define RALINK_REG_PIO9564DIR        0x608
#define RALINK_REG_PIO9564SET        0x638
#define RALINK_REG_PIO9564RESET        0x648


static uint8_t* gpio_mmap_reg = NULL;
static int gpio_mmap_fd = 0;

static int gpio_mmap(void)
{
    if ((gpio_mmap_fd = open(MMAP_PATH, O_RDWR)) < 0) 
    {
        fprintf(stderr, "unable to open mmap file");
        return -1;
    }

    gpio_mmap_reg = (uint8_t*) mmap(NULL, 1024, PROT_READ | PROT_WRITE,
        MAP_FILE | MAP_SHARED, gpio_mmap_fd, 0x10000000);
    if (gpio_mmap_reg == MAP_FAILED) 
    {
        perror("foo");
        fprintf(stderr, "failed to mmap");
        gpio_mmap_reg = NULL;
        close(gpio_mmap_fd);
        return -1;
    }

    return 0;
}

int mt76x8_gpio_get_pin(int pin)
{
    uint32_t tmp = 0;

    /* MT7621, MT7628 */
    if (pin <= 31) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIODATA);
        tmp = (tmp >> pin) & 1u;
    } 
    else if (pin <= 63) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332DATA);
        tmp = (tmp >> (pin-32)) & 1u;
    } 
    else if (pin <= 95) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564DATA);
        tmp = (tmp >> (pin-64)) & 1u;
        tmp = (tmp >> (pin-24)) & 1u;
    }
    return tmp;

}

void mt76x8_gpio_set_pin_direction(int pin, int is_output)
{
    uint32_t tmp;

    /* MT7621, MT7628 */
    if (pin <= 31) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIODIR);
        if (is_output)
            tmp |=  (1u << pin);
        else
            tmp &= ~(1u << pin);
        *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIODIR) = tmp;
    } 
    else if (pin <= 63) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332DIR);
        if (is_output)
            tmp |=  (1u << (pin-32));
        else
            tmp &= ~(1u << (pin-32));
        *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332DIR) = tmp;
    } 
    else if (pin <= 95) 
    {
        tmp = *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564DIR);
        if (is_output)
            tmp |=  (1u << (pin-64));
        else
            tmp &= ~(1u << (pin-64));
        *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564DIR) = tmp;
    }
}

void mt76x8_gpio_set_pin_value(int pin, int value)
{
    uint32_t tmp;

    /* MT7621, MT7628 */
    if (pin <= 31) 
    {
        tmp = (1u << pin);
        if (value)
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIOSET) = tmp;
        else
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIORESET) = tmp;
    } 
    else if (pin <= 63) 
    {
        tmp = (1u << (pin-32));
        if (value)
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332SET) = tmp;
        else
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO6332RESET) = tmp;
    } 
    else if (pin <= 95) 
    {
        tmp = (1u << (pin-64));
        if (value)
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564SET) = tmp;
        else
            *(volatile uint32_t *)(gpio_mmap_reg + RALINK_REG_PIO9564RESET) = tmp;
    }
}



/*********************************************
*GPIO simulation IIC
**********************************************/
#define SCL 41
#define SDA 42
#define high 1
#define low 0

#define  Brightness 0xCF
#define  X_WIDTH   128
#define  Y_WIDTH   64

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
        mt76x8_gpio_set_pin_value(SCL, 1);
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
    OLED_WrCmd((x&0x0f)|0x01);
} 

/*********************OLED fill screen************************************/
void OLED_Fill(unsigned char bmp_dat) 
{
    unsigned char y,x;
    for(y=0;y<8;y++)
    {
        OLED_WrCmd(0xb0+y);
        OLED_WrCmd(0x01);
        OLED_WrCmd(0x10);
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
        OLED_WrCmd(0x01);
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


int main(int argc, char **argv)
{
    int ret = -1;
    unsigned char i;

    init_gpio();
    OLED_Init(); //OLED init

    // OLED_Fill(0xff); //
    // sleep(5);
    // OLED_Fill(0x00); //
    // sleep(5);

    while (1)
    {
        printf("Begin! \n");    
        OLED_P16x16Ch(24,0,1);
        OLED_P16x16Ch(40,0,2);
        OLED_P16x16Ch(57,0,3);
        OLED_P16x16Ch(74,0,4);
        OLED_P16x16Ch(91,0,5);
        for(i=0; i<8; i++)//
        {
//            OLED_P16x16Ch(i*16,0,i);
             OLED_P16x16Ch(i*16,2,i+8);
             OLED_P16x16Ch(i*16,4,i+16);
             OLED_P16x16Ch(i*16,6,i+24);
        }
        sleep(4);
        OLED_CLS();//

        OLED_P8x16Str(0,0,"HelTec");//delay
        OLED_P8x16Str(0,2,"OLED Display");
        OLED_P8x16Str(0,4,"www.heltec.cn");
        OLED_P6x8Str(0,6,"cn.heltec@gmail.com");
        OLED_P6x8Str(0,7,"heltec.taobao.com");
        sleep(4);
        OLED_CLS();

        Draw_BMP(0,0,128,8,BMP1);  //
        sleep(8);
        Draw_BMP(0,0,128,8,BMP2);
        sleep(8);
        OLED_CLS();
    }
    close(gpio_mmap_fd);

    return ret;
}
