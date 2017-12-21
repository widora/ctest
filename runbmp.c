/*-------------------------------------------------------------------------------------------------------
Based on:  libftdi - A library (using libusb) to talk to FTDI's UART/FIFO chips
Original Source:  https://www.intra2net.com/en/developer/libftdi/
by:
    www.intra2net.com    2003-2017 Intra2net AG


++++++++ run all sizes of 24bit_color BMP pic not big than 480x320 ++++++++
complile:
	./openwrt-gcc -L. -lftdi1 -lusb-1.0 -o runmovie2 runbmp.c
usage:
	./runmovie2 path    (use ramfs!!!)



                               -----  NOTEs & BUGs  -----

1. Normally there are only 2-3 bmp files in the path, it will be choppy if the number is great than 5.
   that means something unusual happens, it slows down the processing, check it then.
   The most possible is that decoding speed is faster than runbmp speed.
2. It MAY BE a good idea to put your avi file in TF card while use usb bus for LCD transfer only.
   However, if you install ffmpeg in the TF card, it may be more difficult to launch the application.
   480x320 fps=15 OK
3. TODO: allocate mem for g_GBuffer with continous physical addresses.
4. Playing speed depends on ffmpeg decoding speed, USB transfer speed, and FT232H fanout(baudrate) speed.
5. Everytime when you run the movie re_create the fifo.wav,it may help to avoid choppy.

Midas Zhou
--------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/ftdi.h" //bitbang ops
#include "ft232.h"  //open_ft232(), clos_ft232(); mpss op.
#include "fbmp_op.h" //bmp file operations


/*===================== MAIN   ======================*/
int main(int argc, char **argv)
{
    int baudrate;
    int chunk_size;
    int ret;
    int i,k;
    struct timeval tm_start,tm_end;
    int time_use;
    //---BMP file
    int fp; //file handler
    char str_bmpf_path[128]; //directory for BMP files 
    char str_bmpf_file[STRBUFF];// full path+name for a BMP file.
    int Ncount=-1; //--index number of picture displayed


//---- check argv[] ---------
    if(argc != 2)
    {
	printf("input parameter error!\n");
	printf("Usage: %s path  \n",argv[0]);
	exit(-1);
    }

//-----  prepare control pins -----
    setPinMmap();
//-----  open ft232 usb device -----
    open_ft232(0x0403, 0x6014);
//-----  set to BITBANG MODE -----
    ftdi_set_bitmode(g_ftdi, 0xFF, BITMODE_BITBANG);

//-----  set baudrate,beware of your wiring  -----
//-------  !!!! select low speed if the color is distorted !!!!!  -------
//    baudrate=3150000; //20MBytes/s
//    baudrate=2000000;
      baudrate=750000;
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
    chunk_size=1024*32;// >=1024*32 same effect.    default is 4096
    ftdi_write_data_set_chunksize(g_ftdi,chunk_size);

//-----  Init ILI9488 and turn on display -----
    LCD_INIT_ILI9488();

//------  set LCD pixle format,default is RGB888  -------
//    LCD_Set_PxlFmt16bit();
    LCD_Set_PxlFmt24bit();


//<<<<<<<<<<<<<<<<<  BMP FILE TEST >>>>>>>>>>>>>>>>>>
//strcpy(str_bmpf_path,"/tmp");//set directory
   strcpy(str_bmpf_path,argv[1]);
while(1) //loop showing BMP files in a directory
{
     //-------------- reload total_numbe after one round show ---------
      printf("BMP file  Ncount =%d  \n",Ncount);
      if( Ncount < 0)
      {
          //---- find out all BMP files in specified path 
          Find_BMP_files(str_bmpf_path);
          if(g_BMP_file_total == 0){
             printf("\n No BMP file found! \n");
	     //---- wait for a while ---
	     usleep(100000);
             continue; //continue loop
	  }
          printf("\n\n==========  reload BMP file, totally  %d BMP-files found.   ============\n",g_BMP_file_total);
          Ncount=g_BMP_file_total-1; //---reset Ncount, [Nount] starting from 0
      }

     //----- load BMP file path -------------
     sprintf(str_bmpf_file,"%s/%s",str_bmpf_path,g_BMP_file_name[Ncount]);
     printf("str = %s\n",str_bmpf_file);
     Ncount--;

     //------  show the bmp file and count time -------
     gettimeofday(&tm_start,NULL);

     if( show_bmpf(str_bmpf_file) < 0 ) 
     {
	//----- if show bmp fails, then skip to continue, will NOT delete the file then -----
	continue;
     }

     gettimeofday(&tm_end,NULL);
     time_use=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
     printf("  ------ finish loading a 480*320*24bits bmp file, time_use=%dms -----  \n",time_use);

     //----- delete file after displaying -----

      if(remove(str_bmpf_file) != 0)
		printf("Fail to remove the file!\n");


     //----- keep the image on the display for a while ------
//     usleep(50000);
//	sleep(1);
}


//----- close ft232 usb device -----
    close_ft232();

//----- release pin mmap -----
    resPinMmap();

    return ret;
}
