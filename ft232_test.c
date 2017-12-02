/*-----------------------------------------------------------------------------
Based on:  libftdi - A library (using libusb) to talk to FTDI's UART/FIFO chips
Original Source:  https://www.intra2net.com/en/developer/libftdi/
by:
    www.intra2net.com    2003-2017 Intra2net AG

Midas
-------------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/ftdi.h"

/*------------------------------------
write to  GPIOH0 -GPIOH7
--------------------------------------*/
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

/*------------------------------------
write to  GPIOL0 -GPIOL7
--------------------------------------*/
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


/*===================== MAIN   ======================*/
int main(int argc, char **argv)
{
    struct ftdi_context *ftdi;
    int fret;
    unsigned char buf[2],val;
    int retval = 0;
    int i,k;
    unsigned short usb_val;

//------ Allocate a ftdi device struct  ----- 
    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }

//------ open a ftdi usb device -------
    fret = ftdi_usb_open(ftdi, 0x0403, 0x6014); //VID and PID
    if (fret < 0 && fret != -5) // ??? -5
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", fret, ftdi_get_error_string(ftdi));
        retval = 1;
        goto done;
    }
    printf("ftdi open succeeded: %d\n",fret);

//------- configure to  BITMODE_MPSSE mode -----
    printf("configure to BITMODE_MPSSE  mode\n");
    ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE);
    //usleep(100000); // Is it necessary ?

    //----- run a LED test  -----
    mpsse_write_highbits(ftdi,0x00);
    mpsse_write_lowbits(ftdi,0xee);
    //sleep(3); // pause for a while

//------- configure to  BITMODE_BITBANG mode -----
    //!!!!  no disable BITMODE_MPSSE MODE here  !!!!!
    printf("configure to BITMODE_BITBANG  mode\n");
    ftdi_set_bitmode(ftdi, 0xFF, BITMODE_BITBANG);
    //usleep(100000); // Is it necessary ?

    //----- set baudrate here ------

    //------ purge rx buffer in FT232H  ------
    ftdi_usb_purge_rx_buffer(ftdi);// ineffective ??

    //------  set chunk_size, default is 4096
    //chunk_size=1024*64;// >=1024*32 same effect.    default is 4096
    //ftdi_write_data_set_chunksize(ftdi,chunk_size);
    //ftdi_write_data_get_chunksize(ftdi, &chunk_size);

    //----- run a LED test  -----
    for(i=0;i<30;i++)
    {
	buf[0] = 0xFF-(1<<(i%8));
	if(buf[0] == 0) buf[0]=0xFF;
 	fret = ftdi_write_data(ftdi, buf, 1);
	if (fret < 0)
        {
                fprintf(stderr,"ftdi_write_data() failed, error %d (%s)\n",fret, ftdi_get_error_string(ftdi));
        }
	usleep(200000);
    }

//------- disable bitbang mode ---------
    printf("disabling bitbang mode\n");
    ftdi_disable_bitbang(ftdi);

//------ close ftdi usb device and free mem.   -------
    ftdi_usb_close(ftdi);
done:
    ftdi_free(ftdi);

    return retval;
}
