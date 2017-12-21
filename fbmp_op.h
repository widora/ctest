#ifndef ___FBMP_OP_H__
#define ___FBMP_OP_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h> //stat()
#include <malloc.h>
#include "ILI9488.h"

#define STRBUFF 256   // --- length of file path&name in g_BMP_file_name[][STRBUFF]
#define MAX_FBMP_NUM 256 //--max number of bmp files in g_BMP_file_name[MAX_FBMP_NUM][]
#define MAX_WIDTH 320
#define MAX_HEIGHT 480
#define MIN_CHECK_SIZE 1000 //--- for checking integrity of BMP file 

//----- for BMP files ------
char file_path[256];
char g_BMP_file_name[MAX_FBMP_NUM][STRBUFF]; //---BMP file directory and name
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

//----clear g_BMP_file_name[] -------
bzero(&g_BMP_file_name[0][0],MAX_FBMP_NUM*STRBUFF);

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
 load a 24bit bmp file and show on lcd
 char *strf:  file path
return value:
	0  --OK
	-1 --BMP file is not complete
	-2 --open BMP file fails
	-3 --mmap fails
---------------------------------------------*/
static int show_bmpf(char *strf)
{
  int ret=0;
  int fp;//file handler
//  int LCD_pxlfmt -- global variable
  uint8_t buff[8]; //--for buffering  data temporarily
  uint16_t picWidth, picHeight;
  uint32_t picNum;
  int i;
  long offp; //offset position
  uint16_t Hs,He,Vs,Ve; //--GRAM area definition parameters
  uint16_t Hb,Vb;//--GRAM area corner gap distance from origin
  int MapLen; // file size,mmap size
  uint8_t *pmap;//mmap
  uint8_t *prgb565;//data for RGB565 

  offp=18; // file offset  position for picture Width and Height data

   //----- check integrity of the bmp file ------
   if( get_file_size(strf) < MIN_CHECK_SIZE)  // ------!!!!!!!!!
   {
	printf(" BMP file is not complete!\n");
	return -1;
   }

  //---- open file  ------
  fp=open(strf,O_RDONLY);
  if(fp<0)
	  {
		//--- There is probality that it fails to open 9.bmp,99.bmp,999.bmp.... !!!!!
          	printf("\n Fail to open the file! try to remove it...\n");
	        if(remove(strf) != 0)
	                printf("Fail to remove the file!\n");

		return -2;
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
   printf("  picWidth=%d    picHeight=%d \n",picWidth,picHeight);

   //------------  pic area  ---------
   picNum=picWidth*picHeight;

   /*--------------------- MMAP -----------------------*/
   MapLen=picNum*3+54;
   pmap=(uint8_t*)mmap(NULL,MapLen,PROT_READ,MAP_PRIVATE,fp,0);
   if(pmap == MAP_FAILED){
   	printf(" pmap mmap failed! \n");
	close(fp);
        return -3; 
   }
   else
        printf(" pmap mmap successfully! \n");

   //------------  calculate GRAM area -------------
   //------  picWidth MUST be an 4times number??? -------
   Hb=(480-picWidth+1)/2;
   Vb=(320-picHeight+1)/2;
   Hs=Hb; He=Hb+picWidth-1;
   Vs=Vb; Ve=Vb+picHeight-1;

   //------  write to LCD to show ------
   offp=54; //---offset position where BGR data begins

   //------first  set LCD pixle format in main.c !!!!!!!

   if(LCD_pxlfmt==PXLFMT_RGB565)
   {
	printf("--- pixel format set to PXLFMT_RGB565 ---\n");
	//----- allocate mem. for RGB565 
	prgb565=malloc(picNum*2);

	if( prgb565==NULL )
	{
		printf("Fail to allocate memory for prgb565.\n");
		//---- reset pixle formate then ----
		LCD_pxlfmt=PXLFMT_RGB888;
	}
	else
	{
		//---- convert RBG888  to RBG565 ----
		//uint16_t *pt565;
		uint8_t *pt565;
		uint8_t *pt888;

		pt565=prgb565;

		//----- convert from RGB888 to RGB565 ----
		for(i=0;i<picNum;i++)
		{
			pt888=pmap+offp+3*i;// 3bytes for each pixel
			//*pt565=GET_RGB565(*pt888,*(pt888+2),*(pt888+1)); //BGR -> RGB
			pt565[2*i+1]=( ((pt888[1]<<3) & 0xE0) | pt888[2]>>3 );
			pt565[2*i] = ( ( pt888[0] & 0xF8) | (pt888[1]>>5) );
			//pt565+=1; // 2bytes for each pixel
		}

		//<<<<<<<<<<<<<     Method 1:   write to LCD directly    >>>>>>>>>>>>>>>
		LCD_Write_Block(Hs,He,Vs,Ve,(uint8_t*)prgb565,picNum*2);
	}
   }


   if(LCD_pxlfmt==PXLFMT_RGB888) //--- LCD_fxlfmt == PXLFMT_RGB888
   {
	printf("--- pixel format set to PXLFMT_RGB888 ---\n");
	//<<<<<<<<<<<<<     Method 1:   write to LCD directly    >>>>>>>>>>>>>>>
	LCD_Write_Block(Hs,He,Vs,Ve,pmap+offp,picNum*3);
	//<<<<<<<<<<<<<     Method 2:   write to GBuffer first, then refresh GBuffer  >>>>>>>>>>>>
	//!!!!!!!!   SPEED IS LOW  !!!!!!!!!!wq
	//   GBuffer_Write_Block(Hs, He, Vs, Ve, pmap+offp);
	//   LCD_Write_GBuffer();
   }

   //----- free mem------
   if(prgb565 != NULL)
	   free(prgb565);
   //------ freep mmap ----
   printf("start munmap()...\n\n"); 
   munmap(pmap,MapLen); 
   //----- close fp ----
   close(fp);

}


#endif
