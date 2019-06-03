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
char request[256]="https://free-api.heweather.net/s6/weather/now?location=shanghai&key=";
char strkey[256];

char buff[1024*1024];

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



/* a callback function to handler returned data */
size_t callback_get(void *ptr, size_t size, size_t nmemb, void *userp)
{
	strcat(userp,ptr);
	return size*nmemb;
}


char strpath[256];
char strtemp[5];
int temp;
char strhum[5];
int hum;
EGI_IMGBUF eimg={0};

int main(void)
{
  	int i=0;
        int n=0;
  	CURL *curl;
  	CURLcode res;

	/* display png cond image */
   	init_fbdev(&gv_fb_dev); /* init FB dev */
        /* --- load all symbol pages --- */
        symbol_load_allpages();

	/* read key from EGI config file */
	egi_get_config_value("EGI_WEATHER", "key", strkey);
	printf("key: %s\n",strkey);
	strcat(request,strkey);
	printf("request: %s\n",request);

////////// LOOP TEST ///////////
while(1) {
	n++;

	/* init curl */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	memset(buff,0,1024*1024);
	if(curl) {
		i++;
		printf("start curl_easy %d\n",i);
		curl_easy_setopt(curl, CURLOPT_URL, request);		 	 /* set request URL */
		curl_easy_setopt(curl, CURLOPT_VERBOSE,1);			 /* print more detail */
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);			 /* set timeout */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_get);     /* set write_callback */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, buff); 		 /* set data dest for write_callback */

#ifdef SKIP_PEER_VERIFICATION
    		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef SKIP_HOSTNAME_VERIFICATION
    		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
               		curl_easy_strerror(res));

    		/* always cleanup */
    		curl_easy_cleanup(curl);
  	}
  	curl_global_cleanup();



/* ----------------------- An Example of Returned Weather Data --------------------------
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
---------------------------------------------------------------------------------------*/
	int len;
	json_object *json_ret=NULL;
	json_object *json_weather=NULL;
	json_object *tmp_obj;
	json_object *json_get;

	json_ret=json_tokener_parse(buff);
//	printf("json_ret: %s\n",json_object_to_json_string(json_ret)); /* json..() return a pointer */

  	/* travel json_ret, get json_weather */
	json_object_object_foreach(json_ret, key, obj) {  /* key: char*, obj: json object */
//		printf("key:'%s', val:'%s'\n", key, json_object_to_json_string(obj));
//		printf("type: %s \n", json_type_str[json_object_get_type(obj)]);
		if(!strcmp(key,"HeWeather6")) {
			tmp_obj=json_object_array_get_idx(obj,0);
	   		json_object_object_foreach(tmp_obj, key2, obj2) {  /* key2: char*, obj2: json object */
//	          		printf("key:'%s', val:'%s'\n", key2, json_object_to_json_string(obj2));
//        	  		printf("type: %s \n", json_type_str[json_object_get_type(obj2)]);
				if(!strcmp(key2,"now")) {
					json_weather=obj2;
				}
			}
		}
  	}
	if(json_weather != NULL)
		printf("Weather for now: %s\n",json_object_to_json_string(json_weather)); /* json..() return a pointer */
	else
		printf("json_weather is NULL!\n");

	/* extract cond_code and Temperature */
 	json_object_object_get_ex(json_weather,"cond_code", &json_get);
	sprintf(strpath,"/mmc/heweather/%s.png",json_object_get_string(json_get));
 	printf("strpath:%s\n",strpath);

	json_object_object_get_ex(json_weather,"tmp", &json_get);
	memset(strtemp,0,sizeof(strtemp));
	sprintf(strtemp,"%s",json_object_get_string(json_get));
	temp=atoi(strtemp);

	json_object_object_get_ex(json_weather,"hum", &json_get);
	memset(strhum,0,sizeof(strhum));
	sprintf(strhum,"%s",json_object_get_string(json_get));
	hum=atoi(strhum);

 	printf("Temp:%dC  Hum:%d\n",temp,hum);


	/* load png file acoording and display */
   	if( egi_imgbuf_loadpng(strpath, &eimg ) !=0 ) {
		printf("Fail to loadpng %s!\n", strpath);
		return -3;
   	}
   	EGI_IMGBOX subimg; /* OK, only 1 sub_image */
	subimg.x0=0; subimg.y0=0;
   	subimg.w=100; subimg.h=100;
   	eimg.subimgs=&subimg;
   	eimg.subtotal=1;

        /* <<< Flush FB and Turn on FILO before wirteFB >>>*/
        printf("Flush pixel data in FILO, start  ---> ");
        fb_filo_flush(&gv_fb_dev); /* flush and restore old FB pixel data */
        fb_filo_on(&gv_fb_dev); /* start collecting old FB pixel data */

//      egi_subimg_writeFB(&eimg, &gv_fb_dev, 0, -1, 70, 220);
   	egi_subimg_writeFB(&eimg, &gv_fb_dev, 0, WEGI_COLOR_ORANGE, 70, 220);

        symbol_string_writeFB(&gv_fb_dev, &sympg_numbfont, WEGI_COLOR_ORANGE,
                                  		1, 170, 235, strtemp );


        /* <<<  Turn off FILO after writeFB  >>> */
        fb_filo_off(&gv_fb_dev);

   	/* release source */
   	egi_imgbuf_release(&eimg);

	json_object_put(json_ret);


	printf(" --------------------- N:%d ---------------\n", n);
	sleep(10);

} ////////////// LOOP TEST END /////////////


   	release_fbdev(&gv_fb_dev);

#if 0//////////////////////////////////////////////////////////////////////////////////////
/**
 * Check if the json_object is of a given type
 * @param obj the json_object instance
 * @param type one of:
     json_type_null (i.e. obj == NULL),
     json_type_boolean,
     json_type_double,
     json_type_int,
     json_type_object,
     json_type_array,
     json_type_string,
 */
extern int json_object_is_type(struct json_object *obj, enum json_type type);
  /** Get the element at specificed index of the array (a json_object of type json_type_array)
 * @param obj the json_object instance
 * @param idx the index to get the element at
 * @returns the json_object at the specified index (or NULL)
 */
 extern struct json_object* json_object_array_get_idx(struct json_object *obj, int idx);
/** Get the arraylist of a json_object of type json_type_array
 * @param obj the json_object instance
 * @returns an arraylist
 */
extern struct array_list* json_object_get_array(struct json_object *obj);

/** Get the length of a json_object of type json_type_array
 * @param obj the json_object instance
 * @returns an int
 */
extern int json_object_array_length(struct json_object *obj);


extern json_bool json_object_object_get_ex(struct json_object* obj,
                                                  const char *key,
                                                  struct json_object **value);


#endif ////////////////////////////////////////////////////////////////////////////


  return 0;
}
