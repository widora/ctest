/*-----------------------------------------------------------------------------
Based on:  libftdi - A library (using libusb) to talk to FTDI's UART/FIFO chips
Original Source:  https://www.intra2net.com/en/developer/libftdi/
by:
    www.intra2net.com   2003-2017 Intra2net AG

Midas
-------------------------------------------------------------------------------*/

#ifndef __FT232__H__
#define __FT232__H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/ftdi.h"

struct ftdi_context *g_ftdi; //gloal ftdi context

/*---------------------------------------------
open a FT232H usb device
return:
	0   ok
	<0  fail
	-5  unable to claim device
----------------------------------------------*/
int open_ft232(uint16_t vid, uint16_t pid)
{
    int fret;

    if ((g_ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return -1; //ok, -1 not defined in ftdi_usb_open() retval
    }

    fret = ftdi_usb_open(g_ftdi, vid, pid); 
    if (fret < 0 && fret != -5) // \retval -5: unable to claim device
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", fret, ftdi_get_error_string(g_ftdi));
    }
    else
    	printf("ftdi open succeeded: %d\n",fret);

   return fret;
}

/*----------------------------------------------
      close usb device of FT232H and free mem.
-----------------------------------------------*/
void close_ft232(void)
{
    ftdi_disable_bitbang(g_ftdi);//is this necessary ?????
    ftdi_usb_close(g_ftdi);
    ftdi_free(g_ftdi);
}


/*-------------------------------------------
           functions in ftdi.h
--------------------------------------------*/

// ----- set to BITBANG MODE and MPSSE MODE -----
/*
int ftdi_set_bitmode(ftdi, 0xFF, BITMODE_BITBANG); //0xFF -all 8bits are activated 
int ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE); 
return:
	0: all fine
	-1: can't enable bitbang mode
	-2: USB device unavailable
*/


//------ purge rx buffer in FT232H  ------
//    ftdi_usb_purge_rx_buffer(ftdi);


//----- BITBANG MODE: write data to ft232 ADBUS7~0 -----
/*
int   ftdi_write_data(ftdi, buf, N);
return:
	-666: USB device unavailable
	<0: error code from usb_bulk_write()
	>0: number of bytes written
*/


/*-------------------------------------------------------------------
MPSSE MODE:  write to  GPIOH0 -GPIOH7 (AC0~AC7)
return:
	-666: USB device unavailable
	<0: error code from usb_bulk_write()
	>0: number of bytes written
--------------------------------------------------------------------*/
int  mpsse_write_highbits(struct ftdi_context *ftdi, unsigned char val)
{
	int ret;
	unsigned char buf[3];
	buf[0]=SET_BITS_HIGH;
        buf[1]=val;
	buf[2]=0xff;//1-out

	ret = ftdi_write_data(ftdi, buf, 3);
	if (ret < 0)
    	{
	        fprintf(stderr,"mpsse_write_highbits() write failed, error %d (%s)\n",ret, ftdi_get_error_string(ftdi));
    	}
	return ret;
}

/*-----------------------------------------------------------------
MPSSE MODE:  write to  GPIOL0 -GPIOL7 (AD0-AD7)
return:
	-666: USB device unavailable
	<0: error code from usb_bulk_write()
	>0: number of bytes written
-----------------------------------------------------------------*/
int  mpsse_write_lowbits(struct ftdi_context *ftdi, unsigned char val)
{
	int ret;
	unsigned char buf[3];
	buf[0]=SET_BITS_LOW;
        buf[1]=val;
	buf[2]=0xff;//1-out

	ret = ftdi_write_data(ftdi, buf, 3);
	if (ret < 0)
    	{
	        fprintf(stderr,"mpsse_write_lowbits() write failed, error %d (%s)\n",ret, ftdi_get_error_string(ftdi));
    	}
	return ret;
}



#endif
