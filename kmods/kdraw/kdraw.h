/*********************************************
       GPIO 14-21 INTERRUPT PURPOSE SET

NOTE:
1. Valify_gpio_num() first before call other functions!!!

***********************************************/

#include <linux/gpio.h>  //----- gpio_to_irq()
#include <linux/interrupt.h> //---request_irq()
//#include <linux/workqueue.h>

//#define  GPIO_PIN_NUM 17
#define  GET_GINT_STATUS(pin_num)  (((*GINT_STAT_0)>>(pin_num))&0x1)  // --get GPIO Interrupt status,1 confirmed, 0 not

volatile unsigned long *GPIO_CTRL_0;   //--- GPIO0 to GPIO31 direction control register  0-input 1-output
volatile unsigned long *GPIO_DATA_0;   //-----GPIO0 to GPIO31 data register 
volatile unsigned long *GPIO_POL_0;   //---GPIO0 to GPIO31 polarity control register
volatile unsigned long *GINT_REDGE_0;  //--GPIO0 to GPIO31 rising edge interrupt enable register
volatile unsigned long *GINT_FEDGE_0;  //--GPIO0 to GPIO31 falling edge interrupt enable register
volatile unsigned long *GINT_STAT_0;  //---GPIO0 to GPIO31 interrupt status register 1-int  0 -no int
volatile unsigned long *GINT_EDGE_0;  //---GPIO0 to GPIO31 interrupt edge status register 1-rising 0-falling
volatile unsigned long *GPIO1_MODE; // GPIO1 purpose selection register,for SPIS or GPIO14-17 mode selection
volatile unsigned long *AGPIO_CFG; // analog GPIO configuartion,GPIO14-17 purpose 

/*------------------    map GPIO register      --------------------*/
void map_gpio_register(void)
{
   GPIO1_MODE=(volatile unsigned long *)ioremap(0x10000060,4); // GPIO1 purpose selection register,for SPIS or GPIO14-17 mode selection
   GPIO_DATA_0=(volatile unsigned long *)ioremap(0x10000620,4);   //--- GPIO0 to GPIO31 data register  
   //GPIO_POL_0=(volatile unsigned long *)ioremap(0x10000610,4); //--GPIO0 to GPIO31 plarity control register    
   //AGPIO_CFG=(volatile unsigned long *)ioremap(0x1000003c,4); // analog GPIO configuartion,GPIO14-17 purpose 
   GPIO_CTRL_0=(volatile unsigned long *)ioremap(0x10000600,4);   //--- GPIO0 to GPIO31 direction control register  0-input 1-output
   GINT_REDGE_0=(volatile unsigned long *)ioremap(0x10000650,4);  //--GPIO0 to GPIO31 rising edge interrupt enable register
   GINT_FEDGE_0=(volatile unsigned long *)ioremap(0x10000660,4);  //--GPIO0 to GPIO31 falling edge interrupt enable register
   GINT_STAT_0=(volatile unsigned long *)ioremap(0x10000690,4);  //---GPIO0 to GPIO31 interrupt status register 1-int  0 -no int
   //GINT_EDGE_0;  //---GPIO0 to GPIO31 interrupt edge status register 1-rising 0-falling
}

/*------------------    ummap GPIO register      --------------------*/
void unmap_gpio_register(void)
{
     iounmap(GPIO1_MODE);
     iounmap(GPIO_DATA_0);
     iounmap(GPIO_CTRL_0);
     iounmap(GINT_REDGE_0);
     iounmap(GINT_FEDGE_0);
     iounmap(GINT_STAT_0);
}

/*------------------    validate GPIO Number ------------------*/
static int verify_gpio_num(unsigned int GPIO_PIN_NUM)
{
//------!!!! USE PIN NUMBER 14-21 ONLY, SET AS GPIO PURPOSE BY DEFAULT  !!!!
	if(GPIO_PIN_NUM<14|GPIO_PIN_NUM>21)return 0;
        else
        	return 1;
}

/*------------------      set Pin as GPIO Purpose      ---------------- */
static void set_pin_gpio(unsigned int GPIO_PIN_NUM) //--will set all pin-group as GPIO
{
	if(GPIO_PIN_NUM >13 && GPIO_PIN_NUM<18)
	{
		*GPIO1_MODE |=(0x1<<2); //---set  GPIO14-17 GPIO MODE
		*GPIO1_MODE &=~(0x1<<3);	
	}

	if(GPIO_PIN_NUM >19 && GPIO_PIN_NUM<21)
	{
		*GPIO1_MODE |=(0x1<<26); //---set  GPIO20-21	GPIO MODE
		*GPIO1_MODE &=~(0x1<<27);	
	}
}


/*------------------      set GPIO Direction as Input      ---------------- */
static void set_gpio_input(unsigned int GPIO_PIN_NUM)
{

       *GPIO_CTRL_0 &=~(0x1<<GPIO_PIN_NUM); //---bit set 0, input mode
}

static void set_gpio_output(unsigned int GPIO_PIN_NUM)
{

       *GPIO_CTRL_0 |=(0x1<<GPIO_PIN_NUM); //---bit set 1, output mode
}



/* ----------------     set and  enable gpio interrupt  rise_int() and fall_int() to be interlocked  ------------*/
 void enable_gpio_rise_int(unsigned int GPIO_PIN_NUM)
{
   *GINT_REDGE_0 |=(0x1<<GPIO_PIN_NUM); //--bit set 1,Enable Rising Edge interrupt  
   *GINT_FEDGE_0 &=~(0x1<<GPIO_PIN_NUM); //--bit set 0,disable falling Edge interrupt  
}

 void enable_gpio_fall_int(unsigned int GPIO_PIN_NUM)
{
  /*GPIO_DATA_0 &=~(0x1<<GPIO_PIN_NUM); //--PRESET 0 */
   *GINT_FEDGE_0 |=(0x1<<GPIO_PIN_NUM); //--bit set 1,Enable falling Edge interrupt  
   *GINT_REDGE_0 &=~(0x1<<GPIO_PIN_NUM); //--bit set 0,disable Rising Edge interrupt  
}

/*----------------------  disable gpio interrupt ----------------------*/
 void disable_gpio_int(unsigned int GPIO_PIN_NUM)
{
   *GINT_REDGE_0 &=~(0x1<<GPIO_PIN_NUM); //--bit set 1,disable Rising Edge interrupt  
   *GINT_FEDGE_0 &=~(0x1<<GPIO_PIN_NUM); //--bit set 1,disable falling Edge interrupt  
}


/*-------------------    get gpio data   --------------------*/
unsigned int get_gpio_value(unsigned int GPIO_PIN_NUM)
{
  //??? set_gpio_input() necessary????
  return ((*GPIO_DATA_0)>>GPIO_PIN_NUM)&0x01; 
}

/*-------------------    set gpio data   --------------------*/
void set_gpio_value(unsigned int GPIO_PIN_NUM,unsigned int VALUE)
{
  set_gpio_output(GPIO_PIN_NUM); //--set direction output
  if(VALUE)
    *GPIO_DATA_0|=(0x1<<GPIO_PIN_NUM);
  else
    *GPIO_DATA_0 &=~(0X1<<GPIO_PIN_NUM);
}


/*-------------------    confirm interrupt  --------------------*/
/*-----     NOTE: this function call will cost ~3ms, use MACRO if possible  -----*/
static unsigned int confirm_gpio_interrupt(unsigned int GPIO_PIN_NUM)
{
  if(((*GINT_STAT_0)>>GPIO_PIN_NUM)&0x1)  //-----confirm the Interrupt
    {

       printk("GPIO Interrupt confirmed!\n"); 
       return 1;
    }
  return 0;
}

/*
//   ------------------   schedule work function    ----------------   
static struct work_struct lirc_wq;
static void enable_irq_wq(struct work_struct *data)
{
   printk("Entering work_queue and start msleep......\n");
   msleep_interruptible(150); //----deter re-enabling irq,avoid key-jitter
   printk("Starting enable_irq.....\n");
   enable_irq(LIRC_INT_NUM); //---re-enable_irq allowed in work-queue, NOT in tasklet!!!
   enable_gpio_fall_int(xx);  //---re-enable gpio interrupt
} 
*/

/*********************  The End ***************************/
