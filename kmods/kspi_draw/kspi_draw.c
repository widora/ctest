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
  #include "kgpio.h"  //-- GPIO config. and mode set
  #include "kdraw.h" //-- functions for SPI and GPIO cooperations
  #include "RM68140.h"

  MODULE_LICENSE("GPL");

//  struct base_spi spi_LCD;


#define _SHUT_FUNCTIONS_
#ifndef _SHUT_FUNCTIONS_

  #if defined (CONFIG_RALINK_RT6855A) || defined (CONFIG_RALINK_MT7621) || defined (CONFIG_RALINK_MT7628)
  #else
  //#include "ralink_spi.h"
  #endif

  #include <linux/fcntl.h> /* O_ACCMODE */
  #include <linux/types.h> /* size_t */
  #include <linux/proc_fs.h>



  //#include "bpeer_tft.h"  //-------------------------------------------------

/*------------------ define ra_inl() and ra_outl() based on SPI_DEBUG or NORMAL situation   ---------------------------*/
 //#define SPI_DEBUG
  #if !defined (SPI_DEBUG)
  ////---when SPI_DEBUG is NOT defined ,then ra_inl() will NOT print out func and values
  #define ra_inl(addr)  (*(volatile unsigned int *)(addr))  //--get value of given addr
  #define ra_outl(addr, value)  (*(volatile unsigned int *)(addr) = (value)) //--set value for given addr
  #define ra_dbg(args...) do {} while(0)
  /* #define ra_dbg(args...) do { printk(args); } while(0) */
  #else
  #define ra_dbg(args...) do { printk(args); } while(0)
  #define _ra_inl(addr)  (*(volatile unsigned int *)(addr))
  #define _ra_outl(addr, value)  (*(volatile unsigned int *)(addr) = (value))

  //---when SPI_DEBUG defined ,then ra_inl() should print out func and values
  u32 ra_inl(u32 addr)
  {
          u32 retval = _ra_inl(addr);
          printk("%s(%x) => %x \n", __func__, addr, retval);
           return retval;
   }

  u32 ra_outl(u32 addr, u32 val)
  {
          _ra_outl(addr, val);
           printk("%s(%x, %x) \n", __func__, addr, val);
           return val;
   }

  #endif // SPI_DEBUG //

/*----------------   set value for addr. with and-or,and,or operation   --------------------------------*/
  #define ra_aor(addr, a_mask, o_value)  ra_outl(addr, (ra_inl(addr) & (a_mask)) | (o_value))  // and-or
  #define ra_and(addr, a_mask)  ra_aor(addr, a_mask, 0)
  #define ra_or(addr, o_value)  ra_aor(addr, -1, o_value) //-1= 1111....

/* ---------------------------   SPI DEV. RELEVANT  ---------------------------------------------*/
 #define SPI_MAX_DEV 2
// static struct base_spi spi_pre[SPI_MAX_DEV];

 //static struct tft_config_type tft_config[2];  //------------------tft ------------------------

 //static int spidrv_major = 111;
 //int eye_l_minor =  0;
 //int eye_r_minor =  1;



/*------------------------------------------   GPIO base ---------------------------------------------------------------*/
#define GPIO_OUT_H 1
#define GPIO_OUT_L 0

 static int base_gpio_set(int gpio,int value)
 {
         unsigned int tmp;
         if (gpio <= 31) {
                 tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
                 tmp |= 1 << gpio;
                 *(volatile u32 *)(RALINK_REG_PIODIR) = tmp;

                 tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
                 tmp &= ~(1 << gpio);
                 tmp |= value << gpio;
                 *(volatile u32 *)(RALINK_REG_PIODATA) = cpu_to_le32(tmp);
 //              *(volatile u32 *)(RALINK_REG_PIOSET) = cpu_to_le32(tmp);
         } else if (gpio <= 63) {
                 tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR + 0x04));
                 tmp |= 1 << (gpio - 32);
                 *(volatile u32 *)(RALINK_REG_PIODIR + 0x04) = tmp;

                 tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA + 0x04));
                 tmp &= ~(1 << (gpio - 32));
                 tmp |= value << (gpio - 32);
                 *(volatile u32 *)(RALINK_REG_PIODATA + 0x04) = tmp;
         } else {
                 tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR + 0x08));
                 tmp |= 1 << (gpio - 64);
                 *(volatile u32 *)(RALINK_REG_PIODIR + 0x08) = tmp;

                 tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA + 0x08));
                 tmp &= ~(1 << (gpio - 64));
                 tmp |= value << (gpio - 64);
                 *(volatile u32 *)(RALINK_REG_PIODATA + 0x08) = tmp;
         }

 //      *(volatile u32 *)(RALINK_REG_PIOSET) = cpu_to_le32(arg);
 }

 /*------------------------------- set GPIO mode and init value ------------------------*/
 static int  base_gpio_init(void)
 {
         unsigned int i;
         u32 gpiomode;

         //config these pins to gpio mode
         gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
         gpiomode &= ~(3<<6);//0xc00;  //clear bit[11:10] 00:SD_mode
         gpiomode |= (1<<6);//0xc00;  //clear bit[11:10] 00:SD_mode
         gpiomode &= ~(3<<4);

 //      *(volatile u32 *)(RALINK_REG_AGPIOCFG) = cpu_to_le32(le32_to_cpu(*(volatile u32 *)(RALINK_REG_AGPIOCFG)) | 0x1e0000 );  //set bit[20:17] 1111:Digital_mode
 //      gpiomode &= ~0xc00;  //clear bit[11:10] 00:SD_mode
 //      gpiomode &= ~0x03000000;  //clear bit[25:24] 00:UART1_mode
 //      gpiomode &= ~0x0300;  //clear bit[9:8] 00:UART0_mode
         *(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);
         printk("RALINK_REG_GPIOMODE %08x !\n",le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE)));

         printk("base_gpio_init initialized\n");

      //   base_gpio_set(TFT_GPIO_RS,1); // #define TFT_GPIO_RS   2   in "bpeer_tft.h"
       //  base_gpio_set(TFT_GPIO_LED,0); //#define TFT_GPIO_LED  0   in "bpeer_tft.h"
         return 0;
 }

/*-------------------------SPI base ----------------------------------------------------------------------*/
 void usleep(unsigned int usecs)
 {
         unsigned long timeout = usecs_to_jiffies(usecs);

         while (timeout)
                 timeout = schedule_timeout_interruptible(timeout);
 }


static int base_spi_busy_wait(void)
 {
         int n = 100000;
         do {
                 if ((ra_inl(SPI_REG_CTL) & SPI_CTL_BUSY) == 0)
                         return 0;
                 udelay(1);
         } while (--n > 0);

         printk("%s: fail \n", __func__);
         return -1;
}

 static inline u32 base_spi_read( u32 reg) // --- spi register read
 {
         u32 val;
         val = ra_inl(reg);
 //      printk("mt7621_spi_read reg %08x val %08x \n",reg,val);
         return val;
 }

 static inline void base_spi_write( u32 reg, u32 val)  //--- spi register write
 {
 //      printk("mt7621_spi_write reg %08x val %08x \n",reg,val);
         ra_outl(reg,val);
 }

 static void base_spi_reset(int duplex) // ----------------  0 half duplex ; 1 full duplex
 {
         u32 master = base_spi_read(SPI_REG_MASTER);

         master |= 7 << 29;
         master |= 1 << 2;
         if (duplex)
                 master |= 1 << 10;
         else
                master &= ~(1 << 10);
         base_spi_write(SPI_REG_MASTER, master);
 }

 static void base_spi_set_cs(struct base_spi *spi, int enable) //------ set Chip-Selection
 {
         int cs = spi->chip_select;
         u32 polar = 0;
         base_spi_reset(0);//--set half duplex
         if(cs > 1)
                 base_gpio_set(cs,GPIO_OUT_H);
         if (enable){
                 if(cs > 1)  //-------------------------------- default chip 0,1
                         base_gpio_set(cs,GPIO_OUT_L);
                 else
                         polar = BIT(cs); //-----------------------
                 }
         base_spi_write(SPI_REG_CS_POLAR, polar);
 }

 static int base_spi_prepare(struct base_spi* spi) //----prepare spi, set RATE and MODE
 {
         u32 rate;
         u32 reg;

 //      printk("speed:%u\n", spi->speed);

         rate = DIV_ROUND_UP(spi->sys_freq,spi->speed); //
 //      printk( "rate-1:%u\n", rate);

         if (rate > 4097)
                 return -EINVAL;

         if (rate < 2)
                 rate = 2;

         reg = base_spi_read(SPI_REG_MASTER);  //-------------  set rs_clk_sel
         reg &= ~(0xfff << 16);
         reg |= (rate - 2) << 16;

         reg &= ~SPI_REG_LSB_FIRST; //-------------------------------  MSB FIRST !!!!!!!!!!!!!!!!!!
         if (spi->mode & SPI_LSB_FIRST)
                 reg |= SPI_REG_LSB_FIRST;

         reg &= ~(SPI_REG_CPHA | SPI_REG_CPOL);
         switch(spi->mode & (SPI_CPOL | SPI_CPHA)) {
                 case SPI_MODE_0:
                         break;
                 case SPI_MODE_1:
                         reg |= SPI_REG_CPHA;
                         break;
                 case SPI_MODE_2:
                         reg |= SPI_REG_CPOL;
                         break;
                 case SPI_MODE_3:
                         reg |= SPI_REG_CPOL | SPI_REG_CPHA;
                         break;
         }
         base_spi_write(SPI_REG_MASTER, reg);

         return 0;
 }


 static int base_spi_transfer_half_duplex(struct base_spi *m)
 {
         unsigned long flags;
         int status = 0;
         int i, len = 0;
         int rx_len = 0;
         u32 data[9] = { 0 };
         u32 val;
         spin_lock_irqsave(&m->lock, flags); //---lock data
         base_spi_busy_wait();

         u8 *buf = m->tx_buf;
         for (i = 0; i < m->tx_len; i++, len++)
                 data[len / 4] |= buf[i] << (8 * (len & 3));  //---------- load data

         if (WARN_ON(rx_len > 32)) {
                 status = -EIO;
                 goto msg_done;
         }

         if (base_spi_prepare(m)) {
                 status = -EIO;
                 goto msg_done;
         }

         data[0] = swab32(data[0]);
         if (len < 4)
                 data[0] >>= (4 - len) * 8;

         for (i = 0; i < len; i += 4)
                 base_spi_write(SPI_REG_OPCODE + i, data[i / 4]);

         val = (min_t(int, len, 4) * 8) << 24;
         if (len > 4)
                 val |= (len - 4) * 8;
         val |= (rx_len * 8) << 12;
         base_spi_write(SPI_REG_MOREBUF, val);

         base_spi_set_cs(m, 1); //-------------------------------------- base_spi_set_cs

         val = base_spi_read(SPI_REG_CTL);
         val |= SPI_CTL_START;
         base_spi_write(SPI_REG_CTL, val);

         base_spi_busy_wait();

         base_spi_set_cs(m, 0); //-------------------------------------- base_spi_set_cs

         for (i = 0; i < rx_len; i += 4)
                 data[i / 4] = base_spi_read(SPI_REG_DATA0 + i);

         len = 0;
         buf = m->rx_buf;
         for (i = 0; i < m->rx_len; i++, len++)
                 buf[i] = data[len / 4] >> (8 * (len & 3));

 msg_done:
         spin_unlock_irqrestore(&m->lock, flags);
 //      m->status = status;
 //      spi_finalize_current_message(master);
         return status;
 }

#endif  // _SHUT_FUNCTIONS_ 


static int __init spi_LCD_init(void)
 {
	int k;
         int result=0;
         u32 gpiomode;

         //---------------  to shown and confirm expected gpio mode  --------------------------------

         gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
         printk("--------current RALINK_REG_GPIOMODE value: %08x !\n",gpiomode);

/*
         base_gpio_set(11,1);
         mdelay(100);
         base_gpio_set(11,0);
         mdelay(100);
         base_gpio_set(11,1);
         mdelay(100);

*/
         //struct base_spi spi_LCD;
         spi_LCD.chip_select = 1;
         spi_LCD.mode = SPI_MODE_3 ;
         spi_LCD.speed = 35000000;
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
	LCD_ColorBox(0,0,320,480,1080);//0x8010;
	//mdelay(1000);
	LCD_ColorBox(0,0,320,480,4000);//0x8400);
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

