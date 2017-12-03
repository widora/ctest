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
    int chunk_size;
    int ret;
    int i,k;
    struct timeval tm_start,tm_end;
    int time_use;

//-----  prepare control pins -----
    setPinMmap();
//-----  open ft232 usb device -----
    open_ft232(0x0403, 0x6014);
//-----  set to BITBANG MODE -----
    ftdi_set_bitmode(g_ftdi, 0xFF, BITMODE_BITBANG);
//-----  set baudrate -------
    baudrate=3150000; //20MBytes/s
//    baudrate=2000000;
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
    chunk_size=1024*64;// >=1024*32 same effect.    default is 4096
    ftdi_write_data_set_chunksize(g_ftdi,chunk_size);

//-----  Init ILI9488 and turn on display -----
    LCD_INIT_ILI9488();
/*
//---- fill graphic buffer -----
for(i=0;i<480*320;i++)
{
   g_GBuffer[i][0]=0xff;
   g_GBuffer[i][1]=0x00;
   g_GBuffer[i][2]=0x00;
}
*/

//-----refresh GRAPHIC BUFFER --------
uint32_t  tmpd=1;
//for(k=0;k<200;k++)
while(1)
{
        if(tmpd ==0  ) tmpd=1;
	tmpd=tmpd<<1;
	for(i=0;i<480*320;i++){
		g_GBuffer[i][0]=tmpd & 0xff;
		g_GBuffer[i][1]=((tmpd & 0xff00)>>8);
		g_GBuffer[i][2]=((tmpd & 0xff0000)>>16);
	}

	gettimeofday(&tm_start,NULL);
	LCD_Write_GBuffer();
//	usleep(200000);
	gettimeofday(&tm_end,NULL);
	time_use=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
	printf("  ------ finish refreshing whole graphic buffer, time_use=%dms -----  \n",time_use);
}


//    while(1);





//----- close ft232 usb device -----
    close_ft232();
//----- release pin mmap -----
    resPinMmap();

    return ret;
}
