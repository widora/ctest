/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include <stdio.h>
#include "http_client.h"
#include "cJSON.h"
#include "httpclient.h"

#define BUF_SIZE                                          (1024 * 1)
#define HTTPS_MTK_CLOUD_POST_URL                 "http://10.0.1.173:5000/dev_reg_v1"
//#define HTTPS_MTK_CLOUD_POST_URL       "http://www.imio.ai/ai-bot/stt"
#define HTTPS_POST_TIME_TICK                 5000

#define PRODUCT_ID "mt7687_18"
#define PRODUCT_SECRET "01234567898"
#define DEV_ID "zhuyunchun-18"
#define DEV_KEY "b23c34bacedd3548435d92d405a88afb"

static httpclient_t client = {0};
static int post_count = 5; 


/**
  * @brief      Send/Recv package by "post" method.
  * @param      None
  * @return     0, if OK.\n
  *             <0, Error code, if errors occurred.\n
  */
static HTTPCLIENT_RESULT httpclient_register_keepalive_post(void)
{
    //1. Ready the post data
    HTTPCLIENT_RESULT ret = HTTPCLIENT_ERROR_CONN;
    char *post_url = HTTPS_MTK_CLOUD_POST_URL;
    //char *header = HTTPS_MTK_CLOUD_HEADER;
    httpclient_data_t client_data = {0};
    char *buf;
    char *content_type = "application/json";
    char post_data[32];

    buf = malloc(BUF_SIZE);
    if (buf == NULL) {        
        //LOG_I(http_client_keepalive_example, "memory malloc failed.");
        return ret;
    }

    //2. set json param
    char *product_id = PRODUCT_ID;
    char *product_secret = PRODUCT_SECRET;
    char *dev_id = DEV_ID;
    char *out;

    cJSON *root_reg;
    cJSON *signup_req;
    root_reg = cJSON_CreateObject();
    cJSON_AddItemToObject(root_reg, "signup_req", signup_req = cJSON_CreateObject());
    cJSON_AddStringToObject(signup_req, "product_id", product_id);
    cJSON_AddStringToObject(signup_req, "product_secret", product_secret);
    cJSON_AddStringToObject(signup_req, "dev_id", dev_id);
    out = cJSON_Print(root_reg); /* Print to text */
    cJSON_Delete(root_reg);      /* Delete the cJSON object */
    ptintf("out = %s\n", out); /* Print out the text */

    client_data.response_buf = buf;
    client_data.response_buf_len = BUF_SIZE;        
    client_data.post_content_type = content_type;
    sprintf(post_data, "1,,temperature:%d", (10 + post_count));
    //client_data.post_buf = post_data;
    //client_data.post_buf_len = strlen(post_data);
    client_data.post_buf = out;
    client_data.post_buf_len = strlen(out);
    //httpclient_set_custom_header(&client, header);
    cJSON_free(out);         /* Release the string. */
    
    //3. Begin the http post
    ret = httpclient_send_request(&client, post_url, HTTPCLIENT_POST, &client_data);
    if (ret < 0)                     
        goto fail;
    
    ret = httpclient_recv_response(&client, &client_data);
    if (ret < 0)        
        goto fail;
    //LOG_I(http_client_keepalive_example, "data received: %s", client_data.response_buf);
    cJSON *root = cJSON_Parse(client_data.response_buf);
    cJSON *object = cJSON_GetObjectItem(root, "signup_rep"); 
    cJSON *Item = cJSON_GetObjectItem(object, "dev_key"); 
    printf("data received: %s", Item->valuestring);
    cJSON_Delete(root);

fail:    
    vPortFree(buf); 
    return ret;
}

/**
  * @brief      Http client connect to test server and test "keepalive" function.
  * @param      None
  * @return     None
  */
void httpclient_register_keepalive(void)
{
    HTTPCLIENT_RESULT ret = HTTPCLIENT_ERROR_CONN;
    char *post_url = HTTPS_MTK_CLOUD_POST_URL;
         
    // Connect to server.
    ret = httpclient_connect(&client, post_url);
    if (ret < 0)                     
        goto fail;

    // Send "keepalive" package by "post" method.
    ret = httpclient_register_keepalive_post();
    if (ret < 0)                     
        goto fail;
    
fail:
    // Close http connection
    httpclient_close(&client);

}

