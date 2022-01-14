/*-----------------------------------------------------------------------------
A simple test for MT7688 GPIO registers and interrupt setting.

Based on: https://blog.csdn.net/weixin_36561012/article/details/105958043
Author:   Javier Huang

Note:
1. You need to fix pull_up/down resistors for FallingEdge/RiseEdge interrupt.

XXX 2.   ????---- CAUTION -----???? OK, see 3.
   XXX Setting Falling_EDGE interrupt will activate Rising_EDGE interrupt automatically??
   XXX while setting Rising_EDGO does NOT activate Falling_EDGE automatically.
   XXX OR MT7688 intrinsic problem?

 .   Mechanical jittering or circuit mismatch MAY also miss_trigger falling_edge
     when realsing the button/pen! It's necessary to take other measures to filter
     /avoid it.
Example:   disable the same type interrutp after first triggering.
	   OR Use other signal then to mointor status.
           XXX NOPE! invert interrutp type after first triggering.

3. !!! ---- IMPORTANT ---- !!!
   Return status of gpio_get_value() lags behind interrupt status! so udelay
   a litter to check GPIO stat after interrupt on the GPIO! and it's a way for
   debounce/jittery checking.

4. GINT_FEDGE and GINT_REDGE are all atuo. cleared when entering interrupt handler,
   but GINT_STAT is NOT! it is auto. cleared ONLY when entering Low Part IRQ handler
   (schedule_work())!

5. Ploarity functions ONLY when its value is set to 1.
   Ploarity=1 can also invert edge status AND edge interrupt type.



Jouranl:
2022-01-10:
	1. Add register operation for GPIO interrupt setting.
2022-01-12:
	1. Polarity for IntEdge switch.
2022-01-14:
	1. gpio_get_value() for debounce checking in my_interrupt_handler().

Midas Zhou
--------------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#define GPIO_NUM	17

#define INT_RISING_EDGE   true  /* TRUE */
#define INT_FALLING_EDGE  false  /* FALSE */

int  IntEdge=INT_FALLING_EDGE;
int  polarity=0;		/* 1-data inverted, 0-NOT inverted.
				 *	   ----- CAUTION -----
				 * 1. It affects result of gpio_get_value().
				 * 2. ALSO it makes interrupt of REDGE/FEDGE to switch/invert!
				 */

MODULE_AUTHOR("Javier Huang");
MODULE_LICENSE("GPL");

static int irq=-1;

/* MT7688 GPIO u32 Registers
 * [RO] Read Only [WO] Write Only [RW] Read or Write [RC] Read Clear [W1C] Write One Clear
 */
volatile u32 *GPIO_CTRL;   /* [RW] GPIO direction control register  0-input 1-output */
volatile u32 *GPIO_POL;    /* [RW] GPIO polarity control register */
volatile u32 *GPIO_DATA;   /* [RW] GPIO data register */
volatile u32 *GPIO_DSET;   /* [WO] GPIO data set register */
volatile u32 *GPIO_DCLR;   /* [WO] GPIO data clear register */
volatile u32 *GINT_REDGE;  /* [RW] GPIO rising edge interrupt enable register */
volatile u32 *GINT_FEDGE;  /* [RW] GPIO falling edge interrupt enable register */
volatile u32 *GINT_HLVL;   /* [RW] GPIO high level interrupt enable register */
volatile u32 *GINT_LLVL;   /* [RW] GPIO high level interrupt enable register */
volatile u32 *GINT_STAT;   /* [W1C] GPIO interrupt status register 1-int  0 -no int */
volatile u32 *GINT_EDGE_STAT;   /* [W1C] GPIO interrupt edge status register 1-rising 0-falling, write 1 for clear */

volatile u32 *GPIO_MODE; /* [RW] GPIO1_MODE or GPIO2_MODE, 1-2 bank/group mode/purpose selection register */
volatile u32 *AGPIO_CFG; /* analog GPIO configuartion */

/*------------------- MT7688 pin share scheme ---------------------------------
	I/O Pad group		Normal Mode		GPIO Mode
	...
	( Controlled by P#_LED_AN_MODE registers GPIO2_MODE @10000064 bits[11:2]  )
	    P4 ~ P0 ? 		   LED4~0 ??
	P0_LED_AN		EPHY_LED0_N_JTDO	GPIO#43   [11:10]
	P1_LED_AN		EPHY_LED1_N_JTDI	GPIO#42   [9:8]
	P2_LED_AN		EPHY_LED2_N_JTMS	GPIO#41	  [7:6]
	P3_LED_AN		EPHY_LED3_N_JTCLK	GPIO#40	  [5:4]
	P4_LED_AN		EPHY_LED4_N_JTRST_N	GPIO#39   [3:2]
	...
	( Controled by the EPHY_APGIO_AIO_EN[4:1] and SPIS_MODE registers: GPIO1_MODE @10000060 bits[3:2])
	SPIS(spi slave)		MDI_RN_P1		GPIO#17
				MDI_RP_P1		GPIO#16
				MDI_TN_P1		GPIO#15
				MDI_TP_P1		GPIO#14
	...
------------------------------------------------------------------------------*/

static struct work_struct wq; /* For LowPart of IRQ hander */

static int  map_registers(u32 gpio_num);
static int  set_gpioINT_registers(u32 gpio_num, bool onRisingEdge);
static void set_gpio_polarity(u32 gpio_num, u32 val);
static void set_gpio_value(u32 gpio_num, u32 val);
static void enable_gpio_int(u32 gpio_num, bool onRisingEdge);
static void disable_gpio_int(u32 gpio_num, bool onRisingEdge);
static int  confirm_interrupt_stat(u32 gpio_num);
static void clear_interrupt_stat(u32 gpio_num);
static void clear_edge_stat(u32 gpio_num);
//static int read_edge_stat(u32 gpio_num);
static void unmap_gpio_registers(u32 gpio_num);
static void irq_wq(struct work_struct *data); /* workqueue job */
static irqreturn_t my_interrupt_handler(int irq, void *dev_id);

/***
  IO remap registers
*/
static int map_registers(u32 gpio_num)
{
   /* 0. GPIO 3 banks/groups number: [0]GPIO0-31, [1]GPIO32~63, [2]GPIO64~95(NOT AVAILBALE) */
   int group_num=gpio_num/32;

   /* 1. Check range */
   bool gpio1_mode=(gpio_num>=14 && gpio_num<=17);
   bool gpio2_mode=(gpio_num>=39 && gpio_num<=42);

   if( (!gpio1_mode) && (!(gpio2_mode)) ) {
	printk("Only supports GPIO14~17 and GPIO39~42!\n");
	return -1;
   }
   printk("Map registers for GPIO_%u of group_%d\n", gpio_num, group_num);

   /* 2. Map GPIO registers */
   if(gpio1_mode) /* gpio1_mode */
       GPIO_MODE=(volatile u32 *)ioremap(0x10000060,4);
   else /* gpio2_mode */
       GPIO_MODE=(volatile u32 *)ioremap(0x10000064,4);

   GPIO_CTRL=(volatile u32 *)ioremap(0x10000600+4*group_num,4);
   GPIO_POL=(volatile u32 *)ioremap(0x10000610+4*group_num,4);
   GPIO_DATA=(volatile u32 *)ioremap(0x10000620+4*group_num,4);
   //AGPIO_CFG; // analog GPIO configuartion,GPIO14-17 purpose
   GINT_REDGE=(volatile u32 *)ioremap(0x10000650+4*group_num,4);
   GINT_FEDGE=(volatile u32 *)ioremap(0x10000660+4*group_num,4);
   GINT_HLVL =(volatile u32 *)ioremap(0x10000670+4*group_num,4);
   GINT_LLVL =(volatile u32 *)ioremap(0x10000680+4*group_num,4);
   GINT_STAT =(volatile u32 *)ioremap(0x10000690+4*group_num,4);
   GINT_EDGE_STAT =(volatile u32 *)ioremap(0x100006A0+4*group_num,4);

   return 0;
}


/***
 * Map and set GPIO registers  for gpio interrupt on rising/falling edge.
 * @gpio_num:	GPIO number. ONLY for GPIO 14-17 and GPIO 39-42
 * @risingEdge: True: Interrupt on rising edge.
 *		False: Interrupt on falling edge.
 * Return:
 *	0	OK
 *	<0	Fails
 */
static int set_gpioINT_registers(u32 gpio_num, bool onRisingEdge)
{

   /* 1. Check range */
   bool gpio1_mode=(gpio_num>=14 && gpio_num<=17);
   bool gpio2_mode=(gpio_num>=39 && gpio_num<=42);

   if( (!gpio1_mode) && (!(gpio2_mode)) ) {
	printk("Only supports GPIO14~17 and GPIO39~42!\n");
	return -1;
   }

   /* 2. Setup registers for GPIO interrupt */
   /* 2.1 Set pin to GPIO MODE */
   if(gpio1_mode) {
	/*** GPIO1_MODE: Controled by the EPHY_APGIO_AIO_EN[4:1] and SPIS_MODE registers @10000060 bits[3:2])
		I/O Pad group		Normal Mode		GPIO Mode
		SPIS			MDI_RN_P1		GPIO#17
					MDI_RP_P1		GPIO#16
					MDI_TN_P1		GPIO#15
					MDI_TP_P1		GPIO#14
	*/
	*GPIO_MODE |=(0x1<<2);  /* Set bist[3:2] to be 0b01 */
	*GPIO_MODE &=~(0x1<<3);
	// *GPIO_POL_0 |=(0x1<<GPIO_NUM); ---set polarity 1 as inverted ----crash!!
   } else {
	/*** GPIO2_MODE:  Controlled by P#_LED_AN_MODE registers @10000064 bits[11:2]
		I/O Pad group		Normal Mode		GPIO Mode
		P0_LED_AN		EPHY_LED0_N_JTDO	GPIO#43   [11:10]
		P1_LED_AN		EPHY_LED1_N_JTDI	GPIO#42   [9:8]
		P2_LED_AN		EPHY_LED2_N_JTMS	GPIO#41	  [7:6]
		P3_LED_AN		EPHY_LED3_N_JTCLK	GPIO#40	  [5:4]
		P4_LED_AN		EPHY_LED4_N_JTRST_N	GPIO#39   [3:2]   [ (39-39)*2+3, (39-39)*2+2 ]
	*/
	*GPIO_MODE |=(0x1<<((gpio_num-39)*2 +2));  /* Set to 0b01 */
	*GPIO_MODE &=~(0x1<<((gpio_num-39)*2 +3));
   }
   /* 2.2 Bit set 0, Input mode */
   *GPIO_CTRL &= ~(0x1<<(gpio_num%32));   /* 0-input 1-output */

   /* 2.3 polarity and data for input */
//??   set_gpio_value(gpio_num%32, onRisingEdge?0:1);

   /* 2.3 Set and enable gpio interrupt, GINT_REDGE and GINT_FEDGE to be interlocked */
   enable_gpio_int(gpio_num, onRisingEdge);

   return 0;
}

/* Set value: 0(data is non_inverted) OR 1(data is inverted) */
inline static void set_gpio_polarity(u32 gpio_num, u32 val)
{
    u32 mask = (0x1<<(gpio_num%32));
    if(val)
	*GPIO_POL |= mask;
    else
	*GPIO_POL &= ~mask;
}

/* Set value: 0 OR 1 */
inline static void set_gpio_value(u32 gpio_num, u32 val)
{
    u32 mask = (0x1<<(gpio_num%32));

    if(val)
	*GPIO_DATA |= mask;
    else
	*GPIO_DATA &= ~mask;
}

inline static void enable_gpio_int(u32 gpio_num, bool onRisingEdge)
{
   u32 mask = (0x1<<(gpio_num%32));

#if 0 /* TEST: Clear all other to avoid interference! ----- no use! ---------- */
   *GINT_REDGE = 0x0;
   *GINT_FEDGE = 0x0;
   *GINT_HLVL  =0X0;
   *GINT_LLVL  =0X0;
#endif

   /* Disable High/Low level interrupt */
//   *GINT_HLVL &= ~mask;
//   *GINT_LLVL &= ~mask;

   /* GINT_REDGE and GINT_FEDGE to be interlocked */
   if(onRisingEdge) {
	   *GINT_REDGE |=mask;  /* bit set 1: enable Rising Edge interrupt. 0-disable */
	   *GINT_FEDGE &=~mask; /* bit set 0: disable falling Edge interrupt. 1-enable */
   }
   else { /* On Falling Edge */
	   *GINT_REDGE &=~mask; /* bit set 0: disable falling Edge interrupt. 1-enable */
	   *GINT_FEDGE |=mask;  /* bit set 1: Enable falling Edge interrupt */
   }
}

/* gpio_num: [14~17] [39~42] */
inline static void disable_gpio_int(u32 gpio_num, bool onRisingEdge)
{
   u32 mask = (0x1<<(gpio_num%32));

   if(onRisingEdge)
	   *GINT_REDGE &= ~mask; /* bit set 0 to disable */
   else
	   *GINT_FEDGE &= ~mask; /* bit set 0 to disable */
}

/* gpio_num: [14~17] [39~42]. TODO: CAN NOT read? */
inline static int confirm_interrupt_stat(u32 gpio_num)
{
    u32 tmp=*GINT_STAT;
    return (tmp>>(gpio_num%32))&0x1;
}

/* gpio_num: [14~17] [39~42] */
inline static void clear_interrupt_stat(u32 gpio_num)
{
      *GINT_STAT |= (0x1<<(gpio_num%32));  /*  write 1 for clear! */
}

/* Clear edge status */
inline static void clear_edge_stat(u32 gpio_num)
{
     /* W1C register */
     *GINT_EDGE_STAT |= (0x1<<(gpio_num%32));  /*  write 1 for clear! */
}


/* Unmap and unset gpio registers. gpio_num: [14~17] [39~42] */
static void unmap_gpio_registers(u32 gpio_num)
{
     u32 mask=(0x1<<(gpio_num%32));

     /* Check range */
     bool gpio1_mode=(gpio_num>=14 && gpio_num<=17);
     bool gpio2_mode=(gpio_num>=39 && gpio_num<=42);

     if( (!gpio1_mode) && (!gpio2_mode) ) {
	printk("Only supports GPIO14~17 and GPIO39~42!\n");
	return;
     }

     /* Disable gpio interrupt */
     *GINT_REDGE &= ~mask; /* disable rising edge */
     *GINT_FEDGE &= ~mask; /* disable falling edge */

     /* Unmap GPIO registers */
     iounmap(GPIO_MODE);
     iounmap(GPIO_DATA);
     iounmap(GPIO_CTRL);
     iounmap(GINT_REDGE);
     iounmap(GINT_FEDGE);
     iounmap(GINT_HLVL);
     iounmap(GINT_LLVL);
     iounmap(GINT_STAT);
     iounmap(GINT_EDGE_STAT);
}

/* schedule work as LowPart of IRQ */
static void irq_wq(struct work_struct *data)
{
   u32 tmp;
//   printk("Entering work_queue and start msleep...\n");
   printk("Touch screen PEN_%s!\n", IntEdge==INT_FALLING_EDGE?"DOWN":" UP");

   /* <----- Do your job here -----> */
   msleep_interruptible(150); /* deter re-enabling irq,avoid key-jitter */

   /* <-----  Job END -----> */

   /* Debounce check */
#if 0 /* TEST: ------------ */
    printk("2 gpio_get_value() = %d\n", gpio_get_value(GPIO_NUM));
#endif


#if 0 /* TEST: ------------ */
   tmp=*GINT_REDGE;
   printk("GINT_REDGE 2: 0x%08X \n", tmp);
   tmp=*GINT_FEDGE;
   printk("GINT_FEDGE 2: 0x%08X \n", tmp);
   tmp=*GINT_STAT;
   printk("GINT_STAT 3: 0x%08X \n", tmp);
#endif

   /* NOTICE: GINT_REDGE and GINT_FEDGE ALREADY auto cleared when entering interrupt handler,
    * and GINT_STAT is auto. cleared when entering Lowpart of IRQ handler.
    * SO following is NOT necessary!
    */
   clear_edge_stat(GPIO_NUM); /* So not necessary here. */
   clear_interrupt_stat(GPIO_NUM);

#if 0  /* TEST: ------------ */
   tmp =*GINT_STAT;
   printk("GINT_STAT 4: 0x%08X \n",*GINT_STAT);
#endif

#if 0 /* TEST: --------Invert polarity */
   polarity=!polarity;
   set_gpio_polarity(GPIO_NUM, polarity);
#endif

#if 0 /* TEST: --------Invert intEdge */
   IntEdge=!IntEdge;
#endif

   printk("Starting re_enable_irq...\n");
   /* Since GING_REDGE/FEDGE all cleared... */
   //printk("GINT_REDGE: 0x%08X, GINT_FEDGE: 0x%08X \n",*GINT_REDGE, *GINT_FEDGE);
   enable_gpio_int(GPIO_NUM, IntEdge); /* Re-enable */
   //printk("GINT_REDGE: 0x%08X, GINT_FEDGE: 0x%08X \n",*GINT_REDGE, *GINT_FEDGE);

#if 0 /* TEST: ------------ */
   tmp=*GINT_REDGE;
   printk("GINT_REDGE 3: 0x%08X \n", tmp);
   tmp=*GINT_FEDGE;
   printk("GINT_FEDGE 3: 0x%08X \n", tmp);
#endif

   enable_irq(irq); /* re-enable_irq allowed in work-queue, NOT in tasklet!!! */
}

/* interrupt handler */
static irqreturn_t my_interrupt_handler(int irq, void *dev_id) {
        u32 tmp;
        //tmp=*GINT_EDGE_STAT;  /* 'W1C' register, return all 0s, can NOT read!!? */
        //printk("GINT_EDGE_STAT: 0x%08X\n", tmp);

#if 0 /* TEST: ------------ */
	tmp=*GINT_STAT;
   	printk("GINT_STAT 1: 0x%08X \n", tmp);

   	tmp=*GINT_REDGE;
   	printk("GINT_REDGE 1: 0x%08X \n", tmp);
	tmp=*GINT_FEDGE;
	printk("GINT_FEDGE 1: 0x%08X \n", tmp);

	tmp=*GPIO_POL;
   	printk("GPIO_POL: 0x%08X \n", tmp);
#endif
	/* Notice: GINT_FEDGE and GINT_REDGE all atuo. cleared when entering interrupt handler. GINT_STAT is NOT  */

   	/* Debounce check */
	udelay(1000); /* !!!--- IMPORATANT ---!!! gpio_get_value() lags behind interrupt stat! */
   	//printk("1 gpio_get_value() = %d \n", gpio_get_value(GPIO_NUM));
	if( gpio_get_value(GPIO_NUM)!=(IntEdge==INT_FALLING_EDGE?0:1) ) {
		printk("Debounce react!\n");
		return IRQ_NONE;
	}

	if(confirm_interrupt_stat(GPIO_NUM)) {  /* Read stat NOT allowed? */

		/* Disable IRQ and reset stat */
      		disable_irq_nosync(irq);
      		//disable_irq(irq); !!! WARNING: disable_irq() MUST NOT use in irq handler or in IRQF_SHARED mode,This will cause dead-loop !!!

		disable_gpio_int(GPIO_NUM, IntEdge);

#if 0 /* TEST: ------------ */
   		tmp=*GINT_STAT;
		printk("GINT_STAT 2: 0x%08X \n", tmp);
#endif

         	/* Low Part IRQ handler. low part must NOT put in un-atomic code structure.
		 * It auto. clear register GINT_STAT  when entering low part IRQ handler!
 		 */
          	schedule_work(&wq); /* Re-enable interrupt after low part job. */

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int __init driver_init(void)
{
	int ret=0;

	/* 1. Map registers */
	ret=map_registers(GPIO_NUM);
	if(ret)
		goto FUNC_ERROR;

	/* 2. Setup registers for GPIO interrupt */
	ret=set_gpioINT_registers(GPIO_NUM, IntEdge);
	if(ret)
		goto FUNC_ERROR;

	/* 2A. Set poloarity */
	set_gpio_polarity(GPIO_NUM, polarity);

        /* 3. Init work-queue  for irq handler low part */
        INIT_WORK(&wq, irq_wq);

	/* 4. Request IRQ */
	irq=gpio_to_irq(GPIO_NUM);
	if( (ret=request_irq(irq, my_interrupt_handler, IntEdge==INT_RISING_EDGE?IRQF_TRIGGER_RISING:IRQF_TRIGGER_FALLING,
				"gpio_irq", NULL)) <0) {
		printk("driver: request_irq for gpio_%d failed\n", GPIO_NUM);
		goto FUNC_ERROR;
	}
	else {
		printk("request_irq for gpio_%d succeed! (~0x800)=0x%08X \n", GPIO_NUM, ~(0x800));
	}

	return 0;

FUNC_ERROR:
	if(irq>0)
		free_irq(irq,NULL);
	unmap_gpio_registers(GPIO_NUM);

	return ret;
}

static void __exit driver_exit(void)
{
	if(irq>0) {
		free_irq(irq, NULL);
	}

	/* Destroy work */
	flush_work(&wq);

	/* Unset and free */
	unmap_gpio_registers(GPIO_NUM);
}

module_init(driver_init);
module_exit(driver_exit);
