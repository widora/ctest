/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#ifndef _MAIN_H_
#define _MAIN_H_


//extern int display_time;
//./config --prefix=/home/zyc/Documents/openssl-1.1.0e/build --cross-compile-prefix=mipsel-openwrt-linux-uclibc- shared no-asm
//------------------- functions declaration ---------------------- 
int get_button();
void clear_display();
int test_display_oled();
int display_oled();
int thread_rec();
void clear_display();
void display_oled_string();

#endif