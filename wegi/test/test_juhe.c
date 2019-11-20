/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example for www.juhe.com https news interface.

Usage:	./test_juhe top

Midas Zhou
-------------------------------------------------------------------*/
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <libgen.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <fcntl.h>
#include "egi_common.h"
#include "egi_https.h"
#include "egi_cstring.h"
#include "egi_FTsymbol.h"

static char strkey[256];
static char buff[32*1024]; /* for curl returned data */

static size_t curlget_callback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t download_callback(void *ptr, size_t size, size_t nmemb, void *stream);

char* juhe_get_objitem(const char *strinput, int index, const char *strkey);
void  print_json_object(const json_object *json);


static char* news_type[]=
{
  "yule","shishang", "guonei", "guoji", "caijing", "keji","tiyu", "junshi", "tiyue","shehui",
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

	char *thumb_path="/tmp/thumb.jpg"; /* temp. thumb pic file */
	char pngNews_path[32];		   /* png news files */
	EGI_IMGBUF *imgbuf=NULL;
	EGI_IMGBUF *pad=NULL;

#if 0
	if(argc<2)
	{
		printf("Usage: %s top\n", argv[0]);
		exit(-1);
	}
#endif

        /* <<<<< 	 EGI general init 	 >>>>>> */
        printf("tm_start_egitick()...\n");
        tm_start_egitick();                     /* start sys tick */

#if 0
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


	/* Set screen view type as LANDSCAPE mode */
	fb_position_rotate(&gv_fb_dev, 3);

        /* Create a pad (int height, int width, unsigned char alpha, EGI_16BIT_COLOR color) */
        pad=egi_imgbuf_create(50, 320, 150, WEGI_COLOR_GRAY3);

        /* read key from EGI config file */
        egi_get_config_value("JUHE_NEWS", "key", strkey);


/////////////////////////	  LOOP TEST      ////////////////////////////
for(k=0; k< sizeof(news_type)/sizeof(char *); k++ ) {

	/* prepare request string */
        memset(strRequest,0,sizeof(strRequest));
        strcat(strRequest,"https://v.juhe.cn/toutiao/index?type=");
	strcat(strRequest, news_type[k]); //argv[1]);
        strcat(strRequest,"&key=");
        strcat(strRequest,strkey);

	printf("\n\n ------- [%s] News API powered by www.juhe.cn  ------ \n\n", news_type[k]);
        //printf("Request:%s\n", strRequest);

        /* Get request */
        memset(buff,0,sizeof(buff));
        if(https_curl_request(strRequest, buff, NULL, curlget_callback)!=0) {
                printf("Fail to call https_curl_request()!\n");
                return -1;
        }
        //printf("curl reply:\n %s\n",buff);


   	/* Get top 10 items for each type of news */
	for(i=0; i<10; i++) {
		fb_clear_backBuff(&gv_fb_dev, WEGI_COLOR_BLACK);

		printf("	  url:%s\n", juhe_get_objitem(buff, i, "url"));

		/* Set PNG news picture name string */
		memset(pngNews_path,0,sizeof(pngNews_path));
		snprintf(pngNews_path, sizeof(pngNews_path),"/tmp/%s_%d.png",news_type[k],i);

		/* If file exists, display and continue */
		if( access(pngNews_path, F_OK)==0 ) {
			printf("File exists!\n");

	             /* readin file */
	             imgbuf=egi_imgbuf_readfile(pngNews_path);
        	     if(imgbuf != NULL)
		     {
			/* reset to pos_rotate 0 for display */
			fb_position_rotate(&gv_fb_dev, 0);
			/* display saved news image */
		        egi_imgbuf_windisplay( imgbuf, &gv_fb_dev, -1,         /* img, FB, subcolor */
                		                0, 0,                            /* int xp, int yp */
                               			0, 0, imgbuf->width, imgbuf->height   /* xw, yw, winw,  winh */
                              		      );
			/* Set back to 3 */
			fb_position_rotate(&gv_fb_dev, 3);

			/* Refresh FB */
			fb_page_refresh_flyin(&gv_fb_dev, 20);
			sleep(3);

			/* free */
			egi_imgbuf_free(imgbuf);
			imgbuf=NULL;

			continue;  /* Go back to continue for() .... */
		     } /* END if( imgbuf != NULL ) */
		} /* END if file exists */

		/* Download thumb pic  and display */
		pstr=juhe_get_objitem(buff,i,"thumbnail_pic_s");
		if(pstr != NULL) {
			printf("Start https easy download: %s\n", pstr);
			https_easy_download(pstr, thumb_path, NULL, download_callback);
			free(pstr); pstr=NULL;

	        	/* readin file */
	        	imgbuf=egi_imgbuf_readfile(thumb_path);
        		if(imgbuf==NULL) {
		                printf("Fail to read image file '%s'.\n", thumb_path);
                		continue;
		        }

			#if 0 /* Not necessary for Landscape mode */
			/* rotate the imgbuf */
			egi_imgbuf_rotate_update(&imgbuf, 90);
			/* resize */
			egi_imgbuf_resize_update(&imgbuf, 240,320);
			#endif

		        /* display image */
		        printf("display imgbuf...\n");
		        egi_imgbuf_windisplay( imgbuf, &gv_fb_dev, -1,         /* img, FB, subcolor */
                		                0, 0,                            /* int xp, int yp */
                               			0, 0, imgbuf->width, imgbuf->height   /* xw, yw, winw,  winh */
                              		      );

			/* display pad */
		        egi_imgbuf_windisplay(  pad, &gv_fb_dev, -1,         /* img, FB, subcolor */
                		                0, 0,                            /* int xp, int yp */
                               			0, 240-45, imgbuf->width, imgbuf->height   /* xw, yw, winw,  winh */
                              		      );


			/* Free it */
		        egi_imgbuf_free(imgbuf);imgbuf=NULL;
			//tm_delayms(3000);
		}

		/* Get news title and display */
		pstr=juhe_get_objitem(buff, i, "title");
		if(pstr==NULL)
			break;

        	FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.bold,     /* FBdev, fontface */
                                     16, 16, "Powered by www.juhe.cn",      /* fw,fh, pstr */
                                     240-10, 1, 5,                       /* pixpl, lines, gap */
                                     5, 0,          /* x0,y0, */
                                     WEGI_COLOR_RED, -1, -1 );  /* fontcolor, transcolor,opaque */


        	FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.bold,     /* FBdev, fontface */
                                     15, 15, (const unsigned char *)pstr,      /* fw,fh, pstr */
                                     320-10, 3, 5,   //240-10,3,5          /* pixpl, lines, gap */
                                     5, 240-45+5,      //5,320-75,          /* x0,y0, */
                                     WEGI_COLOR_WHITE, -1, -1 );  /* fontcolor, transcolor,opaque */

		printf(" ----------- k=%d, i=%d ---------- \n", k, i);
		printf("%s news[%02d]: %s\n", news_type[k], i, pstr);
		free(pstr); pstr=NULL;


		/* refresh FB page */
		printf("FB page refresh ...\n");
		//fb_page_refresh(&gv_fb_dev);
		fb_page_refresh_flyin(&gv_fb_dev, 20);
		//tm_delayms(3000);

		/* save to png */
		imgbuf=egi_imgbuf_create(gv_fb_dev.pos_xres, gv_fb_dev.pos_yres, 255,0); /* pos_rotate 3 */
		imgbuf->imgbuf=gv_fb_dev.map_bk;
		egi_imgbuf_savepng(pngNews_path, imgbuf);
		imgbuf->imgbuf=NULL; /*  !!! FB imgbuf NOT transfered */
		egi_imgbuf_free(imgbuf); imgbuf=NULL;

		//sleep(2);
		printf("Press a key to continue. \n");
		getchar();

	} /* END for() */

} //////////////////////////      END LOOP  TEST      ///////////////////////////


	/* free imgbuf */
	egi_imgbuf_free(pad);
	egi_imgbuf_free(imgbuf);

        /* <<<<<  EGI general release >>>>> */
        printf("FTsymbol_release_allfonts()...\n");
        FTsymbol_release_allfonts();
        printf("symbol_release_allpages()...\n");
        symbol_release_allpages();
        printf("release_fbdev()...\n");
        fb_filo_flush(&gv_fb_dev); /* Flush FB filo if necessary */
        release_fbdev(&gv_fb_dev);
        printf("egi_end_touchread()...\n");
        egi_end_touchread();
        printf("egi_quit_log()...\n");
        egi_quit_log();
        printf("<-------  END  ------>\n");

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

	print_json_object(json_data);

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

//	char *pstr=NULL;

//	pstr=strdup((char *)json_object_get_string((json_object *)json));
//	if(pstr==NULL)
//		return;

	printf("%s\n", (char *)json_object_get_string((json_object *)json));
//	free(pstr);
}

