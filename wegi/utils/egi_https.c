/*----------------------------------------------------
		A libcurl http helper
Refer to:  https://curl.haxx.se/libcurl/c/

Midas Zhou
-----------------------------------------------------*/
#include <stdio.h>
#include <curl/curl.h>
#include "egi_https.h"


/*------------------------------------------------------------------------------
			HTTPS request by libcurl

Note: You must have installed ca-certificates before call curl https, or
      define SKIP_PEER/HOSTNAME_VERIFICATION to use http instead.

@request:	request string
@reply_buff:	returned reply string buffer, the Caller must ensure enough space.
@data:		TODO: if any more data needed

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
	curl_easy_setopt(curl, CURLOPT_VERBOSE,1);			 /* print more detail */
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

