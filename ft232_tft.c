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

int show_bmpf(char *strf)
{
  int fp;//file handler
  uint8_t buff[8]; //--for buffering  data temporarily
  uint16_t picWidth, picHeight;
  long offp; //offset position
  int MapLen; // file size,mmap size
  uint8_t *pmap;//mmap 

  offp=18; // file offset  position for picture Width and Height data

  fp=open(strf,O_RDONLY);
  if(fp<0)
	  {
          	printf("\n Fail to open the file!\n");
          }
   else
          printf("%s opened successfully!\n",strf);

   //----    seek position and readin picWidth and picHeight   ------
   if(lseek(fp,offp,SEEK_SET)<0)
   	printf("Fail to offset seek position!\n");
   read(fp,buff,8);

   //----  get pic. size -----
   picWidth=buff[3]<<24|buff[2]<<16|buff[1]<<8|buff[0];
   picHeight=buff[7]<<24|buff[6]<<16|buff[5]<<8|buff[4];
   printf("\n picWidth=%d    picHeight=%d",picWidth,picHeight);

   /*--------------------- MMAP -----------------------*/
   MapLen=picWidth*picHeight*3+54;
   pmap=(uint8_t*)mmap(NULL,MapLen,PROT_READ,MAP_PRIVATE,fp,0);
   if(pmap == MAP_FAILED){
   	printf("\n pmap mmap failed!");
        return -1; 
   }
   else
        printf("\n pmap mmap successfully!");

   //----- copy data to graphic buffer -----
   offp=54; //---offset position where BGR data begins
   memcpy(&g_GBuffer[0][0],pmap+offp, 480*320*3);

   //------  write to LCD to show ------
   LCD_Write_GBuffer();

   //----- close fp ----
   close(fp);

}

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
    chunk_size=1024*64;// >=1024*32 same effect.    default is 4096
    ftdi_write_data_set_chunksize(g_ftdi,chunk_size);

//-----  Init ILI9488 and turn on display -----
    LCD_INIT_ILI9488();

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
gettimeofday(&tm_start,NULL);
show_bmpf("/tmp/widora.bmp");
gettimeofday(&tm_end,NULL);
time_use=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
printf("  ------ finish loading a 480*320*24bits bmp file, time_use=%dms -----  \n",time_use);

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
