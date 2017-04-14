/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include "http_client.h"
#include <stdio.h>


#define BUF_SIZE                                          (1024 * 1)
#define HTTPS_MTK_CLOUD_POST_URL       "http://www.imio.ai/ai-bot/stt"
#define HTTPS_POST_TIME_TICK                 5000


static httpclient_t client = {0};
static int post_count = 5; 


