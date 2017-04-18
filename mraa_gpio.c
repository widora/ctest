#include "./include/mraa.h"
#include <stdio.h>
#include <unistd.h>

int main()
{
    //mraa_result rv; 
    mraa_gpio_context gpio;
    mraa_result_t res;

    const char* board_name = mraa_get_platform_name();
    fprintf(stdout, "hello mraa\n Version: %s\n Running on %s\n", mraa_get_version(), board_name);

    // rv = mraa_init();
    // if (rv != MRAA_SUCCESS)
    // {
        // printf("Error: mraa_init() error!\n");
        // //return 1;
    // }

    gpio = mraa_gpio_init(40);
    if (gpio == NULL)
    {
        printf("Error: mraa_gpio_init() error! \n");
        //return 1;
    }
    
    res = mraa_gpio_dir(gpio, MRAA_GPIO_OUT);
    if (res != MRAA_SUCCESS )
    {
        printf("Error: mraa_gpio_dir() error! %d\n", res);
        //return 1;
    }
    
    while(1)
    {
        mraa_gpio_write(gpio, 1);
        printf("write 1 \n");
        sleep(5);
        mraa_gpio_write(gpio, 0);
        printf("write 0 \n");
        sleep(5);
    }
    
    mraa_gpio_close(gpio);

    return 0;
}

