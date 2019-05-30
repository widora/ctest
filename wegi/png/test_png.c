#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "egi_fbgeom.h"
#include "egi_image.h"
#include "egi_color.h"

#define PNG_FIXED_POINT_SUPPORTED


int main(int argc, char **argv)
{
	int ret;
	int n;
	int i,j;
	char header[8];
	FILE *fil;
	png_structp png_ptr;
	png_infop   info_ptr;

	int bytpp; /* bytes per pixel */

	int	width;
	int	height;
//	char	*imgbuf=NULL;
	EGI_IMGBUF  eimg={0};
	eimg.imgbuf=NULL;
	eimg.alpha=NULL;

	int	pos;
	png_byte color_type;
	png_byte bit_depth;
	png_byte   channels;
	png_byte   pixel_depth;
	png_bytep *row_ptr;

	if( argc<2 ) {
		printf("Please enter png file name!\n");
		return -1;
	}


 /* display all png files in input list */
 for(n=1; n<argc; n++) {

	printf("init_dev() for n=%d \n",n);
        /* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

	printf("Try to open png file:%s.\n", argv[n]);
	/* open PNG file */
	fil=fopen(argv[n],"rb");
	if(fil==NULL) {
		printf("Fail to open png file:%s.\n", argv[n]);
//		return -2;
		continue;
	}


	printf("Check file %s.\n", argv[n]);
	/* check whether it's a PNG file */
	fread(header,1, 8, fil);
	if(png_sig_cmp(header,0,8)) {
		printf("Input file %s is NOT a recognizable PNG file!\n",argv[n]);
		ret=-2;
		goto INIT_FAIL;
	}

	printf("init png_ptr and info_ptr...\n");
	/* Init/prepare png srtuct for read */
	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(png_ptr==NULL) {
		printf("png_create_read_struct failed!\n");
		ret=-3;
		goto INIT_FAIL;
	}
	info_ptr=png_create_info_struct(png_ptr);
	if(info_ptr==NULL) {
		printf("png_create_info_struct failed!\n");
		ret=-4;
		goto INIT_FAIL;
	}
	if( setjmp(png_jmpbuf(png_ptr)) != 0) {
		printf("setjmp(png_jmpbuf(png_ptr)) failed!\n");
		ret=-5;
		goto INIT_FAIL;
	}
	png_init_io(png_ptr,fil);
	png_set_sig_bytes(png_ptr, 8); /* 8 is Max */


	printf("read PNG file info...\n");
	/* read png info: size, type, bit_depth */
    /* Contents of row_info:
    *  png_uint_32 width      width of row
    *  png_uint_32 rowbytes   number of bytes in row
    *  png_byte color_type    color type of pixels
    *  png_byte bit_depth     bit depth of samples
    *  png_byte channels      number of channels (1-4)
    *  png_byte pixel_depth   bits per pixel (depth*channels)
    */
	png_read_png(png_ptr,info_ptr, PNG_TRANSFORM_EXPAND,0);
	width=png_get_image_width(png_ptr, info_ptr);
	height=png_get_image_height(png_ptr,info_ptr);
	color_type=png_get_color_type(png_ptr, info_ptr);
	bit_depth=png_get_bit_depth(png_ptr, info_ptr);
	pixel_depth=info_ptr->pixel_depth;
	channels=png_get_channels(png_ptr, info_ptr);

	printf("PNG file '%s':\n", argv[n]);
	printf("Width=%d, Height=%d, color_type=%d \n", width, height, color_type);
	printf("bit_depth=%d,  channels=%d pixel_depth=%d \n", bit_depth, channels, pixel_depth);

	/* Now we only deal with type_2(real color) and type_6(real color with alpha) PNG
	 * and bit_depth == 8
	 * TODO:
	 */
	if(bit_depth!=8 || (color_type !=2 && color_type !=6) ){
		printf(" Only support PNG color_type=2(real color) or \
			 color_type=6 (real color with alpha channel) and bit_depth must be 8.\n");
		ret=-6;
		goto READ_FAIL;
	}

	printf("alloc eimg.imgbuf ...\n");
	/* alloc RBG image buffer */
	eimg.height=height;
	eimg.width=width;
	eimg.imgbuf=calloc(1, width*height*2);
	if(eimg.imgbuf==NULL) {
		printf("Fail to calloc eimg.imgbuf!\n");
		ret=-7;
		goto READ_FAIL;
	}
	printf("alloc eimg.alpha ...\n");
	/* alloc Alpha channel data */
	if(color_type==6) {
		eimg.alpha=calloc(1,width*height);
		if(eimg.alpha==NULL) {
			printf("Fail to calloc eimg.alpha!\n");
			ret=-8;
			goto READ_FAIL;
		}
	}


	printf("png_get_rows() ...\n");
	/* read RGB data into image buffer */
	row_ptr=png_get_rows(png_ptr,info_ptr);
	pos=0;

	if(color_type==2) /*RGB*/
		bytpp=3;
	else if(color_type==6) /*RGBA*/
		bytpp=4;

	printf("read color to eimg ...\n");
	for(i=0; i<height; i++) {
		for(j=0; j< bytpp*width; j+=bytpp ) {  /* color_type=2 (RGB, 3Bpp );  color_type=6 (RGBA, 4Bpp) */
			// row_ptr[i][j];    /* Red */
			// row_ptr[i][j+1];  /* Green */
			// row_ptr[i][j+2];  /* Blue */
			// row_ptr[i][j+3];  /* Alpha */
		    *(eimg.imgbuf+pos)=COLOR_RGB_TO16BITS(row_ptr[i][j], row_ptr[i][j+1],row_ptr[i][j+2]);
		    if(color_type==6) {
			*(eimg.alpha+pos)=row_ptr[i][j+3];
		    }

 		    pos++;
		}
	}

	printf("display imgbuf ...\n");
        /* window_position displaying */
        egi_imgbuf_windisplay(&eimg, &gv_fb_dev, 0, 0, 0, 0, 240, 320);
//        egi_imgbuf_windisplay(&eimg, &gv_fb_dev, 0, 0, 70, 220, eimg.width, eimg.height);
	sleep(1);

	printf("reset n ...\n");
	/* loop ... */
	if(n==(argc-1))
		n=1;

READ_FAIL:
	printf("imgbuf release...\n");
	egi_imgbuf_release(&eimg);
	printf("png_destroy_read_struct ...\n");
	png_destroy_read_struct(&png_ptr, &info_ptr,0);

INIT_FAIL:
	fclose(fil);
	printf("release fbdev ...\n");
	release_fbdev(&gv_fb_dev);

  } /* end of for() */

	return 0;
}
