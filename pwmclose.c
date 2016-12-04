#include "./include/mraa.h"
#include <stdio.h>
int main(int argc,char *argv[])
{
  int pin;

  if (argc <2)
    printf("please enter GPIO number(18,19,20,21)!\n");

  pin=atoi(argv[1]);
  mraa_result_t res;
  mraa_pwm_context pwm;
  printf("pin=%d\n",pin);
  pwm=mraa_pwm_init(pin);
  res=mraa_pwm_close(pwm);
  printf("res=%x \n",res);
  
  return 0;

}

