/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Create an ERING mechanism for EGI IPC.


Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdio.h>
#include <libubus.h>
#include <libubox/blobmsg_json.h>
#include "egi_ring.h"


/* ERING CALLER: Policy assignment */
static const struct blobmsg_policy ering_ret_policy[] =
{
        [ERING_RET_ID]  = { .name="id", .type=BLOBMSG_TYPE_INT16},         /* APP command id, try TYPE_ARRAY */
        [ERING_RET_DATA] = { .name="data", .type=BLOBMSG_TYPE_INT32 }, 	   /* APP command data */
        [ERING_RET_MSG] = { .name="msg", .type=BLOBMSG_TYPE_STRING },  	   /* APP command msg */
};



static void result_data_handler(struct ubus_request *req, int type, struct blob_attr *msg);


/*------------------------------------
	Return ubus strerror
-------------------------------------*/
inline const char *egi_ring_strerror(int ret)
{
	return ubus_strerror(ret);
}

/*----------------------------------------------------
Send ering command to an APP and get its reply.
params:
@ering_host	ering host (APP) name
@ering_cmd	ering command
TODO @ering_ret	ering returned result

return:
	0	OK, and host reply OK.
	<0	Function fails
	>0	Ering host reply fails.
----------------------------------------------------*/
int egi_ring_call(const char *ering_host, EGI_RING_CMD *ering_cmd) /* EGI_RING_RET *ering_ret */
{
	int ret;
	struct ubus_context *ctx;
	uint32_t host_id; /* ering host id */
	const char *ubus_socket=NULL; /* use default UNIX sock path: /var/run/ubus.sock */
        struct blob_buf bb=
        {
                .grow=NULL,     /* NULL, or blob_buf_init() will fail! */
        };


	if(ering_cmd==NULL)
		return -1;

	/* put caller's name, though if name is NULL also OK. */
	struct ubus_object ering_obj=
	{
		.name=ering_cmd->caller,
	};

	/* 1. create an epoll instatnce descriptor poll_fd */
	uloop_init();

	/* 2. connect to ubusd and get ctx */
	ctx=ubus_connect(ubus_socket);
	if(ctx==NULL) {
		printf("%s: Fail to connect to ubusd!\n",__func__);
		return -1;
	}

	/* 3. registger epoll events to uloop, start sock listening */
	ubus_add_uloop(ctx);

	/* 4. register a usb_object to ubusd */
	ret=ubus_add_object(ctx, &ering_obj);
	if(ret!=0) {
		printf("%s: Fail to register ering_obj, Error: %s\n",__func__, egi_ring_strerror(ret));
		ret=-2;
		goto UBUS_FAIL;

	} else {
		printf("%s: Ering caller '%s' to ubus successfully.\n",__func__,ering_obj.name);
	}

	/* 5 search a registered object with a given name */
	if( ubus_lookup_id(ctx, ering_host, &host_id) ) {
		printf("%s: Ering host '%s' is NOT found in ubus!\n",__func__, ering_host);
		ret=-3;
		goto UBUS_FAIL;
	}
	printf("%s: Get ering host '%s' id=%u\n",__func__, ering_host, host_id);

	/* 6. pack data to blob_buf as per ering_cmd[]::ering_policy[] */
	blob_buf_init(&bb,0);
	if(ering_cmd->cmd_id)
		blobmsg_add_u16(&bb, "id", ering_cmd->cmd_id);
	if(ering_cmd->cmd_data)
		blobmsg_add_u32(&bb, "data", ering_cmd->cmd_data);
	if(ering_cmd->cmd_msg)
		blobmsg_add_string(&bb, "msg", ering_cmd->cmd_msg);

	/* 7. invoke a ering command session
	 *    call the ubus host object with a specified method.
 	 */
	ret=ubus_invoke(ctx, host_id, "ering_cmd", bb.head, result_data_handler, 0, 0);
	if(ret!=0)
		printf("%s Fail to call ering host '%s': %s\n",__func__, ubus_strerror(ret));

//	uloop_run();

	uloop_done();
UBUS_FAIL:
	ubus_free(ctx);

	return ret;
}


/*-------------------------------------
TYPE: ubus_data_handler_t
Host feedback data handler

TODO: 1. put in EGI_RING_RET
--------------------------------------*/
static void result_data_handler(struct ubus_request *req, int type, struct blob_attr *msg)
{
	char *strmsg;
	struct blob_attr *tb[__ERING_RET_MAX]; /* for parsed attr */

	if(!msg)
		return;

	strmsg=blobmsg_format_json_indent(msg,true, 0); /* 0 type of format */
	printf("%s\n", strmsg);
	free(strmsg); /* need to free strmsg */

        /* parse returned msg */
        blobmsg_parse(ering_ret_policy, ARRAY_SIZE(ering_ret_policy), tb, blob_data(msg), blob_len(msg));

        if(tb[ERING_RET_ID])
             printf("%s: ERING_RET_ID: %u \n", __func__, blobmsg_get_u16(tb[ERING_RET_ID]));

        if(tb[ERING_RET_DATA])
             printf("%s: ERING_RET_DATA: %u \n", __func__, blobmsg_get_u32(tb[ERING_RET_DATA]));

        if(tb[ERING_RET_MSG])
             printf("%s: ERING_RET_MSG: '%s' \n", __func__, blobmsg_data(tb[ERING_RET_MSG]));


	/* TODO: how to pass EGI_RING_RET to the caller.... */


}


////////////////////////////////////////////////
void main(void)
{

	#define ERING_HOST_APP		"ering.APP"
	#define ERING_HOST_STOCK	"ering.STOCK"
	#define ERING_HOST_FFPLAY	"ering.FFPLAY"

	#define ERING_STOCK_DISPLAY	1
	#define ERING_STOCK_HIDE	2
	#define ERING_STOCK_QUIT	3
	#define ERING_STOCK_SAVE	4

	int ret;
	int i=0;

	EGI_RING_CMD ering_cmd =
	{
		.caller="UI_stock",
		.cmd_id=12,
		.cmd_data=55555,
		.cmd_msg="Hellllllloooooo",
	};


  while(1) {
	i++;

	if(ret=egi_ring_call(ERING_HOST_APP, &ering_cmd))
		printf("Fail to call '%s', error: %s. \n", ERING_HOST_APP, egi_ring_strerror(ret));
	else
		printf("i=%d, Ering call session completed!'\n", i);

	usleep(300000);
  }

}

