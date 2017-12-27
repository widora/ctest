/*-------------------------------------------------------------------------------------------------------
Based on:  libftdi - A library (using libusb) to talk to FTDI's UART/FIFO chips
Original Source:  https://www.intra2net.com/en/developer/libftdi/
by:
    www.intra2net.com    2003-2017 Intra2net AG


++++++++ run all sizes of 24bit_color BMP pic not big than 480x320 ++++++++
complile:
	./openwrt-gcc -L. -lftdi1 -lusb-1.0 -o runmovie runbmp.c
usage:
	./runmovie path    (use ramfs!!!)


                               -----  NOTEs & BUGs  -----

1. Normally there are only 2-3 bmp files in the path, it will be choppy if the number is great than 5.
   that means something unusual happens, it slows down the processing, check it then.
   The most possible is that decoding speed is faster than runbmp speed.
   TODO: If runbmp cann't catch up with ffmpeg decoding speed, then trim some BMP files in the PATH.
2. It MAY BE a good idea to put your avi file in TF card while use usb bus for LCD transfer only.
   However, if you install ffmpeg in the TF card, it may be more difficult to launch the application.
   480x320 fps=15 OK
3. TODO: allocate mem for g_GBuffer with continous physical addresses.
4. Playing speed depends on ffmpeg decoding speed, USB transfer speed, and FT232H fanout(baudrate) speed.
   Using RBG565 fromat can relieve some USB transmission load, but for MT7688, but converting RGB888 to 
   RGB565 also costs CPU load, which further deteriorates FFmpeg decoding process.
5. Everytime when you run the movie re_create the fifo.wav,it may help to avoid choppy.
6. High CPU usage will cause FTDI transfer bus error! especially when run 480x320 BMP files with RGB565 
   conversion, CPU usage will exceed 98% !!! In that case, application will exit with bus error.
7. Format RGB888 is recommended for runbmp, considering all factors mentioned above.
   ffmpeg convert o RGB565  INCREASE CPU LOAD !!!
   ffmpeg RGB565 convert ot ILI9488 RGB565 INCREASE CPU LOAD !!! 

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
	return -1;
    }

//-----  init ft232 ---------
    if(usb_init_ft232()<0){
        printf("Fail to initiate ft232!\n");
        return -1;
    }
//-----  Init ILI9488 and turn on display -----
    LCD_INIT_ILI9488();  //--set default FXLFMT_RGB888 


/*---------------------    Set Pixle Format    ---------------------------
 default input from bmpfile: RGB888  (from ffmpeg output)
 default output to ili9488:  RGB888  (output to LCD)
-------------------------------------------------------------------------*/
  //---CASE...  input: RGB565 , output: RGB565 ----  WORSE !!! !!! !!!
/*
    FBMP_PxlFmt=PXLFMT_RGB565;//888;//565; //BMP file format
    //----- MUSE adjust RGB order here ------
    LCD_Write_Cmd(0x36); //memory data access control
    LCD_Write_Data(0x68); // oder: BGR, see fbmp_op.h for bits exchange.
    LCD_Set_PxlFmt16bit();
*/

  //---CASE...  input: RGB888 , output: RGB565 ---- BAD !!! !!!
/*
    FBMP_PxlFmt=PXLFMT_RGB888;
    LCD_Set_PxlFmt16bit();
*/

  //---CASE...  input: RGB888 , output: RGB888 ---- GOOD !!!
    FBMP_PxlFmt=PXLFMT_RGB888;
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
     printf("  ------ finish loading a bmp file, time_use=%dms -----  \n",time_use);

     //----- delete file after displaying -----
      if(remove(str_bmpf_file) != 0)
		printf("Fail to remove the file!\n");

     //----- keep the image on the display for a while ------
//     usleep(50000);
//	sleep(1);
}

//----- close ft232 usb device -----
    close_ft232();

//----- free pinmap and close ILI9488 -----
    close_ili9488();

    return ret;
}
