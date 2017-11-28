/*--------------------------------------------------------
 This program is distributed under the GPL, version 2 
 Source:  https://www.intra2net.com/en/developer/libftdi/

./openwrt-gcc -L. -lusb-1.0 -lftdi1 -o bitbang bitbang.c

------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "include/ftdi.h"

#define MM 20

int main(int argc, char **argv)
{
    struct ftdi_context *ftdi;
    int f,i;
    int m;
    int ret;
//    unsigned char buf[1024*MM]={0};
    unsigned char *buf;
    unsigned char rvbuf[1024]={0};
    int retval = 0;
    int k;
    int chunk_size;
    int baudrate;
    struct timeval tm_start,tm_end;
    int time_use;

    buf=malloc(1024*MM);

    memset(buf,0xff,1024*MM);

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

/* MPSSE bitbang modes */
//    BITMODE_RESET  = 0x00,    /**< switch off bitbang mode, back to regular serial/FIFO */
//    BITMODE_BITBANG= 0x01,    /**< classical asynchronous bitbang mode, introduced with B-type chips */
//    BITMODE_MPSSE  = 0x02,    /**< MPSSE mode, available on 2232x chips */
//    BITMODE_SYNCBB = 0x04,    /**< synchronous bitbang mode, available on 2232x and R-type chips  */
//    BITMODE_MCU    = 0x08,    /**< MCU Host Bus Emulation mode, available on 2232x chips */
//    /* CPU-style fifo mode gets set via EEPROM */
//    BITMODE_OPTO   = 0x10,    /**< Fast Opto-Isolated Serial Interface Mode, available on 2232x chips  */
//    BITMODE_CBUS   = 0x20,    /**< Bitbang on CBUS pins of R-type chips, configure in EEPROM before */
//     BITMODE_SYNCFF = 0x40,    /**< Single Channel Synchronous FIFO mode, available on 2232H chips */
//    BITMODE_FT1284 = 0x80,    /**< FT1284 mode, available on 232H chips */

    printf("enabling bitbang mode\n");
    ftdi_set_bitmode(ftdi, 0xFF, BITMODE_BITBANG);//SYNCBB);//BITBANG);


    //----- set baudrate
//for(baudrate=0;baudrate<40000000;baudrate+=10000){
    baudrate=3150000;
    ret=ftdi_set_baudrate(ftdi,baudrate); //4M-UNWORKABLE,3M-20MB/s,2.5M-UNWORKABLE,2000000-20MBytes/s, 500000-10MBytes/s
    if(ret == -1){
	printf("baudrate invalid!\n");
//	continue;
    }
    else if(ret != 0){
	printf("ret=%d set baudrate fails!\n",ret);
//	continue;
    }
    else if(ret ==0 ){
	printf("set baudrate=%d,  actual baudrate=%d \n",baudrate,ftdi->baudrate);
    }
//}
//return 0;

    usleep(2000000);

//    int ftdi_write_data_set_chunksize(struct ftdi_context *ftdi, unsigned int chunksize);
//    int ftdi_write_data_get_chunksize(struct ftdi_context *ftdi, unsigned int *chunksize);
//    int ftdi_usb_purge_rx_buffer(struct ftdi_context *ftdi);
    ftdi_usb_purge_rx_buffer(ftdi);

    chunk_size=1024*64;// >=1024*32 same effect.    default is 4096
    ftdi_write_data_set_chunksize(ftdi,chunk_size); 
    ftdi_write_data_get_chunksize(ftdi, &chunk_size);
    printf("chunk_size=%d \n",chunk_size);
    sleep(1);

//    buf[0] = 0xFF;
//+++++ 
    for(k=0;k<1024/2*MM;k++){
//	buf[2*k]=0x810;
	buf[2*k+1]=0xff-0x81;
     }
    buf[1024*MM-1]=0b11011011;
//    k=0;
    while(1){

      gettimeofday(&tm_start,NULL);

      for(k=0;k<1024;k++)
      {
	//------ write
	f = ftdi_write_data(ftdi, buf, 1024*MM);
	if (f < 0)
    	{
	        fprintf(stderr,"write failed for 0x%x%x, error %d (%s)\n",buf[0],buf[1],f, ftdi_get_error_string(ftdi));
    	}
	else
	;
//		printf(" %dth write %d data buf=0x%02x to FTDI successfully!\n",k,f,buf[1024*MM-1]);

     }//end of for()

     gettimeofday(&tm_end,NULL);
     time_use=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
     printf("  ------ finish transfering %dMBytes data, time_use=%dms -----  \n",MM,time_use);

/*
	//------read------
	f = ftdi_read_data(ftdi, rvbuf, 2048);
	if (f < 0)
    	{
	        fprintf(stderr,"read failed! \n");
    	}
	else
		printf(" read data from FT232H buf=0x%02x \n",buf[2047]);
*/
//    	usleep(40000);
    }
//-----

/*
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
*/


    printf("disabling bitbang mode\n");
    ftdi_disable_bitbang(ftdi);


    ftdi_usb_close(ftdi);
done:
    free(buf);
    ftdi_free(ftdi);

    return retval;
}
