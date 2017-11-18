/*-------------------------------------------

GPIO pinctrl set for motor control

----------------------------------------------*/

#include "./mygpio.h"

#define  PIN_MOTOR_DIRECTION 40//14 //GPIO pin for motor direction control
#define  PIN_MOTOR_EMERGSTOP 39//15 //GPIO pin for motor emergency stop


/* --------- motor running direction control macro. --------- */
#define SET_RUNDIR_FORWARD mt76x8_gpio_set_pin_value(PIN_MOTOR_DIRECTION,1)
#define SET_RUNDIR_REVERSE mt76x8_gpio_set_pin_value(PIN_MOTOR_DIRECTION,0) // set 0 to reserse

/* --------- motor emergency stop control macro. --------- */
#define ACTIVATE_EMERG_STOP mt76x8_gpio_set_pin_value(PIN_MOTOR_EMERGSTOP,0) //set 0 to activate EMERG. STOP
#define DEACTIVATE_EMERG_STOP mt76x8_gpio_set_pin_value(PIN_MOTOR_EMERGSTOP,1)


/*------------------------------------------------------
1. mmap control GPIO.
2. set direction of control_pins as output.
------------------------------------------------------*/
void Prepare_CtlPins()
{
 if(gpio_mmap()){
    	printf("gpio_mmap failed!");
	return;
  }

  //------ SET GPIO DIRCTION AS OUTPUT  ---------
  mt76x8_gpio_set_pin_direction(PIN_MOTOR_DIRECTION,1);
  mt76x8_gpio_set_pin_direction(PIN_MOTOR_EMERGSTOP,1);

}


/*------ release GPIO  mmap -----------*/
void Release_CtlPints()
{
  close(gpio_mmap_fd);
}

