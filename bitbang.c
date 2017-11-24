/*--------------------------------------------------------
 This program is distributed under the GPL, version 2 
 Source:  https://www.intra2net.com/en/developer/libftdi/

./openwrt-gcc -L. -lusb-1.0 -lftdi1 -o bitbang bitbang.c

------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/ftdi.h"

int main(int argc, char **argv)
{
    struct ftdi_context *ftdi;
    int f,i;
    unsigned char buf[2];
    int retval = 0;
    int k;

    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }

    f = ftdi_usb_open(ftdi, 0x0403, 0x6014); //VID and PID

    if (f < 0 && f != -5)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string(ftdi));
        retval = 1;
        goto done;
    }

    printf("ftdi open succeeded: %d\n",f);

    printf("enabling bitbang mode\n");
    ftdi_set_bitmode(ftdi, 0xFF, BITMODE_BITBANG);

    usleep(1000000);

//    buf[0] = 0xFF;
//+++++ 
    k=0;
    while(1){
	if(k == 8) k=0;
	buf[0]=0xFF-(1<<k);
	k++;
	f = ftdi_write_data(ftdi, buf, 1);
	if (f < 0)
    	{
	        fprintf(stderr,"write failed for 0x%x%x, error %d (%s)\n",buf[0],buf[1],f, ftdi_get_error_string(ftdi));
    	}
	else
		printf(" write data buf=0x%02x to FTDI successfully!\n",buf[0]);

    	usleep(50000);
    }
//-----
    printf("turning everything on\n");
    f = ftdi_write_data(ftdi, buf, 1);
    if (f < 0)
    {
        fprintf(stderr,"write failed for 0x%x, error %d (%s)\n",buf[0],f, ftdi_get_error_string(ftdi));
    }

    usleep(3 * 1000000);

    buf[0] = 0xFF;
    printf("turning everything off\n");
    f = ftdi_write_data(ftdi, buf, 1);
    if (f < 0)
    {
        fprintf(stderr,"write failed for 0x%x, error %d (%s)\n",buf[0],f, ftdi_get_error_string(ftdi));
    }

    usleep(3 * 1000000);

    for (i = 0; i < 32; i++)
    {
        buf[0] =  0 | (0xFF ^ 1 << (i % 8));
        if ( i > 0 && (i % 8) == 0)
        {
            printf("\n");
        }
        printf("%02hhx ",buf[0]);
        fflush(stdout);
        f = ftdi_write_data(ftdi, buf, 1);
        if (f < 0)
        {
            fprintf(stderr,"write failed for 0x%x, error %d (%s)\n",buf[0],f, ftdi_get_error_string(ftdi));
        }
        usleep(1 * 1000000);
    }

    printf("\n");

    printf("disabling bitbang mode\n");
    ftdi_disable_bitbang(ftdi);

    ftdi_usb_close(ftdi);
done:
    ftdi_free(ftdi);

    return retval;
}
