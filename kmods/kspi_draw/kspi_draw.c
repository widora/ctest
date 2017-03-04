/*
   * MTD SPI driver for ST M25Pxx flash chips
   *
   * Author: Mike Lavender, mike@steroidmicros.com
   *
   * Copyright (c) 2005, Intec Automation Inc.
   *
   * Some parts are based on lart.c by Abraham Van Der Merwe
   *
   * Cleaned up and generalized based on mtd_dataflash.c
   *
   * This code is free software; you can redistribute it and/or modify
   * it under the terms of the GNU General Public License version 2 as
   * published by the Free Software Foundation.
   *
   */
/* ---------------------------------------------------------------------
Based on Liuchen_csdn's ftf driver.
With reference to  m.blog.csdn.net/article/details?id=51316854
Midas-Zhou
----------------------------------------------------------------------*/
  #include <linux/init.h>
  #include <linux/module.h>
  #include <linux/device.h>
  #include <linux/cdev.h> //cdev_del()
  #include <linux/interrupt.h>
  //#include <linux/interrupt.h>
  #include <linux/mtd/mtd.h>
  #include <linux/mtd/map.h>
  #include <linux/mtd/gen_probe.h>
  #include <linux/mtd/partitions.h>
  #include <linux/semaphore.h>
  #include <linux/slab.h>
  #include <linux/delay.h>
  #include <linux/spi/spi.h>
  #include <linux/spi/flash.h>
  #include <linux/version.h>
  #include <linux/time.h>
  #include <linux/preempt.h>
  #include <linux/sched.h>
  #include <linux/vmalloc.h>
  #include "kgpio.h"  //-- GPIO config. and mode set
  #define SPIBUFF_SIZE 36
  #define COLOR_DATA_SIZE  (480*320*3) // max. bmp file byte size for 24bits color
  #include "kspi.h" //-- functions for SPI and GPIO cooperations
  #include "RM68140.h"

  MODULE_LICENSE("GPL");
  static  struct sched_param sch_param;//scheduler parameter
  static  unsigned char *pmem_color_data=NULL; //pointer to color data mem

  //------------------------------- device relevant ---------------------------
  unsigned char init_flag = 0; //--flag for init result, 1-exicuted(need to release resource later)  0-not exicuted
  unsigned char add_code_flag = 0; //--flag for init user code result, 1-exicuted 0-not exicuted or no code added 
  dev_t dev_num; //---device number
  struct cdev kspi_draw_cdev; //---char. device struct
  struct class *kspi_draw_class=NULL;
  struct device *kspi_draw_device=NULL; //--device 

  static int kspi_draw_open(struct inode *inode,struct file *file)
  {
	printk("kspi_draw drive open...\n");
	return 0;
  }

  static int kspi_draw_close(struct inode *inode, struct file *file)
  {
	printk("kspi_draw drive close...\n");
	return 0;
  }


  static const struct file_operations kspi_draw_fops ={
	.owner = THIS_MODULE,
	.open = kspi_draw_open,
	.release = kspi_draw_close,
};


static int __init spi_LCD_init(void)
 {
         int result=0;
         u32 gpiomode;
	 unsigned long flags;

//-----------------------  create and register device  -----------------
	printk("------ start kspi_draw driver init -----\n");
	if((result=alloc_chrdev_region(&dev_num,0,1,"kspi_draw"))<0) //---allocate dev_num dynamically,create a group of dev numbers in /proc/devices
	{
		goto dev_reg_error;
	}
	init_flag=1;
	printk("------Device number major:%d  minor:%d -----\n",MAJOR(dev_num),MINOR(dev_num));

	cdev_init(&kspi_draw_cdev,&kspi_draw_fops); //---init a char dev
	if((result=cdev_add(&kspi_draw_cdev,dev_num,1))!=0) //--add cdev into system
	{
		goto cdev_add_error;
	}

	kspi_draw_class=class_create(THIS_MODULE,"kspi_draw_class"); //---to create /sys/class/kspi_draw_class
	if(IS_ERR(kspi_draw_class))
	{
		goto class_create_error;
	}

	kspi_draw_device=device_create(kspi_draw_class,NULL,dev_num,NULL,"kspi_draw_dev"); //--to create /dev/kspi_draw_dev
	if(IS_ERR(kspi_draw_device))
	{
		goto device_create_error;
	}



//--------------------- set scheduler policy --------------
//-----Under SCHED_FIFO scheduler policy,you will see kspi_draw interrupted by other spi application in very rare case.
	sch_param.sched_priority=99;//sched_get_priority_max(SCHED_FIFO);
	if(sched_setscheduler(current,SCHED_FIFO,&sch_param) == -1) //--- MUST use current !!! getpid() is not allowed in kernel space, and will casue segment fault!!!
		printk(" ----- !!! sched_setscheduler() fail! -------\n");

//---------------  to show and confirm expected gpio mode  --------------------------------
         gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
         printk("--------current RALINK_REG_GPIOMODE value: %08x !\n",gpiomode);

//---------------  set  SPI base device  spi_LCD  -------------
         spi_LCD.chip_select = 1; //---- useless, since spi_trans function() already set cs_1 inside for safety consideration.
         spi_LCD.mode = SPI_MODE_3 ;
         spi_LCD.speed = 39000000;//100000000;
         spi_LCD.sys_freq = 200000000;//575000000; //system clk 580M
//---- HCLK = 200MHz?? MAX40? 

/*
//========================= init LCD ==============================
	LCD_HD_reset();
	printk("-----LCD_HD_reset finish---\n");
	LCD_INIT_RM68140();
	//preempt_disable();
	printk("----- Start drawing --------\n");
	LCD_ColorBox(0,0,320,480,0b1111100000011111);//0x8010;
	LCD_ColorBox(0,0,320,480,0b0000011111100000);//0x8400);
	printk("----- Finish drawing --------\n");
	//-------- enable preempt  ----------
	//preempt_enable();
	PREEMPT_ON=1;
*/

//----------------------------- load BMP data and prepare LCD  ----------------------
	pmem_color_data=load_user_bmpf("/tmp/hello.bmp"); //load image to mem
	if(pmem_color_data==NULL)goto mem_vmalloc_error;
	else add_code_flag=1;

	LCD_prepare();

//-----------------------------      draw picture        -----------------------
preempt_disable(); //----seems no use
	//local_irq_save(flags);//--this will stop timestamps
	local_irq_disable();//--!!!! However show_user_bmpf() is still interruptable, as there is file read()????? !!!!!---
	result=show_user_bmpf("/tmp/P30.bmp"); //--interruptable !!
	local_irq_enable();
	mdelay(1000);
	display_block_full(pmem_color_data);
//	result=show_user_bmpf("/tmp/P33.bmp");
	mdelay(1000);
//	display_full(pmem_color_data);
	result=show_user_bmpf("/tmp/P35.bmp"); //--interruptable !!
	display_block_full(pmem_color_data);
//local_irq_restore(flags);
preempt_enable();


//--------------------------------  init results proceeding ------------------------ 
goto init_success;

dev_reg_error:
	printk("alloc_chrdev_region failed!\n");
	return result;
cdev_add_error:
	printk("cdev_add failed!\n");
	unregister_chrdev_region(dev_num,1);
	return result;
class_create_error:
	printk("class_create failed!\n");
	cdev_del(&kspi_draw_cdev);
	unregister_chrdev_region(dev_num,1);
	return PTR_ERR(kspi_draw_class);
device_create_error:
	printk("device_create faile!\n");
	cdev_del(&kspi_draw_cdev);
	unregister_chrdev_region(dev_num,1);
	class_destroy(kspi_draw_class);
	return PTR_ERR(kspi_draw_device);
mem_vmalloc_error:
	printk("mem_valloc_error!\n");
	return -1;


init_success:
	printk(" ---------- kspi_draw init succeed! ------------\n");
	return 0;

 }


static void __exit spi_LCD_exit(void)
 {
        printk("------removing spi_LCD driver!--------\n");
	if(add_code_flag == 1)
	{
 		vfree(pmem_color_data);
	}

	if(init_flag == 1)
	{
		cdev_del(&kspi_draw_cdev);
		unregister_chrdev_region(dev_num,1);
		device_unregister(kspi_draw_device);
		class_destroy(kspi_draw_class);
	}

 }

 module_init(spi_LCD_init);
 module_exit(spi_LCD_exit);



