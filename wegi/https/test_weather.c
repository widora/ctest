/*--------------------------------------------------------------------------
		A http example from libcurl source codes

Refer to:  https://curl.haxx.se/libcurl/c/

Midas Zhou
--------------------------------------------------------------------------*/
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include "egi_cstring.h"
#include "egi_image.h"
#include "egi_fbgeom.h"
#include "egi_symbol.h"

#define _SKIP_PEER_VERIFICATION
#define _SKIP_HOSTNAME_VERIFICATION

const char host[]= "free-api.heweather.net";
static char request_now[256]="https://free-api.heweather.net/s6/weather/now?location=shanghai&key=";
static char request_forecast[256]="https://free-api.heweather.net/s6/weather/forecast?location=shanghai&key=";
char strkey[256];

char buff[32*1024]; /* for curl return */

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


/* a callback function for CURL to handler returned data */
static size_t curl_callback_get(void *ptr, size_t size, size_t nmemb, void *userp)
{
	strcat(userp,ptr);
	return size*nmemb;
}

/*------------------------------------------------------------------------
		HTTPS request by libcurl
@request:	request string
@reply_buff:	returned reply string, the Caller must ensure enough space.
@data:		TODO: if any more data needed

Return:
	0	ok
	<0	fails
--------------------------------------------------------------------------*/
int https_curl_request(const char *request, char *reply_buff, void *data)
{
	int ret=0;
  	CURL *curl;
  	CURLcode res;

	/* init curl */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl==NULL) {
		printf("%s: Fail to init curl!\n",__func__);
		return -1;
	}

	/* set curl option */
	curl_easy_setopt(curl, CURLOPT_URL, request);		 	 /* set request URL */
	curl_easy_setopt(curl, CURLOPT_VERBOSE,1);			 /* print more detail */
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);			 /* set timeout */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback_get);     /* set write_callback */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buff); 		 /* set data dest for write_callback */
#ifdef SKIP_PEER_VERIFICATION
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef SKIP_HOSTNAME_VERIFICATION
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);
	if(res != CURLE_OK) {
		printf("%s: curl_easy_perform() failed: %s\n", __func__, curl_easy_strerror(res));
		ret=-2;
	}

	/* always cleanup */
	curl_easy_cleanup(curl);
  	curl_global_cleanup();

	return ret;
}

/*---------------------------------------------------------------------------------------
Parse HeWeather string and return the pointer to string value of specified section item.

!!!! Note: Don't forget to free the string pointer !!!

@strinput	input HeWeather string
@strsect	section name of requested string. Example: "basic","update","now","forcast"
@strkey	        key name of the item in section. Example: "cond_code","tmp",...
		if strkey==NULL, then return string pointer to section objet.
@json_obj	a pointer to json_object as requested.

Return:
	0	ok
	<0	fails
----------------------------------------------------------------------------------------*/
char* heweather_get_item(const char *strinput, const char *strsect, const char *strkey)
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

	/* strip to get json_weather */
#if 1
	json_object_object_get_ex(json_input,"HeWeather6",&json_array);
	if(json_array==NULL)return NULL;
	json_data=json_object_array_get_idx(json_array,0);
	if(json_data==NULL)return NULL;
	json_object_object_get_ex(json_data, strsect, &json_sect); /* get strsect object */
	if(json_sect==NULL)return NULL;
	if(strkey!=NULL) {
		json_object_object_get_ex(json_sect, strkey, &json_get);
		if(json_get==NULL)return NULL;
	} else {
		json_get=json_sect;
	}
#else
	json_object_object_get_ex(json_input,strkey,&json_get);

#endif

	/* Get pointer to the item string */
	pt=strdup((char *)json_object_get_string(json_get));

	/* free input object */
	json_object_put(json_input);

	return pt;
}



char strpath[256];
char strtemp[16];
int temp;
char strhum[16];
int hum;
EGI_IMGBUF eimg={0};

int main(void)
{
  	int i=0;
        int n=0;

	/* display png cond image */
   	init_fbdev(&gv_fb_dev); /* init FB dev */
        /* --- load all symbol pages --- */
        symbol_load_allpages();

	/* read key from EGI config file */
	egi_get_config_value("EGI_WEATHER", "key", strkey);
	strcat(request_now,strkey);
	strcat(request_forecast,strkey);


///////////////////////   LOOP TEST  //////////////////////
while(1) {
	n++;

	printf("Free(NULL).....\n");
	free(NULL);

	memset(buff,0,sizeof(buff));

	https_curl_request(request_now, buff, NULL);
	printf("get curl reply: %s\n",buff);

/* -----------------------    An Example of Returned Weather Data   --------------------------
{"HeWeather6":[ {"basic":{"cid":"CN101020100","location":"上海","parent_city":"上海","admin_area":"上海",
                  	    "cnty":"中国","lat":"31.23170662","lon":"121.47264099","tz":"+8.00"},
                 "update":{"loc":"2019-06-03 15:09","utc":"2019-06-03 07:09"},
  		 "status":"ok",
                 "now":{"cloud":"0","cond_code":"101","cond_txt":"多云","fl":"31","hum":"48","pcpn":"0.0",
			 "pres":"1005", "tmp":"29","vis":"16","wind_deg":"1","wind_dir":"北风","wind_sc":"0",
		         "wind_spd":"1"}
  	        }
	      ]
}
----------------------------------------------------------------------------------------------*/
	char *pstr=NULL;

	pstr=heweather_get_item(buff,"status",NULL);
	if(pstr) {
		printf("status: %s\n",pstr);
		free(pstr);
		sleep(5);
		continue;
	}

	pstr=heweather_get_item(buff, "now", "cond_code");
	if(pstr!=NULL)
		sprintf(strpath,"/mmc/heweather/%s.png",pstr);
	free(pstr);
 	printf("strpath:%s\n",strpath);

	pstr=heweather_get_item(buff, "now", "tmp");
	if(pstr!=NULL)
		sprintf(strtemp,"%sC", pstr);
	free(pstr);
	temp=atoi(strtemp);

	pstr=heweather_get_item(buff, "now", "hum");
	if(pstr!=NULL)
		sprintf(strhum,"%%%s", pstr);
	free(pstr);
	hum=atoi(strhum);

 	printf("Temp:%dC  Hum:%d\n",temp,hum);


	/* load png file acoordingly and display */
   	if( egi_imgbuf_loadpng(strpath, &eimg ) !=0 ) {
		printf("Fail to loadpng %s!\n", strpath);
		return -3;
   	}
   	EGI_IMGBOX subimg; /* OK, only 1 sub_image */
	subimg.x0=0; subimg.y0=0;
   	subimg.w=eimg.width; subimg.h=eimg.height;
   	eimg.subimgs=&subimg;
   	eimg.subtotal=1;

        /* <<< Flush FB and Turn on FILO before wirteFB >>>*/
        printf("Flush pixel data in FILO, start  ---> ");
        fb_filo_flush(&gv_fb_dev); /* flush and restore old FB pixel data */
        fb_filo_on(&gv_fb_dev); /* start collecting old FB pixel data */

//      egi_subimg_writeFB(&eimg, &gv_fb_dev, 0, -1, 70, 220);
   	egi_subimg_writeFB(&eimg, &gv_fb_dev, 0, WEGI_COLOR_WHITE, 20,250); //70, 220);

        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_WHITE,
                                  		1, 70, 250, strtemp);// 	170, 235, strtemp );
        symbol_string_writeFB(&gv_fb_dev, &sympg_testfont, WEGI_COLOR_WHITE,
                                  		1, 70, 275, strhum);

        /* <<<  Turn off FILO after writeFB  >>> */
        fb_filo_off(&gv_fb_dev);

   	/* release source */
   	egi_imgbuf_release(&eimg);

//	json_object_put(json_ret);

	printf(" --------------------- N:%d ---------------\n", n);
	sleep(5); /* Limit 1000 per day, 90s per call */

} ///////////////////// LOOP TEST END ///////////////////////


   	release_fbdev(&gv_fb_dev);

  return 0;
}
