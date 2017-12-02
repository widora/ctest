/*-----------------------------------------------------------------------------
Based on:  libftdi - A library (using libusb) to talk to FTDI's UART/FIFO chips
Original Source:  https://www.intra2net.com/en/developer/libftdi/
by:
    www.intra2net.com    2003-2017 Intra2net AG


./openwrt-gcc -L. -lftdi1 -lusb-1.0 -o ft232_tft ft232_tft.c


Midas
-------------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/ftdi.h"
#include "ft232.h"
#include "ILI9488.h"


/*===================== MAIN   ======================*/
int main(int argc, char **argv)
{
    int baudrate;
    int chunksize;
    int ret;
    int i,k;

//-----  prepare control pins -----
    setPinMmap();
//-----  open ft232 usb device -----
    open_ft232(0x0403, 0x6014);
//-----  set to BITBANG MODE -----
    ftdi_set_bitmode(g_ftdi, 0xFF, BITMODE_BITBANG);
//-----  set baudrate -------
//    baudrate=3150000; //20MBytes/s
    baudrate=2000000;
    ret=ftdi_set_baudrate(g_ftdi,baudrate); 
    if(ret == -1){
        printf("baudrate invalid!\n");
    }
    else if(ret != 0){
        printf("ret=%d set baudrate fails!\n",ret);
    }
    else if(ret ==0 ){
        printf("set baudrate=%d,  actual baudrate=%d \n",baudrate,g_ftdi->baudrate);
    }

//------ purge rx buffer in FT232H  ------
//    ftdi_usb_purge_rx_buffer(g_ftdi);// ineffective ??
//------  set chunk_size, default is 4096
    //chunk_size=1024*64;// >=1024*32 same effect.    default is 4096
    //ftdi_write_data_set_chunksize(ftdi,chunk_size);

//-----  Init ILI9488 and turn on display -----
    LCD_INIT_ILI9488();


    while(1);





//----- close ft232 usb device -----
    close_ft232();
//----- release pin mmap -----
    resPinMmap();

    return ret;
}
