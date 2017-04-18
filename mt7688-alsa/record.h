/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#ifndef _RECORD_H_
#define _RECORD_H_


#include <stdbool.h>

#define  CHECK_FREQ              125    //-- use average energy in 1/CHECK_FREQ (s) to indicate noise level
#define  SAMPLE_RATE            8000 //--4k also OK
#define  CHECK_AVERG             2000 //--threshold value of wave amplitude to trigger record
#define  KEEP_AVERG                1800 //--threshold value of wave amplitude for keeping recording
#define  DELAY_TIME               5      //seconds -- recording time after one trigger
#define  MAX_RECORD_TIME   20   //seconds --max. record time in seconds
#define  MIN_SAVE_TIME        10   //seconds --min. recording time for saving, short time recording will be discarded.


//------------------- functions declaration ----------------------
bool device_open(int mode);
bool device_setparams();
bool device_capture();
bool device_play();
bool device_check_voice();
int record_main();


#endif
