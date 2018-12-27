/*------------------------------------------------------------------------------
Referring to: http://blog.chinaunix.net/uid-22666248-id-285417.html

 本文的copyright归yuweixian4230@163.com 所有，使用GPL发布，可以自由拷贝，转载。
但转载请保持文档的完整性，注明原作者及原链接，严禁用于任何商业用途。

作者：yuweixian4230@163.com
博客：yuweixian4230.blog.chinaunix.net
-----------------------------------------------------------------------------*/
#ifndef __FBLINES_H__
#define __FBLINES_H__

#include <stdio.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdbool.h>
#include "egi.h"

/* for draw_dot(), roll back to start if reach boundary of FB mem */
#define FB_DOTOUT_ROLLBACK /* also check FB_SYMBOUT_ROLLBACK in symbol.h */


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
extern FBDEV   gv_fb_dev;

/* -------- functions ---------*/
void init_dev(FBDEV *dev);
void release_dev(FBDEV *dev);
bool point_inbox(int px,int py,int x1,int y1,int x2,int y2);
void fbset_color(uint16_t color);
void clear_screen(FBDEV *dev, uint16_t color);
int draw_dot(FBDEV *dev,int x,int y); //(x.y) 是坐标
void draw_line(FBDEV *dev,int x1,int y1,int x2,int y2);
void draw_oval(FBDEV *dev,int x,int y);
void draw_rect(FBDEV *dev,int x1,int y1,int x2,int y2);
int draw_filled_rect(FBDEV *dev,int x1,int y1,int x2,int y2);
void draw_circle(FBDEV *dev, int x, int y, int r);
void draw_filled_circle(FBDEV *dev, int x, int y, int r);
int fb_cpyto_buf(FBDEV *fb_dev, int x1, int y1, int x2, int y2, uint16_t *buf);
int fb_cpyfrom_buf(FBDEV *fb_dev, int x1, int y1, int x2, int y2, uint16_t *buf);
/*
void mat_pointrotate_SQMap(int n, int angle, struct egi_point_coord centxy,
                                                        struct egi_point_coord *SQMat_XRYR);
*/
void mat_pointrotate_SQMap(int n, int angle, struct egi_point_coord centxy,
                       					struct egi_point_coord *SQMat_XRYR);

void fb_drawimg_SQMap(int n, struct egi_point_coord x0y0, uint16_t *image,
   	                                           const struct egi_point_coord *SQMat_XRYR);

#endif
