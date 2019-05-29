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
	int i,j;
	char header[8];
	FILE *fil;
	png_structp png_ptr;
	png_infop   info_ptr;

	int	width;
	int	height;
	char	*imgbuf=NULL;
	EGI_IMGBUF  eimg;
	int	pos;
	png_byte color_type;
	png_byte bit_depth;
	png_bytep *row_ptr;

	if( argc<2 ) {
		printf("Please enter png file name!\n");
		return -1;
	}
	fil=fopen(argv[1],"rb");
	if(fil==NULL) {
		printf("Fail to open png file:%s.\n", argv[1]);
		return -2;
	}

	/* check whether it's a PNG file */
	fread(header,1, 8, fil);
	if(png_sig_cmp(header,0,8)) {
		printf("Input file %s is NOT a recognizable PNG file!\n",argv[1]);
		fclose(fil);
		return -3;
	}

	/* Init/prepare png srtuct for read */
	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(png_ptr==NULL) {
		printf("png_create_read_struct failed!\n");
		fclose(fil);
		return -3;
	}
	info_ptr=png_create_info_struct(png_ptr);
	if(info_ptr==NULL) {
		printf("png_create_info_struct failed!\n");
		fclose(fil);
		return -4;
	}
	if( setjmp(png_jmpbuf(png_ptr)) != 0) {
		printf("setjmp(png_jmpbuf(png_ptr)) failed!\n");
		fclose(fil);
		return -5;
	}
	png_init_io(png_ptr,fil);
	png_set_sig_bytes(png_ptr, 8); /* 8 is Max */

	/* read png info */
	png_read_png(png_ptr,info_ptr, PNG_TRANSFORM_EXPAND,0);
	width=png_get_image_width(png_ptr, info_ptr);
	height=png_get_image_height(png_ptr,info_ptr);
	color_type=png_get_color_type(png_ptr, info_ptr);
	bit_depth=png_get_bit_depth(png_ptr, info_ptr);
	printf("PNG image: Width=%d, Height=%d, color_type=%d, bit_depth=%d \n"
							,width,height,color_type,bit_depth);
	eimg.height=height;
	eimg.width=width;


	/* alloc RBG image buffer */
	eimg.imgbuf=calloc(2, width*height);  /* 16bits per pixel */
	if(eimg.imgbuf==NULL) {
		printf("Fail to calloc eimg.imgbuf!\n");
		fclose(fil);
		return -3;
	}

	/* read RGB data into image buffer */
	row_ptr=png_get_rows(png_ptr,info_ptr);
	pos=0;
	for(i=0; i<height; i++) {
		for(j=0; j< 4*width; j+=4 ) {  /* 4, RGBA */
			// row_ptr[i][j+2];  /* Blue */
			// row_ptr[i][j+1];  /* Green */
			// row_ptr[i][j];    /* Red */
			// row_ptr[i][j+3];  /* Alpha */
		    *(eimg.imgbuf+pos)=COLOR_RGB_TO16BITS(row_ptr[i][j], row_ptr[i][j+1],row_ptr[i][j+2]);
 		    pos++;
		}
	}

        /* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

        /* window_position displaying */
        egi_imgbuf_windisplay(&eimg, &gv_fb_dev, 0, 0, 0, 0, eimg.width, eimg.height);


	/* free and close */
	png_destroy_read_struct(&png_ptr, &info_ptr,0);

INIT_FAIL:
	egi_imgbuf_release(&eimg);

	fclose(fil);

	return 0;
}
