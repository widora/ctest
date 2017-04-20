/*---------------------------------------------------------------
 use MRAA c lib to set and control PWM
 compile with lib: openwrt_gcc -L include -lmraa pwmtest_32M.c 
------------------------------------------------------------------*/
#include "include/mraa.h"
#include  <stdio.h>
#include  <stdlib.h>

typedef enum {   // pin number 18,19,20,21
        GPIO18=18,
        GPIO19=19,
        GPIO20=20,
        GPIO21=21} GPIO_pin;

int main(int argc,char* argv[])
{

  int perus=1000;// period us
  float pers; // period  s
  float tmp_duty=0;
  float duty=0.5; // duty percentage
  GPIO_pin pin=20;
  int temp;
  int Max_Period_Us,Min_Period_Us; 

  printf("Please enter:\n   GPIO number----(18,19,20,21 defaul=20)\
  \n   period----(us, default=1000us)\n   Duty----(0~1, default=0.5)!\n");
  printf("Periods 1us ~ 26999us  \n");

 if(argc>1)
  {
     temp=atoi(argv[1]);
    if(temp<18 || temp>21)
      printf("GPIO number error!");
    else
     pin=temp;
  }

 if(argc>2)
  perus=atoi(argv[2]);

 if(argc>3)
  tmp_duty=atof(argv[3]);

  if(tmp_duty>=0 && tmp_duty<=1)
    duty=tmp_duty;

  mraa_result_t res;
  mraa_pwm_context pwm;

 
  pwm=mraa_pwm_init(pin); 
  if(pwm == NULL) printf("mraa_pwm_init fail!\n");
  res=mraa_pwm_owner(pwm,0);
  if(res ==MRAA_SUCCESS)printf("mraa_pwm_owner() succeed!\n");
  mraa_pwm_enable(pwm,1);
  mraa_pwm_write(pwm,0); //---set duty to  0 first
  
   if(perus<=1000)
    res=mraa_pwm_period_us(pwm,perus); //-- us MAX 26214us
   if(perus>=20000)
    {
     pers=perus/1000000.0;
     printf("pers=%9.6f\n",pers);
     res=mraa_pwm_period(pwm,pers);  //-- s
    }
   if(perus>1000 && perus<20000)
    res=mraa_pwm_period_ms(pwm,perus/1000);  //-- ms MAX 26ms
   
  if(res!=0)
     printf("Period set fail!\n");
  else
     printf("Period:%dus set succeed!\n",perus);

  res=mraa_pwm_write(pwm,duty);
  if(res!=0)
    printf("Duty set fail!\n");
  else
    printf("Duty:%4.3f set succeedy!\n",duty);

  return 0;
}
