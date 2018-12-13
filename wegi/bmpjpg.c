/* -------------------------------------------------------------------------
original source: https://blog.csdn.net/luxiaoxun/article/details/7622988

1. Modified for a 240x320 SPI LCD display.
2. The width of the displaying picture must be a multiple of 4.
3. Applay show_jpg() or show_bmp() in main().

TODO:
    1. to pad width to a multiple of 4 for bmp file.
    2. jpgshow() picture flips --OK
    3. in show_jpg(), just force 8bit color data to be 24bit, need to improve.!!!


./open-gcc -L./lib -I./include -ljpeg -o jpgshow fbshow.c

Modified by Midas
---------------------------------------------------------------------------*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h> //u
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <jpeglib.h>
#include <jerror.h>
#include "bmpjpg.h"

BITMAPFILEHEADER FileHead;
BITMAPINFOHEADER InfoHead;

static int xres = 240;
//static int yres = 320;
static int bits_per_pixel = 16; //tft lcd

int show_bmp(char* fpath, FBDEV *fb_dev)
{
	FILE *fp;
	int rc;
	int line_x, line_y;
	long int location = 0;

	/* get fb map */
	char *fbp =fb_dev->map_fb;

	printf("fpath=%s\n",fpath);
	fp = fopen( fpath, "rb" );
	if (fp == NULL)
	{
		return( -1 );
	}

	rc = fread( &FileHead, sizeof(BITMAPFILEHEADER),1, fp );
	if ( rc != 1)
	{
		printf("read header error!\n");
		fclose( fp );
		return( -2 );
	}

	//检测是否是bmp图像
	if (memcmp(FileHead.cfType, "BM", 2) != 0)
	{
		printf("it's not a BMP file\n");
		fclose( fp );
		return( -3 );
	}

	rc = fread( (char *)&InfoHead, sizeof(BITMAPINFOHEADER),1, fp );
	if ( rc != 1)
	{
		printf("read infoheader error!\n");
		fclose( fp );
		return( -4 );
	}

	printf("BMP width=%d,height=%d  ciBitCount=%d\n",(int)InfoHead.ciWidth, (int)InfoHead.ciHeight, (int)InfoHead.ciBitCount);
	//检查是否24bit色
	if(InfoHead.ciBitCount != 24 ){
		printf("It's not 24bit_color BMP!\n");
		return -1;
	}
	//检查宽度是否240x320
	if(InfoHead.ciWidth > 240 ){
		printf("The width is great than 240!\n");
		return -1;
	}
	if(InfoHead.ciHeight > 320 ){
		printf("The height is great than 320!\n");
		return -1;
	}

	//跳转的数据区
	fseek(fp, FileHead.cfoffBits, SEEK_SET);

	int x0=0,y0=0; //set original coodinate
	if( InfoHead.ciHeight+y0 > 320 || InfoHead.ciWidth+x0 > 240 )
	{
		printf("The origin of picture (x0,y0) is too big for a 240x320 LCD.\n");
		return -1;
	}

	line_x = line_y = 0;
	//向framebuffer中写BMP图片

	while(!feof(fp))
	{
		PIXEL pix;
		//unsigned short int tmp;
		rc = fread( (char *)&pix, 1, sizeof(PIXEL), fp);
		if (rc != sizeof(PIXEL))
			break;

		// frame buffer location
		location = line_x * bits_per_pixel / 8 +x0 + (InfoHead.ciHeight - line_y - 1 +y0) * xres * bits_per_pixel / 8;

		//显示每一个像素, in ili9431 node of dts, color sequence is defined as 'bgr'(as for ili9431) .
		// little endian is noticed.
		// ---- converting to format R5G6B5(as for framebuffer) -----
		*(fbp + location + 0)=(((pix.green)&0b11100)<<3) + (((pix.blue)&0b11111000)>>3);
		*(fbp + location + 1)=((pix.red)&0b11111000) + (((pix.green)&0b11100000)>>5);

		line_x++;
		if (line_x == InfoHead.ciWidth )
		{
			line_x = 0;
			line_y++;
			//printf("line_y = %d\n",line_y);
			if(line_y == InfoHead.ciHeight)
				break;
		}
	}

	fclose( fp );
	return( 0 );
}


/*------------------------------------------------------
    blackoff: if balckoff>0, then make black transparent
    (x0,y0): original coordinate of picture in LCD
--------------------------------------------------------*/
int show_jpg(char* fpath, FBDEV *fb_dev, int blackoff, int x0, int y0)
{
	int width,height;
	int components; 
	unsigned char *imgbuf;
	unsigned char *dat;
	unsigned char tmpa,tmpb;
	long int location = 0;
	int line_x,line_y;

	FILE *infile=NULL;

	/* get fb map */
	char *fbp =fb_dev->map_fb;

	imgbuf=open_jpgImg(fpath,&width,&height,&components, &infile);

	if(imgbuf==NULL) {
		printf("open_jpgImg fails!\n");
		return -1;
	}

	/* WARNING: need to be improve here: converting 8bit to 24bit color*/
	if(components==1) /* 8bit color */
	{
		height=height/3; /* force to be 24bit pic, however shorten the height */
	}

	dat=imgbuf;


//       printf("open_jpgImg() succeed, width=%d, height=%d\n",width,height);

#if 0	//---- normal data sequence ------
	/* WARNING: blackoff not apply here */
	line_x = line_y = 0;
	for(line_y=0;line_y<height;line_y++) {
		for(line_x=0;line_x<width;line_x++) {
			location = (line_x+x0) * bits_per_pixel / 8 + (height - line_y - 1 +y0) * xres * bits_per_pixel / 8;
			//显示每一个像素, in ili9431 node of dts, color sequence is defined as 'bgr'(as for ili9431) .
			// little endian is noticed.
   	        	// ---- dat(R8G8B8) converting to format R5G6B5(as for framebuffer) -----
        	        *(fbp + location + 0)=((*(dat+1)&0b11100)<<3) + ((*(dat+2) & 0b11111000)>>3);
	                *(fbp + location + 1)=(*dat&0b11111000) + ((*(dat+1)&0b11100000)>>5);

			dat+=3;
		}
	}
#else
	//---- flip picture to be same data sequence of BMP file ---
	line_x = line_y = 0;
	for(line_y=height-1;line_y>0;line_y--) {
		for(line_x=0;line_x<width;line_x++) {
			location = (line_x+x0) * bits_per_pixel / 8 + (height - line_y - 1 +y0) * xres * bits_per_pixel / 8;
			//显示每一个像素, in ili9431 node of dts, color sequence is defined as 'bgr'(as for ili9431) .
			// little endian is noticed.
   	        	// ---- dat(R8G8B8) converting to format R5G6B5(as for framebuffer) -----
        	        tmpa=((*(dat+1)&0b11100)<<3) + ((*(dat+2) & 0b11111000)>>3);
	                tmpb=(*dat&0b11111000) + ((*(dat+1)&0b11100000)>>5);

			if(blackoff) /* don't write black to fb, so make it transparent to back color */
			{
				/* --- ignore black pixels, make it transparent for background. ----- */
				if(tmpa || tmpb) /* only write non-black data */
				{
        	        		*(fbp + location + 0)=tmpa;
	                		*(fbp + location + 1)=tmpb;
				}
			}
			else	/* write all color data to fb */
			{
        	        		*(fbp + location + 0)=tmpa;
	                		*(fbp + location + 1)=tmpb;
			}


			dat+=3;
		}
	}
#endif
	close_jpgImg(imgbuf);
	fclose(infile);
	return 0;
}

/*--------------------------------------------------------------
 open jpg file and return decompressed image buffer pointer
 int *w,*h:   with and height of the image
 int *components:  out color components
 return:
	=NULL fail
	>0 decompressed image buffer pointer
--------------------------------------------------------------*/
unsigned char * open_jpgImg(char * filename, int *w, int *h, int *components, FILE **fil)
{
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;
        FILE *infile;
        unsigned char *buffer;
        unsigned char *temp;

        if (( infile = fopen(filename, "rb")) == NULL) {
                fprintf(stderr, "open %s failed\n", filename);
                return NULL;
        }

	/* return FILE */
	*fil = infile;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);

        jpeg_stdio_src(&cinfo, infile);

        jpeg_read_header(&cinfo, TRUE);

        jpeg_start_decompress(&cinfo);
        *w = cinfo.output_width;
        *h = cinfo.output_height;
	*components=cinfo.out_color_components;

//	printf("output color components=%d\n",cinfo.out_color_components);
//        printf("output_width====%d\n",cinfo.output_width);
//        printf("output_height====%d\n",cinfo.output_height);

	/* --- check size ----*/
        if ((cinfo.output_width > 240) ||
                   (cinfo.output_height > 320)) {
                printf("too large size JPEG file,cannot display\n");
                return NULL;
        }
        buffer = (unsigned char *) malloc(cinfo.output_width *
                        cinfo.output_components * cinfo.output_height);
        temp = buffer;

        while (cinfo.output_scanline < cinfo.output_height) {
                jpeg_read_scanlines(&cinfo, &buffer, 1);
                buffer += cinfo.output_width * cinfo.output_components;
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        return temp;

        fclose(infile);
}

/*    release mem for decompressed jpeg image buffer */
void close_jpgImg(unsigned char *imgbuf)
{
	if(imgbuf != NULL)
		free(imgbuf);
}
