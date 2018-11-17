/*------------------------------------------------------------------------------
Referring to: http://blog.chinaunix.net/uid-22666248-id-285417.html

 本文的copyright归yuweixian4230@163.com 所有，使用GPL发布，可以自由拷贝，转载。
但转载请保持文档的完整性，注明原作者及原链接，严禁用于任何商业用途。

作者：yuweixian4230@163.com
博客：yuweixian4230.blog.chinaunix.net
-----------------------------------------------------------------------------*/
    #include "fblines.h"

    void init_dev(FBDEV *dev)
    {
        FBDEV *fr_dev=dev;

        fr_dev->fdfd=open("/dev/fb0",O_RDWR);
        printf("the framebuffer device was opended successfully.\n");
        ioctl(fr_dev->fdfd,FBIOGET_FSCREENINFO,&(fr_dev->finfo)); //获取 固定参数
        ioctl(fr_dev->fdfd,FBIOGET_VSCREENINFO,&(fr_dev->vinfo)); //获取可变参数
        fr_dev->screensize=fr_dev->vinfo.xres*fr_dev->vinfo.yres*fr_dev->vinfo.bits_per_pixel/8; 
        fr_dev->map_fb=(char *)mmap(NULL,fr_dev->screensize,PROT_READ|PROT_WRITE,MAP_SHARED,fr_dev->fdfd,0);
        printf("init_dev successfully.\n");
    }

    void draw_dot(FBDEV *dev,int x,int y) //(x.y) 是坐标
    {
        FBDEV *fr_dev=dev;
        int *xx=&x;
        int *yy=&y;

        long int location=0;
        location=(*xx+fr_dev->vinfo.xoffset)*(fr_dev->vinfo.bits_per_pixel/8)+
                     (*yy+fr_dev->vinfo.yoffset)*fr_dev->finfo.line_length;
        int b=10;
        int g=10;
        int r=220;
        unsigned short int t=r<<11|g<<5|b;
        *((unsigned short int *)(fr_dev->map_fb+location))=t;
    }


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
        if(*xx2>=*xx1)
        {
            for(i=*xx1;i<=*xx2;i++)
            {
                j=(i-*xx1)*tekyy/tekxx+*yy1;
                draw_dot(fr_dev,i,j);
		draw_dot(fr_dev,i+1,j);
		draw_dot(fr_dev,i,j+1);
		draw_dot(fr_dev,i+1,j+1);
            }
        }
        else
        {
            //if(*xx2<*xx1)
            for(i=*xx2;i<*xx1;i++)
            {
                j=(i-*xx2)*tekyy/tekxx+*yy2;
                draw_dot(fr_dev,i,j);
            }
        }


    }

    void draw_rect(FBDEV *dev,int x1,int y1,int x2,int y2)
    {
        FBDEV *fr_dev=dev;
        int *xx1=&x1;
        int *yy1=&y1;
        int *xx2=&x2;
        int *yy2=&y2;
        int i=0,j=0;

        for(j=*yy1;j<*yy2;j++) //注意 这里要 xx1 < xx2
            for(i=*xx1;i<*xx2;i++)
            {

                draw_dot(fr_dev,i,j);
            }
    }


/*
    int main()
    {
        FBDEV     fr_dev;
        fr_dev.fdfd=-1;
        init_dev(&fr_dev);

        draw_line(&fr_dev,0,0,100,100);
        draw_line(&fr_dev,200,200,110,110);
        draw_line(&fr_dev,10,200,150,100);
        draw_line(&fr_dev,300,10,160,90);
        draw_rect(&fr_dev,300,200,320,240);

        printf("bye the framebuffer\n");
        munmap(fr_dev.map_fb,fr_dev.screensize);
        close(fr_dev.fdfd);

        return 0;    
    }
*/

