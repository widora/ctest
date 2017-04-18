/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "mem_gpio.h"
#include "key.h"
#include "oled_iic.h"
#include "record.h"
#include "cJSON.h"
#include "http_client.h"


//extern oled_iic.c variable
extern unsigned char BMP1[];
extern unsigned char BMP2[];
extern unsigned char BMP3[];
extern unsigned char BMP4[];

int scan_button = 1;
int key_num = 1;

/***************************************************************
* test c 
***************************************************************/
void test_button()
{
    init_button_gpio();

    while (1)
    {
        if (0 == read_button())
        {
            printf("Button not push ! \n");    
        }
        if (1 == read_button())
        {
            printf("Button push! \n");    
        }
        sleep(2);
    }

    close_gpio();
}

void test_oled()
{
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

    close_gpio();
}

void test_cjson()
{
    char *product_id = "mt7687_a";
    char *dev_id = "zhuyunchun-1a";
    char *dev_key = "b23c34bacedd3548435d92d405a88afb";
    char *out;

    cJSON *root_reg;
    cJSON *signup_req;
    root_reg = cJSON_CreateObject();
    cJSON_AddItemToObject(root_reg, "signin_req", signup_req = cJSON_CreateObject());
    cJSON_AddStringToObject(signup_req, "product_id", product_id);
    cJSON_AddStringToObject(signup_req, "dev_id", dev_id);
    cJSON_AddStringToObject(signup_req, "dev_key", dev_key);
    out = cJSON_Print(root_reg); /* Print to text */
    cJSON_Delete(root_reg);      /* Delete the cJSON object */
    printf("The json date out = %s\n", out);
}

void display_oled()
{ 
    unsigned char i;

    OLED_P8x16Str(0,0,"Processing...");//delay

    while (key_num)
    {
        //printf("Begin! \n");    
        for (i = 0; i < 102; i += 2) //26 128
        {
            Draw_BMP(i, 4, 128 + i, 5, BMP3);  //64 8*8
            usleep(20000);//20ms
            //OLED_CLS();
        }
        for (i = 102; i > 0; i -= 2) //26 128
        {
            Draw_BMP(i, 4, 128 + i, 5, BMP3);  //64 8*8
            usleep(20000);//20ms
            //OLED_CLS();
        }
        //key_num--;
    }
    
    OLED_CLS();
    key_num = 1;
    scan_button = 1;
    pthread_exit(0);
    //close_gpio();
}

void thread_rec()
{
    //record_main();
    sleep(4);
    key_num = 0;
    pthread_exit(0);
}

void thread_key()
{
    pthread_t id_dis, id_rec;
    int ret_dis, ret_rec;
    
    while (1)
    {
        if(scan_button)
        {
            if (0 == read_button())
            {
                printf("Button not push ! \n");    
                OLED_CLS();
                //sleep(2);
            }
            else if (1 == read_button())
            {
                printf("Button push ! \n");    
                scan_button = 0;
                ret_dis = pthread_create(&id_dis, NULL, (void *)display_oled, NULL);
                if (ret_dis != 0)
                {
                    printf("Create the pthread error!\n");
                }
                ret_rec = pthread_create(&id_rec, NULL, (void *)thread_rec, NULL);
                if (ret_rec != 0)
                {
                    printf("Create the pthread error!\n");
                }
            }
        }
        //printf("thread_key()\n");
        usleep(20000);//sleep 20ms
    }
    
    pthread_exit(0);
}

/*========================= MAIN ====================================*/
int main(int argc, char* argv[])
{
    pthread_t id;
    int ret;

    //init func
    init_gpio();
    init_button_gpio();
    OLED_Init(); //OLED init
    OLED_CLS();//
    
    ret = pthread_create(&id, NULL, (void *)thread_key, NULL);
    if (ret != 0)
    {
        printf("Create the pthread error!\n");
    }
        
   while(1)
   {
        sleep(1);
        printf("I am main thread!\n");
   }

    

    return 0;
}
