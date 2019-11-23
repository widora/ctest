/*----------------------------------------------------
		A libcurl http helper
Refer to:  https://curl.haxx.se/libcurl/c/

Midas Zhou
-----------------------------------------------------*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <curl/curl.h>
#include "egi_https.h"


/*------------------------------------------------------------------------------
			HTTPS request by libcurl

Note: You must have installed ca-certificates before call curl https, or
      define SKIP_PEER/HOSTNAME_VERIFICATION to use http instead.

@request:	request string
@reply_buff:	returned reply string buffer, the Caller must ensure enough space.
@data:		TODO: if any more data needed

		!!! CURL will disable egi tick timer? !!!
Return:
	0	ok
	<0	fails
--------------------------------------------------------------------------------*/
int https_curl_request(const char *request, char *reply_buff, void *data,
								curlget_callback_t get_callback)
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
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);			 /* 1 print more detail */
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);			 /* set timeout */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_callback);     /* set write_callback */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, reply_buff); 		 /* set data dest for write_callback */
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


/*----------------------------------------------------------------------------
			HTTPS request by libcurl

Note: You must have installed ca-certificates before call curl https, or
      define SKIP_PEER/HOSTNAME_VERIFICATION to use http instead.

@file_url:		file url for downloading.
@file_save:		file path for saving received file.
@data:			TODO: if any more data needed
@write_callback:	Callback for writing received data

		!!! CURL will disable egi tick timer? !!!

Return:
	0	ok
	<0	fails
-------------------------------------------------------------------------------*/
int https_easy_download(const char *file_url, const char *file_save,   void *data,
							      curlget_callback_t write_callback )
{
	int ret=0;
  	CURL *curl;
  	CURLcode res;
	FILE *fp;	/* FILE to save received data */

	/* Open file for saving file */
	fp=fopen(file_save,"wb");
	if(fp==NULL) {
		printf("%s: open file %s: %s", __func__, file_save, strerror(errno));
		return -1;
	}

	/* init curl */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl==NULL) {
		printf("%s: Fail to init curl!\n",__func__);
		ret=-2;
		goto CURL_FAIL;
	}

	/***   --- Set options for CURL  ---  ***/
	/* set download file URL */
	curl_easy_setopt(curl, CURLOPT_URL, file_url);
	/*  1L --print more detail,  0L --disbale */
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
   	/*  1L --disable progress meter, 0L --enable and disable debug output  */
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	/*  1L --enable redirection */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	/*  set timeout */
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

	/* set write_callback to write/save received data  */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

#ifdef SKIP_PEER_VERIFICATION
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef SKIP_HOSTNAME_VERIFICATION
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
	//curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");

	/* Set data destination for write_callback */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);
	if(res != CURLE_OK) {
		printf("%s: curl_easy_perform() failed: %s\n", __func__, curl_easy_strerror(res));
		ret=-3;
		goto CURL_FAIL;
	}

CURL_FAIL:
	/* Close file */
	fclose(fp);

	/* Always cleanup */
	curl_easy_cleanup(curl);
  	curl_global_cleanup();

	return ret;
}
