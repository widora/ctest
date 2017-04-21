/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "main.h"
#include "mem_gpio.h"
#include "key.h"
#include "oled_iic.h"
#include "record.h"


//extern oled_iic.c variable
extern unsigned char BMP1[];
extern unsigned char BMP2[];
extern unsigned char BMP3[];
extern unsigned char BMP4[];

int display_time;

/***************************************************************/
int get_button()
{
    int ret;
    init_button_gpio();

    if (0 == read_button())
    {
        printf("Button not push ! \n");    
        ret = 0;
    }
    if (1 == read_button())
    {
        printf("Button push! \n");    
        ret = 1;
    }

    //close_gpio();
    
    return ret;
}

int display_oled()
{ 
    unsigned char i;
    int num;

    init_gpio();
    OLED_Init();
    OLED_P8x16Str(0,0,"Processing...");//delay

    while (display_time)
    {  
        for (i = 0; i < 102; i += 2) //26 128
        {
            Draw_BMP(i, 4, 128 + i, 5, BMP3);  //64 8*8
            usleep(20000);//20ms
        }
        for (i = 102; i > 0; i -= 2) //26 128
        {
            Draw_BMP(i, 4, 128 + i, 5, BMP3);  //64 8*8
            usleep(20000);//20ms
        }
        //sleep(1);
        OLED_CLS();
    }
    
    OLED_CLS();
    //close_gpio();

    pthread_exit(0);
    //return 1;
}

int thread_rec()
{
    pthread_t id_dis;
    int ret, ret_dis;

    display_time = 1;
    printf("thread_rec() begin ! \n");

    ret_dis = pthread_create(&id_dis, NULL, (void *)display_oled, NULL);
    if (ret_dis != 0)
    {
        printf("Create the pthread_dis error!\n");
    }

    ret = record_main();
    if (ret < 0)
    {
        printf("record_main() error ! \n");
        return 0;
    }
    display_time = 0;
    sleep(1); 
    init_gpio();
    OLED_Init();
    OLED_CLS();
    printf("thread_rec() finish ! \n");

    return 1;
}

/*========================= MAIN ====================================*/
int main(int argc, char* argv[])
{

        
   while(1)
   {
        sleep(2);
        printf("I am main thread!\n");
   }

    return 0;
}
