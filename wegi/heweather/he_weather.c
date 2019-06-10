/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example for www.heweahter.com https interface.

Midas Zhou
-------------------------------------------------------------------*/
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include "egi_cstring.h"
#include "egi_image.h"
#include "egi_fbgeom.h"
#include "egi_symbol.h"
#include "egi_https.h"
#include "he_weather.h"

EGI_WEATHER_DATA weather_data[4]; /* 0=now; forcast: 1=today; 2=tomorrow; 3=the day aft. tomorrow */

static char strrequest_mode[4][256]=
{
	"https://free-api.heweather.net/s6/weather/now?location=shanghai&key=",
	"https://free-api.heweather.net/s6/weather/forecast?location=shanghai&key=",
	"https://free-api.heweather.net/s6/weather/hourly?location=zhoushan&key=",
	"https://free-api.heweather.net/s6/weather/lifestyle?location=shanghai&key="
};
static char strkey[256];
static char buff[32*1024]; /* for curl return */

#if 0
char *json_type_str[]=
{
     "json_type_null",
     "json_type_boolean",
     "json_type_double",
     "json_type_int",
     "json_type_object",
     "json_type_array",
     "json_type_string"
};
#endif

/*---------------------------------------------
A callback function to deal with replied data.
----------------------------------------------*/
static size_t curlget_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	strcat(userp,ptr);
	return size*nmemb;
}


/*-----------------------------------------------------------------------------------------------
Parse HeWeather string and return string pointer to value of specified strkey, or to the value of
specified section if strkey is NULL.

Note:
	1. Strkey available only for non_array Sections. ---"basic","update","status","now"
	   However, strkey in array object "daily_forcast" CAN NOT be extracted by this function.
	   call heweather_get_arrayitem() instead.
	2. !!! Don't forget to free the returned string pointer !!!

@strinput	input HeWeather string
@strsect	section name of requested string. Example: "basic","update","now","forcast"
@strkey	        key name of the item in section. Example: "cond_code","tmp",...
		Note: if strkey==NULL, then return string pointer to section objet.

Return:
	0	ok
	<0	fails
----------------------------------------------------------------------------------------------*/
char* heweather_get_objitem(const char *strinput, const char *strsect, const char *strkey)
{
	char *pt;

	json_object *json_input=NULL;
	json_object *json_array=NULL;
	json_object *json_data=NULL;
        json_object *json_sect=NULL;
	json_object *json_get=NULL;

	/* parse returned string */
	json_input=json_tokener_parse(strinput);
	if(json_input==NULL) return NULL;

	/* strip to get section obj */
	json_object_object_get_ex(json_input,"HeWeather6",&json_array);
	if(json_array==NULL)return NULL;
	json_data=json_object_array_get_idx(json_array,0);
	if(json_data==NULL)return NULL;
	json_object_object_get_ex(json_data, strsect, &json_sect); /* get strsect object */
	if(json_sect==NULL)return NULL;

	/* if strkey, get key obj */
	if(strkey!=NULL) {
		json_object_object_get_ex(json_sect, strkey, &json_get);
		if(json_get==NULL)  return NULL;
	} else {
		json_get=json_sect;
	}

	/* Get pointer to the item string */
	pt=strdup((char *)json_object_get_string(json_get));

	/* free input object */
	json_object_put(json_input);

	return pt;
}


/*---------------------------------------------------------------------------------------------------
Parse HeWeather string and return string pointer to value of specified strkey in forecast array[index]
Note:
	1. !!! Don't forget to free the returned string pointer !!!

@strinput	input HeWeather string
@index		index of forecast json array object, for 3days, 0-2
@strkey	        key name of the item in forecast array, Example:'cond_code_d','cond_code_n','data','hum'

Return:
	0	ok
	<0	fails
--------------------------------------------------------------------------------------------------*/
char* heweather_get_forecast(const char *strinput, int index, const char *strkey)
{
	char *pt;

	json_object *json_input=NULL;
	json_object *json_array=NULL;
	json_object *json_data=NULL;
        json_object *json_sect=NULL;
	json_object *json_get=NULL;

	/* set forcast index limit, 3 days */
	if(index<0 || index>2)
		return NULL;

	/* parse returned string */
	json_input=json_tokener_parse(strinput);
	if(json_input==NULL) return NULL;

	/* strip to get section obj */
	json_object_object_get_ex(json_input,"HeWeather6",&json_array);
	if(json_array==NULL)return NULL;
	json_data=json_object_array_get_idx(json_array,0);
	if(json_data==NULL)return NULL;
	json_object_object_get_ex(json_data, "daily_forecast", &json_sect); /* get strsect object */
	if(json_sect==NULL)return NULL;
	json_array=json_object_array_get_idx(json_sect,index);

	/* if strkey, get key obj in array[index] */
	if(strkey!=NULL) {
		json_object_object_get_ex(json_array, strkey, &json_get);
		if(json_get==NULL)  return NULL;
	} else {
		return NULL;
	}

	/* Get pointer to the item string */
	pt=strdup((char *)json_object_get_string(json_get));

	/* free input object */
	json_object_put(json_input);

	return pt;
}



/*--------------------------------------------------------------------
Get weather data from HeWeather.com

Note:
1. If hwdata[]->eimg is NULL, then egi_imgbuf_new() will be called to allocatge
   it.
2. If hwdata[]->eimg is NOT null, then egi_imgbuf_freedata() will be called
   to empty its old data before put new data into it.


@hwdata:	an array of EGI_WEATHER_DATA
@data_type:	weather data type to be requested

Return:
	0	Ok
	<0	Fails
----------------------------------------------------------------------*/
int heweather_httpget_data(EGI_WEATHER_DATA *hwdata, enum heweather_data_type data_type)
{
  	int i=0;
	int index;
//        int n=0;
	char *pstr=NULL;
	char strrequest[256+64];
	char strpath[256];
	char strtemp[16];
	int  temp;
	char strhum[16];
	int  hum;
//	EGI_IMGBUF *eimg=NULL;

	if(hwdata==NULL) {
		printf("%s: input hwdata is NULL!\n",__func__);
		return -1;
	}

	/* read key from EGI config file */
	egi_get_config_value("EGI_WEATHER", "key", strkey);
	sprintf(strrequest,"%s%s", strrequest_mode[data_type], strkey);

	/* Get request */
	memset(buff,0,sizeof(buff));
	https_curl_request(strrequest, buff, NULL, curlget_callback);
	printf("%s: get curl reply: %s\n",__func__, buff);

	/* Extract section item 'status' string */
	pstr=heweather_get_objitem(buff,"status",NULL);
	if(pstr) {
		printf("status: %s\n",pstr);
		free(pstr);
	}

/*---------   (  PARSE Request_NOW  )   ------------
{"HeWeather6":[ {"basic":{"cid":"CN101020100","location":"上海","parent_city":"上海","admin_area":"上海",
                  	    "cnty":"中国","lat":"31.23170662","lon":"121.47264099","tz":"+8.00"},
                 "update":{"loc":"2019-06-03 15:09","utc":"2019-06-03 07:09"},
  		 "status":"ok",
                 "now":{"cloud":"0","cond_code":"101","cond_txt":"多云","fl":"31","hum":"48","pcpn":"0.0",
			 "pres":"1005", "tmp":"29","vis":"16","wind_deg":"1","wind_dir":"北风","wind_sc":"0",
		         "wind_spd":"1"}
  	        }
	      ]
*/
  if(data_type==data_now)
  {
	/* Extract key item 'cond_code' in section 'now' */
	pstr=heweather_get_objitem(buff, "now","cond_code");
	if(pstr!=NULL)
		sprintf(strpath,"%s/%s.png",HEWEATHER_ICON_PATH,pstr);
	free(pstr);
 	printf("strpath:%s\n",strpath);

	/* Extract key item 'tmp' in section 'now' */
	pstr=heweather_get_objitem(buff,"now", "tmp");
	if(pstr!=NULL)
		sprintf(strtemp,"%sC", pstr);
	free(pstr);
	temp=atoi(strtemp);

	/* Extract key item 'hum' in section 'now' */
	pstr=heweather_get_objitem(buff, "now", "hum");
	if(pstr!=NULL)
		sprintf(strhum,"%s", pstr);
	free(pstr);
	hum=atoi(strhum);

 	printf("Temp:%dC  Hum:%%%d\n",temp,hum);

	/* fill into EGI_WEATHER_DATA */
	if(hwdata[0].eimg==NULL) {
		hwdata[0].eimg=egi_imgbuf_new();
		if(hwdata[0].eimg==NULL) {
			printf("%s: fail to call egi_imgbuf_new()!\n");
			return -2;
		}
	}
	/* empty old data */
	egi_imgbuf_freedata(hwdata[0].eimg);
	/* load png file acoordingly */
   	if( egi_imgbuf_loadpng(strpath, hwdata[0].eimg ) !=0 ) {
		printf("%s: Fail to loadpng %s!\n", __func__, strpath);
		return -3;
   	}
	hwdata[0].temp=temp;
	hwdata[0].hum=hum;

   }

/*---------   (  PARSE Request_FORECAST  )   ------------

{"HeWeather6":[ { "basic": {"cid":"CN101020100","location":"上海","parent_city":"上海","admin_area":"上海",
			      "cnty":"中国","lat":"31.23170662","lon":"121.47264099","tz":"+8.00"},
		   "update":{"loc":"2019-06-07 16:57","utc":"2019-06-07 08:57"},
		   "status":"ok",
		   "daily_forecast":[ { "cond_code_d":"101","cond_code_n":"101","cond_txt_d":"多云","cond_txt_n":"多云",
					"date":"2019-06-07","hum":"70","mr":"08:23","ms":"22:37","pcpn":"5.0","pop":"79","pres":"1008","sr":"04:49","ss":"18:56","tmp_max":"28","tmp_min":"20","uv_index":"3","vis":"25","wind_deg":"359","wind_dir":"北风","wind_sc":"1-2","wind_spd":"3"},
				      { "cond_code_d":"101","cond_code_n":"101","cond_txt_d":"多云","cond_txt_n":"多云",
				        "date":"2019-06-08","hum":"76","mr":"09:30","ms":"23:24","pcpn":"0.0","pop":"24","pres":"1003","sr":"04:49","ss":"18:57","tmp_max":"29","tmp_min":"21","uv_index":"3","vis":"25","wind_deg":"129","wind_dir":"东南风","wind_sc":"3-4","wind_spd":"24"},
				      { "cond_code_d":"101","cond_code_n":"101","cond_txt_d":"多云","cond_txt_n":"多云",
					"date":"2019-06-09","hum":"87","mr":"10:36","ms":"00:00","pcpn":"0.0","pop":"1","pres":"1001","sr":"04:49","ss":"18:57","tmp_max":"30","tmp_min":"21","uv_index":"6","vis":"24","wind_deg":"146","wind_dir":"东南风","wind_sc":"3-4","wind_spd":"13"}
				    ]
                  }
                ]
  }
*/

  else if(data_type=data_forecast)
  {
	printf("Daily_forecast ");
	for(i=0;i<3;i++) {
		pstr=heweather_get_forecast(buff,i,"cond_txt_d");
		if(pstr!=NULL) {
			printf("Day[%d]_%s ",i,pstr);
		}
		free(pstr);

		pstr=heweather_get_forecast(buff,i,"tmp_max");
		if(pstr!=NULL) {
			printf("Tmax.%s ",pstr);
		}
		free(pstr);

		pstr=heweather_get_forecast(buff,i,"tmp_min");
		if(pstr!=NULL) {
			printf("Tmin.%s ",pstr);
		}
		free(pstr);

		pstr=heweather_get_forecast(buff,i,"hum");
		if(pstr!=NULL) {
			printf("Hum.%s ",pstr);
		}
		free(pstr);

	}
	printf("\n");


  }


/*---------   (  PARSE Request_Hourly  )   ------------

*/

  return 0;
}
