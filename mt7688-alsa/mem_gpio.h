/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#ifndef _MEM_GPIO_H_
#define _MEM_GPIO_H_


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


#define GPIO1_MODE 0x60


// extern uint8_t* gpio_mmap_reg = NULL;
// extern int gpio_mmap_fd = 0;


int gpio_mmap(void);
int mt76x8_gpio_get_pin(int);
void mt76x8_gpio_set_pin_direction(int, int);
void mt76x8_gpio_set_pin_value(int, int);
void close_gpio();


#endif
