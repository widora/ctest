/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#ifndef _KEY_H_
#define _KEY_H_


#define BUTTON 40


void init_button_gpio();
int read_button();

#endif
