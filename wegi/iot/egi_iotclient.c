/*------------------  iot_client.c  ---------------------------
A simple example to log on BIGIOT and keep alive.

Midas Zhou
--------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
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


#define BUFFSIZE 2048

int sockfd;
char *strjson_checkin= "{\"M\":\"checkin\",\"ID\":421, \"K\":\"f80ea043e\"}\n";
char *strjson_keepalive= "{\"M\":\"beat\"}\n";
char *strjson_reply= "{\"M\":\"say\",\"C\":\"Hello from Widora_NEO\",\"SIGN\":\"Widora_NEO\"}";//\n";



//////----- get key value of a string json object  ----
#if 0
static char *json_dup_key_strval(json_obj *json, char *key)
{
				/* get visitor's ID */
				json_item=json_object_object_get(json, "ID");
				//NEW: json_object_object_get_ext()....
				if( json_item == NULL )
				{
					printf("It's not a string type json object, fail to get key value.\n");
					continue;
				}
				else
					printf("visitor's ID = %s\n",json_object_get_string(json_item));

				/* get string val by dublicating */
				strval=strdup(json_object_get_string(json_item));
				printf("ID key, strval=%s\n",strval);

#endif

void bigiot_keepalive(void)
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
			printf("Fail to send keepalive msg to BIGIOT!\n");
			perror("send keepalive msg to BIGIOT.");
		}
		else {
		        /* get time stamp */
        		t=time(NULL);
        		tm=localtime(&t);
        		/* time stamp and msg */
        		printf("[%d-%02d-%02d %02d:%02d:%02d] ",
                                tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour, tm->tm_min,tm->tm_sec );
			printf("Keepalive msg has been sent to BIGIOT.\n");
		}
	}
}


int main(int argc, char *argv[])
{
	int ret;
	char recv_buf[BUFFSIZE]={0};
	struct sockaddr_in svr_addr;

	json_object *json=NULL;
	json_object *json_item=NULL;
	json_object *json_reply=NULL;
	json_object *json_tmp=NULL;

	char *strreply=NULL;
	const char *pstrval=NULL;
	const char *pstrtmp=NULL;

	pthread_t  	pthd_keepalive;


	/* prepare jason replay */
	json_reply=json_tokener_parse(strjson_reply);
	if(json_reply == NULL)
	{
		printf("egi_iotclient: fail to prepare json_reply.\n");
		return -1;
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
			json=json_tokener_parse(recv_buf);
			if(!json)
			{
				printf("eig_iotclient: Fail to parse received string by json_tokener_parse()!\n");
			}
			/* extract items and values */
			else
			{
//////----- get key value of a string json object  ----
				/* get visitor's ID */
				json_item=json_object_object_get(json, "ID");
				//NEW: json_object_object_get_ext()....
				if( json_item == NULL )
				{
					printf("It's not a string type json object, fail to get key value.\n");
					continue;
				}
				else
					printf("visitor's ID = %s\n",json_object_get_string(json_item));

				if(!json_object_is_type(json_item, json_type_string))
					printf("Error, json_item is NOT a string type object!\n");
				else
					printf("OK, json_item is a string type object.\n");

				/* get a pointer to string value */
				pstrval=json_object_get_string(json_item); /* return a pointer */
				printf("key 'ID': string val=%s\n",pstrval);

//////----- renew ID key object in reply json  ----
				/* delete old "ID" key,  then add new */
				json_object_object_del(json_reply, "ID"); /* */
				if( ( json_tmp=json_object_new_string(pstrval) ) == NULL )
				{
					printf("json_object_new_string(pstrval) fails!\n");
				}
				else
					printf("json_tmp: %s\n",json_object_to_json_string(json_tmp));

				printf("json_object_object_add()...\n");
				json_object_object_add(json_reply, "ID", json_object_new_string(pstrval)); /* with strdup() inside */
				/* NOTE: json_object_new_string() with strdup() inside */

//////----- generate corresponding reply string for sockfd  ----
				/* prepare reply string for socket send */
				pstrtmp=json_object_to_json_string(json_reply);/* json..() return a pointer */
				strreply=(char *)malloc(strlen(pstrtmp)+2); /* 2 for '/n/0' */
				if(strreply==NULL)
				{
					printf("egi_iotclient: fail to malloc strreply.\n");
					goto fail;
				}
				memset(strreply,0,strlen(pstrtmp)+2);
				printf("sprintf strreply from json_object_to_json_string(json_reply)...\n");
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
	return 0;

}
