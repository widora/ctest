/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include <stdio.h>
#include "mem_gpio.h"
#include "key.h"


//init GPIO port
void init_button_gpio()
{
    if (gpio_mmap())
        printf("gpio_mmap() error!\n");

    //printf("set pin BUTTON input 0\n");
    //mt76x8_gpio_set_pin_direction(BUTTON, 1);
    //mt76x8_gpio_set_pin_value(BUTTON, 0);

    mt76x8_gpio_set_pin_direction(BUTTON, 0);
    printf("get pin BUTTON input %d\n", mt76x8_gpio_get_pin(BUTTON));
}

int read_button()
{
    if (0 == mt76x8_gpio_get_pin(BUTTON))
    {
        return 0;   
    }
    else if (1 == mt76x8_gpio_get_pin(BUTTON))
    {
        return 1; 
    }
    //close(gpio_mmap_fd);

    return 0;
}

