#include "egi_image.h"
#include "egi_bjp.h"


/*---------------------------------------
Release data in a static EGI_IMGBUF
----------------------------------------*/
void egi_imgbuf_release(EGI_IMGBUF *egi_imgbuf)
{
        if(egi_imgbuf == NULL)
                return;

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
        egi_imgbuf->height=0;
        egi_imgbuf->width=0;
}

/*--------------------------------------------
   	   Free a EGI_IMGBUF
---------------------------------------------*/
void egi_imgbuf_free(EGI_IMGBUF *egi_imgbuf)
{
	if(egi_imgbuf) {
		egi_imgbuf_release(egi_imgbuf);
		free(egi_imgbuf);
	}
	egi_imgbuf=NULL;
}

/*--------------------------------------------
   	   Allocate  a EGI_IMGBUF
---------------------------------------------*/
EGI_IMGBUF *egi_imgbuf_new(void)
{
	EGI_IMGBUF *eimg;
	eimg=calloc(1, sizeof(EGI_IMGBUF));
	if(eimg==NULL) {
		printf("%s: Fail to calloc EGI_IMGBUF.\n",__func__);
		return NULL;
	}
	return eimg;
}


/*-------------------------------------------------------------------------------
For 16bits color only!!!!

Note: draw_dot() for FILO, or write directly to FB.

1. Write image data of an EGI_IMGBUF to a window in FB.
2. Set outside color as black.
3. window(xw, yw) defines a looking window to the original picture, (xp,yp) is the left_top
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
int egi_imgbuf_windisplay(const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subcolor,
			   		int xp, int yp, int xw, int yw, int winw, int winh)
{
        /* check data */
        if(egi_imgbuf == NULL)
        {
                printf("%s: egi_imgbuf is NULL. fail to display.\n",__func__);
                return -1;
        }
        int imgw=egi_imgbuf->width;     /* image Width and Height */
        int imgh=egi_imgbuf->height;
        if( imgw<0 || imgh<0 )
        {
                printf("%s: egi_imgbuf->width or height is negative. fail to display.\n",__func__);
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
                        /* FB data location */
                        locfb = (i+yw)*xres+(j+xw);
                        /* check if exceed image boundary */
                        if( ( xp+j > imgw-1 || xp+j <0 ) || ( yp+i > imgh-1 || yp+i <0 ) )
                        {
//replaced by draw_dot()        *(uint16_t *)(fbp+locfb)=0; /* black for outside */
                                fbset_color(0); /* black for outside */
                                draw_dot(fb_dev,j+xw,i+yw); /* call draw_dot */
                        }
                        else {
                                /* image data location */
                                locimg= (i+yp)*imgw+(j+xp);

                                /*  FB from EGI_IMGBUF */
//replaced by draw_dor()        *(uint16_t *)(fbp+locfb)=*(uint16_t *)(imgbuf+locimg/bytpp);

                             /*  ---- draw_dot(), only within screen  ---- */
                             if( locfb <= (screen_pixels-1) ) {

				if(subcolor<0) {
	                                fbset_color(*(uint16_t *)(imgbuf+locimg));
				}
				else {  /* use subcolor */
	                                fbset_color((uint16_t)subcolor);
				}

                                draw_dot(fb_dev,j+xw,i+yw); /* call draw_dot */
                            }
                        }
                }
        }
  }
  else /* with alpha channel */
  {
	printf("----- alpha ON -----\n");
        for(i=0;i<winh;i++)  { /* row of the displaying window */
                for(j=0;j<winw;j++)  {
                        /* FB data location, in pixel */
                        locfb = (i+yw)*xres+(j+xw); /*in pixel,  2 bytes per pixel */
                        /* check if exceed image boundary */
                        if( ( xp+j > imgw-1 || xp+j <0 ) || ( yp+i > imgh-1 || yp+i <0 ) )
                        {
                                fbset_color(0); /* black for outside */
                                draw_dot(fb_dev,j+xw,i+yw); /* call draw_dot */
                        }
                        else {
                                /* image data location, 2 bytes per pixel */
                                locimg= (i+yp)*imgw+(j+xp);
                                /*  FB from EGI_IMGBUF */
//replaced by draw_dor()        *(uint16_t *)(fbp+locfb)=*(uint16_t *)(imgbuf+locimg/bytpp);

                            /*  ---- draw_dot() only within screen  ---- */
                            if(  locfb>=0 && locfb<screen_pixels ) {

                                if(alpha[locimg]==0) {   /* ---- 100% backgroud color ---- */
                                        /* Transparent for background, do nothing */
                                        //fbset_color(*(uint16_t *)(fbp+(locfb<<1)));
                                }
				else if(subcolor<0) {	/* ---- No subcolor ---- */
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
                                     draw_dot(fb_dev,j+xw,i+yw);

				}
				else  {  		/* ---- use subcolor ----- */
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
                                     draw_dot(fb_dev,j+xw,i+yw);
				}

                            }
                        }
                } /* for() */
        }/* for()  */
  }/* end alpha case */

  return 0;
}




/*------------------------------------------------------------------------------------------
For 16bits color only!!!!

Note: No subcolor and write directly to FB

1. Write image data of an EGI_IMGBUF to a window in FB.
2. Set outside color as black.
3. window(xw, yw) defines a looking window to the original picture, (xp,yp) is the left_top
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
int egi_imgbuf_windisplay2(const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev,
			   		int xp, int yp, int xw, int yw, int winw, int winh)
{
        /* check data */
        if(egi_imgbuf == NULL)
        {
                printf("%s: egi_imgbuf is NULL. fail to display.\n",__func__);
                return -1;
        }
        int imgw=egi_imgbuf->width;     /* image Width and Height */
        int imgh=egi_imgbuf->height;
        if( imgw<0 || imgh<0 )
        {
                printf("%s: egi_imgbuf->width or height is negative. fail to display.\n",__func__);
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

                                /*  only within FB  */
                             if( locfb <= (screen_pixels-1) ) {
			         *(uint16_t *)(fbp+locfb*2)=*(uint16_t *)(imgbuf+locimg);
                            }
                        }
                }
        }
  }
  else /* with alpha channel */
  {
        for(i=0;i<winh;i++)  { /* row of the displaying window */
                for(j=0;j<winw;j++)  {
                        /* FB data location, in pixel */
                        locfb = (i+yw)*xres+(j+xw); /*in pixel,  2 bytes per pixel */
                        /* check if exceed image boundary */
                        if( ( xp+j > imgw-1 || xp+j <0 ) || ( yp+i > imgh-1 || yp+i <0 ) )
                        {
       				*(uint16_t *)(fbp+locfb*2)=0;
                        }
                        else {
                            /* image data location, 2 bytes per pixel */
                            locimg= (i+yp)*imgw+(j+xp);

                            /*  ---- draw only within screen  ---- */
                            if( locfb <= (screen_pixels-1) ) {

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
                       }
                } /* for() */
        }/* for()  */
  }/* end alpha case */

  return 0;
}


/*-----------------------------------------------------------------------------------
Write a sub image in the EGI_IMGBUF to FB.

egi_imgbuf:     an EGI_IMGBUF struct which hold bits_color image data of a picture.
fb_dev:		FB device
subnum:		number of the sub image
subcolor:	substituting color, only applicable when >0.
(x0,y0):        displaying window origin, relate to the LCD coord system.

Return:
		0	OK
		<0	fails
-------------------------------------------------------------------------------------*/
int egi_subimg_writeFB(const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subnum,
							int subcolor, int x0,	int y0)
{
	int ret;
	int xp,yp;
	int w,h;

	if(egi_imgbuf==NULL || egi_imgbuf->subimgs==NULL) {
		printf("%s: EGI_IMGBUF is invalid!\n",__func__);
		return -1;;
	}
	if(egi_imgbuf->subtotal-1 < subnum ) {
		printf("%s: EGI_IMGBUF subnum is out of range!\n",__func__);
		return -2;
	}

	xp=egi_imgbuf->subimgs[subnum].x0;
	yp=egi_imgbuf->subimgs[subnum].y0;
	w=egi_imgbuf->subimgs[subnum].w;
	h=egi_imgbuf->subimgs[subnum].h;

	ret=egi_imgbuf_windisplay(egi_imgbuf, fb_dev, subcolor, xp, yp, x0, y0, w, h);

	return ret;
}
