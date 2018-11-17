/*------------------------------------------------------------------------------
Referring to: http://blog.chinaunix.net/uid-22666248-id-285417.html

 本文的copyright归yuweixian4230@163.com 所有，使用GPL发布，可以自由拷贝，转载。
但转载请保持文档的完整性，注明原作者及原链接，严禁用于任何商业用途。

作者：yuweixian4230@163.com
博客：yuweixian4230.blog.chinaunix.net
-----------------------------------------------------------------------------*/
    #include <unistd.h>
    #include <stdio.h>
    #include <fcntl.h>
    #include <linux/fb.h>
    #include <sys/mman.h>
    #include <sys/ioctl.h>

    typedef struct fbdev{
        int fdfd; //open "dev/fb0"
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        long int screensize;
        char *map_fb;
    }FBDEV;

    void init_dev(FBDEV *dev);
    void draw_dot(FBDEV *dev,int x,int y); //(x.y) 是坐标
    void draw_line(FBDEV *dev,int x1,int y1,int x2,int y2);
    void draw_rect(FBDEV *dev,int x1,int y1,int x2,int y2);


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

