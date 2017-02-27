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
  #include <linux/sched.h>
  #include "kgpio.h"  //-- GPIO config. and mode set
  #include "kdraw.h" //-- functions for SPI and GPIO cooperations
  #include "RM68140.h"
 
  #define SPIBUFF_SIZE 36

  MODULE_LICENSE("GPL");


  static  struct sched_param sch_param;//scheduler parameter

static void LCD_prepare(void)
{
	LCD_HD_reset();
	LCD_INIT_RM68140();
        WriteComm(0x3a);WriteData(0x66); //set pixel format 18bits pixel
        WriteComm(0x36);WriteData(0x00); //R-G-B set color data order same as BMP file
// SET ENTRY MODE, FLIP MODE HERE IF NECESSARY
//BRIGHTNESS CONTROL HERE IF NECESSARY
}


static int read_user_bmpf(char* str_f)
{
	int i;
//	uint8_t SPIBUFF_SIZE=36;//-- buffsize for one-time SPI transmital. MAX. 32*9=36bytes
	uint8_t buff[8];//---for temp. data buffering
	uint8_t data_buff[SPIBUFF_SIZE];//--temp. data buff for SPI transmit
	loff_t offp; //-- pfile offset position, use long type will case vfs_read() fail
	u32 picWidth,picHeight; //---BMP file width and height
	u32 total; //--total bytes of bmp file
	u16 residual; //--residual after total/
	u16 nbuff; //=total/SPIBUFF_SIZE
	u16 Hs,He,Vs,Ve; //--GRAM area definition parameters
	u16 Hb,Vb; //--GRAM area corner gap distance from coordinate origin
	ssize_t nret;//vfs_read() return value

//	char str_f[]="/tmp/P35.bmp";
	struct file *fp;
	mm_segment_t fs; //virtual address space parameter


	fp=filp_open(str_f,O_RDONLY,0);
	if(IS_ERR(fp))
	{
		printk("Fail to open %s!\n",str_f);
		return -1;
	}
	fs=get_fs(); //retrieve and store virtual address space boundary limit of current process
	set_fs(KERNEL_DS); // set address space limit to that of the kernel (whole of 4GB)

	offp=18; //where bmp width-height data starts
	vfs_read(fp,buff,sizeof(buff),&offp);// offp must be loff_t type!!!  vfs_read() will return 0 for first bytes if offp defined as long int. type.
	picWidth=buff[3]<<24|buff[2]<<16|buff[1]<<8|buff[0];
	picHeight=buff[7]<<24|buff[6]<<16|buff[5]<<8|buff[4];
	printk("----%s:  picWidth=%d   picHeight=%d -----\n",str_f,picWidth,picHeight);
	if(picWidth > 320 | picHeight > 480)
	{
		printk("----- picture size too large -----\n");
	 	return -1;
	}

	//----------------- calculate GRAM area ---------------------
	Hb=(320-picWidth+1)/2;
	Vb=(480-picHeight+1)/2;
	Hs=Hb;He=Hb+picWidth-1;
	Vs=Vb;Ve=Vb+picHeight-1;
	printk("Hs=%d  He=%d \n",Hs,He);
	printk("Vs=%d  Ve=%d \n",Vs,Ve);

	GRAM_Block_Set(Hs,He,Vs,Ve); //--set GRAM area
	WriteComm(0x2c); //--prepare for continous GRAM write

	total=picWidth*picHeight*3; //--total bytes of BGR data,for 24bits BMP file
	printk("total=%d\n",total);
	nbuff=total/SPIBUFF_SIZE; //-- how many times of SPI transmits needed,with SPIBUFF_SIZE each time.
	printk("nbuff=%d\n",nbuff);
	residual=total%SPIBUFF_SIZE; //--residual data 

	printk("--------------------- Start drawing the picture --------------------\n");
	offp=54; //--offset where BGR data begins
	//-------------------------- SPI transmit data to LCD  ---------------------
	for(i=0;i<nbuff;i++)
	{
		nret=vfs_read(fp,data_buff,sizeof(data_buff),&offp);// offp must be loff_t type!!!  vfs_read() will return 0 for first bytes if offp defined as long int. type.
		//printk("nret=%d\n",nret);
		WriteNData(data_buff,SPIBUFF_SIZE);
		//offp+=SPIBUFF_SIZE;
	}
	//for(i=0;i<36;i++)printk("data_buff[%d]: 0x%0x\n",i,data_buff[i]);
	if(residual!=0)
	{
		vfs_read(fp,data_buff,residual,&offp);// offp must be loff_t type!!!  vfs_read() will return 0 for first bytes if offp defined as long int. type.
		WriteNData(data_buff,residual);
	}
	printk("--------------------- Finish drawing the picture --------------------\n");

	filp_close(fp,NULL);
	set_fs(fs);//reset address space limit to the original one
	return 0;
}

static int __init spi_LCD_init(void)
 {
  	 int k;
         int result=0;
         u32 gpiomode;

/* --------------- set_scheduler will cause segment fault!!!!!!! -------------------
	//--------------------- set scheduler policy --------------
	sch_param.sched_priority=99;//sched_get_priority_max(SCHED_FIFO);
	if(sched_setscheduler(getpid(),SCHED_FIFO,&sch_param) == -1)
		printk(" ----- !!! sched_setscheduler() fail! -------\n");
------*/

         //---------------  to shown and confirm expected gpio mode  --------------------------------

         gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
         printk("--------current RALINK_REG_GPIOMODE value: %08x !\n",gpiomode);

         //struct base_spi spi_LCD;
         spi_LCD.chip_select = 1;
         spi_LCD.mode = SPI_MODE_3 ;
         spi_LCD.speed =100000000;
         spi_LCD.sys_freq = 575000000; //system clk 580M
//---- HCLK = 200MHz?? MAX66? 
         spi_LCD.tx_buf[0] = 0xff;
         spi_LCD.tx_buf[1] = 0x00;
         spi_LCD.tx_len = 2;

         //while(1)
	       result=base_spi_transfer_half_duplex(&spi_LCD);
         printk("-----base_spi_transfer_half() result=%d\n",result);
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
	LCD_prepare();
	result=read_user_bmpf("/tmp/P30.bmp");
	mdelay(1000);
	result=read_user_bmpf("/tmp/P33.bmp");
	result=read_user_bmpf("/tmp/P35.bmp");

	return result;
 }


static void __exit spi_LCD_exit(void)
 {
 //              dev_t devno = MKDEV (hello_major, hello_minor);
         printk("------rmmod spi_LCD driver!--------\n");

 }

 module_init(spi_LCD_init);
 module_exit(spi_LCD_exit);

