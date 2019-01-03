#ifndef __EGI_DEBUG_H__
#define __EGI_DEBUG_H__

#include <stdio.h>


#define DEGI_DEBUG


#ifdef EGI_DEBUG

	#define PDEBUG(fmt, args...)   fprintf(stderr,fmt, ## args)

#else
	#define PDEBUG(fmt,args...)

#endif







#endif
