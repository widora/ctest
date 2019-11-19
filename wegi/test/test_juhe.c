/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example for www.juhe.com https interface.

Usage:	./test_juhe top

Midas Zhou
-------------------------------------------------------------------*/
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <libgen.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include "egi_common.h"
#include "egi_https.h"
#include "egi_cstring.h"
#include "egi_FTsymbol.h"

static char strkey[256];
static char buff[32*1024]; /* for curl return */

static size_t curlget_callback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t download_callback(void *ptr, size_t size, size_t nmemb, void *stream);
char* juhe_get_objitem(const char *strinput, int index, const char *strkey);
void  print_json_object(const json_object *json);


static char* news_type[10]=
{
"top", "shehui", "guonei","guoji", "yule", "tiyu", "junshi", "keji", "caijing", "shishang"
};

/* 	---------- juhe.cn  News Types -----------

	top(头条，默认),shehui(社会),guonei(国内),guoji(国际),yule(娱乐),tiyu(体育)
        junshi(军事),keji(科技),caijing(财经),shishang(时尚)


	------------------  www.juhe.cn FREE NEWS : DATA FORMAT ------------------
{
	"reason":"成功的返回",
	"result":{
		"stat":"1",
		"data":[
			{
				"uniquekey":"af9debdd0055d05cdccd18a59e4067a8",
				"title":"花钱买空气！印度空气质量告急 民众可花50元吸氧15分钟",
				"date":"2019-11-19 15:07",
				"category":"国际",
				"author_name":"海外网",
				"url":"http:\/\/mini.eastday.com\/mobile\/191119150735829.html",
				"thumbnail_pic_s":"http:\/\/06imgmini.eastday.com\/mobile\/20191119\/20191119150735_3fc1f9aadc1a836124f07bea01ee06ee_1_mwpm_03200403.jpg"
			},
			{
				"uniquekey":"9128a366ebb58c36aa60865a840b45e5",
				"title":"日韩19日再度举行双边贸易磋商 日媒：恐难成共识",
				"date":"2019-11-19 15:05",
				"category":"国际",
				"author_name":"中国青年网",
				"url":"http:\/\/mini.eastday.com\/mobile\/191119150546120.html",
				"thumbnail_pic_s":"http:\/\/09imgmini.eastday.com\/mobile\/20191119\/20191119150546_b19351988160f8d7ffce98ad8628e179_1_mwpm_03200403.jpg"
			},

			{
			... ...
*/



int main(int argc, char **argv)
{
	int i;
	int k;
	char *pstr=NULL;
        static char strRequest[256+64];
	char *file_url;

	char *thumb_path="/tmp/thumb.jpg";
	EGI_IMGBUF *imgbuf;

	if(argc<2)
	{
		printf("Usage: %s top\n", argv[0]);
		exit(-1);
	}



        /* <<<<< 	 EGI general init 	 >>>>>> */
#if 0
        printf("tm_start_egitick()...\n");
        tm_start_egitick();                     /* start sys tick */
        printf("egi_init_log()...\n");
        if(egi_init_log("/mmc/log_test") != 0) {        /* start logger */
                printf("Fail to init logger,quit.\n");
                return -1;
        }
#endif
        printf("symbol_load_allpages()...\n");
        if(symbol_load_allpages() !=0 ) {       /* load sys fonts */
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
        if(FTsymbol_load_appfonts() !=0 ) {     /* load FT fonts LIBS */
                printf("Fail to load FT appfonts, quit.\n");
                return -2;
        }
        printf("init_fbdev()...\n");
        if( init_fbdev(&gv_fb_dev) )            /* init sys FB */
                return -1;
#if 0
        printf("start touchread thread...\n");
        egi_start_touchread();                  /* start touch read thread */
#endif
        /* <<<<< 	 End EGI Init 	 >>>>> */


        /* read key from EGI config file */
        egi_get_config_value("JUHE_NEWS", "key", strkey);

/////////////////////////	  LOOP TEST      ////////////////////////////
for(k=0; ;k==9?k=0:(k++) ) {

	/* prepare request string */
        strcat(strRequest,"https://v.juhe.cn/toutiao/index?type=");
	strcat(strRequest, news_type[k]); //argv[1]);
        strcat(strRequest,"&key=");
        strcat(strRequest,strkey);
        //printf("strRequest:%s\n", strRequest);

        /* Get request */
        memset(buff,0,sizeof(buff));
        if(https_curl_request(strRequest, buff, NULL, curlget_callback)!=0) {
                printf("Fail to call https_curl_request()!\n");
                return -1;
        }
        //printf("curl reply:\n %s\n",buff);


	printf("\n\n -------  News[%s] API powered by www.juhe.cn  ------ \n\n", news_type[k]);

	for(i=0; i<100; i++) {
		fb_clear_backBuff(&gv_fb_dev, WEGI_COLOR_BLACK);

		pstr=juhe_get_objitem(buff, i, "url");
		printf("	  url:%s\n",pstr);
		free(pstr); pstr=NULL;

		/* Download thumb pic  and display */
		pstr=juhe_get_objitem(buff,i,"thumbnail_pic_s");
		if(pstr != NULL) {
			printf("Start https easy download: %s\n", pstr);
			https_easy_download(pstr, thumb_path, NULL, download_callback);

	        	/* readin file */
	        	imgbuf=egi_imgbuf_readfile(thumb_path);
        		if(imgbuf==NULL) {
		                printf("Fail to read image file '%s'.\n", thumb_path);
				free(pstr); pstr=NULL;
                		continue;
		        }

		        /* display */
		        printf("display imgbuf...\n");
		        egi_imgbuf_windisplay( imgbuf, &gv_fb_dev, -1,         /* img, FB, subcolor */
                		                0, 0,                            /* int xp, int yp */
                               			0, 0, imgbuf->width, imgbuf->height   /* xw, yw, winw,  winh */
                              		      );

			/* Free it */
		        egi_imgbuf_free(imgbuf);
			//tm_delayms(3000);
		}

		/* Get news title and display */
		pstr=juhe_get_objitem(buff, i, "title");
		if(pstr==NULL)
			break;

        	FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.bold,     /* FBdev, fontface */
                                     16, 16, "Powered by www.juhe.cn",      /* fw,fh, pstr */
                                     240-10, 1, 5,                       /* pixpl, lines, gap */
                                     5, 5,          /* x0,y0, */
                                     WEGI_COLOR_RED, -1, -1 );  /* fontcolor, transcolor,opaque */


        	FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.bold,     /* FBdev, fontface */
                                     18, 18, (const unsigned char *)pstr,      /* fw,fh, pstr */
                                     240-10, 3, 5,                       /* pixpl, lines, gap */
                                     5, 320-75,          /* x0,y0, */
                                     WEGI_COLOR_GRAYB, -1, -1 );  /* fontcolor, transcolor,opaque */

		printf("%s news[%02d]: %s\n", news_type[k], i, pstr);
		free(pstr); pstr=NULL;

		/* refresh FB page */
		fb_page_refresh(&gv_fb_dev);
		sleep(2);
	}

} //////////////////////////      END LOOP  TEST      ///////////////////////////


	return 0;
}


/*-----------------------------------------------
 A callback function to deal with replied data.
------------------------------------------------*/
static size_t curlget_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
        strcat(userp, ptr);
        return size*nmemb;
}


/*-----------------------------------------------
 A callback function for write data to file.
------------------------------------------------*/
static size_t download_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
       size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
       return written;
}



/*--------------------------------------------------------------------------------------------
Parse juhe.cn free news string and return string pointer to the vale of specified strkey of
data[index], or to data[index] if strkey is NULL.

Note:
        !!! Don't forget to free the returned string pointer !!!

@strinput       input juhe.cn free news return  string
@index		index of news array data[]
@strkey         key name of the news items in data[index]. Example: "uniquekey","title","url"...
                Note: if strkey==NULL, then return string pointer to data[index].

Return:
        0       ok
        <0      fails
----------------------------------------------------------------------------------------------*/
char* juhe_get_objitem(const char *strinput, int index, const char *strkey)
{
        char *pt=NULL;

        json_object *json_input=NULL;
        json_object *json_result=NULL;
        json_object *json_array=NULL; /* Array of titles */
        json_object *json_data=NULL;
        json_object *json_get=NULL;

        /* parse returned string */
        json_input=json_tokener_parse(strinput);
        if(json_input==NULL) goto GET_FAIL; //return NULL;

        /* strip to get array data[]  */
        json_object_object_get_ex(json_input,"result",&json_result);
        if(json_result==NULL)goto GET_FAIL; //return NULL;

	json_object_object_get_ex(json_result,"data",&json_array);
        if(json_array==NULL)goto GET_FAIL; //return NULL;

	/* TODO: limit index */
	json_data=json_object_array_get_idx(json_array,index);  /* Title array itmes */
	if(json_data==NULL)goto GET_FAIL; //return NULL;

//	print_json_object(json_data);

        /* if strkey, get key obj */
        if(strkey!=NULL) {
                json_object_object_get_ex(json_data, strkey, &json_get);
                if(json_get==NULL)goto GET_FAIL; // return NULL;
        } else {
                json_get=json_data;
        }

        /* Get pointer to the item string */
        pt=strdup((char *)json_object_get_string(json_get));


GET_FAIL:
        /* free input object */
        json_object_put(json_input);

        return pt;
}


/*--------------------------------------
	Print a json object
--------------------------------------*/
void  print_json_object(const json_object *json)
{

	char *pstr=NULL;

	pstr=strdup((char *)json_object_get_string((json_object *)json));
	if(pstr==NULL)
		return;

	printf("%s\n",pstr);
	free(pstr);
}

