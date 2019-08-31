/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

NOTE: Try not to use EGI_PDEBUG() here!

EGI_IMGBUF functions

Midas Zhou
-------------------------------------------------------------------*/

#include <pthread.h>
#include "egi_image.h"
#include "egi_bjp.h"

typedef struct fbdev FBDEV;

/*--------------------------------------------
   	   Allocate  a EGI_IMGBUF
---------------------------------------------*/
EGI_IMGBUF *egi_imgbuf_alloc(void) //new(void)
{
	EGI_IMGBUF *eimg;
	eimg=calloc(1, sizeof(EGI_IMGBUF));
	if(eimg==NULL) {
		printf("%s: Fail to calloc EGI_IMGBUF.\n",__func__);
		return NULL;
	}

        /* init imgbuf mutex */
        if(pthread_mutex_init(&eimg->img_mutex,NULL) != 0)
        {
                printf("%s: fail to initiate img_mutex.\n",__func__);
		free(eimg);
                return NULL;
        }

	return eimg;
}


/*------------------------------------------------------------
Free data in EGI_IMGBUF, but NOT itself!

NOTE:
  1. WARNING!!!: No mutex operation here, the caller shall take
     care of imgbuf mutex lock.

-------------------------------------------------------------*/
void egi_imgbuf_cleardata(EGI_IMGBUF *egi_imgbuf)
{
	if(egi_imgbuf != NULL) {
	        if(egi_imgbuf->imgbuf != NULL) {
        	        free(egi_imgbuf->imgbuf);
                	egi_imgbuf->imgbuf=NULL;
        	}
     	   	if(egi_imgbuf->alpha != NULL) {
        	        free(egi_imgbuf->alpha);
                	egi_imgbuf->alpha=NULL;
        	}
       		if(egi_imgbuf->data != NULL) {
               	 	free(egi_imgbuf->data);
                	egi_imgbuf->data=NULL;
        	}
		if(egi_imgbuf->subimgs != NULL) {
			free(egi_imgbuf->subimgs);
			egi_imgbuf->subimgs=NULL;
		}

		/* reset size and submax */
		egi_imgbuf->height=0;
		egi_imgbuf->width=0;
		egi_imgbuf->submax=0;
	}
}

/*---------------------------------------
Free EGI_IMGBUF and its data
----------------------------------------*/
void egi_imgbuf_free(EGI_IMGBUF *egi_imgbuf)
{
        if(egi_imgbuf == NULL)
                return;

	/* Hope there is no other user */
	pthread_mutex_lock(&egi_imgbuf->img_mutex);

	/* free data inside */
	egi_imgbuf_cleardata(egi_imgbuf);

	/* TODO :  ??????? necesssary ????? */
	pthread_mutex_unlock(&egi_imgbuf->img_mutex);

	free(egi_imgbuf);

	egi_imgbuf=NULL;
}


/*-------------------------------------------------------------
Initiate/alloc imgbuf as an image canvas, with all alpha=0

NOTE:
  1. !!!!WARNING!!!: No mutex operation here, the caller shall take
     care of imgbuf mutex lock.

@height		height of image
@width		width of image

Return:
	0	OK
	<0	Fails
----------------------------------------------------------------*/
int egi_imgbuf_init(EGI_IMGBUF *egi_imgbuf, int height, int width)
{
	if(egi_imgbuf==NULL)
		return -1;

	/* empty old data if any */
	egi_imgbuf_cleardata(egi_imgbuf);

        /* calloc imgbuf->imgbuf */
        egi_imgbuf->imgbuf = calloc(1,height*width*sizeof(uint16_t));
        if(egi_imgbuf->imgbuf == NULL) {
                printf("%s: fail to calloc egi_imgbuf->imgbuf.\n",__func__);
		return -2;
        }

        /* calloc imgbuf->alpha, alpha=0, 100% canvas color. */
        egi_imgbuf->alpha= calloc(1, height*width*sizeof(unsigned char)); /* alpha value 8bpp */
        if(egi_imgbuf->alpha == NULL) {
                printf("%s: fail to calloc egi_imgbuf->alpha.\n",__func__);
		free(egi_imgbuf->imgbuf);
                return -3;
        }

        /* retset height and width for imgbuf */
        egi_imgbuf->height=height;
        egi_imgbuf->width=width;

	return 0;
}

/* ------------------------------------------------------
Create an EGI_IMGBUF, set color and alpha value.

@height,width:	height and width of the imgbuf.
@alpha:		>0, alpha value for all pixels.
		(default 0)
@color:		>0 basic color of the imgbuf.
		(default 0)

-------------------------------------------------------*/
EGI_IMGBUF *egi_imgbuf_create( int height, int width,
				unsigned char alpha, EGI_16BIT_COLOR color )
{
	int i;

	EGI_IMGBUF *imgbuf=egi_imgbuf_alloc();
	if(imgbuf==NULL)
		return NULL;

	/* init the struct */
	if ( egi_imgbuf_init(imgbuf, height, width) !=0 )
		return NULL;

	/* set alpha and color */
	if(alpha>0)     /* alpha default calloced as 0 already */
		memset( imgbuf->alpha, alpha, height*width );
	if(color>0) {   /* color default calloced as 0 already */
		for(i=0; i< height*width; i++)
			*(imgbuf->imgbuf+i)=color;
	}

	return imgbuf;
}


/*-----------------------------------------------------------
Set/create a frame for an EGI_IMGBUF.
The frame are formed by different patterns of alpha
values.
Note:
1. Usually the frame is to be used as cutout_shape of a image.
2. If input eimg has no data of alpha value, then allocate
   and assign with 255 for all pixels, if input alpha<0.

@eimg:		An EGI_IMGBUF holding pixel colors.
@type:          type/shape of the frame.
@alpha:		reset alpha value for all pixels, applied only >=0.
@pn:            numbers of paramters
@param:         array of params

Return:
        0      OK
        <0     Fail
---------------------------------------------------------*/
int egi_imgbuf_setframe( EGI_IMGBUF *eimg, enum imgframe_type type,
			 int alpha, int pn, const int *param )
{
	int i,j;

	if( pn<1 || param==NULL ) {
		printf("%s: Input param invalid!\n",__func__);
		return -1;
	}
	if( eimg==NULL || eimg->imgbuf==NULL ) {
		printf("%s: Invali input eimg!\n",__func__);
		return -1;
	}

	int height=eimg->height;
	int width=eimg->width;

	/* allocate alpha if NULL, and set alpha value */
	if( eimg->alpha == NULL ) {
		eimg->alpha=calloc(1, height*width*sizeof(unsigned char));
		if(eimg->alpha==NULL) {
			printf("%s: Fail to calloc eimg->alpha!\n",__func__);
			return -2;
		}
		/* set alpha value */
		if( alpha<0 )  /* all set to 255 */
			memset( eimg->alpha, 255, height*width );
		else
			memset( eimg->alpha, alpha, height*width );
	}
	else {  /* reset alpha, if input alpha >=0  */
		if ( alpha >=0 )
			memset( eimg->alpha, alpha, height*width );
	}

	/*  --- edit alpha value to create a frame --- */
	/* 1. Rectangle with 4 round corners */
	if(type==frame_round_rect)
	{
		int rad=param[0];
		int ir;

		/* adjust radius */
		if( rad < 0 )rad=0;
		if( rad > height/2 ) rad=height/2;
		if( rad > width/2 ) rad=width/2;

		/* cut out 4 round corners */
		for(i=0; i<rad; i++) {
		        ir=rad-round(sqrt(rad*rad*1.0-(rad-i)*(rad-i)*1.0));
			for(j=0; j<ir; j++) {
				/* upp. left corner */
				*(eimg->alpha+i*width+j)=0; /* set alpha 0 at corner */
				/* upp. right corner */
				*(eimg->alpha+((i+1)*width-1)-j)=0;
				/* down. left corner */
				*(eimg->alpha+(height-1-i)*width+j)=0;
				/* down. right corner */
				*(eimg->alpha+(height-i)*width-1-j)=0;
			}
		}
	}
	/* TODO: other types */
	else {
		/* Original rectangle shape  */
	}

	return 0;
}



/*--------------------------------------------------------
Create a new imgbuf with certain shape/frame.
The frame are formed by different patterns of alpha
values.
Usually the frame is to be used as cutout_shape of a image.

@width,height   basic width/height of the frame.
@color:		basic color of the shape
@type:		type/shape of the frame.
@pn:		numbers of paramters
@param:		array of params

Return:
	A pointer to an EGI_IMGBUF	OK
	NULL				Fail
---------------------------------------------------------*/
EGI_IMGBUF *egi_imgbuf_newFrameImg( int height, int width,
				 unsigned char alpha, EGI_16BIT_COLOR color,
				 enum imgframe_type type,
				 int pn, const int *param )
{
	int i,j;

	EGI_IMGBUF *imgbuf=egi_imgbuf_create(height, width, alpha, color);
	if(imgbuf==NULL)
		return NULL;

	if( egi_imgbuf_setframe(imgbuf, type, alpha, pn, param ) !=0 ) {
		printf("%s: Frame imgbuf created, but fail to set frame type!\n",__func__);
		/* Go on anyway....*/
	}

	return imgbuf;
}


/*-----------------------------------------------------------------
To soft/blur the image by averaging pixel colors/alpha.

@eimg:	object image.
@size:  number of pixels taken to average.

Return:
	A pointer to a new EGI_IMGBUF	OK
	NULL				Fail
------------------------------------------------------------------*/
EGI_IMGBUF  *egi_imgbuf_avgsoft(const EGI_IMGBUF *ineimg, int size)
{
	int i,j,k;
	EGI_16BIT_COLOR color;
	unsigned int avgR,avgG,avgB,avgALPHA;
	int height, width;

	if( ineimg==NULL || ineimg->imgbuf==NULL || size<=0 )
		return NULL;

	height=ineimg->height;
	width=ineimg->width;

	/* adjust size */
	if(size>width/2)
		size=width/2;

	/* create output imgbuf */
	EGI_IMGBUF *outeimg= egi_imgbuf_create( height, width, 255, 0);
	if(outeimg==NULL)
		return NULL;

	/* average color */
	for(i=0; i< height; i++) {
		/* fore half, avg from right */
		for(j=0; j< width/2; j++) {
			avgR=0; avgG=0; avgB=0; avgALPHA=0;
			for(k=0; k<size; k++) {
				color = ineimg->imgbuf[i*width+j+k];
				avgR += ((color&0xF800)>>8);
				avgG += ((color&0x7E0)>>3);
				avgB += ((color&0x1F)<<3);
				if(ineimg->alpha)
					avgALPHA += ineimg->alpha[i*width+j+k];
			}
			outeimg->imgbuf[i*width+j]=COLOR_RGB_TO16BITS(avgR/size, avgG/size, avgB/size);
			if(ineimg->alpha)
				outeimg->alpha[i*width+j]=avgALPHA/size;
		}
		/* end half, avg from left */
		for(j=width-1; j>=width; j--) {
			avgR=0; avgG=0; avgB=0;	avgALPHA=0;
			for(k=0; k<size; k++) {
				color = ineimg->imgbuf[i*width+j-k];
				avgR += ((color&0xF800)>>8);
				avgG += ((color&0x7E0)>>3);
				avgB += ((color&0x1F)<<3);
				if(ineimg->alpha)
					avgALPHA += ineimg->alpha[i*width+j-k];
			}
			outeimg->imgbuf[i*width+j]=COLOR_RGB_TO16BITS(avgR/size, avgG/size, avgB/size);
			if(ineimg->alpha)
				outeimg->alpha[i*width+j]=avgALPHA/size;
		}
	}


	return outeimg;
}



/*------------------------------------------------------------------------------
Blend two images of EGI_IMGBUF together, and allocate alpha if eimg->alpha is
NULL.

1. The holding eimg should already have image data inside, or has been initialized
   by egi_imgbuf_init() with certain size of a canvas (height x width) inside.
2. The canvas size of the eimg shall be big enough to hold the bitmap,
   or pixels out of the canvas will be omitted.
3. Size of eimg canvas keeps the same after blending.

@eimg           The EGI_IMGBUF to hold blended image.
@xb,yb          origin of the adding image relative to EGI_IMGBUF canvas coord,
                left top as origin.
@addimg		The adding EGI_IMGBUF.

return:
        0       OK
        <0      fails
--------------------------------------------------------------------------------*/
int egi_imgbuf_blend_imgbuf(EGI_IMGBUF *eimg, int xb, int yb, EGI_IMGBUF *addimg )
{
        int i,j;
        EGI_16BIT_COLOR color;
        unsigned char alpha;
        unsigned long size; /* alpha size */
        int     sumalpha;
        int epos,apos;

        if(eimg==NULL || eimg->imgbuf==NULL || eimg->height==0 || eimg->width==0 ) {
                printf("%s: input holding eimg is NULL or uninitiliazed!\n", __func__);
                return -1;
        }
        if( addimg==NULL || addimg->imgbuf==NULL ) {
                printf("%s: input addimg or its imgbuf is NULL!\n", __func__);
                return -2;
        }

        /* calloc and assign alpha, if NULL */
        if( eimg->alpha==NULL ) {
                size=eimg->height*eimg->width;
                eimg->alpha = calloc(1, size); /* alpha value 8bpp */
                if(eimg->alpha==NULL) {
                        printf("%s: Fail to calloc eimg->alpha\n",__func__);
                        return -3;
                }
                memset(eimg->alpha, 255, size); /* init alpha as 255  */
        }

        for( i=0; i< addimg->height; i++ ) {            /* traverse bitmap height  */
                for( j=0; j< addimg->width; j++ ) {   /* traverse bitmap width */
                        /* check range limit */
                        if( yb+i <0 || yb+i >= eimg->height ||
                                    xb+j <0 || xb+j >= eimg->width )
                                continue;

                        epos=(yb+i)*(eimg->width) + xb+j; /* eimg->imgbuf position */
			apos=i*addimg->width+j;		  /* addimg->imgbuf position */

			/* get color in addimg */
                        color=addimg->imgbuf[apos];

			if(addimg->alpha==NULL)
				alpha=255;
			else
				alpha=addimg->alpha[apos];

                        /* blend color (front,back,alpha) */
                        color=COLOR_16BITS_BLEND( color, eimg->imgbuf[epos], alpha);

			/* assign blended color to imgbuf */
                        eimg->imgbuf[epos]=color;

                        /* blend alpha value */
                        sumalpha=eimg->alpha[epos]+alpha;
                        if( sumalpha > 255 )
				sumalpha=255;
                        eimg->alpha[epos]=sumalpha;
                }
        }

        return 0;
}



/*--------------------------------------------------------------------------------------
For 16bits color only!!!!

Note:
1. Advantage: call draw_dot(), it is effective for FILO,
2. Advantage: draw_dot() will ensure x,y range within screen.
3. Write image data of an EGI_IMGBUF to a window in FB.
4. Set outside color as black.
5. window(xw, yw) defines a looking window to the original picture, (xp,yp) is the left_top
   start point of the window. If the looking window covers area ouside of the picture,then
   those area will be filled with BLACK.

egi_imgbuf:     an EGI_IMGBUF struct which hold bits_color image data of a picture.
fb_dev:		FB device
subcolor:	substituting color, only applicable when >0.
(xp,yp):        coodinate of the displaying window origin(left top) point, relative to
                the coordinate system of the picture(also origin at left top).
(xw,yw):        displaying window origin, relate to the LCD coord system.
winw,winh:      width and height(row/column for fb) of the displaying window.
                !!! Note: You'd better set winw,winh not exceeds acutual LCD size, or it will
                waste time calling draw_dot() for pixels outsie FB zone.

Return:
		0	OK
		<0	fails
------------------------------------------------------------------------------------------*/
int egi_imgbuf_windisplay( EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
			   		int xp, int yp, int xw, int yw, int winw, int winh)
{
        /* check data */
        if(egi_imgbuf == NULL) {
                printf("%s: egi_imgbuf is NULL. fail to display.\n",__func__);
                return -1;
        }
        if(egi_imgbuf->imgbuf == NULL) {
                printf("%s: egi_imgbuf->imgbuf is NULL. fail to display.\n",__func__);
                return -1;
        }

	/* get mutex lock */
	if(pthread_mutex_lock(&egi_imgbuf->img_mutex) !=0){
		printf("%s: Fail to lock image mutex!\n",__func__);
		return -1;
	}

        int imgw=egi_imgbuf->width;     /* image Width and Height */
        int imgh=egi_imgbuf->height;
        if( imgw<=0 || imgh<=0 )
        {
                printf("%s: egi_imgbuf->width or height is <=0. fail to display.\n",__func__);
		pthread_mutex_unlock(&egi_imgbuf->img_mutex);
                return -2;
        }

        int i,j;
        int xres=fb_dev->vinfo.xres;
        int yres=fb_dev->vinfo.yres;
        long int screen_pixels=xres*yres;

        unsigned char *fbp =fb_dev->map_fb;

        uint16_t *imgbuf = egi_imgbuf->imgbuf;
        unsigned char *alpha=egi_imgbuf->alpha;
        long int locfb=0; /* location of FB mmap, in pxiel, xxxxxbyte */
        long int locimg=0; /* location of image buf, in pixel, xxxxin byte */
//      int bytpp=2; /* bytes per pixel */

  /* if no alpha channle*/
  if( egi_imgbuf->alpha==NULL )
  {
        for(i=0;i<winh;i++) {  /* row of the displaying window */
                for(j=0;j<winw;j++) {

			/* Rule out points which are out of the SCREEN boundary */
			if( i+yw<0 || i+yw>yres-1 || j+xw<0 || j+xw>xres-1 )
				continue;

                        /* FB data location */
                        locfb = (i+yw)*xres+(j+xw);

                     /*  ---- draw only within screen  ---- */
                     if( locfb>=0 && locfb <= (screen_pixels-1) ) {

                        /* check if exceeds IMAGE(not screen) boundary */
                        if( ( xp+j > imgw-1 || xp+j <0 ) || ( yp+i > imgh-1 || yp+i <0 ) )
                        {
//replaced by draw_dot()        *(uint16_t *)(fbp+locfb)=0; /* black for outside */
                                fbset_color(0); /* black for outside */
                                draw_dot(fb_dev,j+xw,i+yw); /* call draw_dot */
                        }
                        else {
                                /* image data location */
                                locimg= (i+yp)*imgw+(j+xp);

				if(subcolor<0) {
	                                fbset_color(imgbuf[locimg]);
				}
				else {  /* use subcolor */
	                                fbset_color((uint16_t)subcolor);
				}

                                draw_dot(fb_dev,j+xw,i+yw); /* call draw_dot */
                         }
                     } /* if within screen */
                }
        }
  }
  else /* with alpha channel */
  {
	//printf("----- alpha ON -----\n");
        for(i=0;i<winh;i++)  { /* row of the displaying window */
                for(j=0;j<winw;j++)  {

			/* Rule out points which are out of the SCREEN boundary */
			if( i+yw<0 || i+yw>yres-1 || j+xw<0 || j+xw>xres-1 )
				continue;

                        /* FB data location, in pixel */
                        locfb = (i+yw)*xres+(j+xw); /*in pixel,  2 bytes per pixel */

                     /*  ---- draw_dot() only within screen  ---- */
                     if(  locfb>=0 && locfb<screen_pixels ) {

                        /* check if exceeds IMAGE(not screen) boundary */
                        if( ( xp+j > imgw-1 || xp+j <0 ) || ( yp+i > imgh-1 || yp+i <0 ) )
                        {
                                fbset_color(0); /* black for outside */
                                draw_dot(fb_dev,j+xw,i+yw); /* call draw_dot */
                        }
                        else {
                                /* image data location, 2 bytes per pixel */
                                locimg= (i+yp)*imgw+(j+xp);

                                if( alpha[locimg]==0 ) {   /* ---- 100% backgroud color ---- */
                                        /* Transparent for background, do nothing */
                                        //fbset_color(*(uint16_t *)(fbp+(locfb<<1)));

				    	/* for Virt FB ???? */
                                }

				else if(subcolor<0) {	/* ---- No subcolor ---- */
#if 0  ///////////////////////////// replaced by fb.pxialpha ///////////////////////
                                     if(alpha[locimg]==255) {    /* 100% front color */
                                          fbset_color(*(uint16_t *)(imgbuf+locimg));
				     }
                                     else {                           /* blend */
                                          fbset_color(
                                              COLOR_16BITS_BLEND( *(uint16_t *)(imgbuf+locimg),   /* front pixel */
                                                            *(uint16_t *)(fbp+(locfb<<1)),  /* background */
                                                             alpha[locimg]  )               /* alpha value */
                                             );
                                     }
#endif  ///////////////////////////////////////////////////////////////////////////
				     fb_dev->pixalpha=alpha[locimg];
				     fbset_color(imgbuf[locimg]);
                                     draw_dot(fb_dev,j+xw,i+yw);
				}

				else  {  		/* ---- use subcolor ----- */
#if 0  ///////////////////////////// replaced by fb.pxialpha ////////////////////////
                                    if(alpha[locimg]==255) {    /* 100% subcolor as front color */
                                          fbset_color(subcolor);
				     }
                                     else {                           /* blend */
                                          fbset_color(
                                              COLOR_16BITS_BLEND( subcolor,   /* subcolor as front pixel */
                                                            *(uint16_t *)(fbp+(locfb<<1)),  /* background */
                                                             alpha[locimg]  )               /* alpha value */
                                             );
                                     }
#endif  //////////////////////////////////////////////////////////////////////////
				     fb_dev->pixalpha=alpha[locimg];
				     fbset_color(subcolor);
                                     draw_dot(fb_dev,j+xw,i+yw);
				}

                            }
                        }  /* if within screen */
                } /* for() */
        }/* for()  */
  }/* end alpha case */

  /* put mutex lock */
  pthread_mutex_unlock(&egi_imgbuf->img_mutex);

  return 0;
}



#if 1 ////////////////////////// TODO: range limit check ///////////////////////////
/*---------------------------------------------------------------------------------------
For 16bits color only!!!!

WARING:
1. Writing directly to FB without calling draw_dot()!!!
   FB_FILO, Virt_FB disabled!!!
2. Take care of image boudary check and locfb check to avoid outrange points skipping
   to next line !!!!
3. No range limit check, which may cause segmentation fault!!!

Note:
1. No subcolor and write directly to FB, so FB FILO is ineffective !!!!!
2. FB.pos_rotate is NOT supported.
3. Write image data of an EGI_IMGBUF to a window in FB.
4. Set outside color as black.
5. window(xw, yw) defines a looking window to the original picture, (xp,yp) is the left_top
   start point of the window. If the looking window covers area ouside of the picture,then
   those area will be filled with BLACK.

egi_imgbuf:     an EGI_IMGBUF struct which hold bits_color image data of a picture.
fb_dev:		FB device
(xp,yp):        coodinate of the displaying window origin(left top) point, relative to
                the coordinate system of the picture(also origin at left top).
(xw,yw):        displaying window origin, relate to the LCD coord system.
winw,winh:      width and height(row/column for fb) of the displaying window.
                !!! Note: You'd better set winw,winh not exceeds acutual LCD size, or it will
                waste time calling draw_dot() for pixels outsie FB zone.

Return:
		0	OK
		<0	fails
------------------------------------------------------------------------------------------*/
int egi_imgbuf_windisplay2(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
			   		int xp, int yp, int xw, int yw, int winw, int winh)
{
        /* check data */
        if(egi_imgbuf == NULL)
        {
                printf("%s: egi_imgbuf is NULL. fail to display.\n",__func__);
                return -1;
        }

	/* get mutex lock */
	if( pthread_mutex_lock(&egi_imgbuf->img_mutex)!=0 ){
		printf("%s: Fail to lock image mutex!\n",__func__);
		return -2;
	}

        int imgw=egi_imgbuf->width;     /* image Width and Height */
        int imgh=egi_imgbuf->height;
        if( imgw<0 || imgh<0 )
        {
                printf("%s: egi_imgbuf->width or height is negative. fail to display.\n",__func__);
		pthread_mutex_unlock(&egi_imgbuf->img_mutex);
                return -3;
        }

        int i,j;
        int xres=fb_dev->vinfo.xres;
        int yres=fb_dev->vinfo.yres;
        long int screen_pixels=xres*yres;

        unsigned char *fbp =fb_dev->map_fb;
        uint16_t *imgbuf = egi_imgbuf->imgbuf;
        unsigned char *alpha=egi_imgbuf->alpha;
        long int locfb=0; /* location of FB mmap, in pxiel, xxxxxbyte */
        long int locimg=0; /* location of image buf, in pixel, xxxxin byte */
//      int bytpp=2; /* bytes per pixel */


  /* if no alpha channle*/
  if( egi_imgbuf->alpha==NULL )
  {
        for(i=0;i<winh;i++) {  /* row of the displaying window */
                for(j=0;j<winw;j++) {

			/* Rule out points which are out of the SCREEN boundary */
			if( i+yw<0 || i+yw>yres-1 || j+xw<0 || j+xw>xres-1 )
				continue;

                        /* FB data location */
                        locfb = (i+yw)*xres+(j+xw);

                        /* check if exceed image boundary */
                        if( ( xp+j > imgw-1 || xp+j <0 ) || ( yp+i > imgh-1 || yp+i <0 ) )
                        {
			        *(uint16_t *)(fbp+locfb)=0; /* black for outside */
                        }
                        else {
                                /* image data location */
                                locimg= (i+yp)*imgw+(j+xp);

			         *(uint16_t *)(fbp+locfb*2)=*(uint16_t *)(imgbuf+locimg);
                            }
                }
        }
  }
  else /* with alpha channel */
  {
        for(i=0;i<winh;i++)  { /* row of the displaying window */
                for(j=0;j<winw;j++)  {

			/* Rule out points which are out of the SCREEN boundary */
			if( i+yw<0 || i+yw>yres-1 || j+xw<0 || j+xw>xres-1 )
				continue;

                        /* FB data location, in pixel */
                        locfb = (i+yw)*xres+(j+xw); /*in pixel,  2 bytes per pixel */

                        if( ( xp+j > imgw-1 || xp+j <0 ) || ( yp+i > imgh-1 || yp+i <0 ) )
                        {
       				*(uint16_t *)(fbp+locfb*2)=0;   /* black */
                        }
                        else {
                            /* image data location, 2 bytes per pixel */
                            locimg= (i+yp)*imgw+(j+xp);

                            /*  ---- draw only within screen  ---- */
                            if( locfb>=0 && locfb <= (screen_pixels-1) ) {

                                if(alpha[locimg]==0) {           /* use backgroud color */
                                        /* Transparent for background, do nothing */
                                        //fbset_color(*(uint16_t *)(fbp+(locfb<<1)));
                                }
                                else if(alpha[locimg]==255) {    /* use front color */
			               *(uint16_t *)(fbp+locfb*2)=*(uint16_t *)(imgbuf+locimg);
				}
                                else {                           /* blend */
                                            *(uint16_t *)(fbp+locfb*2)= COLOR_16BITS_BLEND(
							*(uint16_t *)(imgbuf+locimg),   /* front pixel */
                                                        *(uint16_t *)(fbp+(locfb<<1)),  /* background */
                                                        alpha[locimg]  );               /* alpha value */
				}
                            }
                       } /* if  within screen */
                } /* for() */
        }/* for()  */
  }/* end alpha case */

  /* put mutex lock */
  pthread_mutex_unlock(&egi_imgbuf->img_mutex);

  return 0;
}
#endif ///////////////////////////////////////////////////////////////////////


/*-----------------------------------------------------------------------------------
Write a sub image in the EGI_IMGBUF to FB.

egi_imgbuf:     an EGI_IMGBUF struct which hold bits_color image data of a picture.
fb_dev:		FB device
subindex:		index number of the sub image.
		if subindex<0 or EGI_IMGBOX subimgs==NULL, no sub_image defined in the
		egi_imgbuf.
subcolor:	substituting color, only applicable when >0.
(x0,y0):        displaying window origin, relate to the LCD coord system.

Return:
		0	OK
		<0	fails
-------------------------------------------------------------------------------------*/
int egi_subimg_writeFB(EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subindex,
							int subcolor, int x0,	int y0)
{
	int ret;
	int xp,yp;
	int w,h;

	if(egi_imgbuf==NULL || egi_imgbuf->imgbuf==NULL ) {
		printf("%s: egi_imbug or egi_imgbuf->imgbuf is NULL!\n",__func__);
		return -1;;
	}
	/* get mutex lock */
	if( pthread_mutex_lock(&egi_imgbuf->img_mutex)!=0 ){
		printf("%s: Fail to lock image mutex!\n",__func__);
		return -2;
	}

	if(subindex > egi_imgbuf->submax) {
		printf("%s: EGI_IMGBUF subindex is out of range!\n",__func__);
	  	pthread_mutex_unlock(&egi_imgbuf->img_mutex);
		return -4;
	}

	/* get position and size of the subimage */
	if( subindex<=0 || egi_imgbuf->subimgs==NULL ) {	/* No subimg, only one image */
		xp=0;
		yp=0;
		w=egi_imgbuf->width;
		h=egi_imgbuf->height;
	}
	else {				/* otherwise, get subimg size and location in image data */
		xp=egi_imgbuf->subimgs[subindex].x0;
		yp=egi_imgbuf->subimgs[subindex].y0;
		w=egi_imgbuf->subimgs[subindex].w;
		h=egi_imgbuf->subimgs[subindex].h;
	}

  	/* put mutex lock, before windisplay()!!! */
  	pthread_mutex_unlock(&egi_imgbuf->img_mutex);

	/* !!!! egi_imgbuf_windisplay() will get/put image mutex by itself */
	ret=egi_imgbuf_windisplay(egi_imgbuf, fb_dev, subcolor, xp, yp, x0, y0, w, h); /* with mutex lock */

	return ret;
}


/*-----------------------------------------------------------------------------------
Clear a subimag in an EGI_IMGBUF, reset all color and alpha data.

@egi_imgbuf:    an EGI_IMGBUF struct which hold bits_color image data of a picture.
@subindex:	index number of the sub image.
		if subindex<=0 or EGI_IMGBOX subimgs==NULL, no sub_image defined in the
		egi_imgbuf.
@color:		color value for all pixels in the imgbuf
		if <0, ignored.  meaningful  [16bits]
@alpha:		alpha value for all pixels in the imgbuf
		if <0, ignored.  meaningful [0 255]

Return:
		0	OK
		<0	fails
-------------------------------------------------------------------------------------*/
int egi_imgbuf_reset(EGI_IMGBUF *egi_imgbuf, int subindex, int color, int alpha)
{
	int height, width; /* of host image */
	int hs, ws;	   /* of sub image */
	int xs, ys;
	int i,j;
	unsigned long pos;

	if(egi_imgbuf==NULL || egi_imgbuf->imgbuf==NULL ) {
		printf("%s: egi_imbug or egi_imgbuf->imgbuf is NULL!\n",__func__);
		return -1;;
	}

	/* get mutex lock */
	if( pthread_mutex_lock(&egi_imgbuf->img_mutex)!=0 ){
		printf("%s: Fail to lock image mutex!\n",__func__);
		return -2;
	}

	if(egi_imgbuf->submax < subindex) {
		printf("%s: submax < subindex! \n",__func__);
	  	pthread_mutex_unlock(&egi_imgbuf->img_mutex);
		return -3;
	}

	height=egi_imgbuf->height;
	width=egi_imgbuf->width;

	/* upper limit: color and alpha */
	if(alpha>255)alpha=255;
	if(color>0xffff)color=0xFFFF;

	/* if only 1 image, NO subimg, or RESET whole image data */
	if( egi_imgbuf->submax <= 0 || egi_imgbuf->subimgs==NULL ) {
		for( i=0; i<height*width; i++) {
			/* reset color */
			if(color>=0) {
				egi_imgbuf->imgbuf[i]=color;
			}
			/* reset alpha */
			if(egi_imgbuf->alpha && alpha>=0 ) {
				egi_imgbuf->alpha[i]=alpha;
			}
		}
	}

	/* else, >1 image */
	else if( egi_imgbuf->submax > 0 && egi_imgbuf->subimgs != NULL ) {
		xs=egi_imgbuf->subimgs[subindex].x0;
		ys=egi_imgbuf->subimgs[subindex].y0;
		hs=egi_imgbuf->subimgs[subindex].h;
		ws=egi_imgbuf->subimgs[subindex].w;

		for(i=ys; i<ys+hs; i++) {           	/* transverse subimg Y */
			if(i < 0 ) continue;
			if(i > height-1) break;

			for(j=xs; j<xs+ws; j++) {   	/* transverse subimg X */
				if(j < 0) continue;
				if(j > width -1) break;

				pos=i*width+j;
				/* reset color and alpha */
				if( color >=0 )
					egi_imgbuf->imgbuf[pos]=color;
				if( egi_imgbuf->alpha && alpha >=0 )
					egi_imgbuf->alpha[pos]=alpha;
			}
		}
	}

  	/* put mutex lock */
  	pthread_mutex_unlock(&egi_imgbuf->img_mutex);

	return 0;
}



/*------------------------------------------------------------------------------------
        	   Add FreeType FT_Bitmap to EGI imgbuf

1. The input eimg should has been initialized by egi_imgbuf_init()
   with certain size of a canvas (height x width) inside.

2. The canvas size of the eimg shall be big enough to hold the bitmap,
   or pixels out of the canvas will be omitted.

3.   		!!! -----   WARNING   ----- !!!
   In order to algin all characters in same horizontal level, every char bitmap must be
   align to the same baseline, i.e. the vertical position of each char's origin
   (baseline) MUST be the same.
   So (xb,yb) should NOT be left top coordinate of a char bitmap,
   while use char's 'origin' coordinate relative to eimg canvan as (xb,yb) can align all
   chars at the same level !!!


@eimg		The EGI_IMGBUF to hold the bitmap
@xb,yb		coordinates of bitmap origin relative to EGI_IMGBUF canvas coord.!!!!

@bitmap		pointer to a bitmap in a FT_GlyphSlot.
		typedef struct  FT_Bitmap_
		{
			    unsigned int    rows;
			    unsigned int    width;
			    int             pitch;
			    unsigned char*  buffer;
			    unsigned short  num_grays;
			    unsigned char   pixel_mode;
			    unsigned char   palette_mode;
			    void*           palette;
		} FT_Bitmap;

@subcolor	>=0 as substituting color
		<0  use bitmap buffer data as gray value. 0-255 (BLACK-WHITE)

return:
	0	OK
	<0	fails
--------------------------------------------------------------------------------*/
int egi_imgbuf_blend_FTbitmap(EGI_IMGBUF* eimg, int xb, int yb, FT_Bitmap *bitmap,
								EGI_16BIT_COLOR subcolor)
{
	int i,j;
	EGI_16BIT_COLOR color;
	unsigned char alpha;
	unsigned long size; /* alpha size */
	int	sumalpha;
	int pos;

	if(eimg==NULL || eimg->imgbuf==NULL || eimg->height==0 || eimg->width==0 ) {
		printf("%s: input EGI_IMBUG is NULL or uninitiliazed!\n", __func__);
		return -1;
	}
	if( bitmap==NULL || bitmap->buffer==NULL ) {
		printf("%s: input FT_Bitmap or its buffer is NULL!\n", __func__);
		return -2;
	}
	/* calloc and assign alpha, if NULL */
	if(eimg->alpha==NULL) {
		size=eimg->height*eimg->width;
		eimg->alpha = calloc(1, size); /* alpha value 8bpp */
		if(eimg->alpha==NULL) {
			printf("%s: Fail to calloc eimg->alpha\n",__func__);
			return -3;
		}
		memset(eimg->alpha, 255, size); /* assign to 255 */
	}

	for( i=0; i< bitmap->rows; i++ ) {	      /* traverse bitmap height  */
		for( j=0; j< bitmap->width; j++ ) {   /* traverse bitmap width */
			/* check range limit */
			if( yb+i <0 || yb+i >= eimg->height ||
				    xb+j <0 || xb+j >= eimg->width )
				continue;

			/* buffer value(0-255) deemed as gray value OR alpha value */
			alpha=bitmap->buffer[i*bitmap->width+j];

			pos=(yb+i)*(eimg->width) + xb+j; /* eimg->imgbuf position */

			/* blend color	*/
			if( subcolor>=0 ) {	/* use subcolor */
				//color=subcolor;
				/* !!!WARNG!!!  NO Gamma Correctio in color blend macro,
				 * color blend will cause some unexpected gray
				 * areas/lines, especially for two contrasting colors.
				 * Select a suitable backgroud color to weaken this effect.
				 */
				if(alpha>180)alpha=255;  /* set a limit as for GAMMA correction, too simple! */
				color=COLOR_16BITS_BLEND( subcolor, eimg->imgbuf[pos], alpha );
							/* front, background, alpha */
			}
			else {			/* use Font bitmap gray value */
				/* alpha=0 MUST keep unchanged! */
				if(alpha>0 && alpha<180)alpha=255; /* set a limit as for GAMMA correction, too simple! */
				color=COLOR_16BITS_BLEND( COLOR_RGB_TO16BITS(alpha,alpha,alpha),
							  eimg->imgbuf[pos], alpha );
			}
			eimg->imgbuf[pos]=color; /* assign color to imgbuf */

			/* blend alpha value */
			sumalpha=eimg->alpha[pos]+alpha;
			if( sumalpha > 255 ) sumalpha=255;
			eimg->alpha[pos]=sumalpha; //(alpha>0 ? 255:0); //alpha;
		}
	}

	return 0;
}

