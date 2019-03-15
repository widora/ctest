#ifndef __EGI_DEBUG_H__
#define __EGI_DEBUG_H__

#include <stdio.h>


//#define EGI_DEBUG

/* debug flags */
#define DBG_NONE	(0<<0)
#define	DBG_EGI 	(1<<0)
#define	DBG_TXT		(1<<1)
#define DBG_BTN		(1<<2)
#define DBG_LIST        (1<<3)
#define DBG_PIC		(1<<4)
#define DBG_PAGE	(1<<5)
#define DBG_COLOR	(1<<6)
#define DBG_SYMBOL	(1<<7)
#define DBG_OBJTXT	(1<<8)
#define DBG_FBGEOM	(1<<9)
#define DBG_TOUCH	(1<<10)
#define DBG_BMPJPG	(1<<11)
#define DBG_FFPLAY	(1<<12)
#define DBG_IOT		(1<<13)
#define DBG_FIFO	(1<<14)

#define DBG_TEST	(1<<15)

/* default debug flags */
#define DEFAULT_DBG_FLAGS   (DBG_NONE|DBG_PAGE|DBG_IOT|DBG_FIFO) //DBG_FFPLAY) //DBG_TOUCH) //DBG_TOUCH)//DBG_SYMBOL|DBG_COLOR|DBG_LIST)

#ifdef EGI_DEBUG

	#define PDEBUG(fmt, args...)   fprintf(stderr,fmt, ## args)

#else
	#define PDEBUG(fmt,args...)   /* blank space */

#endif


/* define egi_pdebug() */
//#define egi_pdebug(flags, fmt, args...)
#define EGI_PDEBUG(flags, fmt, args...)			\
	do {						\
		if( flags & DEFAULT_DBG_FLAGS)		\
		{					\
			fprintf(stderr,"%s(): ",__func__); 	\
			fprintf(stderr,fmt, ## args);	\
		}					\
	} while(0)




#endif
