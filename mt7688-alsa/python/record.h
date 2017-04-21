/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#ifndef _RECORD_H_
#define _RECORD_H_


#include <stdbool.h>


//------------------- functions declaration ----------------------
bool device_open(int mode);
bool device_setparams();
bool device_capture();
bool device_play();
bool device_check_voice();
int record_main();


#endif
