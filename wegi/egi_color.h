/*-----------------------------------------------------
Color classification method:
	Douglas.R.Jacobs' RGB Hex Triplet Color Chart
  	http://www.jakesan.com

Midas Zhou
-----------------------------------------------------*/
#ifndef	__EGI_COLOR_H__
#define __EGI_COLOR_H__

#include <stdint.h>

/* convert 24bit rgb to 16bit LCD rgb */
#define COLOR_RGB_TO16BITS(r,g,b)	 (uint16_t)( (r>>3)<<11 | (g>>2)<<5 | b>>3 )

/* color definition */
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

#define WEGI_COLOR_GRAY			 COLOR_RGB_TO16BITS(0xCC,0xCC,0xCC)
#define WEGI_COLOR_GRAY1		 COLOR_RGB_TO16BITS(0x88,0x88,0x88)
#define WEGI_COLOR_GRAY2	 	 COLOR_RGB_TO16BITS(0x55,0x55,0x55)
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
uint16_t egi_color_random(enum egi_color_range range);







#endif
