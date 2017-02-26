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
  #include "kgpio.h"  //-- GPIO config. and mode set
  #include "kdraw.h" //-- functions for SPI and GPIO cooperations
  #include "RM68140.h"

  MODULE_LICENSE("GPL");



static int __init spi_LCD_init(void)
 {
	int k;
         int result=0;
         u32 gpiomode;

         //---------------  to shown and confirm expected gpio mode  --------------------------------

         gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
         printk("--------current RALINK_REG_GPIOMODE value: %08x !\n",gpiomode);

         //struct base_spi spi_LCD;
         spi_LCD.chip_select = 1;
         spi_LCD.mode = SPI_MODE_3 ;
         spi_LCD.speed =160000000;
         spi_LCD.sys_freq = 575000000; //system clk 580M

         spi_LCD.tx_buf[0] = 0xff;
         spi_LCD.tx_buf[1] = 0x00;
         spi_LCD.tx_len = 2;

         //while(1)
	       result=base_spi_transfer_half_duplex(&spi_LCD);
         printk("-----base_spi_transfer_half() result=%d\n",result);
//========================= init LCD ==============================
	LCD_HD_reset();
	//mdelay(100);
	printk("-----LCD_HD_reset finish---\n");
	LCD_INIT_RM68140();
	printk("----- Start drawing --------\n");
	LCD_ColorBox(0,0,320,480,0b1111100000000000);//0x8010;
	//mdelay(1000);
//	LCD_ColorBox(0,0,320,480,0b0000011111100000);//0x8400);
	printk("----- Finish drawing --------\n");

	return result;
 }


static void __exit spi_LCD_exit(void)
 {
 //              dev_t devno = MKDEV (hello_major, hello_minor);
         printk("------rmmod spi_LCD driver!--------\n");

 }

 module_init(spi_LCD_init);
 module_exit(spi_LCD_exit);

