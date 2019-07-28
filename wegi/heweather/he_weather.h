/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example for www.heweahter.com https interface.

Midas Zhou
-------------------------------------------------------------------*/
#ifndef __HE_WEATHER__
#define __HE_WEATHER__

#include <stdio.h>
#include "egi_image.h"

#define _SKIP_PEER_VERIFICATION
#define _SKIP_HOSTNAME_VERIFICATION

#define HEWEATHER_NOW		0
#define HEWEATHER_FORECAST	1
#define HEWEATHER_HOURLY	2
#define HEWEATHER_LIFESTYLE	3

#define HEWEATHER_ICON_PATH	"/mmc/heweather" /* png icons */

enum heweather_data_type {
	data_now=0,
	data_forecast,
	data_hourly,
	data_lifestyle
};

typedef struct  heweather_data {
	EGI_IMGBUF	*eimg; 		/* weather Info image */
	char		*icon_path;
	char		*cond_txt;	/* weather condition txt */
	int		temp;  		/* temperature */
	int		temp_max;
	int		temp_min;
	int		hum;   		/* humidity */
	int		hum_max;
	int		hum_min;
}EGI_WEATHER_DATA;

/* 0=now;
   Forcast: 1=today;  2=tomorrow; 3=the day aft. tomorrow */
extern EGI_WEATHER_DATA weather_data[4];

/* function */
char * heweather_get_objitem(const char *strinput, const char *strsect, const char *strkey);
char * heweather_get_forecast(const char *strinput, int index, const char *strkey);
void   heweather_data_clear(EGI_WEATHER_DATA *weather_data);
int heweather_httpget_data(enum heweather_data_type data_type);


#endif
