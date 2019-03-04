#ifndef __EGI_IMAGE_H__
#define __EGI_IMAGE_H__

typedef struct
{
        int height;
        int width;
        uint16_t *imgbuf; /* image data, for RGB565 format */
	void *data; /* image data, for other pixel format */
} EGI_IMGBUF;


#endif
