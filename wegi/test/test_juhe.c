/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

An example for www.juhe.com https news interface.


Usage:	./test_juhe

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
#include "egi_utils.h"

static char strkey[256];	/* for name of json_obj key */
static char buff[32*1024]; 	/* for curl returned data */

/* Callback functions for libcurl API */
static size_t curlget_callback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t download_callback(void *ptr, size_t size, size_t nmemb, void *stream);

/* Functions */
int 	juhe_get_totalItemNum(const char *strinput);
char* 	juhe_get_newsURL(const char *strinput, int index, const char *strkey);
void 	juhe_get_newsContent(const char* url);
void  	print_json_object(const json_object *json);

/*   ---------- juhe.cn  News Types -----------

  top(头条，默认),shehui(社会),guonei(国内),guoji(国际),yule(娱乐),tiyu(体育)
  junshi(军事),keji(科技),caijing(财经),shishang(时尚)

*/

static char* news_type[]=
{
   "guoji", "shishang", "keji", "guonei", "yule","top"
};


/*----------------------------
	    MAIN
-----------------------------*/
int main(int argc, char **argv)
{
	int i;
	int k;
	char *pstr=NULL;
	char *purl=NULL;
        static char strRequest[256+64];

	char *thumb_path="/tmp/thumb.jpg"; /* temp. thumb pic file */
	char pngNews_path[32];		   /* png news files */
	int  totalItems;		   /* total news items in one returned session */
	char attrMark[128];		   /* JUHE Mark */
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
#if 0
        printf("tm_start_egitick()...\n");
        tm_start_egitick();                     /* start sys tick */
#endif

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
        pad=egi_imgbuf_create(50, 320, 175, WEGI_COLOR_GYBLUE);

        /* read key from EGI config file */
        egi_get_config_value("JUHE_NEWS", "key", strkey);

//	juhe_get_newsContent("/tmp/yule_1.html");
//	exit(0);


while(1) { /////////////////////////	  LOOP TEST      ////////////////////////////

 for(k=0; k< sizeof(news_type)/sizeof(char *); k++ ) {
	/* Clear returned data buffer */
       	memset(buff,0,sizeof(buff));
	totalItems=0;

	/* Check whether type_0.png exists, to deduce that this type of news already downloaded */
	memset(pngNews_path,0,sizeof(pngNews_path));
	snprintf(pngNews_path, sizeof(pngNews_path),"/tmp/%s_0.png",news_type[k]);

	/* If file does NOT exist, then start Https GET request. */
	if( access(pngNews_path, F_OK) !=0 )
	{
		/* prepare GET request string */
        	memset(strRequest,0,sizeof(strRequest));
	        strcat(strRequest,"https://v.juhe.cn/toutiao/index?type=");
		strcat(strRequest, news_type[k]); //argv[1]);
	        strcat(strRequest,"&key=");
        	strcat(strRequest,strkey);

		printf("\n\n ------- [%s] News API powered by www.juhe.cn  ------ \n\n", news_type[k]);
        	//printf("Request:%s\n", strRequest);

	        /* Https GET request */
	        if(https_curl_request(strRequest, buff, NULL, curlget_callback)!=0) {
        	        printf("Fail to call https_curl_request()!\n");
                	//return -1;  Go on....
	        }
        	printf("	Http GET reply:\n %s\n",buff);

		totalItems=juhe_get_totalItemNum(buff);
		printf("\n  -----  Return total %d news items ----- \n\n", totalItems);

	} else {
		printf("\n\n ------- News type [%s] already downloaded  ----- \n\n", news_type[k]);
	}

   	/* Get top N items for each type of news */
	if(totalItems==0)	/* if 0, then try to use saved images */
		totalItems=30;

	for(i=0; i<totalItems; i++) {
		fb_clear_backBuff(&gv_fb_dev, WEGI_COLOR_BLACK);

		purl=juhe_get_newsURL(buff, i, "url");
		printf("	  url:%s\n", purl);

		/* Set PNG news picture name string */
		memset(pngNews_path,0,sizeof(pngNews_path));
		snprintf(pngNews_path, sizeof(pngNews_path),"/tmp/%s_%d.png",news_type[k],i);

		/* If file exists, display and continue for(i) */
		if( access(pngNews_path, F_OK)==0 ) {
			printf(" ---  News image file exists!  --- \n");

	             /* readin file */
	             imgbuf=egi_imgbuf_readfile(pngNews_path);
        	     if(imgbuf != NULL)
		     {
			printf("display readin file %s\n", pngNews_path);
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
			fb_page_refresh_flyin(&gv_fb_dev, 10);
			printf("tm_delayms...\n");
			//tm_delayms(6000);
			sleep(3);

			/* free */
			egi_imgbuf_free(imgbuf);
			imgbuf=NULL;

			continue;  /* Go back to continue for(i) .... */

		     } /* END if( imgbuf != NULL ) */

		} /* END if file exists */

		/* --- Get thumbnail pic URL and download it --- */
		/* Get thumbnail URL */
		pstr=juhe_get_newsURL(buff,i,"thumbnail_pic_s");
		if(pstr == NULL) {
		    #if 1
		    printf("News type [%s] item[%d]: thumbnail URL not found, try next item...\n",
											news_type[k], i );
		    continue;	/* continue for(i) */
		    #else
		    printf("News type [%s] item[%d]: thumbnail URL not found, skip to next news type...\n",
											news_type[k], i );
		    break;	/* Jump out of for(i) */
		    #endif
		}

		/* Download thumbnail pic */
		printf("Start https easy download URL: %s\n", pstr);
		https_easy_download(pstr, thumb_path, NULL, download_callback);
		free(pstr); pstr=NULL;

        	/* read in the thumbnail pic file */
        	imgbuf=egi_imgbuf_readfile(thumb_path);
       		if(imgbuf==NULL) {
	                printf("Fail to read image file '%s'.\n", thumb_path);
               		continue; /* Continue for(i) */
	        }

		#if 0 /* Not necessary for Landscape mode */
		/* rotate the imgbuf */
		egi_imgbuf_rotate_update(&imgbuf, 90);
		/* resize */
		egi_imgbuf_resize_update(&imgbuf, 240,320);
		#endif

	        /* display the thumbnail  */
	        egi_imgbuf_windisplay( imgbuf, &gv_fb_dev, -1,         /* img, FB, subcolor */
               		                0, 0,                            /* int xp, int yp */
                       			0, 0, imgbuf->width, imgbuf->height   /* xw, yw, winw,  winh */
                       		      );

		/* display pad for words writing */
	        egi_imgbuf_windisplay(  pad, &gv_fb_dev, -1,         /* img, FB, subcolor */
               		                0, 0,                            /* int xp, int yp */
                       			0, 240-45, imgbuf->width, imgbuf->height   /* xw, yw, winw,  winh */
                       		      );
		/* draw a red line at top of pad */
		#if 0
		fbset_color(WEGI_COLOR_ORANGE);
		draw_wline_nc(&gv_fb_dev, 0, 240-45, 320-1, 240-45, 1);
		#endif

		/* Free the imgbuf */
	        egi_imgbuf_free(imgbuf);imgbuf=NULL;
			//tm_delayms(3000);


		/* --- Get news title and display it --- */
		pstr=juhe_get_newsURL(buff, i, "title");
		if(pstr==NULL) {
			#if 1
			printf("News type [%s] item[%d]: Title not found, try next item...\n",
											news_type[k], i );
			continue;	/* continue for(i) */
			#else
			printf("News type [%s] item[%d]: Title not found, skip to next news type...\n",
											news_type[k], i );
			break;  	/* Jump out of for(i) */
			#endif
		}

		/* Prepare attribute mark string */
		memset(attrMark, 0, sizeof(attrMark));
		sprintf(attrMark, "%s_%d:  ",news_type[k], i);
		strcat(attrMark,"Powered by www.juhe.cn");
        	FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.regular,     /* FBdev, fontface */
                                     16, 16, attrMark,      		/* fw,fh, pstr */
                                     320-10, 1, 5,                       /* pixpl, lines, gap */
                                     5, 0,          /* x0,y0, */
                                     WEGI_COLOR_GRAY, -1, -1 );  /* fontcolor, transcolor,opaque */

        	FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.bold,     /* FBdev, fontface */
                                     15, 15, (const unsigned char *)pstr,      /* fw,fh, pstr */
                                     320-10, 3, 5,   //240-10,3,5          /* pixpl, lines, gap */
                                     5, 240-45+5,      //5,320-75,          /* x0,y0, */
                                     WEGI_COLOR_WHITE, -1, -1 );  /* fontcolor, transcolor,opaque */

		printf(" ----------- %s News, Item %d ---------- \n", news_type[k], i);
		printf(" Title: %s\n", pstr);
		egi_free_char(&pstr);
		//free(pstr); pstr=NULL;

		/* Refresh FB page */
		printf("FB page refresh ...\n");
		//fb_page_refresh(&gv_fb_dev);
		fb_page_refresh_flyin(&gv_fb_dev, 10);
		//tm_delayms(3000);
		/* save FB data to a PNG file */
		egi_save_FBpng(&gv_fb_dev, pngNews_path);

		/* Hold on for a while */
		sleep(2);
		//printf("Press a key to continue. \n");
		//getchar();

		/* Get news content and display it */
		juhe_get_newsContent(purl);
		egi_free_char(&purl);
		//free(purl); purl

	} /* END for(i) */
 } /* END for(k) */

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
#if 0
        printf("egi_quit_log()...\n");
        egi_quit_log();
#endif
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

/*--------------------------------------------------------------------------
Parse JUHE.cn returned data and get length of 'data' array, as total number
of news items in the strinput.

@strinput       input juhe.cn free news return  string

Return:
        Total number of news items in the strinput.
--------------------------------------------------------------------------*/
int juhe_get_totalItemNum(const char *strinput)
{
	int total=0;
        json_object *json_input=NULL;
        json_object *json_result=NULL;
        json_object *json_array=NULL;

        /* parse returned string */
        json_input=json_tokener_parse(strinput);
        if(json_input==NULL) {
		printf("%s: json_input is NULL!\n",__func__);
		goto GET_FAIL;
	}

        json_object_object_get_ex(json_input,"result",&json_result); /* Ref count NOT change */
        if(json_result==NULL) {
		printf("%s: json_result is NULL!\n",__func__);
		goto GET_FAIL;
	}

	json_object_object_get_ex(json_result,"data",&json_array);
        if(json_array==NULL) {
		printf("%s: json_array is NULL!\n",__func__);
		goto GET_FAIL;
	}

	total=json_object_array_length(json_array);

GET_FAIL:
	/* Free json obj */
        json_object_put(json_input);

	return total;
}


/*--------------------------------------------------------------------------------------------
Parse juhe.cn news json string and return string pointer to the vale of specified strkey of
data[index], or to data[index] if strkey is NULL.

 		-----  JUHE.cn returned data format -----
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
                                "thumbnail_pic_s":"http:\/\/06imgmini.eastday.com\/mobile\/20191119\/2019111915$
                        },
			... ...  data array ...
		...
	...
}

Note:
        !!! Don't forget to free the returned string pointer !!!

@strinput       input juhe.cn free news return string
@index		index of news array data[]
@strkey         key name of the news items in data[index]. Example: "uniquekey","title","url"...
		See above juhe data format.
                Note: if strkey==NULL, then return string pointer to data[index].

Return:
        0       ok
        <0      fails
----------------------------------------------------------------------------------------------*/
char* juhe_get_newsURL(const char *strinput, int index, const char *strkey)
{
        char *pt=NULL;

        json_object *json_input=NULL;
        json_object *json_result=NULL;
        json_object *json_array=NULL; /* Array of news items */
        json_object *json_data=NULL;
        json_object *json_get=NULL;

        /* parse returned string */
        json_input=json_tokener_parse(strinput);
        if(json_input==NULL) goto GET_FAIL;

        /* strip to get array data[]  */
        json_object_object_get_ex(json_input,"result",&json_result); /* Ref count NOT change */
        if(json_result==NULL)goto GET_FAIL;

	json_object_object_get_ex(json_result,"data",&json_array);
        if(json_array==NULL)goto GET_FAIL;

	/* Get an item by index from the array , TODO: limit index */
	json_data=json_object_array_get_idx(json_array,index);  /* Title array itmes */
	if(json_data==NULL)goto GET_FAIL;
	//print_json_object(json_data);

        /* if strkey, get key obj */
        if(strkey!=NULL) {
                json_object_object_get_ex(json_data, strkey, &json_get);
                if(json_get==NULL)goto GET_FAIL;
        } else {
                json_get=json_data;
        }

        /* Get pointer to the item string */
        pt=strdup((char *)json_object_get_string(json_get));


GET_FAIL:
        json_object_put(json_input);
	json_object_put(json_data);

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



/*----------------------------------------------------------
Get html text from URL or a local html file, and extract
paragraphs content.

Limit:
1. Only try one HTTP request session and max. buffer is 32k.

@url:	  URL address of html
	  If it contains "//", it deems as a web address and
	  will call https_curl_request() to get content.
	  Else, it is a local html file.

-----------------------------------------------------------*/
void juhe_get_newsContent(const char* url)
{
	int i;
	int fd;
	int fsize=0;
	struct stat  sb;
	bool Is_FilePath=false;
	char buff[32*1024]; /* For returned html text */
        char *content=NULL;
        int len;
        char *pstr=NULL;
	EGI_FILO* filo=NULL; /* to buffer content pointers */

	if(url==NULL)
		return;

	/* init filo */
	filo=egi_malloc_filo(8, sizeof(char *), FILO_AUTO_DOUBLE);
	if(filo==NULL)
		return;

	/* Check if it's web address or file path */
   	if( strstr(url,"//") == NULL )
		Is_FilePath=true;
	else
		Is_FilePath=false;

	/* For Web address */
	if( Is_FilePath==false )
	{
        	/* clear buff */
	        memset(buff,0,sizeof(buff));
        	/* Https GET request */
	        if(https_curl_request(url, buff, NULL, curlget_callback)!=0) {
        	         printf("Fail to call https_curl_request()!\n");
			 goto FUNC_END;
		}

		/* assign buff to pstr */
        	pstr=buff;

	}
	/* For file path */
	else
	{
	       /* open local file and mmap */
        	fd=open(url,O_RDONLY);
	        if(fd<0) {
			printf("%s: Fail to open file '%s'\n", __func__, url);
                	goto FUNC_END;
	        }
        	/* obtain file stat */
	        if( fstat(fd,&sb)<0 ) {
			printf("%s: Fail to get fstat of file '%s'\n", __func__, url);
                	goto FUNC_END;
	        }
        	fsize=sb.st_size;
	        pstr=mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
        	if(pstr==MAP_FAILED) {
			printf("%s: Fail to mmap file '%s'\n", __func__, url);
	                goto FUNC_END;
        	}
	}

        /* Parse HTML */
        do {
                /* parse returned data as html */
                pstr=cstr_parse_html_tag(pstr, "p", &content, &len);
                if(content!=NULL) {
                        //printf("%s\n",content);
			egi_filo_push(filo,&content);
		}
                //egi_free_char(&content);
                //printf("Get %d bytes content: %s\n", len, content);
        } while( pstr!=NULL );

	/* read filo and get pointer to paragraph content */
	for(i=0; egi_filo_read(filo, i, &content)==0; i++ )
	{
		printf(" ----- paragraph %d -----\n", i);
		printf("%s\n",content);
		/* Write to FB */
		draw_filled_rect2(&gv_fb_dev, WEGI_COLOR_LTYELLOW, 0, 0, 320-1, 240-45-1);
        	FTsymbol_uft8strings_writeFB(&gv_fb_dev, egi_appfonts.bold,       /* FBdev, fontface */
                                     18, 18, (const unsigned char *)content,      /* fw,fh, pstr */
                                     320-10, (240-45)/(18+6), 6,      	    /* pixpl, lines, gap */
                                     5, 4,      //5,320-75,          /* x0,y0, */
                                     WEGI_COLOR_BLACK, -1, -1 );  /* fontcolor, transcolor,opaque */
		fb_page_refresh(&gv_fb_dev);

		sleep(2);
	}


FUNC_END:
	/* free all content pointers in FILO  */
	while( egi_filo_pop(filo, &content)==0 )
		free(content);

	/* close file and mumap */
	if(Is_FilePath) {
		close(fd);
		munmap(pstr,fsize);
	}

	/* free filo */
	egi_free_filo(filo);
}
