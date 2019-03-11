/*------------------  iot_client.c  ---------------------------
A simple example to log on BIGIOT and keep alive.

Midas Zhou
--------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <json.h>
#include "egi_iotclient.h"
#include "../egi.h"
#include "../egi_log.h"
#include "../egi_page.h"
#include "../egi_color.h"

#define BUFFSIZE 2048
#define BULB_OFF_COLOR 	0x000000 	/* default color when bulb turn on */
#define BULB_ON_COLOR 	0xBBBBBB   	/* default color when buld turn off */
#define BULB_COLOR_STEP 0x111111	/* step for turn up and turn down */

static int sockfd;
//static EGI_16BIT_COLOR iotbtn_subcolor=0;//WEGI_COLOR_BLACK;
static EGI_24BIT_COLOR subcolor=0;
//static uint16_t colorseg;
static bool bulb_off=true;
/*
  1. After receive command from the server, push it to incmd[] and confirm server for the reception.
  2. After incmd[] executed, set flag incmd_executed to be true.
  3. Reply server to confirm result of execution.
*/
struct egi_iotdata
{
	//iot_status status;
	bool	connected;
	char 	incmd[64];
	bool	incmd_executed; /* set to false once after renew incmd[] */
};


/* routine json models */
const static char *strjson_checkin= "{\"M\":\"checkin\",\"ID\":421, \"K\":\"f80ea043e\"}\n";
const static char *strjson_keepalive= "{\"M\":\"beat\"}\n";
const static char *strjson_reply= "{\"M\":\"say\",\"C\":\"Hello from Widora_NEO\",\"SIGN\":\"Widora_NEO\"}";//\n";


/*----------------------------------------------------------------
Get pionter to a string type key value from an input json object

json:	a json object.
key:	key name of a json object

return:
	a char pointer	OK
	NULL		Fail
----------------------------------------------------------------*/
const static char *get_jsonkey_pstrval(json_object *json, const char* key)
{
	json_object *json_item=NULL;

	/* get item object */
	//json_item=json_object_object_get(json, key);
	if( json_object_object_get_ex(json, key, &json_item)==false )
	{
		printf("Fail to get an object with the key name from the input json object.\n");
		return NULL;
	}
	else
	{
		printf("get_jsonkey_pstrval(): object key '%s' with value '%s'\n",
							key, json_object_get_string(json_item));
	}
	/* check json object type */
	if(!json_object_is_type(json_item, json_type_string))
	{
		printf("get_jsonkey_pstrval(): Error, json_item is NOT a string type object!\n");
		return NULL;
	}

	/* get a pointer to string value */
	return json_object_get_string(json_item); /* return a pointer */
}


/*-----------------------------------
Send hear-beat msg to BIGIOT server
-----------------------------------*/
static void bigiot_keepalive(void)
{
	int ret;
	int i;
	time_t t;
	struct tm *tm;

	while(1)
	{
		/* idle time */
		for(i=0;i<150;i++)
			usleep(100000);

		/* send heart_beat msg */
		ret=send(sockfd, strjson_keepalive, strlen(strjson_keepalive), MSG_CONFIRM);
		if(ret<0) {
			printf("Fail to send heart_beat msg to BIGIOT!\n");
			perror("send keepalive msg to BIGIOT.");
		}
		else {
		        /* get time stamp */
        		t=time(NULL);
        		tm=localtime(&t);
        		/* time stamp and msg */
        		printf("[%d-%02d-%02d %02d:%02d:%02d] ",
                                tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour, tm->tm_min,tm->tm_sec );
			printf("%s: A heart_beat msg is sent to BIGIOT.\n",__func__);
		}
	}
}

/*-----------  Page Runner Function  ------------
BIGIOT client
------------------------------------------------*/
void egi_iotclient(EGI_PAGE *page)
{
	int ret;
	char recv_buf[BUFFSIZE]={0};
	struct sockaddr_in svr_addr;

	json_object *json=NULL;
	json_object *json_item=NULL;
	json_object *json_reply=NULL;
	char tmpbuf[126]={0};
	char *strreply=NULL;
	const char *pstrIDval=NULL; /* key ID string pointer */
	const char *pstrCval=NULL; /* key C string pointer*/
	const char *pstrtmp=NULL;

	pthread_t  	pthd_keepalive;


	/* get related ebox form the page, id number for the IoT button is 7 */
	EGI_EBOX *iotbtn=egi_page_pickebox(page, type_btn, 7);
	if(iotbtn == NULL)
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to pick the IoT button in page '%s'\n.",
									__func__,  page->ebox->tag);
		return;
	}
	egi_btnbox_setsubcolor(iotbtn, COLOR_24TO16BITS(BULB_OFF_COLOR)); /* set subcolor */
	egi_ebox_needrefresh(iotbtn);
	egi_ebox_refresh(iotbtn);
	printf("egi_iotclient: got iot button '%s' and reset subcolor.\n", iotbtn->tag);


	/* prepare jason replay */
	json_reply=json_tokener_parse(strjson_reply);
	if(json_reply == NULL)
	{
		printf("egi_iotclient: fail to prepare json_reply.\n");
		return;
	}
	printf("prepare json_reply as: %s\n",json_object_to_json_string(json_reply));

	/* create a socket file descriptor */
	while( (sockfd=socket(AF_INET,SOCK_STREAM,0)) <0 )
	{
		perror("try to create socket file descriptor");

	}
	printf("Succeed to create a socket file descriptor!\n");

	/* set IOT server address */
	svr_addr.sin_family=AF_INET;
	svr_addr.sin_port=htons(8181);
	svr_addr.sin_addr.s_addr=inet_addr("121.42.180.30"); //www.bigiot.net");
	bzero(&(svr_addr.sin_zero),8);

	/* connect to BIGIOT. */
	while( connect(sockfd,(struct sockaddr *)&svr_addr, sizeof(struct sockaddr)) <0 )
	{
		perror("connect to server");
	}
	printf("Succeed to connect to BIGIOT!");
	ret=recv(sockfd, recv_buf, BUFFSIZE, 0);
	recv_buf[ret]='\0';
	printf("Reply from the server: %s\n",recv_buf);

	/* send json string for checkin to BIGIO */
	ret=send(sockfd, strjson_checkin, strlen(strjson_checkin), MSG_CONFIRM);
	if(ret<0)
	{
		printf("Fail to send msg to BIGIOT!\n");
		perror("send message to BIGIOT.");
	}
	else
	{
		printf("%d bytes of totally %d bytes message has been sent to BIGIOT..\n",
									ret, strlen(strjson_checkin));
	}

	/* launch keepalive thread, BIGIOT */
	if( pthread_create(&pthd_keepalive, NULL, (void *)bigiot_keepalive, NULL) !=0 )
        {
                printf("Fail to create bigiot_keepalive thread!\n");
                goto fail;
        }
	else
	{
		printf("Create bigiot_keepavlie thread successfully!\n");
		/* get reply from the server */
		ret=recv(sockfd, recv_buf, BUFFSIZE, 0);
		recv_buf[ret]='\0';
		printf("Reply from the server: %s\n",recv_buf);
	}

	/* ------------ IoT Talk Loop:  loop recv and send processing ------------ */
	while(1)
	{
	   	if( (ret=recv(sockfd,recv_buf,BUFFSIZE,0)) >0 )
		{
			recv_buf[ret]='\0';
			printf("Message from the server: %s\n",recv_buf);

			/* parse string for json */
			json=json_tokener_parse(recv_buf);/* _parse() creates a new json obj */
			if(!json)
			{
				printf("egi_iotclient: Fail to parse received string by json_tokener_parse()!\n");
			}
			else /* extract items and values */
			{
//////----- get key value of a json string object  ----
				pstrIDval=get_jsonkey_pstrval(json, "ID");
				/* Need to check pstrIDvall ????? */
				if(pstrIDval==NULL)
				{
					printf("egi_ioclient: key 'ID' has no value! continue...\n");
					continue;
				}
//////----- renew reply_json's ID value: with visitor's ID value   ----
				/* delete old "ID" key, then add own ID for reply  */
				json_object_object_del(json_reply, "ID"); /* */
				//printf("json_object_object_add()...\n");
				json_object_object_add(json_reply, "ID", json_object_new_string(pstrIDval)); /* with strdup() inside */
				/* NOTE: json_object_new_string() with strdup() inside */

//////----- get IoT Command string and parse  ------
				pstrCval=get_jsonkey_pstrval(json, "C");/* get visiotr's Command value */
			if(pstrCval != NULL)
			{
				printf("receive command: %s\n",pstrCval);
				/* parse command string */
				if(strcmp(pstrCval,"offOn")==0)
				{
					printf("Execute command 'offOn' ....\n");
					/* toggle the subcolor */
					bulb_off=!bulb_off;
					if(bulb_off)
					{
						printf("Switch bulb OFF \n");
						subcolor=BULB_OFF_COLOR;
					}
					else
					{
						printf("Switch bulb ON \n");
						subcolor=BULB_ON_COLOR; /* mild white */
					}

				}
				if( !bulb_off && (  (strcmp(pstrCval,"plus")==0)
						    || (strcmp(pstrCval,"up")==0)  ) )
				{
  				    printf("Execute command 'plus' ....\n");
				    if(  subcolor <  0XFFFFFF ) /* up limit value */
				    {
					subcolor += BULB_COLOR_STEP;
					//colorseg= (COLOR_16TO24BITS(iotbtn_subcolor)) & 0xff;
					//iotbtn_subcolor=COLOR_RGB_TO16BITS( (colorseg+0x11), (colorseg+0x11),
					//						(colorseg+0x11) );
				    }
				}
				else if( !bulb_off && (  (strcmp(pstrCval,"up")==0)
						         ||(strcmp(pstrCval,"down")==0)  ) )
				//else if( !bulb_off && (strcmp(pstrCval,"minus")==0) )
				{
				    printf("Execute command 'minus' ....\n");
				    //if(iotbtn_subcolor <= COLOR_RGB_TO16BITS(0x66,0x66,0x66) ) /* low limit value */
				    //	iotbtn_subcolor=COLOR_RGB_TO16BITS(0x66,0x66,0x66);
				    if( subcolor < 0x222222 ) subcolor=0x222222; /* low limit */
				    else //if(iotbtn_subcolor >= COLOR_RGB_TO16BITS(0x66,0x66,0x66) ) /* low limit value */
				    {
					//colorseg= (COLOR_16TO24BITS(iotbtn_subcolor)) & 0xff;
					//iotbtn_subcolor=COLOR_RGB_TO16BITS( (colorseg-0x11), (colorseg-0x11),
					//						(colorseg-0x11) );
					subcolor -= BULB_COLOR_STEP;
				    }
				}

				printf("subcolor=0x%08X \n",subcolor);
				/* set subcolor to iotbtn */
				egi_btnbox_setsubcolor(iotbtn, COLOR_24TO16BITS(subcolor)); //iotbtn_subcolor);
				/* refresh iotbtn */
				egi_ebox_needrefresh(iotbtn);
				egi_ebox_refresh(iotbtn);
			}
//////----- renew reply_json's C value: with message string for reply  ----
				memset(tmpbuf,0,sizeof(tmpbuf));
				sprintf(tmpbuf,"Command '%s' is confirmed.",pstrCval);
				json_object_object_del(json_reply, "C");
				json_object_object_add(json_reply,"C", json_object_new_string(tmpbuf));

//////----- generate corresponding reply string ----
				/* prepare reply string for socket send */
				pstrtmp=json_object_to_json_string(json_reply);/* json..() return a pointer */
				strreply=(char *)malloc(strlen(pstrtmp)+100+2); /* at least +2 for '/n/0' */
				if(strreply==NULL)
				{
					printf("egi_iotclient: fail to malloc strreply.\n");
					goto fail;
				}
				memset(strreply,0,strlen(pstrtmp)+2);
				//printf("sprintf strreply from json_object_to_json_string(json_reply)...\n");
				sprintf(strreply, "%s\n", json_object_to_json_string(json_reply) );/* json().. retrun a pointer */
				printf("reply json string: %s\n",strreply);

//////----- send reply string by sockfd ----
				/* reply to the visitor */
				ret=send(sockfd, strreply, strlen(strreply), MSG_CONFIRM);
				if(ret<0)
				{
					perror("reply to visitor by send()");
				}
				else
					printf("Reply to visitor successfully!\n");
			}

//////----- clear arean for next IoT talk ----
			/* free dupstr and deref json objects */
			if(strreply !=NULL )
				free(strreply);
			//printf("json_object_put(json)...\n");
			json_object_put(json); /* func with if(json) inside  */
			//printf("json_object_put(json_item)...\n");
			json_object_put(json_item); /* func with if(json) inside */
		}
		usleep(100000);
	}



	/* joint threads */
	pthread_join(pthd_keepalive,NULL);


fail:
	/* free var */
	if(strreply != NULL )
		free(strreply);
	/* release json object */
	json_object_put(json); /* func with if(json) inside */
	json_object_put(json_item); /* func with if(json) inside */
	//// put other json objs //////

	/* close socket FD */
	close(sockfd);

	return ;
}
