/*------------------------------------------------

GPIO pinctrl set for DC motor driver module L298N

------------------------------------------------*/
#include "mygpio.h"

#define  PIN1_DIRECTION 40// GPIO pin for motor direction control IN1
#define  PIN2_DIRECTION 39// GPIO pin for motor direction control IN2

/*--------------- L298N Module --------------
ENA/ENAB	IN1	IN2
	1	0	0	BRAKE
	1	0	1	FORWARD
	1	1	0	REVERSE
	1	1	1	BRAKE
--------------------------------------------*/

/* --------- motor running direction control macro. --------- */
#define SET_MOTOR_FORWARD  do{\
				mt76x8_gpio_set_pin_value(PIN1_DIRECTION,1);\
				mt76x8_gpio_set_pin_value(PIN2_DIRECTION,0);\
			    }while(0)

#define SET_MOTOR_REVERSE  do{\
				mt76x8_gpio_set_pin_value(PIN1_DIRECTION,0);\
				mt76x8_gpio_set_pin_value(PIN2_DIRECTION,1);\
			    }while(0)

#define SET_MOTOR_BRAKE  do{\
				mt76x8_gpio_set_pin_value(PIN1_DIRECTION,0);\
				mt76x8_gpio_set_pin_value(PIN2_DIRECTION,0);\
			    }while(0)

/*------------------------------------------------------
1. mmap control GPIO.
2. set direction of control_pins as output.
Return:
	0  --- OK
	<0 --- Fails
------------------------------------------------------*/
int Prepare_CtlPins(void)
{
 if(gpio_mmap()){
    	printf("gpio_mmap failed!");
	return -1;
  }
  //------ SET GPIO DIRCTION AS OUTPUT  ---------
  mt76x8_gpio_set_pin_direction(PIN1_DIRECTION, RALINK_GPIO_DIR_OUT); //0-input, 1-output
  mt76x8_gpio_set_pin_direction(PIN2_DIRECTION, RALINK_GPIO_DIR_OUT);
  printf(" finish preparing GPIO pins for motor direction control ...\n");

  return 0;
}


/*------ release GPIO  mmap -----------*/
void Release_CtlPints()
{
  close(gpio_mmap_fd);
}

