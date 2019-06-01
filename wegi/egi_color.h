/*----------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Color classification method:
	Douglas.R.Jacobs' RGB Hex Triplet Color Chart
  	http://www.jakesan.com

Midas Zhou
----------------------------------------------------------------------*/
#ifndef	__EGI_COLOR_H__
#define __EGI_COLOR_H__

#include <stdint.h>

/* color definition */
typedef uint16_t			 EGI_16BIT_COLOR;
typedef uint32_t			 EGI_24BIT_COLOR;

/* convert 24bit rgb(3*8bits) to 16bit LCD rgb */
#define COLOR_RGB_TO16BITS(r,g,b)	  ((uint16_t)( ( ((r)>>3)<<11 ) | ( ((g)>>2)<<5 ) | ((b)>>3) ))
#define COLOR_24TO16BITS(rgb)	(COLOR_RGB_TO16BITS( (rgb>>16), ((rgb&0x00ff00)>>8), (rgb&0xff) ) )
#define COLOR_16TO24BITS(rgb)   ((uint32_t)( ((rgb&0xF800)<<8) + ((rgb&0x7E0)<<5) + ((rgb&0x1F)<<3) ))  //1111,1000,0000,0000 //111,1110,0000

/* front_color(16bits), background_color(16bits), alpha channel value(0-255) */
#define COLOR_16BITS_BLEND(front, back, alpha)							\
		COLOR_RGB_TO16BITS (								\
			  ( ((front&0xF800)>>8)*alpha + ((back&0xF800)>>8)*(255-alpha) )/255, 	\
			  ( ((front&0x7E0)>>3)*alpha + ((back&0x7E0)>>3)*(255-alpha) )/255,   	\
			  ( ((front&0x1F)<<3)*alpha + ((back&0x1F)<<3)*(255-alpha) )/255     	\
		)										\



#define WEGI_COLOR_BLACK 		 COLOR_RGB_TO16BITS(0,0,0)
#define WEGI_COLOR_WHITE 		 COLOR_RGB_TO16BITS(255,255,255)

#define WEGI_COLOR_RED 			 COLOR_RGB_TO16BITS(255,0,0)
#define WEGI_COLOR_ORANGE		 COLOR_RGB_TO16BITS(255,125,0)
#define WEGI_COLOR_YELLOW		 COLOR_RGB_TO16BITS(255,255,0)
#define WEGI_COLOR_SPRINGGREEN		 COLOR_RGB_TO16BITS(125,255,0)
#define WEGI_COLOR_GREEN		 COLOR_RGB_TO16BITS(0,255,0)
#define WEGI_COLOR_TURQUOISE		 COLOR_RGB_TO16BITS(0,255,125)
#define WEGI_COLOR_CYAN			 COLOR_RGB_TO16BITS(0,255,255)
#define WEGI_COLOR_OCEAN		 COLOR_RGB_TO16BITS(0,125,255)
#define WEGI_COLOR_BLUE			 COLOR_RGB_TO16BITS(0,0,255)
#define WEGI_COLOR_VIOLET		 COLOR_RGB_TO16BITS(125,0,225)
#define WEGI_COLOR_MAGENTA		 COLOR_RGB_TO16BITS(255,0,255)
#define WEGI_COLOR_RASPBERRY		 COLOR_RGB_TO16BITS(255,0,125)
/* GRAY2 deeper than GRAY1 */
#define WEGI_COLOR_GRAY			 COLOR_RGB_TO16BITS(0xCC,0xCC,0xCC)
#define WEGI_COLOR_GRAY1		 COLOR_RGB_TO16BITS(0xBB,0xBB,0xBB)
#define WEGI_COLOR_GRAY2		 COLOR_RGB_TO16BITS(0xAA,0xAA,0xAA)
#define WEGI_COLOR_GRAY3		 COLOR_RGB_TO16BITS(0x99,0x99,0x99)
#define WEGI_COLOR_GRAY4		 COLOR_RGB_TO16BITS(0x88,0x88,0x88)
#define WEGI_COLOR_GRAY5	 	 COLOR_RGB_TO16BITS(0x77,0x77,0x77)
#define WEGI_COLOR_MAROON	 	 COLOR_RGB_TO16BITS(128,0,0)
#define WEGI_COLOR_BROWN	 	 COLOR_RGB_TO16BITS(0xFF,0x66,0)



/* color range */
enum egi_color_range
{
	light=3,
	medium=2,
	deep=1,
	all=0,
};

/* functions */
EGI_16BIT_COLOR egi_color_random(enum egi_color_range range);
EGI_16BIT_COLOR egi_colorgray_random(enum egi_color_range range);
EGI_16BIT_COLOR egi_colorbrt_adjust(EGI_16BIT_COLOR color, int k);
int egi_color_getY(EGI_16BIT_COLOR color);




#endif
