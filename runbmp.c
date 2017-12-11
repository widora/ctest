/*-----------------------------------------------------------------------------
Based on:  libftdi - A library (using libusb) to talk to FTDI's UART/FIFO chips
Original Source:  https://www.intra2net.com/en/developer/libftdi/
by:
    www.intra2net.com    2003-2017 Intra2net AG


./openwrt-gcc -L. -lftdi1 -lusb-1.0 -o runbmp runbmp.c


Midas
-------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h> //stat()
#include "include/ftdi.h"
#include "ft232.h"
#include "ILI9488.h"


#define STRBUFF 256   // --- length of file path&name
#define MAX_WIDTH 320
#define MAX_HEIGHT 480

//----- for BMP files ------
char file_path[256];
char g_BMP_file_name[256][STRBUFF]; //---BMP file directory and name
int  g_BMP_file_num=0;//----BMP file name index
int  g_BMP_file_total=0; //--total number of BMP files


/*------------------------------------------
get file size
return:
	>0 ok
	<0 fail
-------------------------------------------*/
unsigned long get_file_size(const char *fpath)
{
	unsigned long filesize=-1;
	struct stat statbuff;
	if(stat(fpath,&statbuff)<0)
	{
		return filesize;
	}
	else
	{
		filesize = statbuff.st_size;
	}
	return filesize;
}

/* --------------------------------------------
 find out all BMP files in a specified directory
 return value:
	 0 --- OK
	<0 --- fails
----------------------------------------------*/
static int Find_BMP_files(char* path)
{
DIR *d;
struct dirent *file;
int fn_len;

g_BMP_file_total=0; //--reset total  file number
g_BMP_file_num=0; //--reset file  index

//-------- if open dir error ------
if(!(d=opendir(path)))
{
  printf("error open dir: %s !\n",path);
  return -1;
}

while((file=readdir(d))!=NULL)
{
   //------- find out all bmp files  --------
   fn_len=strlen(file->d_name);
   if(strncmp(file->d_name+fn_len-4,".bmp",4)!=0 )
       continue;
   strncpy(g_BMP_file_name[g_BMP_file_num++],file->d_name,fn_len);
   g_BMP_file_total++;
 }

 closedir(d);
 return 0;
}






/*--------------------------------------------
 load a 480x320x24bit bmp file and show on lcd
 char *strf:  file path
return value:
	0  --OK
	-1 --BMP file is not complete
	-2 --mmap fails
---------------------------------------------*/
static int show_bmpf(char *strf)
{
  int ret=0;
  int fp;//file handler
  uint8_t buff[8]; //--for buffering  data temporarily
  uint16_t picWidth, picHeight;
  long offp; //offset position
  int MapLen; // file size,mmap size
  uint8_t *pmap;//mmap 


  offp=18; // file offset  position for picture Width and Height data

   //----- check integrity of the bmp file ------
   if( get_file_size(strf) < 460854 )
   {
	printf(" BMP file is not complete!\n");
	return -1;
   }

  //---- open file  ------
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
        return -2; 
   }
   else
        printf("\n pmap mmap successfully!");

   //----- copy data to graphic buffer -----
   offp=54; //---offset position where BGR data begins
   printf("memcpy RBG data to GBuffer...\n");
   memcpy(&g_GBuffer[0][0],pmap+offp, 480*320*3);

   //------  write to LCD to show ------
   printf("write to GBuffer...\n");
   LCD_Write_GBuffer();

   //------ freep mmap ----
   printf("start munmap()...\n"); 
   munmap(pmap,MapLen); 
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
    char str_bmpf_path[128]; //directory for BMP files 
    char str_bmpf_file[STRBUFF];// full path+name for a BMP file.
//    char strf[STRBUFF];
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

     if( show_bmpf(str_bmpf_file) <0 )
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
//     usleep(30000);

}


//----- close ft232 usb device -----
    close_ft232();

//----- release pin mmap -----
    resPinMmap();

    return ret;
}
