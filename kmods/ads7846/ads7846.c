/*--------------------------------------------------------------------------------------
 * ADS7846 based touchscreen and sensor driver
 *
 * Copyright (c) 2005 David Brownell
 * Copyright (c) 2006 Nokia Corporation
 * Various changes: Imre Deak <imre.deak@nokia.com>
 *
 * Using code from:
 *  - corgi_ts.c
 *	Copyright (C) 2004-2005 Richard Purdie
 *  - omap_ts.[hc], ads7846.h, ts_osk.c
 *	Copyright (C) 2002 MontaVista Software
 *	Copyright (C) 2004 Texas Instruments
 *	Copyright (C) 2005 Dirk Behme
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 ---------------------------------------------------------------------------------


		 <<<-----  Modified for Openwrt on WidoraNEO  ----->>>

__Functions__:
 1. ads7846_setup_spi_msg(): setup xfers for ts->msg[]
 //2. ads7846_read_state(): read msg as as above setup ts-msg[].
 3. ads7846_probe_dt(): read params from device tree.
 4. ads7846_get_value(ts, *m): Convert and return the meaningful value of the variable that  spi_message rx_buf refers.
 5. ads7846_update_value *m, val):  Update the value of the variable that spi_message pointer rx_buf refers
 6. ads7846_read_state(ts): SPI write commands then read data, item by item, as per ts->msg[5]. (READ_Y, READ_X, READ_Z1,READ_Z2, PWRDOWN)
			   ts->msg[] are setup by ads7846_setup_spi_msg().
 7. ads7846_report_state(ts); Reprot x/y/Rt as ABS_X/Y/PRESSURE to input subsystem.
 8. ads7846_read12_ser(dev, command): read value separately. one call one read.

__DTS_config__:
     ... ...
              ads7846@2 {
                      #address-cells = <1>;
                      #size-cells = <1>;
                      compatible = "ti,ads7846";
                      reg = <2 0>;
                      spi-max-frequency = <2000000>;
                      cs-gpios = <&gpio1 9 1>;       // GPIO41

		 1. ///// gpio_to_irq() to get proper IRQ numerber  ///////
                      interrupt-parent = <&gpio1>;   // TODO: irq_of_parse_and_map() get wrong IRQ!
                      interrupts = <8>;              // GPIO40
		      // interrupts = <8>;  of parse and get hwirq=8!
		      // interrupts = <40>;  then of parse and get spi->irq=0, ERROR!

                      pendown-gpio = <&gpio1 8 1>;   // GPIO40
                      //vcc-supply = <&reg_vcc3>;

                      ti,x-min = <0>;
                      ti,x-max = <240>;
                      ti,y-min = <0>;
                      ti,y-max = <320>;
                      ti,x-plate-ohms = <40>;
                      ti,pressure-max = <255>;

                      //wakeup-source;
              };
      ... ...

Note:
1. spi->irq ERROR! (see mark _h_k_ )
2. For half_dual SPI stransfer:
   2.1 hdsync_write_then_read_spi() to be called as per spi_sync(). OR spi irq MAY be disrupted?!
   2.2 MAX 36Bytes total TX+RX buffer in one spi_message.
   2.3 XXX spi_sync(spi, msg): the msg allows MAX. 2 spi_transfers!
3. Set SPI_MODE = 3!
4. The touch pressure resistance Rt: ( Convert to 8bits before report as ABS_PRESSURE )
   4.1 Light touch pressure results in larger raw_Rt value.
   4.2 Big touch area results in larger raw_Rt value. ( Finger vs. pen )
   4.3 Equation#2 for Rt calcuation has 12bit resolution. we need 8bit ONLY, so >>4 first!
       see in ads7846_report_state(). <---------
   4.4 To revert above values(see abov 4.1-4.2), and get ABS_PRESSURE as:
       ABS_PRESSURE = pressure_max - (Rt>>4)   ( pressure_max is given in dts, here =255 )
5. x/y report as ABS_X/ABS_Y and keep in 12bit resolution.
6. Delay a while after truning on inernal vREF, OR tmp0 value will be unstable.
   See MidasHK_VrefDelay.

Journal:
2022-01-16:
	1. Check and correct: spi->irq=gpio_to_irq()! <----- ERROR SOURCE?
2022-01-17:
	1. Add hdsync_read_spi() and hdsync_write_spi()
	2. Add hdsync_write_then_read_spi()
	3. Rewrite ads7846_read12_ser(): for harl dual SPI transfer
2022-01-18:
	1. Set SPI_MODE = 3
	2. Test X/Y/Rt
	3. Test read /dev/input/eventX and mouseX
2022-01-24:
	1. Enable and test hwmon.
2022-01-28:
	1. ads7846_read12_ser(): delay a while after truning on inernal vREF, to let it be stable
	   before reading tmp0/tmp1 etc...

TODO:
XXX 1. Second insmod will cause gpio_request_one() <test_and_set_bit??>  error!
2. To confirm x-plate-ohms. ti,x-plate-ohms = <40> ??
                           ti,pressure-max = <255>
                input_report_abs(input, ABS_X, x);
                input_report_abs(input, ABS_Y, y);
                input_report_abs(input, ABS_PRESSURE, ts->pressure_max - Rt);
3. spi->irq ERROR!
   spi->irq=irq_of_parse_and_map() and irq=gpio_to_irq(40) is NOT SAME!


Midas Zhou
midaszhou@yahoo.com
--------------------------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/hwmon.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/of_irq.h> /* irq_of_parse_and_map() */
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <asm/irq.h>


/* MidasHK_hwmon: -------------------------- */
#define CONFIG_HWMON 1


/*
 * This code has been heavily tested on a Nokia 770, and lightly
 * tested on other ads7846 devices (OSK/Mistral, Lubbock, Spitz).
 * TSC2046 is just newer ads7846 silicon.
 * Support for ads7843 tested on Atmel at91sam926x-EK.
 * Support for ads7845 has only been stubbed in.
 * Support for Analog Devices AD7873 and AD7843 tested.
 *
 * IRQ handling needs a workaround because of a shortcoming in handling
 * edge triggered IRQs on some platforms like the OMAP1/2. These
 * platforms don't handle the ARM lazy IRQ disabling properly, thus we
 * have to maintain our own SW IRQ disabled status. This should be
 * removed as soon as the affected platform's IRQ handling is fixed.
 *
 * App note sbaa036 talks in more detail about accurate sampling...
 * that ought to help in situations like LCDs inducing noise (which
 * can also be helped by using synch signals) and more generally.
 * This driver tries to utilize the measures described in the app
 * note. The strength of filtering can be set in the board-* specific
 * files.
 */

#define TS_POLL_DELAY	1	/* ms delay before the first sample */
#define TS_POLL_PERIOD	5	/* ms delay between samples */

/* this driver doesn't aim at the peak continuous sample rate */
#define	SAMPLE_BITS	(8 /*cmd*/ + 16 /*sample*/ + 2 /* before, after */)

struct ts_event {
	/*
	 * For portability, we can't read 12 bit values using SPI (which
	 * would make the controller deliver them as native byte order u16
	 * with msbs zeroed).  Instead, we read them as two 8-bit values,
	 * *** WHICH NEED BYTESWAPPING *** and range adjustment.
	 */
	u16	x;
	u16	y;
	u16	z1, z2;
	bool	ignore;
	u8	x_buf[3];
	u8	y_buf[3];
};

/*
 * We allocate this separately to avoid cache line sharing issues when
 * driver is used with DMA-based SPI controllers (like atmel_spi) on
 * systems where main memory is not DMA-coherent (most non-x86 boards).
 */
struct ads7846_packet {
	u8			read_x, read_y, read_z1, read_z2, pwrdown;
	u16			dummy;		/* for the pwrdown read */
	struct ts_event		tc;
	/* for ads7845 with mpc5121 psc spi we use 3-byte buffers */
	u8			read_x_cmd[3], read_y_cmd[3], pwrdown_cmd[3];
};

struct ads7846 {
	struct input_dev	*input;
	char			phys[32];
	char			name[32];

	struct spi_device	*spi;
	struct regulator	*reg;

#if IS_ENABLED(CONFIG_HWMON)
	struct device		*hwmon;
#endif

	u16			model;
	u16			vref_mv;
	u16			vref_delay_usecs;
	u16			x_plate_ohms;
	u16			pressure_max;

	bool			swap_xy;
	bool			use_internal;

	struct ads7846_packet	*packet;

	struct spi_transfer	xfer[18];
	struct spi_message	msg[5];
	int			msg_count;
	wait_queue_head_t	wait;

	bool			pendown;

	int			read_cnt;
	int			read_rep;
	int			last_read;

	u16			debounce_max;
	u16			debounce_tol;
	u16			debounce_rep;

	u16			penirq_recheck_delay_usecs;

	struct mutex		lock;
	bool			stopped;	/* P: lock */
	bool			disabled;	/* P: lock */
	bool			suspended;	/* P: lock */

	int			(*filter)(void *data, int data_idx, int *val);
	void			*filter_data;
	void			(*filter_cleanup)(void *data);
	int			(*get_pendown_state)(void);
	int			gpio_pendown;

	void			(*wait_for_sync)(void);
};

/* leave chip selected when we're done, for quicker re-select? */
#if	0
#define	CS_CHANGE(xfer)	((xfer).cs_change = 1)
#else
#define	CS_CHANGE(xfer)	((xfer).cs_change = 0)
#endif

/*--------------------------------------------------------------------------*/

/*---------------------- ADS7846 Control Bytes --------------------
   Control byte:
    Bit 7(MSb)   6     5    4     3      2        1     0(LSB)
        S       A2    A1    A0   MODE  SER/~DFR  PD1    PD0

	  S:  Start bit
      A2-A0:  Channel select bits.
       MODE:  12-Bit/8-Bit conversion select bit.
    ER/~DRF:  Single-Ended/Differential referene select bit.
    PD1-PD0:  Power-down mode select.
	      00  Power-down between conversion
	      01  Disabled, Reference is OFF and ADC is ON
	      10  Enabled, Reference is ON and ADC is OFF
	      11  Disabled, device is always powered. Refrence is ON and ACD is ON.


Note:
   1. With Vrev equals to +2.5V, one LSB is 610uV = 2.5/4096.
   2. Typically, the internal reference voltage is only used inn the single-ended mode
      for battery monitoring, temperature measurement, and for using the auxiliary input.
      TODO: pin vbatt NOW is grounded.
   
   3. Temperautre measurement:
      The PENIRQ diode is used(turned on) during tmp0/tmp1 measurement cycle!
      Vrev/4096=2.5v/4096=610uv=0.61mv
      3.1 tmp0 ONLY: (typically 600mV at +25C, and -2.1mV/C)
	C=25-(600-x*0.61)*2.1 = 25-(600-873*0.61)*2.1= XXX
      3.2 tmp0+tmp1: (Dx=Xtmp1-Xtmp0)
	C=2.573*DV(mv)-273 =2.573*Dx*0.61-273=1.57*Dx-273=
	  1.57*(1003-873)-273= 	XXX (internal Vref ON)
	  1.57*(891-773)-273=	    (internal Vref OFF)
 ----------------------------------------------------------------*/

/* The ADS7846 has touchscreen and other sensors.
 * Earlier ads784x chips are somewhat compatible.
 */
#define	ADS_START		(1 << 7)
#define	ADS_A2A1A0_d_y		(1 << 4)	/* differential */
#define	ADS_A2A1A0_d_z1		(3 << 4)	/* differential */
#define	ADS_A2A1A0_d_z2		(4 << 4)	/* differential */
#define	ADS_A2A1A0_d_x		(5 << 4)	/* differential */
#define	ADS_A2A1A0_temp0	(0 << 4)	/* non-differential */
#define	ADS_A2A1A0_vbatt	(2 << 4)	/* non-differential */
#define	ADS_A2A1A0_vaux		(6 << 4)	/* non-differential */
#define	ADS_A2A1A0_temp1	(7 << 4)	/* non-differential */
#define	ADS_8_BIT		(1 << 3)
#define	ADS_12_BIT		(0 << 3)
#define	ADS_SER			(1 << 2)	/* non-differential */
#define	ADS_DFR			(0 << 2)	/* differential */
#define	ADS_PD10_PDOWN		(0 << 0)	/* low power mode + penirq */
#define	ADS_PD10_ADC_ON		(1 << 0)	/* ADC on */
#define	ADS_PD10_REF_ON		(2 << 0)	/* vREF on + penirq */
#define	ADS_PD10_ALL_ON		(3 << 0)	/* ADC + vREF on + penirq */

#define	MAX_12BIT	((1<<12)-1)

/* leave ADC powered up (disables penirq) between differential samples */

#if 0 /* MidasHK_1: ------------Keep ADC/vREF OFF ... Same effect... */
  #define READ_12BIT_DFR(x, adc, vref) (ADS_START | ADS_A2A1A0_d_ ## x \
	| ADS_12_BIT | ADS_DFR )
#else
  #define READ_12BIT_DFR(x, adc, vref) (ADS_START | ADS_A2A1A0_d_ ## x \
	| ADS_12_BIT | ADS_DFR |  \
	(adc ? ADS_PD10_ADC_ON : 0) | (vref ? ADS_PD10_REF_ON : 0))
#endif

#define	READ_Y(vref)	(READ_12BIT_DFR(y,  1, vref))
#define	READ_Z1(vref)	(READ_12BIT_DFR(z1, 1, vref))
#define	READ_Z2(vref)	(READ_12BIT_DFR(z2, 1, vref))

#define	READ_X(vref)	(READ_12BIT_DFR(x,  1, vref))
#define	PWRDOWN		(READ_12BIT_DFR(y,  0, 0))	/* LAST */

/* single-ended samples need to first power up reference voltage;
 * we leave both ADC and VREF powered
 */
#define	READ_12BIT_SER(x) (ADS_START | ADS_A2A1A0_ ## x \
	| ADS_12_BIT | ADS_SER)

#define	REF_ON	(READ_12BIT_DFR(x, 1, 1))
#define	REF_OFF	(READ_12BIT_DFR(y, 0, 0))

/* Must be called with ts->lock held */
static void ads7846_stop(struct ads7846 *ts)
{
	if (!ts->disabled && !ts->suspended) {
		/* Signal IRQ thread to stop polling and disable the handler. */
		ts->stopped = true;
		mb();
		wake_up(&ts->wait);
		disable_irq(ts->spi->irq);
	}
}

/* Must be called with ts->lock held */
static void ads7846_restart(struct ads7846 *ts)
{
	if (!ts->disabled && !ts->suspended) {
		/* Tell IRQ thread that it may poll the device. */
		ts->stopped = false;
		mb();
		enable_irq(ts->spi->irq);
	}
}

/* Must be called with ts->lock held */
static void __ads7846_disable(struct ads7846 *ts)
{
	ads7846_stop(ts);
	regulator_disable(ts->reg);

	/*
	 * We know the chip's in low power mode since we always
	 * leave it that way after every request
	 */
}

/* Must be called with ts->lock held */
static void __ads7846_enable(struct ads7846 *ts)
{
	int error;

	error = regulator_enable(ts->reg);
	if (error != 0)
		dev_err(&ts->spi->dev, "Failed to enable supply: %d\n", error);

	ads7846_restart(ts);
}

static void ads7846_disable(struct ads7846 *ts)
{
	mutex_lock(&ts->lock);

	if (!ts->disabled) {

		if  (!ts->suspended)
			__ads7846_disable(ts);

		ts->disabled = true;
	}

	mutex_unlock(&ts->lock);
}

static void ads7846_enable(struct ads7846 *ts)
{
	mutex_lock(&ts->lock);

	if (ts->disabled) {

		ts->disabled = false;

		if (!ts->suspended)
			__ads7846_enable(ts);
	}

	mutex_unlock(&ts->lock);
}

/////////// MidasHK_15 : To test irq_of_parse_and_map() if it parse the right hwirq! ///////////

int test_of_parse_hwirq(struct device_node *dev, int index)
{
        struct of_phandle_args oirq;
        irq_hw_number_t hwirq;
	struct irq_domain *domain;
        unsigned int type = IRQ_TYPE_NONE;
        unsigned int virq;

        if (of_irq_parse_one(dev, index, &oirq)) {
printk("of_irq_parse_one(dev, index, &oirq) fails!\n");
                return 0;
	}

        domain = (oirq.np ? irq_find_host(oirq.np) : NULL);
        if (!domain) {
                pr_warn("no irq domain found for %s !\n",
                        of_node_full_name(oirq.np));
                return 0;
        }

        /* If domain has no translation, then we assume interrupt line */
        if (domain->ops->xlate == NULL) {
printk("%s: domain->ops->xlate == NULL! hwirq = oirq.args[0]! \n", __func__);
                hwirq = oirq.args[0];
	}
        else {
                if (domain->ops->xlate(domain, oirq.np, oirq.args,
                                        oirq.args_count, &hwirq, &type))
                        return 0;
        }

printk("%s: of parse and get hwireq=%d\n", __func__, (int)hwirq);

	return hwirq;
}


/*--------------------------------------------------------------------------*/

/*
 * Non-touchscreen sensors only use single-ended conversions.
 * The range is GND..vREF. The ads7843 and ads7835 must use external vREF;
 * ads7846 lets that pin be unconnected, to use internal vREF.
 */

struct ser_req {
	u8			ref_on;
	u8			command;
	u8			ref_off;
	u16			scratch;
	struct spi_message	msg;
	struct spi_transfer	xfer[6];
	/*
	 * DMA (thus cache coherency maintenance) requires the
	 * transfer buffers to live in their own cache lines.
	 */
	__be16 sample ____cacheline_aligned;
};

struct ads7845_ser_req {
	u8			command[3];
	struct spi_message	msg;
	struct spi_transfer	xfer[2];
	/*
	 * DMA (thus cache coherency maintenance) requires the
	 * transfer buffers to live in their own cache lines.
	 */
	u8 sample[3] ____cacheline_aligned;
};

///////////////// MidasHK_2:  half-dual SPI transfer ///////////////
int hdsync_write_spi(struct spi_device *spi, const void *buf, size_t len)
{

	struct spi_message m;
	struct spi_transfer xfer= {
		.tx_buf =buf,
		.len =len,
	};

	if(spi==NULL || buf==NULL)
		return -1;

	/* TODO: CHECK DMA: m.is_dma_apped =1; */
	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);

	return spi_sync(spi, &m);
}
int hdsync_read_spi(struct spi_device *spi, void *buf, size_t len)
{
	struct spi_message m;
	struct spi_transfer xfer= {
		.rx_buf =buf,
		.len =len,
	};

	if(spi==NULL || buf==NULL)
		return -1;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);

	return spi_sync(spi, &m);
}
int hdsync_write_then_read_spi(struct spi_device *spi, const void *txbuf, size_t txlen, void *rxbuf, size_t rxlen)
{

	struct spi_message m;
	struct spi_transfer xfer[2];

	if(spi==NULL || txbuf==NULL || rxbuf==NULL )
		return -1;

	memset(xfer, 0, sizeof(xfer));
	xfer[0].tx_buf=txbuf;
	xfer[0].len=txlen;
	xfer[1].rx_buf=rxbuf;
	xfer[1].len=rxlen;

	spi_message_init(&m);
	spi_message_add_tail(&xfer[0], &m);
	spi_message_add_tail(&xfer[1], &m);

#if 0 /* TEST: -------CHECK DMA: m.is_dma_mapped =1; */
	if(m.is_dma_mapped)
		printk(" ------ spi DMA ------\n");
	else
		printk(" ------ NO spi DMA ------\n");
#endif

	return spi_sync(spi, &m);
}


#if 0 /////////////////////////////////////////////////////////////////
/* MidasHK_3 Rewrite spi_sync() for half-dual SPI transfer
 * NOTE:
 * 1. Corresponding with ads7846_setup_spi_msg()
 * 2. For ads7846 ONLY.
 */
int spi_hdsync(struct ads7846 *ts, int msg_count)
//int spi_hdsync(struct spi_device *spi, struct spi_message *m)
{
	struct spi_message *m = &ts->msg[0];
	struct spi_transfer *x = ts->xfer;
	struct ads7846_packet *packet = ts->packet;

/* msg_count=0: READ_Y  */
	hdsync_write_then_read_spi(ts->spi, x->tx_buf, 1, (x+1)->rx_buf, 2);
	x += 2;
	/* TODO:  if (pdata->settle_delay_usecs) { } */

/* msg_count=1: READ_X  */
	hdsync_write_then_read_spi(ts->spi, x->tx_buf, 1, (x+1)->rx_buf, 2);
	x += 2;
	/* TODO:  if (pdata->settle_delay_usecs) { } */

/* msg_count=2: READ_Z1  */
	hdsync_write_then_read_spi(ts->spi, x->tx_buf, 1, (x+1)->rx_buf, 2);
	x += 2;
	/* TODO:  if (pdata->settle_delay_usecs) { } */

/* msg_count=3: READ_Z2  */
	hdsync_write_then_read_spi(ts->spi, x->tx_buf, 1, (x+1)->rx_buf, 2);
	x += 2;
	/* TODO:  if (pdata->settle_delay_usecs) { } */

/* msg_count=4: PWRDOWN  */
	hdsync_write_then_read_spi(ts->spi, x->tx_buf, 1, (x+1)->rx_buf, 2);
	x += 2;
	/* TODO:  if (pdata->settle_delay_usecs) { } */

	return 0;
}
#endif ///////////////////////////////////////////////////////////////////////////////////////

#if 1 //////////// MidasHK_4: Re_write 2ds7846_read12_ser() for harl dual SPI transfer ////////////
static int ads7846_read12_ser(struct device *dev, unsigned command)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads7846 *ts = dev_get_drvdata(dev);
	struct ser_req *req;
	int status=0;  /* <0 as fails */
	int k;

printk("%s: --- 0 ---\n", __func__);
	req=kzalloc(sizeof *req, GFP_KERNEL);
	if(!req)
		return -ENOMEM;

	spi_message_init(&req->msg);

/* ---------------> Mutex_lock AND Disable_IRQ */
	mutex_lock(&ts->lock);
	ads7846_stop(ts);

/* MidasHK_5: turn ON/OFF internal vREF --- OK, ALready set ts->use_internal in ads784x_hwmon_register().  */
//ts->use_internal=true;

	/* maybe trun on inernal vREF, and let it settle */
	if(ts->use_internal) {
printk("%s: Use internal vREF!\n", __func__);
		req->ref_on=REF_ON;
		req->xfer[0].tx_buf=&req->ref_on;
		req->xfer[0].len=1;
//		status +=hdsync_write_spi(spi, req->xfer[0].tx_buf, req->xfer[0].len);
		req->xfer[1].rx_buf=&req->scratch;
		req->xfer[1].len=2;
		/* for 1uF, settle for 800 usec; no cap, 100 usec.  */
		//req->xfer[1].delay_usecs = ts->vref_delay_usecs;
		req->xfer[1].delay_usecs = 200;
//		status +=hdsync_read_spi(spi, req->xfer[1].rx_buf, req->xfer[1].len);

#if 0 /* ------------------ Only xfer[2] and xfer[3]! -- */
		spi_message_add_tail(&req->xfer[0], &req->msg);
		spi_message_add_tail(&req->xfer[1], &req->msg);
		spi_sync(ts->spi, &req->msg);
#else
		status +=hdsync_write_then_read_spi(spi, req->xfer[0].tx_buf, req->xfer[0].len,
							req->xfer[1].rx_buf, req->xfer[1].len );
#endif

		/* Enable refrence voltage */
		command |= ADS_PD10_REF_ON;
	}
	else {
printk("%s: NOT use internal vREF!\n", __func__);
	}

/* MidasHK_VrefDelay: ------------delay to let internal vref settle down... */
	udelay(100000);

	/* Enable ADC in every case */
	command |= ADS_PD10_ADC_ON;

for(k=0; k<10; k++) {
	/* take sample */
	req->command = (u8)command;
	req->xfer[2].tx_buf = &req->command;
	req->xfer[2].len = 1;
	req->xfer[3].rx_buf = &req->sample;   /* <---------- SAMPLE --- */
	req->xfer[3].len = 2;

#if 0 /* ------------------ Only xfer[2] and xfer[3]! -- */
	spi_message_add_tail(&req->xfer[2], &req->msg);
	spi_message_add_tail(&req->xfer[3], &req->msg);
	spi_sync(ts->spi, &req->msg);
#else
	status +=hdsync_write_then_read_spi(spi, req->xfer[2].tx_buf, req->xfer[2].len,
						req->xfer[3].rx_buf, req->xfer[3].len);
#endif
	/* REVISIT:  take a few more samples, and compare ... */
	if( req->sample !=0 )
		break;
	udelay(10000);
}

	/* convert in low power mode & enable PENIRQ */
	req->ref_off = PWRDOWN;
	req->xfer[4].tx_buf = &req->ref_off;
	req->xfer[4].len = 1;
	req->xfer[5].rx_buf = &req->scratch;
	req->xfer[5].len = 2;
	CS_CHANGE(req->xfer[5]);


	status +=hdsync_write_then_read_spi(spi, req->xfer[4].tx_buf, req->xfer[4].len,
						req->xfer[5].rx_buf, req->xfer[5].len);

printk("%s: Sumup hdsync_spi() status=%d\n", __func__, status);

/* <--------------- Mutex_unlock AND Enable_IRQ */
	ads7846_restart(ts);  /* Enable IRQ */
	mutex_unlock(&ts->lock);

	/* If all _read/write_spi() succeeded */
	if(status==0) {
		/* On-wire is a must-ignore bit, a BE12 value, then adding */
		status = be16_to_cpu(req->sample);
//printk("%s: ads7846 raw status: 0x%04X\n", __func__, status); // ==0x07F8
		status = status >>3;
		status &= 0x0fff;
	}
//printk("%s: ads7846 ret status: 0x%04X\n", __func__, status); // ==0x07F8

	kfree(req);
	return status;
}

#else  //////////////// The old/orignal ads7846_read12_ser()  ////////////////////

static int ads7846_read12_ser(struct device *dev, unsigned command)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads7846 *ts = dev_get_drvdata(dev);
	struct ser_req *req;
	int status;

	req = kzalloc(sizeof *req, GFP_KERNEL);
	if (!req)
		return -ENOMEM;

//printk("%s: ----------- 1 --------\n", __func__);
	spi_message_init(&req->msg);

	/* maybe turn on internal vREF, and let it settle */
	if (ts->use_internal) {
		req->ref_on = REF_ON;
		req->xfer[0].tx_buf = &req->ref_on;
		req->xfer[0].len = 1;
		spi_message_add_tail(&req->xfer[0], &req->msg);

		req->xfer[1].rx_buf = &req->scratch;
		req->xfer[1].len = 2;

		/* for 1uF, settle for 800 usec; no cap, 100 usec.  */
		req->xfer[1].delay_usecs = ts->vref_delay_usecs;
		spi_message_add_tail(&req->xfer[1], &req->msg);

		/* Enable reference voltage */
		command |= ADS_PD10_REF_ON;
	}

	/* Enable ADC in every case */
	command |= ADS_PD10_ADC_ON;

	/* take sample */
	req->command = (u8) command;
	req->xfer[2].tx_buf = &req->command;
	req->xfer[2].len = 1;
	spi_message_add_tail(&req->xfer[2], &req->msg);

	req->xfer[3].rx_buf = &req->sample;
	req->xfer[3].len = 2;
	spi_message_add_tail(&req->xfer[3], &req->msg);

	/* REVISIT:  take a few more samples, and compare ... */

	/* converter in low power mode & enable PENIRQ */
	req->ref_off = PWRDOWN;
	req->xfer[4].tx_buf = &req->ref_off;
	req->xfer[4].len = 1;
	spi_message_add_tail(&req->xfer[4], &req->msg);

	req->xfer[5].rx_buf = &req->scratch;
	req->xfer[5].len = 2;
	CS_CHANGE(req->xfer[5]);
	spi_message_add_tail(&req->xfer[5], &req->msg);

	mutex_lock(&ts->lock);
	ads7846_stop(ts);

	status = spi_sync(spi, &req->msg);

	ads7846_restart(ts);
	mutex_unlock(&ts->lock);

printk("%s: ----------- 5 --------\n", __func__);
	if (status == 0) {
		/* on-wire is a must-ignore bit, a BE12 value, then padding */
		status = be16_to_cpu(req->sample);
		status = status >> 3;
		status &= 0x0fff;
	}
printk("%s: ads7846 ret status: 0x%04X\n", __func__, status);

	kfree(req);
	return status;
}
#endif /////////////////////////////////////////////////////////////////////


static int ads7845_read12_ser(struct device *dev, unsigned command)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads7846 *ts = dev_get_drvdata(dev);
	struct ads7845_ser_req *req;
	int status;

	req = kzalloc(sizeof *req, GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	spi_message_init(&req->msg);

	req->command[0] = (u8) command;
	req->xfer[0].tx_buf = req->command;
	req->xfer[0].rx_buf = req->sample;
	req->xfer[0].len = 3;

	spi_message_add_tail(&req->xfer[0], &req->msg);

	mutex_lock(&ts->lock);
	ads7846_stop(ts);
	status = spi_sync(spi, &req->msg);
	ads7846_restart(ts);
	mutex_unlock(&ts->lock);

	if (status == 0) {
		/* BE12 value, then padding */
		status = be16_to_cpu(*((u16 *)&req->sample[1]));
		status = status >> 3;
		status &= 0x0fff;
	}

	kfree(req);
	return status;
}


#if IS_ENABLED(CONFIG_HWMON)

#define SHOW(name, var, adjust) static ssize_t \
name ## _show(struct device *dev, struct device_attribute *attr, char *buf) \
{ \
	struct ads7846 *ts = dev_get_drvdata(dev); \
	ssize_t v = ads7846_read12_ser(&ts->spi->dev, \
			READ_12BIT_SER(var)); \
	if (v < 0) \
		return v; \
	return sprintf(buf, "%u\n", adjust(ts, v)); \
} \
static DEVICE_ATTR(name, S_IRUGO, name ## _show, NULL);


/* Sysfs conventions report temperatures in millidegrees Celsius.
 * ADS7846 could use the low-accuracy two-sample scheme, but can't do the high
 * accuracy scheme without calibration data.  For now we won't try either;
 * userspace sees raw sensor values, and must scale/calibrate appropriately.
 */
static inline unsigned null_adjust(struct ads7846 *ts, ssize_t v)
{
	return v;
}

SHOW(temp0, temp0, null_adjust)		/* temp1_input */
SHOW(temp1, temp1, null_adjust)		/* temp2_input */


/* sysfs conventions report voltages in millivolts.  We can convert voltages
 * if we know vREF.  userspace may need to scale vAUX to match the board's
 * external resistors; we assume that vBATT only uses the internal ones.
 */
static inline unsigned vaux_adjust(struct ads7846 *ts, ssize_t v)
{
	unsigned retval = v;

	/* external resistors may scale vAUX into 0..vREF */
	retval *= 2500; //ts->vref_mv;  /* If internal, assume vref_mv = 2500, one LSB = 2.5v/4096 */
	retval = retval >> 12; /* Convert to mV */

	return retval;
}

static inline unsigned vbatt_adjust(struct ads7846 *ts, ssize_t v)
{
	unsigned retval = vaux_adjust(ts, v);

	/* ads7846 has a resistor ladder to scale this signal down */
	if (ts->model == 7846)
		retval *= 4;

	return retval;
}

SHOW(in0_input, vaux, vaux_adjust)
SHOW(in1_input, vbatt, vbatt_adjust)

static umode_t ads7846_is_visible(struct kobject *kobj, struct attribute *attr,
				  int index)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct ads7846 *ts = dev_get_drvdata(dev);

	if (ts->model == 7843 && index < 2)	/* in0, in1 */
		return 0;
	if (ts->model == 7845 && index != 2)	/* in0 */
		return 0;

	return attr->mode;
}

static struct attribute *ads7846_attributes[] = {
	&dev_attr_temp0.attr,		/* 0 */
	&dev_attr_temp1.attr,		/* 1 */
	&dev_attr_in0_input.attr,	/* 2 */
	&dev_attr_in1_input.attr,	/* 3 */
	NULL,
};

static struct attribute_group ads7846_attr_group = {
	.attrs = ads7846_attributes,
	.is_visible = ads7846_is_visible,
};
__ATTRIBUTE_GROUPS(ads7846_attr);

static int ads784x_hwmon_register(struct spi_device *spi, struct ads7846 *ts)
{
	/* hwmon sensors need a reference voltage */
	switch (ts->model) {
	case 7846:
		if (!ts->vref_mv) {
			dev_dbg(&spi->dev, "assuming 2.5V internal vREF\n");
			ts->vref_mv = 2500;
			ts->use_internal = true;
		}
		break;
	case 7845:
	case 7843:
		if (!ts->vref_mv) {
			dev_warn(&spi->dev,
				"external vREF for ADS%d not specified\n",
				ts->model);
			return 0;
		}
		break;
	}

	ts->hwmon = hwmon_device_register_with_groups(&spi->dev, spi->modalias,
						      ts, ads7846_attr_groups);
	if (IS_ERR(ts->hwmon))
		return PTR_ERR(ts->hwmon);

	return 0;
}

static void ads784x_hwmon_unregister(struct spi_device *spi,
				     struct ads7846 *ts)
{
	if (ts->hwmon)
		hwmon_device_unregister(ts->hwmon);
}

#else
static inline int ads784x_hwmon_register(struct spi_device *spi,
					 struct ads7846 *ts)
{
	return 0;
}

static inline void ads784x_hwmon_unregister(struct spi_device *spi,
					    struct ads7846 *ts)
{
}
#endif

static ssize_t ads7846_pen_down_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ads7846 *ts = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ts->pendown);
}

static DEVICE_ATTR(pen_down, S_IRUGO, ads7846_pen_down_show, NULL);

static ssize_t ads7846_disable_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ads7846 *ts = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ts->disabled);
}

static ssize_t ads7846_disable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct ads7846 *ts = dev_get_drvdata(dev);
	unsigned int i;
	int err;

	err = kstrtouint(buf, 10, &i);
	if (err)
		return err;

	if (i)
		ads7846_disable(ts);
	else
		ads7846_enable(ts);

	return count;
}

static DEVICE_ATTR(disable, 0664, ads7846_disable_show, ads7846_disable_store);

static struct attribute *ads784x_attributes[] = {
	&dev_attr_pen_down.attr,
	&dev_attr_disable.attr,
	NULL,
};

static struct attribute_group ads784x_attr_group = {
	.attrs = ads784x_attributes,
};

/*--------------------------------------------------------------------------*/

static int get_pendown_state(struct ads7846 *ts)
{
	if (ts->get_pendown_state)
		return ts->get_pendown_state();

	return !gpio_get_value(ts->gpio_pendown);
}

static void null_wait_for_sync(void)
{
}

static int ads7846_debounce_filter(void *ads, int data_idx, int *val)
{
	struct ads7846 *ts = ads;

	if (!ts->read_cnt || (abs(ts->last_read - *val) > ts->debounce_tol)) {
		/* Start over collecting consistent readings. */
		ts->read_rep = 0;
		/*
		 * Repeat it, if this was the first read or the read
		 * wasn't consistent enough.
		 */
		if (ts->read_cnt < ts->debounce_max) {
			ts->last_read = *val;
			ts->read_cnt++;
			return ADS7846_FILTER_REPEAT;
		} else {
			/*
			 * Maximum number of debouncing reached and still
			 * not enough number of consistent readings. Abort
			 * the whole sample, repeat it in the next sampling
			 * period.
			 */
			ts->read_cnt = 0;
			return ADS7846_FILTER_IGNORE;
		}
	} else {
		if (++ts->read_rep > ts->debounce_rep) {
			/*
			 * Got a good reading for this coordinate,
			 * go for the next one.
			 */
			ts->read_cnt = 0;
			ts->read_rep = 0;
			return ADS7846_FILTER_OK;
		} else {
			/* Read more values that are consistent. */
			ts->read_cnt++;
			return ADS7846_FILTER_REPEAT;
		}
	}
}

static int ads7846_no_filter(void *ads, int data_idx, int *val)
{
	return ADS7846_FILTER_OK;
}

/* Convert and return the meaningful value of the variable that  spi_message rx_buf refers. */
static int ads7846_get_value(struct ads7846 *ts, struct spi_message *m)
{
	struct spi_transfer *t =
		list_entry(m->transfers.prev, struct spi_transfer, transfer_list);

	if (ts->model == 7845) {
		return be16_to_cpup((__be16 *)&(((char*)t->rx_buf)[1])) >> 3;
	} else {
		/*
		 * adjust:  on-wire is a must-ignore bit, a BE12 value, then
		 * padding; built from two 8 bit values written msb-first.
		 */
		return be16_to_cpup((__be16 *)t->rx_buf) >> 3;
	}
}

/* Update value of the variable that spi_message pointer rx_buf refers */
static void ads7846_update_value(struct spi_message *m, int val)
{
	struct spi_transfer *t =
		list_entry(m->transfers.prev, struct spi_transfer, transfer_list);

/* MidasHK_6: Notice in  ads7846_setup_spi_msg() set  x->rx_buf = &packet->tc.x,->tc.y, -> etc ...; */

	*(u16 *)t->rx_buf = val;
}

static void ads7846_read_state(struct ads7846 *ts)
{
	struct ads7846_packet *packet = ts->packet;
	struct spi_message *m;
	int msg_idx = 0;
	int val;
	int action;
	int error;

	while (msg_idx < ts->msg_count) {   /* ts->msg[5] */

		ts->wait_for_sync();

		m = &ts->msg[msg_idx];

/* MidasHK_7: -------- See spi_message limit for half_dual spi transfer  */
		error = spi_sync(ts->spi, m);
		if (error) {
			dev_err(&ts->spi->dev, "spi_sync --> %d\n", error);
			packet->tc.ignore = true;
			return;
		}

		/***
		 * Last message is power down request, no need to convert
		 * or filter the value.
		 */
		if (msg_idx < ts->msg_count - 1) {

			val = ads7846_get_value(ts, m);
//if(msg_idx==0) printk("%s: X val=0x%04X\n", __func__, val);
//if(msg_idx==1) printk("%s: Y val=0x%04X\n", __func__, val);

			action = ts->filter(ts->filter_data, msg_idx, &val);
			switch (action) {
			case ADS7846_FILTER_REPEAT:
				continue;

			case ADS7846_FILTER_IGNORE:
				packet->tc.ignore = true;
				msg_idx = ts->msg_count - 1;
				continue;

			case ADS7846_FILTER_OK:
//printk( "ADS7846_FILTER_OK\n");
				ads7846_update_value(m, val);
				packet->tc.ignore = false;
				msg_idx++;
				break;

			default:
				BUG();
			}
		} else {
			msg_idx++;
		}
	}
}

static void ads7846_report_state(struct ads7846 *ts)
{
	struct ads7846_packet *packet = ts->packet;
	unsigned int Rt;
	u16 x, y, z1, z2;

	/*
	 * ads7846_get_value() does in-place conversion (including byte swap)
	 * from on-the-wire format as part of debouncing to get stable
	 * readings.
	 */
	if (ts->model == 7845) {
		x = *(u16 *)packet->tc.x_buf;
		y = *(u16 *)packet->tc.y_buf;
		z1 = 0;
		z2 = 0;
	} else {
		x = packet->tc.x;
		y = packet->tc.y;
		z1 = packet->tc.z1;
		z2 = packet->tc.z2;
	}
//printk( "x=%d, y=%d, z1=%d, z2=%d\n", x,y,z1,z2);

	/* range filtering */
	if (x == MAX_12BIT)
		x = 0;

	if (ts->model == 7843) {
		Rt = ts->pressure_max / 2;
	} else if (ts->model == 7845) {
		if (get_pendown_state(ts))
			Rt = ts->pressure_max / 2;
		else
			Rt = 0;
		dev_vdbg(&ts->spi->dev, "x/y: %d/%d, PD %d\n", x, y, Rt);
	} else if (likely(x && z1)) {
		/* compute touch pressure resistance using equation #2 */
		Rt = z2;
		Rt -= z1;
		Rt *= x;
		Rt *= ts->x_plate_ohms;
		Rt /= z1;
		Rt = (Rt + 2047) >> 12;  /* OK, +2047 as +0.49999... */

/* MidasHK_8: ---------- Convert Rt from 12bit resolustion to 8bits */
		Rt = (Rt>>4);

	} else {
		Rt = 0;
	}


	/*
	 * Sample found inconsistent by debouncing or pressure is beyond
	 * the maximum. Don't report it to user space, repeat at least
	 * once more the measurement
	 */
	if (packet->tc.ignore || Rt > ts->pressure_max) {
		dev_vdbg(&ts->spi->dev, "ignored %d pressure %d\n",
			 packet->tc.ignore, Rt);
		return;
	}

	/*
	 * Maybe check the pendown state before reporting. This discards
	 * false readings when the pen is lifted.
	 */
	if (ts->penirq_recheck_delay_usecs) {
		udelay(ts->penirq_recheck_delay_usecs);
		if (!get_pendown_state(ts))
			Rt = 0;
	}

	/*
	 * NOTE: We can't rely on the pressure to determine the pen down
	 * state, even this controller has a pressure sensor. The pressure
	 * value can fluctuate for quite a while after lifting the pen and
	 * in some cases may not even settle at the expected value.
	 *
	 * The only safe way to check for the pen up condition is in the
	 * timer by reading the pen signal state (it's a GPIO _and_ IRQ).
	 */
	if (Rt) {
		struct input_dev *input = ts->input;

		if (ts->swap_xy)
			swap(x, y);

		if (!ts->pendown) {
			input_report_key(input, BTN_TOUCH, 1);
			ts->pendown = true;
			dev_vdbg(&ts->spi->dev, "DOWN\n");
		}

/* MidasHK: TODO user space may NOT receive ABS_X/Y both as pair, one of them may be missed!??? */
		input_report_abs(input, ABS_X, x);
		input_report_abs(input, ABS_Y, y);
/* see MidasHK_8: Rt limits to 255 */
//		input_report_abs(input, ABS_PRESSURE, ts->pressure_max - Rt);
		input_report_abs(input, ABS_PRESSURE, 255 - Rt);

		input_sync(input);
		dev_vdbg(&ts->spi->dev, "%4d/%4d/%4d\n", x, y, Rt);

printk( "x=%d, y=%d, Rt=%d\n", x,y, Rt);
	}
}

static irqreturn_t ads7846_hard_irq(int irq, void *handle)
{
	struct ads7846 *ts = handle;

printk( "hard_irq triggered!\n");

	/* Check pendown sate to ensure FALLINGEDGE interrupt, and ignore debounce/jittery.  */
	return get_pendown_state(ts) ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

#if 0 //////////////////  MidasHK_9: ------- TEST: read x/y  ----  ////////////////////
static irqreturn_t ads7846_irq(int irq, void *handle)
{
	struct ads7846 *ts = handle;
	u8 CMD_READ_X=0xD0; //0xD0 //1101,0000  /* read X position data */
	u8 CMD_READ_Y=0x90; //0x90 //1001,0000 /* read Y position data */
	u16 data;

	/* Start with a small delay before checking pendown state */
	msleep(TS_POLL_DELAY);

	while (!ts->stopped && get_pendown_state(ts)) {
		if( hdsync_write_then_read_spi(ts->spi, &CMD_READ_X, 1, &data, 2)==0)
			printk("X: 0x%04X\n", data);
		else
			printk("X: ERR!\n");
		if( hdsync_write_then_read_spi(ts->spi, &CMD_READ_Y, 1, &data, 2)==0)
			printk("Y: 0x%04X\n", data);
		else
			printk("Y: ERR!\n");
	}

	/* Reset pendow token */
	ts->pendown=false;

	return IRQ_HANDLED;
}

#else //////////////////////////////////////////////////////////

/* This is wakeup function for request_threaded_irq() */
static irqreturn_t ads7846_irq(int irq, void *handle)
{
	struct ads7846 *ts = handle;

	/* Start with a small delay before checking pendown state */
	msleep(TS_POLL_DELAY);

/* TEST: --------- */
if(get_pendown_state(ts))
	printk("ads7846_irq(): pendown\n");

	while (!ts->stopped && get_pendown_state(ts)) {


		/* pen is down, continue with the measurement */
		ads7846_read_state(ts);

		if (!ts->stopped) {
//printk("ads7846_irq(): report state\n");
			ads7846_report_state(ts);

/* MidasHK_10: ------------- Assign true to  ts->pendown */
			ts->pendown=true;
		}

		wait_event_timeout(ts->wait, ts->stopped,
				   msecs_to_jiffies(TS_POLL_PERIOD));
	}

	if (ts->pendown) {
		struct input_dev *input = ts->input;

printk("ads7846_irq(): penup\n");
		input_report_key(input, BTN_TOUCH, 0);
		input_report_abs(input, ABS_PRESSURE, 0);
		input_sync(input);

		ts->pendown = false;
		dev_vdbg(&ts->spi->dev, "UP\n");
	}

	return IRQ_HANDLED;
}
#endif /////////////////////////////////////////////////////////


#ifdef CONFIG_PM_SLEEP
static int ads7846_suspend(struct device *dev)
{
	struct ads7846 *ts = dev_get_drvdata(dev);

	mutex_lock(&ts->lock);

	if (!ts->suspended) {

		if (!ts->disabled)
			__ads7846_disable(ts);

		if (device_may_wakeup(&ts->spi->dev))
			enable_irq_wake(ts->spi->irq);

		ts->suspended = true;
	}

	mutex_unlock(&ts->lock);

	return 0;
}

static int ads7846_resume(struct device *dev)
{
	struct ads7846 *ts = dev_get_drvdata(dev);

	mutex_lock(&ts->lock);

	if (ts->suspended) {

		ts->suspended = false;

		if (device_may_wakeup(&ts->spi->dev))
			disable_irq_wake(ts->spi->irq);

		if (!ts->disabled)
			__ads7846_enable(ts);
	}

	mutex_unlock(&ts->lock);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(ads7846_pm, ads7846_suspend, ads7846_resume);

static int ads7846_setup_pendown(struct spi_device *spi,
				 struct ads7846 *ts,
				 const struct ads7846_platform_data *pdata)
{
	int err;

	/*
	 * REVISIT when the irq can be triggered active-low, or if for some
	 * reason the touchscreen isn't hooked up, we don't need to access
	 * the pendown state.
	 */

	if (pdata->get_pendown_state) {
		ts->get_pendown_state = pdata->get_pendown_state;
	} else if (gpio_is_valid(pdata->gpio_pendown)) {

/* MidasHK_11: ----------------------------
   Note:
	gpio_request_one() finally calls test_and_set_bit(), it sets a bit and return its old value.
	if its already set by others, it fails with EBUSY!
        See __gpiod_request() @ drivers/gpio/gpiolib.c:
 	... ...
        if (test_and_set_bit(FLAG_REQUESTED, &desc->flags) == 0) {
                desc_set_label(desc, label ? : "?");
                status = 0;
        } else {
                status = -EBUSY;
                goto done;
        }
 	... ...
 */
		err = gpio_request_one(pdata->gpio_pendown, GPIOF_IN,
				       "ads7846_pendown");
		if(err) {
			dev_err(&spi->dev,
				"failed to request/setup pendown GPIO%d: %d\n",
				pdata->gpio_pendown, err);
			return err;
		}

		ts->gpio_pendown = pdata->gpio_pendown;

		if (pdata->gpio_pendown_debounce)
			gpio_set_debounce(pdata->gpio_pendown,
					  pdata->gpio_pendown_debounce);
	} else {
		dev_err(&spi->dev, "no get_pendown_state nor gpio_pendown?\n");
		return -EINVAL;
	}

	return 0;
}

/*
 * Set up the transfers to read touchscreen state; this assumes we
 * use formula #2 for pressure, not #3.
 */
static void ads7846_setup_spi_msg(struct ads7846 *ts,
				  const struct ads7846_platform_data *pdata)
{
	struct spi_message *m = &ts->msg[0];
	struct spi_transfer *x = ts->xfer;
	struct ads7846_packet *packet = ts->packet;
	int vref = pdata->keep_vref_on;

	if (ts->model == 7873) {
		/*
		 * The AD7873 is almost identical to the ADS7846
		 * keep VREF off during differential/ratiometric
		 * conversion modes.
		 */
		ts->model = 7846;
		vref = 0;
	}

//7846:  m=ts->msg[0], msg_count=0   READ_Y
	ts->msg_count = 1;
	spi_message_init(m);
	m->context = ts;

	if (ts->model == 7845) {
		packet->read_y_cmd[0] = READ_Y(vref);
		packet->read_y_cmd[1] = 0;
		packet->read_y_cmd[2] = 0;
		x->tx_buf = &packet->read_y_cmd[0];
		x->rx_buf = &packet->tc.y_buf[0];
		x->len = 3;
		spi_message_add_tail(x, m);
	} else {
		/* y- still on; turn on only y+ (and ADC) */
		packet->read_y = READ_Y(vref);
		x->tx_buf = &packet->read_y;
		x->len = 1;
		spi_message_add_tail(x, m);

		x++;
		x->rx_buf = &packet->tc.y;
		x->len = 2;
		spi_message_add_tail(x, m);
	}

	/*
	 * The first sample after switching drivers can be low quality;
	 * optionally discard it, using a second one after the signals
	 * have had enough time to stabilize.
	 */
	if (pdata->settle_delay_usecs) {
		x->delay_usecs = pdata->settle_delay_usecs;

		x++;
		x->tx_buf = &packet->read_y;
		x->len = 1;
		spi_message_add_tail(x, m);

		x++;
		x->rx_buf = &packet->tc.y;
		x->len = 2;
		spi_message_add_tail(x, m);
	}

	ts->msg_count++;
	m++;
//7846: m=ts->msg[1], msg_count=1
	spi_message_init(m);
	m->context = ts;

	if (ts->model == 7845) {
		x++;
		packet->read_x_cmd[0] = READ_X(vref);
		packet->read_x_cmd[1] = 0;
		packet->read_x_cmd[2] = 0;
		x->tx_buf = &packet->read_x_cmd[0];
		x->rx_buf = &packet->tc.x_buf[0];
		x->len = 3;
		spi_message_add_tail(x, m);
	} else {
		/* turn y- off, x+ on, then leave in lowpower */
		x++;
		packet->read_x = READ_X(vref);   //READ_12BIT_DFR(x,  1, vref)
		x->tx_buf = &packet->read_x;
		x->len = 1;
		spi_message_add_tail(x, m);

		x++;
		x->rx_buf = &packet->tc.x;
		x->len = 2;
		spi_message_add_tail(x, m);
	}

	/* ... maybe discard first sample ... */
	if (pdata->settle_delay_usecs) {
		x->delay_usecs = pdata->settle_delay_usecs;

		x++;
		x->tx_buf = &packet->read_x;
		x->len = 1;
		spi_message_add_tail(x, m);

		x++;
		x->rx_buf = &packet->tc.x;
		x->len = 2;
		spi_message_add_tail(x, m);
	}

	/* turn y+ off, x- on; we'll use formula #2 */
	if (ts->model == 7846) {
		ts->msg_count++;
		m++;
//7846:  m=ts->msg[2], msg_count=2
		spi_message_init(m);
		m->context = ts;

		x++;
		packet->read_z1 = READ_Z1(vref);
		x->tx_buf = &packet->read_z1;
		x->len = 1;
		spi_message_add_tail(x, m);

		x++;
		x->rx_buf = &packet->tc.z1;
		x->len = 2;
		spi_message_add_tail(x, m);

		/* ... maybe discard first sample ... */
		if (pdata->settle_delay_usecs) {
			x->delay_usecs = pdata->settle_delay_usecs;

			x++;
			x->tx_buf = &packet->read_z1;
			x->len = 1;
			spi_message_add_tail(x, m);

			x++;
			x->rx_buf = &packet->tc.z1;
			x->len = 2;
			spi_message_add_tail(x, m);
		}

		ts->msg_count++;
		m++;
//7846:  m=ts->msg[3], msg_count=3
		spi_message_init(m);
		m->context = ts;

		x++;
		packet->read_z2 = READ_Z2(vref);
		x->tx_buf = &packet->read_z2;
		x->len = 1;
		spi_message_add_tail(x, m);

		x++;
		x->rx_buf = &packet->tc.z2;
		x->len = 2;
		spi_message_add_tail(x, m);

		/* ... maybe discard first sample ... */
		if (pdata->settle_delay_usecs) {
			x->delay_usecs = pdata->settle_delay_usecs;

			x++;
			x->tx_buf = &packet->read_z2;
			x->len = 1;
			spi_message_add_tail(x, m);

			x++;
			x->rx_buf = &packet->tc.z2;
			x->len = 2;
			spi_message_add_tail(x, m);
		}
	}

	/* power down */
	ts->msg_count++;
	m++;
//7846: m=ts->msg[4], msg_count=5
	spi_message_init(m);
	m->context = ts;

	if (ts->model == 7845) {
		x++;
		packet->pwrdown_cmd[0] = PWRDOWN;
		packet->pwrdown_cmd[1] = 0;
		packet->pwrdown_cmd[2] = 0;
		x->tx_buf = &packet->pwrdown_cmd[0];
		x->len = 3;
	} else {
		x++;
		packet->pwrdown = PWRDOWN;
		x->tx_buf = &packet->pwrdown;
		x->len = 1;
		spi_message_add_tail(x, m);

		x++;
		x->rx_buf = &packet->dummy;
		x->len = 2;
	}

	CS_CHANGE(*x);
	spi_message_add_tail(x, m);
}

#ifdef CONFIG_OF

static const struct of_device_id ads7846_dt_ids[] = {
	{ .compatible = "ti,tsc2046",	.data = (void *) 7846 },
	{ .compatible = "ti,ads7843",	.data = (void *) 7843 },
	{ .compatible = "ti,ads7845",	.data = (void *) 7845 },
	{ .compatible = "ti,ads7846",	.data = (void *) 7846 },
	{ .compatible = "ti,ads7873",	.data = (void *) 7873 },
	{ }
};
MODULE_DEVICE_TABLE(of, ads7846_dt_ids);


static const struct ads7846_platform_data *ads7846_probe_dt(struct device *dev) {
	struct ads7846_platform_data *pdata;
	struct device_node *node = dev->of_node;
	const struct of_device_id *match;

	if (!node) {
		dev_err(dev, "Device does not have associated DT data\n");
		return ERR_PTR(-EINVAL);
	}

/* MidasHK_TEST: ----------- */
int irq;
irq=irq_of_parse_and_map(node, 0);
printk(" ---> irq=irq_of_parse_and_map =%d\n", irq);

printk(" ---> hwirq=test_of_parse_hwirq(node, 0) =%d\n", test_of_parse_hwirq(node, 0));


	match = of_match_device(ads7846_dt_ids, dev);
	if (!match) {
		dev_err(dev, "Unknown device model\n");
		return ERR_PTR(-EINVAL);
	}

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->model = (unsigned long)match->data;

	/* vref supply delay in usecs, 0 for external vref (u16). */
	of_property_read_u16(node, "ti,vref-delay-usecs",
			     &pdata->vref_delay_usecs);


 	/* The VREF voltage, in millivolts (u16). Set to 0 to use internal references(ADS7846) */
	of_property_read_u16(node, "ti,vref-mv", &pdata->vref_mv);

	/* set to keep vref on for differential measurements as well */
	pdata->keep_vref_on = of_property_read_bool(node, "ti,keep-vref-on");
/* MidasHK_mv: ----------- vref in The VREF voltage, in millivolts */
	pdata->keep_vref_on=true;

	/* swap x and y axis */
	pdata->swap_xy = of_property_read_bool(node, "ti,swap-xy");

	/* Settling time of the analog signals;
	 *  a function of Vcc and the capacitance on the X/Y drivers.  If set to non-zero,
	 *  two samples are taken with settle_delay us apart, and the second one is used.
	 *  ~150 uSec with 0.01uF caps (u16).
	 */
	of_property_read_u16(node, "ti,settle-delay-usec",
			     &pdata->settle_delay_usecs);

	/* If set to non-zero, after samples are taken this delay is applied and penirq
	   is rechecked, to help avoid false events.  This value is affected by the
	   material used to build the touch layer(u16).
	 */
	of_property_read_u16(node, "ti,penirq-recheck-delay-usecs",
			     &pdata->penirq_recheck_delay_usecs);

	/* Resistance of the X/Y-plate, in Ohms (u16). */
	of_property_read_u16(node, "ti,x-plate-ohms", &pdata->x_plate_ohms);
	of_property_read_u16(node, "ti,y-plate-ohms", &pdata->y_plate_ohms);

	/* Minimum value on the X/Y axis (u16). */
	of_property_read_u16(node, "ti,x-min", &pdata->x_min);
	of_property_read_u16(node, "ti,y-min", &pdata->y_min);

	/* Maximum value on the X/Y axis (u16). */
	of_property_read_u16(node, "ti,x-max", &pdata->x_max);
	of_property_read_u16(node, "ti,y-max", &pdata->y_max);

	/* Minimum reported pressure value(threshold) -u16. */
	of_property_read_u16(node, "ti,pressure-min", &pdata->pressure_min);
	/* Maximum reported pressure value(u16) */
	of_property_read_u16(node, "ti,pressure-max", &pdata->pressure_max);

	/*Max number of additional readings per sample (u16). */
	of_property_read_u16(node, "ti,debounce-max", &pdata->debounce_max);
	/* Tolerance used for filtering (u16). */
	of_property_read_u16(node, "ti,debounce-tol", &pdata->debounce_tol);
	/* Additional consecutive good readings	required after the first two (u16). */
	of_property_read_u16(node, "ti,debounce-rep", &pdata->debounce_rep);

	/* Platform specific debounce time for the pendown-gpio (u32). */
	of_property_read_u32(node, "ti,pendown-gpio-debounce",
			     &pdata->gpio_pendown_debounce);

	/* use any event on touchscreen as wakeup event. (Legacy property support: "linux,wakeup") */
	pdata->wakeup = of_property_read_bool(node, "linux,wakeup");

	/* GPIO handle describing the pin the !PENIRQ line is connected to. */
	pdata->gpio_pendown = of_get_named_gpio(dev->of_node, "pendown-gpio", 0);

printk( "gpio_pendown =%d\n", pdata->gpio_pendown);

	return pdata;
}
#else
static const struct ads7846_platform_data *ads7846_probe_dt(struct device *dev)
{
	dev_err(dev, "no platform data defined\n");
	return ERR_PTR(-EINVAL);
}
#endif

static int ads7846_probe(struct spi_device *spi)
{
	const struct ads7846_platform_data *pdata;
	struct ads7846 *ts;
	struct ads7846_packet *packet;
	struct input_dev *input_dev;
	unsigned long irq_flags;
	int err;

dev_dbg(&spi->dev, "----- DEBUG Active ------\n");

printk( "%s: !spi->irq...\n",__func__);
	/* Check spi->irq */
	if (!spi->irq) {
		dev_dbg(&spi->dev, "no IRQ?\n");
		return -EINVAL;
	}

printk( "%s: Check if exceed max sample rate...\n",__func__);
	/* don't exceed max specified sample rate */
	if (spi->max_speed_hz > (125000 * SAMPLE_BITS)) {
		dev_err(&spi->dev, "f(sample) %d KHz?\n",
				(spi->max_speed_hz/SAMPLE_BITS)/1000);
		return -EINVAL;
	}

	/*
	 * We'd set TX word size 8 bits and RX word size to 13 bits ... except
	 * that even if the hardware can do that, the SPI controller driver
	 * may not.  So we stick to very-portable 8 bit words, both RX and TX.
	 */
	spi->bits_per_word = 8;
/* MidasHK_12: -------------SPI mode 3 */
	//spi->mode = SPI_MODE_0;
	spi->mode = SPI_MODE_3;

printk( "%s: spi_setup(spi)...\n",__func__);
	err = spi_setup(spi);
	if (err < 0) {
		dev_err(&spi->dev, "spi_setup(spi) error!\n");
		return err;
	}

	/* Allocate ts and packet */
	ts = kzalloc(sizeof(struct ads7846), GFP_KERNEL);
	packet = kzalloc(sizeof(struct ads7846_packet), GFP_KERNEL);
printk( "%s: input_allocate_device()...\n",__func__);
	input_dev = input_allocate_device();
	if (!ts || !packet || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	/* Set up spi driver data */
	spi_set_drvdata(spi, ts);

	ts->packet = packet;
	ts->spi = spi;
	ts->input = input_dev;

	mutex_init(&ts->lock);
	init_waitqueue_head(&ts->wait);

printk( "%s: dev_get_platdata()...\n",__func__);
	/* Get spi dev platform data */
	pdata = dev_get_platdata(&spi->dev);
	if (!pdata) {
		pdata = ads7846_probe_dt(&spi->dev);
		if (IS_ERR(pdata)) {
			err = PTR_ERR(pdata);
			goto err_free_mem;
		}
	}

	ts->model = pdata->model ? : 7846;
	ts->vref_delay_usecs = pdata->vref_delay_usecs ? : 100;
	ts->x_plate_ohms = pdata->x_plate_ohms ? : 400;
	ts->pressure_max = pdata->pressure_max ? : ~0;

	ts->vref_mv = pdata->vref_mv;
	ts->swap_xy = pdata->swap_xy;

	/* Set ts filter/debounce */
	if (pdata->filter != NULL) {
		if (pdata->filter_init != NULL) {
printk( "%s: pdata->filter_init()...\n",__func__);
			err = pdata->filter_init(pdata, &ts->filter_data);
			if (err < 0)
				goto err_free_mem;
		}
		ts->filter = pdata->filter;
		ts->filter_cleanup = pdata->filter_cleanup;
	} else if (pdata->debounce_max) {
		ts->debounce_max = pdata->debounce_max;
		if (ts->debounce_max < 2)
			ts->debounce_max = 2;
		ts->debounce_tol = pdata->debounce_tol;
		ts->debounce_rep = pdata->debounce_rep;
printk( "%s: set ts->filter = ads7846_debounce_filter...\n",__func__);
		ts->filter = ads7846_debounce_filter;
		ts->filter_data = ts;
	} else {
		ts->filter = ads7846_no_filter;
printk( "%s: set ts->filter = ads7846_no_filter...\n",__func__);
	}

	/* Set up pendown */
printk( "%s: ads7846_setup_pendown()...\n",__func__);
	err = ads7846_setup_pendown(spi, ts, pdata);
	if (err) {
printk( "%s: ads7846_setup_pendown() failed!\n",__func__);
		goto err_cleanup_filter;
	}

	if (pdata->penirq_recheck_delay_usecs)
		ts->penirq_recheck_delay_usecs =
				pdata->penirq_recheck_delay_usecs;

	ts->wait_for_sync = pdata->wait_for_sync ? : null_wait_for_sync;

/* MidasHK__  as display in log:
   input: ADS7846 Touchscreen as /devices/10000000.palmbus/10000b00.spi/spi_master/spi32766/spi32766.2/input/input0
*/
	snprintf(ts->phys, sizeof(ts->phys), "%s/input0", dev_name(&spi->dev));
	snprintf(ts->name, sizeof(ts->name), "ADS%d Touchscreen", ts->model);

	/* Setup input_dev */
	input_dev->name = ts->name;
	input_dev->phys = ts->phys;
	input_dev->dev.parent = &spi->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X,
			pdata->x_min ? : 0,
			pdata->x_max ? : MAX_12BIT,
			0, 0);
	input_set_abs_params(input_dev, ABS_Y,
			pdata->y_min ? : 0,
			pdata->y_max ? : MAX_12BIT,
			0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE,
			pdata->pressure_min, pdata->pressure_max, 0, 0);

	/* Setup spi MSG */
	ads7846_setup_spi_msg(ts, pdata);

printk( "%s: regulator_get(&spi->dev, 'vcc')...\n",__func__);
	ts->reg = regulator_get(&spi->dev, "vcc");
	if (IS_ERR(ts->reg)) {
		err = PTR_ERR(ts->reg);
		dev_err(&spi->dev, "unable to get regulator: %d\n", err);
		goto err_free_gpio;
	}

printk( "%s: regulator_enable(ts->reg)...\n",__func__);
	err = regulator_enable(ts->reg);
	if (err) {
		dev_err(&spi->dev, "unable to enable regulator: %d\n", err);
		goto err_put_regulator;
	}

/* MidasHK_13:----  WRONG IRQ!  Use gpio_to_irq(), hwirq same as gpio_pendown!  */
printk( "Original spi->irq=%d \n", spi->irq);
	spi->irq=gpio_to_irq(pdata->gpio_pendown);
printk( "IRQ for GPIO40: %d \n", spi->irq);

	irq_flags = pdata->irq_flags ? : IRQF_TRIGGER_FALLING;
	irq_flags |= IRQF_ONESHOT; /* No new interrup accepted until finish this round! */

printk( "%s: request_threaded_irq()...\n",__func__);
	/* request_threaded_irq(irq, handler, thread_fn, irqflags, devname, devID) */
	err = request_threaded_irq(spi->irq, ads7846_hard_irq, ads7846_irq,
				   irq_flags, spi->dev.driver->name, ts);

	if (err && !pdata->irq_flags) {
		printk( "err && !pdata->irq_flags\n");
		dev_info(&spi->dev,
			"trying pin change workaround on irq %d\n", spi->irq);
		irq_flags |= IRQF_TRIGGER_RISING;
		err = request_threaded_irq(spi->irq,
				  ads7846_hard_irq, ads7846_irq,
				  irq_flags, spi->dev.driver->name, ts);
	}

	if (err) {
		printk( "irq %d busy?\n", spi->irq);

		dev_dbg(&spi->dev, "irq %d busy?\n", spi->irq);
		goto err_disable_regulator;
	}

	err = ads784x_hwmon_register(spi, ts);
	if (err)
		goto err_free_irq;

	dev_info(&spi->dev, "touchscreen, irq %d\n", spi->irq);

	/*
	 * Take a first sample, leaving nPENIRQ active and vREF off; avoid
	 * the touchscreen, in case it's not connected.
	 */
	if (ts->model == 7845)
		ads7845_read12_ser(&spi->dev, PWRDOWN);
	else
		(void)ads7846_read12_ser(&spi->dev, READ_12BIT_SER(vaux));

printk( "---------- TRACE MARK ----------\n");

printk( "%s: sysfs_create_group...\n", __func__);
	err = sysfs_create_group(&spi->dev.kobj, &ads784x_attr_group);
	if (err)
		goto err_remove_hwmon;

printk( "%s: input_register_device...\n", __func__);
	err = input_register_device(input_dev);
	if (err)
		goto err_remove_attr_group;


#if 1 /* MidasHK_14:  NO wakeup for request_irq() -------------------- */
printk( "input_register_device...\n");
	device_init_wakeup(&spi->dev, pdata->wakeup);
#endif
	/*
	 * If device does not carry platform data we must have allocated it
	 * when parsing DT data.
	 */
	if (!dev_get_platdata(&spi->dev)) {
		printk("Device does not carry platform data! kfree pdata which is devm_kzalloced by ads7846_probe_dt().\n");
		devm_kfree(&spi->dev, (void *)pdata);
	}

	return 0;


 err_remove_attr_group:
	sysfs_remove_group(&spi->dev.kobj, &ads784x_attr_group);
 err_remove_hwmon:
	ads784x_hwmon_unregister(spi, ts);
 err_free_irq:
	free_irq(spi->irq, ts);
 err_disable_regulator:
	regulator_disable(ts->reg);
 err_put_regulator:
	regulator_put(ts->reg);
 err_free_gpio:
	if (!ts->get_pendown_state)
		gpio_free(ts->gpio_pendown);
 err_cleanup_filter:
	if (ts->filter_cleanup)
		ts->filter_cleanup(ts->filter_data);
 err_free_mem:
	input_free_device(input_dev);
	kfree(packet);
	kfree(ts);
	return err;

}

static int ads7846_remove(struct spi_device *spi)
{
	struct ads7846 *ts = spi_get_drvdata(spi);

	device_init_wakeup(&spi->dev, false);

	sysfs_remove_group(&spi->dev.kobj, &ads784x_attr_group);

	ads7846_disable(ts);
	free_irq(ts->spi->irq, ts);

	input_unregister_device(ts->input);

	ads784x_hwmon_unregister(spi, ts);

	regulator_disable(ts->reg);
	regulator_put(ts->reg);

	if (!ts->get_pendown_state) {
		/*
		 * If we are not using specialized pendown method we must
		 * have been relying on gpio we set up ourselves.
		 */
		gpio_free(ts->gpio_pendown);
	}

	if (ts->filter_cleanup)
		ts->filter_cleanup(ts->filter_data);

	kfree(ts->packet);
	kfree(ts);

	dev_dbg(&spi->dev, "unregistered touchscreen\n");

	return 0;
}


static struct spi_driver ads7846_driver = {
	.driver = {
		.name	= "ads7846",
		.owner	= THIS_MODULE,
		.pm	= &ads7846_pm,
		.of_match_table = of_match_ptr(ads7846_dt_ids),
	},
	.probe		= ads7846_probe,
	.remove		= ads7846_remove,
};

module_spi_driver(ads7846_driver);

MODULE_DESCRIPTION("ADS7846 TouchScreen Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:ads7846");

