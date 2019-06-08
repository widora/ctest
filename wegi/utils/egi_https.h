/*----------------------------------------------------
		A libcurl http helper
Refer to:  https://curl.haxx.se/libcurl/c/

Midas Zhou
-----------------------------------------------------*/
#ifndef __EGI_HTTPS__
#define __EGI_HTTPS__

#include <stdio.h>
#include <curl/curl.h>

/*  erase '__' to use http instead */
#define __SKIP_PEER_VERIFICATION
#define __SKIP_HOSTNAME_VERIFICATION

/* a callback function for CURL to handle returned data */
typedef size_t (* curlget_callback_t)(void *ptr, size_t size, size_t nmemb, void *userp);
/*
static size_t curlget_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	strcat(userp,ptr);
	return size*nmemb;
}
*/

int https_curl_request(const char *request, char *reply_buff, void *data,
							curlget_callback_t get_callback);


#endif
