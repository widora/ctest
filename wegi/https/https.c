/*--------------------------------------------------------------------------
		A http example from libcurl source codes

Refer to:  https://curl.haxx.se/libcurl/c/

Midas Zhou
--------------------------------------------------------------------------*/
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include "egi_cstring.h"

#define _SKIP_PEER_VERIFICATION
#define _SKIP_HOSTNAME_VERIFICATION

const char host[]= "free-api.heweather.net";
char request[256]="https://free-api.heweather.net/s6/weather/now?location=shanghai&key=";
char strkey[256];
char buff[1024*1024];


/* a callback function to handler returned data */
size_t callback_get(void *ptr, size_t size, size_t nmemb, void *userp)
{
	strcat(userp,ptr);
	return size*nmemb;
}


int main(void)
{
  int i=0;
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  /* read key from EGI config file */
  egi_get_config_value("EGI_WEATHER", "key", strkey);
  printf("key: %s\n",strkey);
  strcat(request,strkey);
  printf("request: %s\n",request);

  if(curl) {
    i++;
    printf("start curl_easy %d\n",i);

    curl_easy_setopt(curl, CURLOPT_URL, request);		 /* set request URL */
    curl_easy_setopt(curl, CURLOPT_VERBOSE,1);			 /* print more detail */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);			 /* set timeout */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_get); /* set write_callback */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buff); 		 /* set data dest for write_callback */

#ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
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

  printf("------------------- result --------------\n");
  printf("%s\n", buff);

  curl_global_cleanup();

  return 0;
}
