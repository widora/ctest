/*-----------------------  iot_client.c  ------------------------------
A simple example to connect BIGIOT and control a bulb icon.

TODO:
1. recv() may return serveral sessions of BIGIOT command at one time:
   [2019-03-12 10:21:52] [LOGLV_INFO] Message from the server: {"M":"say","ID":"Pc0a809a00a2900000b2b","NAME":"guest","C":"down","T":"1552357312"}
{"M":"say","ID":"Pc0a809a00a2900000b2b","NAME":"guest","C":"down","T":"1552357312"}
{"M":"say","ID":"Pc0a809a0[2019-03-12 10:22:03] [LOGLV_INFO] Message from the server: {"M":"say","ID":"Pc0a809a00a2900000b2b","NAME":"guest","C":"down","T":"1552357323"}
   or logger error?

  ret==0:
	1). WiFi disconnects, network is down.

  wifi down:
	1). Error performing SIOCGIWSTATS: Operation not supported
.

2. Since egi_iotclient run as a runner thread, when its host page exit,
   It will crash if old page is still referred.

Midas Zhou
---------------------------------------------------------------------*/
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
#include "egi.h"
#include "egi_log.h"
#include "egi_page.h"
#include "egi_color.h"
#include "egi_timer.h"

#define IOT_SERVER_ADDR		"121.42.180.30" /* WWW.BIGIOT.NET */
#define IOT_SERVER_PORT 	8181

#define BUFFSIZE 2048
#define BULB_OFF_COLOR 	0x000000 	/* default color for bulb turn_off */
#define BULB_ON_COLOR 	0xDDDDDD   	/* default color for bulb turn_on */
#define BULB_COLOR_STEP 0x111111	/* step for turn up and turn down */

static int sockfd;
static struct sockaddr_in svr_addr;


static EGI_24BIT_COLOR subcolor=0;
static bool bulb_off=false;
static char *bulb_status[2]={"ON","OFF"};

/*
  1. After receive command from the server, push it to incmd[] and confirm server for the receipt.
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
static void iot_keepalive(void)
{
	int ret;
	//int i;
	time_t t;
	struct tm *tm;

	while(1)
	{
		/* idle time */
		tm_delayms(15000);
		//for(i=0;i<150;i++)
		//	usleep(100000);

		/* send heart_beat msg */
		ret=send(sockfd, strjson_keepalive, strlen(strjson_keepalive), MSG_CONFIRM|MSG_NOSIGNAL);
		if(ret=<0) {
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

/*-------------------------------------------------
1. Step up socket FD
2. Connect to IoT server
3. Check in.

Return:
	0	OK
	<O	fails
--------------------------------------------------*/
static int iot_connect_checkin(void)
{
	int ret;
	char recv_buf[BUFFSIZE]={0}; /* for recv() buff */

   /* loop trying .... */
   for(;;)
   {
	tm_delayms(2000);
	EGI_PLOG(LOGLV_CRITICAL,"------ Start connect and checkin to ther server ------\n");

	/* create a socket file descriptor */
	close(sockfd); /*close it  first */
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		perror("try to create socket file descriptor");
		EGI_PLOG(LOGLV_ERROR,"%s :: socket(): %s \n",__func__,strerror(errno));

		continue; //return -1;
	}
	//printf("Succeed to create a socket file descriptor!\n");

	/* set IOT server address  */
	svr_addr.sin_family=AF_INET;
	svr_addr.sin_port=htons(IOT_SERVER_PORT);
	svr_addr.sin_addr.s_addr=inet_addr(IOT_SERVER_ADDR);
	bzero(&(svr_addr.sin_zero),8);

   	/* connect to socket. */
	ret=connect(sockfd,(struct sockaddr *)&svr_addr, sizeof(struct sockaddr));
	if(ret<0)
	{
		EGI_PLOG(LOGLV_ERROR,"%s :: connect(): %s \n",__func__,strerror(errno));
		continue;//return -2;
	}

	EGI_PLOG(LOGLV_CRITICAL,"%s: Succeed to connect to the BIGIOT socket!\n",__func__);
	memset(recv_buf,0,sizeof(recv_buf));
	ret=recv(sockfd, recv_buf, BUFFSIZE-1, 0);
	if(ret<=0)
	{
		EGI_PLOG(LOGLV_ERROR,"%s :: recv(): %s \n",__func__,strerror(errno));
		continue;
	}

	//recv_buf[ret]='\0';
	EGI_PLOG(LOGLV_CRITICAL,"Reply from the server: %s\n",recv_buf);

	/* send json string for checkin to BIGIO */
	EGI_PLOG(LOGLV_CRITICAL,"Start sending CheckIn msg to BIGIOT...\n");
	ret=send(sockfd, strjson_checkin, strlen(strjson_checkin), MSG_CONFIRM|MSG_NOSIGNAL);
	if(ret<=0)
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to send login request to BIGIOT:%s\n"
								,__func__,strerror(errno));
		continue;//return -3;
	}
	else
		EGI_PLOG(LOGLV_CRITICAL,"Checkin msg has been sent to BIGIOT.\n");

	/* wait for reply from the server */
	memset(recv_buf,0,sizeof(recv_buf));
	ret=recv(sockfd, recv_buf, BUFFSIZE-1, 0);
	if(ret<=0)
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to recv() CheckIn confirm msg from BIGIOT, %s\n"
								,__func__,strerror(errno));
		continue;//return -4;
	}
	else if( strstr(recv_buf,"checkinok") ==NULL ) {
		EGI_PLOG(LOGLV_ERROR,"Checkin confirm msg is NOT received.\n");
		continue;//return -5;
	}
	else {
		EGI_PLOG(LOGLV_CRITICAL,"Checkin confirm msg is received.\n");
	}

	EGI_PLOG(LOGLV_CRITICAL,"CheckIn reply from the server: %s\n",recv_buf);

	/* finally break the loop */
	break;

    }/* loop end */

	return 0;
}


/*----------------  Page Runner Function  ---------------------
Note: runner's host page may exit, so check *page to get status
BIGIOT client
--------------------------------------------------------------*/
void egi_iotclient(EGI_PAGE *page)
{
	int ret;
	char recv_buf[BUFFSIZE]={0}; /* for recv() buff */

	json_object *json=NULL;
	json_object *json_item=NULL;
	json_object *json_reply=NULL;

	char  keyC_buf[128]={0}; /* for key 'C' reply string*/
	char *strreply=NULL;
	const char *pstrIDval=NULL; /* key ID string pointer */
	const char *pstrCval=NULL; /* key C string pointer*/
	const char *pstrtmp=NULL;

	pthread_t  	pthd_keepalive;


	/* get related ebox form the page, id number for the IoT button */
	EGI_EBOX *iotbtn=egi_page_pickebox(page, type_btn, 7);
	if(iotbtn == NULL)
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to pick the IoT button in page '%s'\n.",
								__func__,  page->ebox->tag);
		return;
	}
	egi_btnbox_setsubcolor(iotbtn, COLOR_24TO16BITS(BULB_ON_COLOR)); /* set subcolor */
	egi_ebox_needrefresh(iotbtn);
	//egi_page_flag_needrefresh(page); /* !!!SLOW!!!, put flag, let page routine to do the refresh job */
	egi_ebox_refresh(iotbtn);
	printf("egi_iotclient: got iot button '%s' and reset subcolor.\n", iotbtn->tag);


	/* prepare json replay */
	json_reply=json_tokener_parse(strjson_reply);
	if(json_reply == NULL)
	{
		printf("egi_iotclient: fail to prepare json_reply.\n");
		return;
	}
	printf("prepare json_reply as: %s\n",json_object_to_json_string(json_reply));

/////////////////////
	iot_connect_checkin();
#if 0
	/* create a socket file descriptor */
	close(sockfd); /*close it  first */
	while( (sockfd=socket(AF_INET,SOCK_STREAM,0)) <0 )
	{
		perror("try to create socket file descriptor");
		EGI_PLOG(LOGLV_ERROR,"%s socket(): %s \n",__func__,strerror(errno));
		return;
	}
	//printf("Succeed to create a socket file descriptor!\n");

	/* set IOT server address  */
	svr_addr.sin_family=AF_INET;
	svr_addr.sin_port=htons(IOT_SERVER_PORT);
	svr_addr.sin_addr.s_addr=inet_addr(IOT_SERVER_ADDR);
	bzero(&(svr_addr.sin_zero),8);

   	/* connect to socket. */
	while( connect(sockfd,(struct sockaddr *)&svr_addr, sizeof(struct sockaddr)) <0 )
	{
		EGI_PLOG(LOGLV_ERROR,"%s connect(): %s \n",__func__,strerror(errno));
		return;
	}

	EGI_PLOG(LOGLV_CRITICAL,"%s: Succeed to connect to the BIGIOT socket!\n",__func__);
	memset(recv_buf,0,sizeof(recv_buf));
	ret=recv(sockfd, recv_buf, BUFFSIZE-1, 0);
	//recv_buf[ret]='\0';
	EGI_PLOG(LOGLV_CRITICAL,"Reply from the server: %s\n",recv_buf);

	/* send json string for checkin to BIGIO */
	EGI_PLOG(LOGLV_CRITICAL,"Start sending CheckIn msg to BIGIOT...\n");
	ret=send(sockfd, strjson_checkin, strlen(strjson_checkin), MSG_CONFIRM|MSG_NOSIGNAL);
	if(ret<=0)
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to send login request to BIGIOT:%s\n"
								,__func__,strerror(errno));
		return;
	}
	else
		EGI_PLOG(LOGLV_CRITICAL,"Checkin msg has been sent to BIGIOT.\n");

	/* wait for reply from the server */
	memset(recv_buf,0,sizeof(recv_buf));
	ret=recv(sockfd, recv_buf, BUFFSIZE-1, 0);
	if(ret<=0)
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to recv() CheckIn confirm msg from BIGIOT, %s\n"
								,__func__,strerror(errno));
		return;
	}
	else if( strstr(recv_buf,"checkinok") ==NULL ) {
		EGI_PLOG(LOGLV_ERROR,"Checkin confirm msg is NOT received.\n");
	}
	else {
		EGI_PLOG(LOGLV_CRITICAL,"Checkin confirm msg is received.\n");
	}

	EGI_PLOG(LOGLV_CRITICAL,"CheckIn reply from the server: %s\n",recv_buf);
#endif
//////////////

	/* launch keepalive thread, BIGIOT */
	if( pthread_create(&pthd_keepalive, NULL, (void *)iot_keepalive, NULL) !=0 )
        {
                printf("Fail to create bigiot_keepalive thread!\n");
                goto fail;
        }
	else
	{
		printf("Create bigiot_keepavlie thread successfully!\n");
	}

	/* ------------ IoT Talk Loop:  loop recv and send processing ------------ */
	while(1)
	{
		/* clear buff */
		memset(recv_buf,0,sizeof(recv_buf));
	   	if( (ret=recv(sockfd,recv_buf,BUFFSIZE-1,0)) >0 )
		{
			//recv_buf[ret]='\0';
			EGI_PLOG(LOGLV_INFO,"Message from the server: %s\n",recv_buf);

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

//////----- get IoT Command string pointer and parse it  ------
				pstrCval=get_jsonkey_pstrval(json, "C");/* get visiotr's Command value */
			if(pstrCval != NULL)
			{
				printf("receive command: %s\n",pstrCval);
				/* parse command string */
				if(strcmp(pstrCval,"offOn")==0)
				{
					printf("Execute command 'offOn' ....\n");
					/* toggle the bulb color */
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
				    }
				}
				else if( !bulb_off && (  (strcmp(pstrCval,"minus")==0)
						         ||(strcmp(pstrCval,"down")==0)  ) )
				{
				    printf("Execute command 'minus' ....\n");
				    if( subcolor < 0x222222 ) subcolor=0x222222; /* low limit */
				    else
				    {
					subcolor -= BULB_COLOR_STEP;
				    }
				}

				printf("subcolor=0x%08X \n",subcolor);
				/* set subcolor to iotbtn */
				egi_btnbox_setsubcolor(iotbtn, COLOR_24TO16BITS(subcolor)); //iotbtn_subcolor);
				/* refresh iotbtn */
				egi_ebox_needrefresh(iotbtn);
				//egi_page_flag_needrefresh(page); /* !!!SLOW!!!! let page routine to do the refresh job */
				egi_ebox_refresh(iotbtn);
			}
//////----- renew reply_json's key 'C' value: with message string for reply  ----
				memset(keyC_buf,0,sizeof(keyC_buf));
				sprintf(keyC_buf,"Gotcha '%s', bulb:%s light:0x%06X",
								pstrCval,bulb_status[bulb_off], subcolor);
				json_object_object_del(json_reply, "C");
				json_object_object_add(json_reply,"C", json_object_new_string(keyC_buf));

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
				ret=send(sockfd, strreply, strlen(strreply), MSG_CONFIRM|MSG_NOSIGNAL);
				if(ret<0)
				{
					perror("reply to visitor by send()");
				}
				else
					printf("Reply to visitor successfully!\n");
			}

//////----- clear arena for next IoT talk ----
			/* free dupstr and deref json objects */
			if(strreply !=NULL )
				free(strreply);
			//printf("json_object_put(json)...\n");
			json_object_put(json); /* func with if(json) inside  */
			//printf("json_object_put(json_item)...\n");
			json_object_put(json_item); /* func with if(json) inside */
		}
		else if(ret==0)
		{
			EGI_PLOG(LOGLV_ERROR,"%s: recv() ret=0 \n",__func__ );

			/* reconnect socket and checkin */
			iot_connect_checkin();
		}
		else /* ret<0 */
		{
			EGI_PLOG(LOGLV_ERROR,"%s: recv() error, %s\n",__func__, strerror(errno));

			if(ret==EBADF) /* invalid sock fd */
				EGI_PLOG(LOGLV_ERROR,"%s: Invalid socket file descriptor!\n",__func__);
			if(ret==ENOTSOCK) /* invalid sock fd */
				EGI_PLOG(LOGLV_ERROR,"%s: The file descriptor sockfd does not refer to a socket!\n",__func__);
		}

		usleep(100000);

	} /* end of while() */



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
