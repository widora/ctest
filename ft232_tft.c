/*-----------------------------------------------------------------------------
Based on:  libftdi - A library (using libusb) to talk to FTDI's UART/FIFO chips
Original Source:  https://www.intra2net.com/en/developer/libftdi/
by:
    www.intra2net.com    2003-2017 Intra2net AG


Loop showing 24bit_color BMP files on a LCD display connected with Widora via FT232H

compile:
	./openwrt-gcc -L. -lftdi1 -lusb-1.0 -o ft232_tft ft232_tft.c

usage:
	./ft232_tft path

Midas Zhou
-------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "include/ftdi.h"//bitbang ops
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
    LCD_INIT_ILI9488();  //--set default FXLFMT_RGB888 

//------  set LCD pixle format  -------
    LCD_Set_PxlFmt16bit();
//    LCD_Set_PxlFmt24bit();


//<<<<<<<<<<<<<  refresh GRAPHIC BUFFER test >>>>>>>>>>>>>>>>
uint32_t  tmpd=1;
/*
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

	LCD_Write_Cmd(0x22);//all pixels off
//	LCD_Write_Cmd(0x28);//display off
	LCD_Write_GBuffer();
//        LCD_Write_Cmd(0x23);//all pixels on
//	LCD_Write_Cmd(0x29);//display on
	LCD_Write_Cmd(0x13);//normal display mode on
	usleep(200000);
	gettimeofday(&tm_end,NULL);
	time_use=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
	printf("  ------ finish refreshing whole graphic buffer, time_use=%dms -----  \n",time_use);
}
*/

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
          printf("\n\n==========  reload BMP file, totally  %d BMP-files found.   ============\n",g_BMP_file_total);
          if(g_BMP_file_total == 0){
             printf("\n No BMP file found! \n");
             return -1;
	  }
          Ncount=g_BMP_file_total-1; //---reset Ncount, [Nount] starting from 0
      }

     //----- load BMP file path -------------
     sprintf(str_bmpf_file,"%s/%s",str_bmpf_path,g_BMP_file_name[Ncount]);
     printf("str = %s\n",str_bmpf_file);
     Ncount--;

     //------  show the bmp file and count time -------
     gettimeofday(&tm_start,NULL);

     show_bmpf(str_bmpf_file);

     gettimeofday(&tm_end,NULL);
     time_use=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
     printf(" \n  ------ finish loading a file, time_use=%dms -----  \n",time_use);


      //
//     usleep(60000);
     sleep(3); //---hold on for a while

}

//<<<<<<<<<<<<<<< color block test  >>>>>>>>>>>>>
uint8_t color_buf[3];
color_buf[0]=0xff;color_buf[1]=0x00;color_buf[2]=0x00;
LCD_ColorBox(0,0,30,300,color_buf);
color_buf[0]=0x00;color_buf[1]=0xff;color_buf[2]=0x00;
LCD_ColorBox(30,0,30,300,color_buf);
color_buf[0]=0x00;color_buf[1]=0x00;color_buf[2]=0xff;
LCD_ColorBox(60,0,30,300,color_buf);


//----- close ft232 usb device -----
    close_ft232();

//----- release pin mmap -----
    resPinMmap();

    return ret;
}
