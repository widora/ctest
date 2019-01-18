/*------------------------------------------------------------------------------
Referring to: http://blog.chinaunix.net/uid-22666248-id-285417.html

 本文的copyright归yuweixian4230@163.com 所有，使用GPL发布，可以自由拷贝，转载。
但转载请保持文档的完整性，注明原作者及原链接，严禁用于任何商业用途。

作者：yuweixian4230@163.com
博客：yuweixian4230.blog.chinaunix.net


TODO:
0.  draw_dot(): check FB mem. boundary during writing. ---OK


Modified by Midas-Zhou

-----------------------------------------------------------------------------*/
#include "egi_fbgeom.h"
#include "egi.h"
#include "egi_debug.h"
#include <unistd.h>
#include <string.h> /*memset*/
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <math.h>
#include <stdlib.h>


#define MATH_PI 3.1415926535897932

#ifndef _TYPE_FBDEV_
#define _TYPE_FBDEV_
    typedef struct fbdev{
        int fdfd; //open "dev/fb0"
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        long int screensize;
        char *map_fb;
    }FBDEV;
#endif


/* global variale, Frame buffer device */
FBDEV  gv_fb_dev;


 /* default color set */
 static uint16_t fb_color=(30<<11)|(10<<5)|10;  //r(5)g(6)b(5)

 void init_dev(FBDEV *dev)
 {
        FBDEV *fr_dev=dev;

        fr_dev->fdfd=open("/dev/fb0",O_RDWR);
//        printf("the framebuffer device was opended successfully.\n");
        ioctl(fr_dev->fdfd,FBIOGET_FSCREENINFO,&(fr_dev->finfo)); //获取 固定参数
        ioctl(fr_dev->fdfd,FBIOGET_VSCREENINFO,&(fr_dev->vinfo)); //获取可变参数
        fr_dev->screensize=fr_dev->vinfo.xres*fr_dev->vinfo.yres*fr_dev->vinfo.bits_per_pixel/8;
        fr_dev->map_fb=(char *)mmap(NULL,fr_dev->screensize,PROT_READ|PROT_WRITE,MAP_SHARED,fr_dev->fdfd,0);
//        printf("init_dev successfully. fr_dev->map_fb=%p\n",fr_dev->map_fb);

	printf(" \n------- FB Parameters -------:\n");
	printf(" bits_per_pixel: %d bits \n",fr_dev->vinfo.bits_per_pixel);
	printf(" line_length: %d bytes\n",fr_dev->finfo.line_length);
	printf(" xres: %d pixels, yres: %d pixels \n", fr_dev->vinfo.xres, fr_dev->vinfo.yres);
	printf(" xoffset: %d,  yoffset: %d \n", fr_dev->vinfo.xoffset, fr_dev->vinfo.yoffset);
	printf(" screensize: %ld bytes\n", fr_dev->screensize);
	printf(" ----------------------------\n\n");
  }

    void release_dev(FBDEV *dev)
    {
	if(!dev->map_fb)return;

	munmap(dev->map_fb,dev->screensize);
	close(dev->fdfd);
    }


  /*------------------------------------
  check if (px,py) in box(x1,y1,x2,y2)
  return true or false
  -------------------------------------*/
  bool point_inbox(int px,int py, int x1, int y1,int x2, int y2)
  {
      int xl,xh,yl,yh;

	if(x1>=x2){
		xh=x1;xl=x2;
	}
	else {
		xl=x1;xh=x2;
	}

	if(y1>=y2){
		yh=y1;yl=y2;
	}
	else {
		yh=y2;yl=y1;
	}

	if( (px>=xl && px<=xh) && (py>=yl && py<=yh))
		return true;
	else
		return false;
  }


    /*  set color for every dot */
    void fbset_color(uint16_t color)
    {
	fb_color=color;
    }

    /* -------------------------------------------
      clear screen with given color

      BUG:
		!!!!call egi_colorxxxx_randmon() error
     --------------------------------------------*/
    void clear_screen(FBDEV *dev, uint16_t color)
    {
	int i,j;
	FBDEV *fr_dev=dev;
	int xres=dev->vinfo.xres;
	int yres=dev->vinfo.yres;
	int bytes_per_pixel=fr_dev->vinfo.bits_per_pixel/8;
	long int location=0;

	for(location=0; location < (fr_dev->screensize/bytes_per_pixel); location++)
	        *((unsigned short int *)(fr_dev->map_fb+location*bytes_per_pixel))=color;//color;

/*   ---------------following is more accurate!?!?  ///
	for(i=0;i<yres;i++)
	{
		for(j=0;j<xres;j++)
		{

		        location=(j+fr_dev->vinfo.xoffset)*(fr_dev->vinfo.bits_per_pixel/8)+
                	     (i+fr_dev->vinfo.yoffset)*fr_dev->finfo.line_length;
       			 *((unsigned short int *)(fr_dev->map_fb+location))=fb_color;
		}
	}

*/

    }

    /*-----------------------------------------------------

    Return:
	0	OK
	-1	get out of FB mem.(ifndef FB_DOTOUT_ROLLBACK)
    --------------------------------------------------------*/
    int draw_dot(FBDEV *dev,int x,int y) //(x.y) 是坐标
    {
        FBDEV *fr_dev=dev;
	int fx=x;
	int fy=y;
        long int location=0;
	int xres=fr_dev->vinfo.xres;
	int yres=fr_dev->vinfo.yres;

#ifdef FB_DOTOUT_ROLLBACK
	/* map to LCD(X,Y) */
	if(fx>xres-1)
		fx=fx%xres;
	else if(fx<0)
	{
		fx=xres-(-fx)%xres;
		fx=fx%xres; /* here fx=1-240 */
	}
	if( fy > yres-1) {
		fy=fy%yres;
	}
	else if(fy<0) {
		fy=yres-(-fy)%yres;
		fy=fy%yres; /* here fy=1-320 */
	}
#else /* NO ROLLBACK */
	/* ignore out_ranged points */
	if( fx>(xres-1) || fx<0) {
		return -1;
	}
	if( fy>(yres-1) || fy<0 ) {
		return -1;
	}
#endif

        location=(fx+fr_dev->vinfo.xoffset)*(fr_dev->vinfo.bits_per_pixel/8)+
                     (fy+fr_dev->vinfo.yoffset)*fr_dev->finfo.line_length;

	/* NOT necessary ???  check if no space left for a 16bit_pixel in FB mem */
	if( location<0 || location > (fr_dev->screensize-sizeof(uint16_t)) )
	{
		printf("WARNING: point location out of fb mem.!\n");
		return -1;
	}

        *((unsigned short int *)(fr_dev->map_fb+location))=fb_color;

	return 0;
    }


#if 0   /*------ OBSOLETE, seems the same effect as above !! --------*/
    void draw_dot(FBDEV *dev,int x,int y) //(x.y) 是坐标
    {
        FBDEV *fr_dev=dev;
        int *xx=&x;
        int *yy=&y;
        long int location=0;

        location=(*xx+fr_dev->vinfo.xoffset)*(fr_dev->vinfo.bits_per_pixel/8)+
                     (*yy+fr_dev->vinfo.yoffset)*fr_dev->finfo.line_length;

        *((unsigned short int *)(fr_dev->map_fb+location))=fb_color;
    }
#endif



    void draw_line(FBDEV *dev,int x1,int y1,int x2,int y2) 
    {
        FBDEV *fr_dev=dev;
        int *xx1=&x1;
        int *yy1=&y1;
        int *xx2=&x2;
        int *yy2=&y2;

        int i=0;
        int j=0;
        int tekxx=*xx2-*xx1;
        int tekyy=*yy2-*yy1;


        //if((*xx2>=*xx1)&&(*yy2>=*yy1))
        if(*xx2>*xx1)
        {
            for(i=*xx1;i<=*xx2;i++)
            {
                j=(i-*xx1)*tekyy/tekxx+*yy1;
                draw_dot(fr_dev,i,j);
            }
        }
	else if(*xx2 == *xx1)
	{
	   if(*yy2>=*yy1)
	   {
		for(i=*yy1;i<=*yy2;i++)
		   draw_dot(fr_dev,*xx1,i);
	    }
	    else //yy2<yy1
	   {
		for(i=*yy2;i<=*yy1;i++)
			draw_dot(fr_dev,*xx1,i);
	   }
	}
        else
        {
            //if(*xx2<*xx1)
            for(i=*xx2;i<=*xx1;i++)
            {
                j=(i-*xx2)*tekyy/tekxx+*yy2;
                draw_dot(fr_dev,i,j);
            }
        }


    }


    /*------------------------------------------------
	           draw an oval
    -------------------------------------------------*/
    void draw_oval(FBDEV *dev,int x,int y) //(x.y) 是坐标
    {
        FBDEV *fr_dev=dev;
        int *xx=&x;
        int *yy=&y;

	draw_line(fr_dev,*xx,*yy-3,*xx,*yy+3);
	draw_line(fr_dev,*xx-1,*yy-2,*xx-1,*yy+2);
	draw_line(fr_dev,*xx-2,*yy-1,*xx-2,*yy+1);
	draw_line(fr_dev,*xx+1,*yy-2,*xx+1,*yy+2);
	draw_line(fr_dev,*xx+2,*yy-2,*xx+2,*yy+2);
    }


    void draw_rect(FBDEV *dev,int x1,int y1,int x2,int y2)
    {
        FBDEV *fr_dev=dev;

	draw_line(fr_dev,x1,y1,x1,y2);
	draw_line(fr_dev,x1,y2,x2,y2);
	draw_line(fr_dev,x2,y2,x2,y1);
	draw_line(fr_dev,x2,y1,x1,y1);
    }


    /*-------------------------------------------------------------
    Draw a filled rectangle defined by two end points of its
    diagonal line. Both points are also part of the rectangle.

    Return:
		0	OK
		//ignore -1	point out of FB mem
    Midas
    ------------------------------------------------------------*/
    int draw_filled_rect(FBDEV *dev,int x1,int y1,int x2,int y2)
    {
	int xr,xl,yu,yd;
	int i,j;

        /* sort point coordinates */
        if(x1>x2)
        {
                xr=x1;
		xl=x2;
        }
        else
	{
                xr=x2;
		xl=x1;
	}

        if(y1>y2)
        {
                yu=y1;
		yd=y2;
        }
        else
	{
                yu=y2;
		yd=y1;
	}

	for(i=yd;i<=yu;i++)
	{
		for(j=xl;j<=xr;j++)
		{
                	draw_dot(dev,j,i); /* ignore range check */
		}
	}

	return 0;
    }



    /*------------------------------------------------
    draw a circle,
	(x,y)	circle center
	r	radius
    Midas
    -------------------------------------------------*/
    void draw_circle(FBDEV *dev, int x, int y, int r)
    {
	int i;
	int s;

	for(i=0;i<r;i++)
	{
		s=sqrt(r*r*1.0-i*i*1.0);
		if(i==0)s-=1; /* erase four tips */
		draw_dot(dev,x-s,y+i);
		draw_dot(dev,x+s,y+i);
		draw_dot(dev,x-s,y-i);
		draw_dot(dev,x+s,y-i);
		/* flip X-Y */
		draw_dot(dev,x-i,y+s);
		draw_dot(dev,x+i,y+s);
		draw_dot(dev,x-i,y-s);
		draw_dot(dev,x+i,y-s);
	}
    }

    /*------------------------------------------------
    draw a filled circle
    Midas
    -------------------------------------------------*/
    void draw_filled_circle(FBDEV *dev, int x, int y, int r)
    {
	int i;
	int s;

	for(i=0;i<r;i++)
	{
		s=sqrt(r*r-i*i);
		if(i==0)s-=1; /* erase four tips */
		draw_line(dev,x-s,y+i,x+s,y+i);
		draw_line(dev,x-s,y-i,x+s,y-i);
	}

    }


   /*----------------------------------------------------------------------------
   copy a block of FB memory  to buffer
   x1,y1,x2,y2:	  LCD area corresponding to FB mem. block
   buf:		  data dest.

   Return
		1	partial area out of FB mem boundary
		0	OK
		-1	fails
   Midas
   ----------------------------------------------------------------------------*/
   int fb_cpyto_buf(FBDEV *fb_dev, int x1, int y1, int x2, int y2, uint16_t *buf)
   {
	int i,j;
	int xl,xr; /* left right */
	int yu,yd; /* up down */
	int ret=0;
	long int location=0;
	int xres=fb_dev->vinfo.xres;
	int yres=fb_dev->vinfo.yres;
	int tmpx,tmpy;

	/* check buf */
	if(buf==NULL)
	{
		printf("fb_cpyto_buf(): buf is NULL!\n");
		return -1;
	}

	/* sort point coordinates */
	if(x1>x2){
		xr=x1;
		xl=x2;
	}
	else{
		xr=x2;
		xl=x1;
	}
	if(y1>y2){
		yu=y1;
		yd=y2;
	}
	else{
		yu=y2;
		yd=y1;
	}


	/* ---------  copy mem --------- */
	for(i=yd;i<=yu;i++)
	{
        	for(j=xl;j<=xr;j++)
		{

#ifdef FB_DOTOUT_ROLLBACK /* -------------   ROLLBACK  ------------------*/
			/* map i,j to LCD(Y,X) */
			if(i<0) /* map Y */
			{
				tmpy=yres-(-i)%yres; /* here tmpy=1-320 */
				tmpy=tmpy%yres;
			}
			else if(i > yres-1)
				tmpy=i%yres;
			else
				tmpy=i;

			if(j<0) /* map X */
			{
				tmpx=xres-(-j)%xres; /* here tmpx=1-240 */ 
				tmpx=tmpx%xres;
			}
			else if(j > xres-1)
				tmpx=j%xres;
			else
				tmpx=j;

			location=(tmpx+fb_dev->vinfo.xoffset)*(fb_dev->vinfo.bits_per_pixel/8)+
        	        	     (tmpy+fb_dev->vinfo.yoffset)*fb_dev->finfo.line_length;


#else  /* -----------------  NO ROLLBACK  ------------------------*/
			if( i<0 || j<0 || i>yres-1 || j>xres-1 )
			{
				egi_pdebug(DBG_FBGEOM,"WARNING: fb_cpyfrom_buf(): coordinates out of range!\n");
				ret=1;
			}
			/* map i,j to LCD(Y,X) */
			if(i>yres-1) /* map Y */
				tmpy=yres-1;
			else if(i<0)
				tmpy=0;
			else
				tmpy=i;

			if(j>xres-1) /* map X */
				tmpx=xres-1;
			else if(j<0)
				tmpx=0;
			else
				tmpx=j;

			location=(tmpx+fb_dev->vinfo.xoffset)*(fb_dev->vinfo.bits_per_pixel/8)+
        	        	     (tmpy+fb_dev->vinfo.yoffset)*fb_dev->finfo.line_length;

#endif
			/* copy to buf */
        		*buf = *((uint16_t *)(fb_dev->map_fb+location));
			 buf++;
		}
	}
//	printf(" fb_cpyfrom_buf = %d\n", ret);

	return ret;
   }



 /*----------------------------------------------------------------------------
   copy a block of buffer to FB memory.
   x1,y1,x2,y2:	  LCD area corresponding to FB mem. block
   buf:		  data source

   Return
		1	partial area out of FB mem boundary
		0	OK
		-1	fails
   Midas
   ----------------------------------------------------------------------------*/
   int fb_cpyfrom_buf(FBDEV *fb_dev, int x1, int y1, int x2, int y2, uint16_t *buf)
   {
	int i,j;
	int xl,xr; /* left right */
	int yu,yd; /* up down */
	int ret=0;
	long int location=0;
	int xres=fb_dev->vinfo.xres;
	int yres=fb_dev->vinfo.yres;
	int tmpx,tmpy;

	/* check buf */
	if(buf==NULL)
	{
		printf("fb_cpyfrom_buf(): buf is NULL!\n");
		return -1;
	}

	/* sort point coordinates */
	if(x1>x2){
		xr=x1;
		xl=x2;
	}
	else{
		xr=x2;
		xl=x1;
	}

	if(y1>y2){
		yu=y1;
		yd=y2;
	}
	else{
		yu=y2;
		yd=y1;
	}


	/* ------------ copy mem ------------*/
	for(i=yd;i<=yu;i++)
	{
        	for(j=xl;j<=xr;j++)
		{
#ifdef FB_DOTOUT_ROLLBACK /* -------------   ROLLBACK  ------------------*/
			/* map i,j to LCD(Y,X) */
			if(i<0) /* map Y */
			{
				tmpy=yres-(-i)%yres; /* here tmpy=1-320 */
				tmpy=tmpy%yres;
			}
			else if(i>yres-1)
				tmpy=i%yres;
			else
				tmpy=i;

			if(j<0) /* map X */
			{
				tmpx=xres-(-j)%xres; /* here tmpx=1-240 */
				tmpx=tmpx%xres;
			}
			else if(j>xres-1)
				tmpx=j%xres;
			else
				tmpx=j;


			location=(tmpx+fb_dev->vinfo.xoffset)*(fb_dev->vinfo.bits_per_pixel/8)+
        	        	     (tmpy+fb_dev->vinfo.yoffset)*fb_dev->finfo.line_length;

#else  /* -----------------  NO ROLLBACK  ------------------------*/
			if( i<0 || j<0 || i>yres-1 || j>xres-1 )
			{
				egi_pdebug(DBG_FBGEOM,"WARNING: fb_cpyfrom_buf(): coordinates out of range!\n");
				ret=1;
			}
			/* map i,j to LCD(Y,X) */
			if(i>yres-1) /* map Y */
				tmpy=yres-1;
			else if(i<0)
				tmpy=0;
			else
				tmpy=i;

			if(j>xres-1) /* map X */
				tmpx=xres-1;
			else if(j<0)
				tmpx=0;
			else
				tmpx=j;

			location=(tmpx+fb_dev->vinfo.xoffset)*(fb_dev->vinfo.bits_per_pixel/8)+
        	        	     (tmpy+fb_dev->vinfo.yoffset)*fb_dev->finfo.line_length;
#endif
			/* --- copy to fb ---*/
        		*((uint16_t *)(fb_dev->map_fb+location))=*buf;
			buf++;
		}
	}

	return ret;
   }


/*---------------------------------------------------------------------------------------
1. rotation center is the center of the square area.
2. The square side length must be in form of 2n+1, so the center is just at the middle of
   the symmetric axis.
3. Method 1 (BUG!!!!): from input coordinates (x,y) to get rotated coordinate(i,j) matrix,
   because of calculation precision limit, this is NOT a one to one mapping!!!  there are
   points that may map to the same point!!! and also there are points in result matrix that
   NOT have been mapped,they keep original coordinates.
4. Method 2: from rotated coordinate (i,j) to get input coordinates (x,y), this can ensure
   that the result points mastrix all be mapped.

n: 		pixel number for square side.
angle: 		rotation angle. clockwise is positive.
centxy 		the center point coordinate of the concerning square area of LCD(fb).
SQMat_XRYR: 	after rotation
	      square matrix after rotation mapping, coordinate origin is same as LCD(fb) origin.


Midas Zhou
-------------------------------------------------------------------------------------------*/
/*------------------------------- Method 1 --------------------------------------------*/

#if 0
void mat_pointrotate_SQMap(int n, double angle, struct egi_point_coord centxy,
                       					 struct egi_point_coord *SQMat_XRYR)
{
	int i,j;
	double sinang,cosang;
	double xr,yr;
	struct egi_point_coord  x0y0; /* the left top point of the square */
	double pi=3.1415926535897932;


	/* check if n can be resolved in form of 2*m+1 */
	if( (n-1)%2 != 0)
	{
		printf("mat_pointrotate_SQMap(): the number of pixels on the square side must be n=2*m+1.\n");
	 	return;
	}

/* 1. generate matrix of point_coordinates for the concerning square area, origin LCD(fb) */
	/* get left top coordinate */
	x0y0.x=centxy.x-((n-1)>>1);
	x0y0.y=centxy.y-((n-1)>>1);
	/* */
	for(i=0;i<n;i++) /* row index,Y */
	{
		for(j=0;j<n;j++) /* column index,X */
		{
			SQMat_XRYR[i*n+j].x=x0y0.x+j;
			SQMat_XRYR[i*n+j].y=x0y0.y+i;
			//printf("LCD origin: SQMat_XY[%d]=(%d,%d)\n",i*n+j,SQMat_XY[i*n+j].x, SQMat_XY[i*n+j].y);
		}
	}

/* 2. transform coordinate origin from LCD(fb) origin to the center of the square*/
	for(i=0;i<n*n;i++)
	{
		SQMat_XRYR[i].x -= x0y0.x+((n-1)>>1);
		SQMat_XRYR[i].y -= x0y0.y+((n-1)>>1);
	}

/* 3. map coordinates of all points to a new matrix by rotation transformation */
	sinang=sin(angle/180.0*pi);//MATH_PI);
	cosang=cos(angle/180.0*pi);//MATH_PI);
	//printf("sinang=%f,cosang=%f\n",sinang,cosang);

	for(i=0;i<n*n;i++)
	{
		/* point rotation transformation ..... */
		xr=(SQMat_XRYR[i].x)*cosang+(SQMat_XRYR[i].y)*sinang;
		yr=(SQMat_XRYR[i].x)*(-sinang)+(SQMat_XRYR[i].y)*cosang;
		//printf("i=%d,xr=%f,yr=%f,\n",i,xr,yr);

		/* check if new piont coordinate is within the square */
		if(  (xr >= -((n-1)/2)) && ( xr <= ((n-1)/2))
					 && (yr >= -((n-1)/2)) && (yr <= ((n-1)/2))  )
		{
			SQMat_XRYR[i].x=nearbyint(xr);//round(xr);
			SQMat_XRYR[i].y=nearbyint(yr);//round(yr);
		/*
			printf("JUST IN RANGE: xr=%f,yr=%f , origin point (%d,%d), new point (%d,%d)\n",
				xr,yr,SQMat_XY[i].x,SQMat_XY[i].y,SQMat_XRYR[i].x, SQMat_XRYR[i].y);
		*/
		}
		/*else:	just keep old coordinate */
		else
		{
			//printf("NOT IN RANGE: xr=%f,yr=%f , origin point (%d,%d)\n",xr,yr,SQMat_XY[i].x,SQMat_XY[i].y);
		}
	}


/* 4. transform coordinate origin back to X0Y0  */
	for(i=0;i<n*n;i++)
	{
                SQMat_XRYR[i].x += (n-1)>>1;
                SQMat_XRYR[i].y += (n-1)>>1;
		//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);
	}
	//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);
}
#endif

#if 1
/*----------------------- Method 2: revert rotation  -------------------------*/
void mat_pointrotate_SQMap(int n, double angle, struct egi_point_coord centxy,
                       					 struct egi_point_coord *SQMat_XRYR)
{
	int i,j;
	double sinang,cosang;
	double xr,yr;

	sinang=sin(1.0*angle/180.0*MATH_PI);
	cosang=cos(1.0*angle/180.0*MATH_PI);

	/* malloc a tmp Matrix */
	//Mat_tmp=malloc(n*n*sizeof(struct egi_point_coord));
	memset(SQMat_XRYR,0,n*n*sizeof(struct egi_point_coord));

	/* check if n can be resolved in form of 2*m+1 */
	if( (n-1)%2 != 0)
	{
		printf("mat_pointrotate_SQMap(): the number of pixels on the square side must be n=2*m+1.\n");
	 	return;
	}

/* 1. generate matrix of point_coordinates for the square, result Matrix is centered at square center. */
	/* */
	for(i=-n/2;i<=n/2;i++) /* row index,Y */
	{
		for(j=-n/2;j<=n/2;j++) /* column index,X */
		{
			/*   get XRYR revert rotation matrix centered at SQMat_XRYU's center
			this way can ensure all SQMat_XRYR[] points are filled!!!  */

			xr = j*cosang+i*sinang;
			yr = -j*sinang+i*cosang;

                	/* check if new piont coordinate is within the square */
                	if(  ( xr >= -((n-1)>>1)) && ( xr <= ((n-1)>>1))
                                         && ( yr >= -((n-1)>>1)) && ( yr <= ((n-1)>>1))  )
			{
				SQMat_XRYR[((2*i+n)/2)*n+(2*j+n)/2].x= round(xr);
				SQMat_XRYR[((2*i+n)/2)*n+(2*j+n)/2].y= round(yr);
			}
		}
	}


/* 2. transform coordinate origin back to X0Y0  */
	for(i=0;i<n*n;i++)
	{
//		if( SQMat_XRYR[i].x == 0 )
//			printf(" ---- SQMat_XRYR[%d].x == %d\n",i,SQMat_XRYR[i].x );

                SQMat_XRYR[i].x += ((n-1)>>1); //+centxy.x);
                SQMat_XRYR[i].y += ((n-1)>>1); //+centxy.y);
		//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);
	}
	//printf("FINAL: SQMat_XRYR[%d]=(%d,%d)\n",i, SQMat_XRYR[i].x, SQMat_XRYR[i].y);



//	free(Mat_tmp);
}
#endif



/*--------------------------------------- Method 1 -------------------------------------------
1. draw an image through a map of rotation.
2.

n:		side pixel number of a square image. to be inform of 2*m+1;
x0y0:		left top coordinate of the square.
image:		16bit color buf of a square image
SQMat_XRYR:	Rotation map matrix of the square.
 		point(i,j) map to egi_point_coord SQMat_XRYR[n*i+j]
Midas
------------------------------------------------------------------------------------------*/
/*----------------------- Drawing for method 1 : revert rotation  -------------------------*/
#if 0
void fb_drawimg_SQMap(int n, struct egi_point_coord x0y0, uint16_t *image,
						const struct egi_point_coord *SQMat_XRYR)
{
	int i,j,k,m;
	int s;
//	int mapx,mapy;
	uint16_t color;

	/* check if n can be resolved in form of 2*m+1 */
	if( (n-1)%2 != 0)
	{
		printf("fb_drawimg_SQMap(): the number of pixels on the square side must be n=2*m+1.\n");
	 	return;
	}

	/* map each image point index k to get rotated position index m,
	   then draw the color to index m position
	   only sort out points within the circle */
	for(i=-n/2-1;i<n/2;i++) /* row index,from top to bottom, -1 to compensate precision loss */
	{
		s=sqrt(n*n/4-i*i);
		for(j=n/2+1-s;j<=n/2+s;j++) /* +1 to compensate precision loss */
		{
			k=(i+n/2)*n+j;
			/*  for current point index k, get mapped point index m, origin left top */
			m=SQMat_XRYR[k].y*n+SQMat_XRYR[k].x; /* get point index after rotation */
			fbset_color(image[k]);
			draw_dot(&gv_fb_dev, x0y0.x+m%n, x0y0.y+m/n);
		}
	}
}
#endif

/*----------------------- Drawing for Method: revert rotation  -------------------------*/
void fb_drawimg_SQMap(int n, struct egi_point_coord x0y0, uint16_t *image,
						const struct egi_point_coord *SQMat_XRYR)
{
	int i,j,k;

	/* check if n can be resolved in form of 2*m+1 */
	if( (n-1)%2 != 0)
	{
		printf("fb_drawimg_SQMap(): the number of pixels on the square side must be n=2*m+1.\n");
	 	return;
	}

	for(i=0;i<n;i++)
		for(j=0;j<n;j++)
		{
			/* since index i point map to  SQMat_XRYR[i](x,y), so get its mapped index k */
			k=SQMat_XRYR[i*n+j].y*n+SQMat_XRYR[i*n+j].x;
			//printf("k=%d\n",k);
			fbset_color(image[k]);
			draw_dot(&gv_fb_dev, x0y0.x+j, x0y0.y+i); /* ???? n-j */
		}
}



/*-----------------------------------------------------------------------------
scale a block of pixel buffer.
 owid,ohgt:	old/original width and height of the pixel area
 nwid,nhgt:   new width and height
 obuf:        old data buffer for 16bit color pixels
 nbuf;	scaled data buffer for 16bit color pixels

Return
	1	partial area out of FB mem boundary
	0	OK
	-1	fails
------------------------------------------------------------------------------*/
int fb_scale_pixbuf(unsigned int owid, unsigned int ohgt, unsigned int nwid, unsigned int nhgt,
			uint16_t *obuf, uint16_t *nbuf)
{
	int i,j;
	int imap,jmap;

	/* 1. check data */
	if(owid==0 || ohgt==0 || nwid==0 || nhgt==0)
	{
		printf("fb_scale_pixbuf(): pixel area is 0! fail to scale.\n");
		return -1;
	}
	if(obuf==NULL || nbuf==NULL)
	{
		printf("fb_scale_pixbuf(): old or new pixel buffer is NULL! fail to scale.\n");
		return -2;
	}

	/* 2. map pixel from old area to new area */
	for(i=1;i<nhgt+1;i++) /* pixel row index */
	{
		for(j=1;j<nwid+1;j++) /* pixel column index */
		{
			/* map i,j to old buf index imap,jmap */
			//imap= round( (float)i/(float)nhgt*(float)ohgt );
			//jmap= round( (float)j/(float)nwid*(float)owid );
			imap= i*ohgt/nhgt;
			jmap= j*owid/nwid;
			//PDEBUG("fb_scale_pixbuf(): imap=%d, jmap=%d\n",imap,jmap);
			/* get mapped pixel color */
			nbuf[ (i-1)*nwid+(j-1) ] = obuf[ (imap-1)*owid+(jmap-1) ];
		}
	}

	return 0;
}
