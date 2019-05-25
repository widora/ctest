/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Create an ERING mechanism for EGI IPC.

Test:
	ubus call ering.APP ering_cmd "{'msg':'Hello ERING'}"

TODO:
	1. APP cotrol command classification.
	2. Method_calling feedback data format.


Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <libubus.h>
#include "egi_ring.h"


static int ering_handler( struct ubus_context *ctx, struct ubus_object *obj,
			  struct ubus_request_data *req, const char *method,
			  struct blob_attr *msg );

/*------------------------------------
        Return ubus strerror
-------------------------------------*/
inline const char *egi_ring_strerror(int ret)
{
        return ubus_strerror(ret);
}


/* ERING HOST: Policy assignment */
static const struct blobmsg_policy ering_cmd_policy[] =
{
	[ERING_CMD_ID]  = { .name="id", .type=BLOBMSG_TYPE_INT16},	   /* APP command id, try TYPE_ARRAY */
	[ERING_CMD_DATA] = { .name="data", .type=BLOBMSG_TYPE_INT32 }, /* APP command data */
	[ERING_CMD_MSG] = { .name="msg", .type=BLOBMSG_TYPE_STRING },  /* APP command msg */
};

/* ERING HOST:  Methods assignment */
static const struct ubus_method ering_methods[] =
{
	UBUS_METHOD("ering_cmd", ering_handler, ering_cmd_policy),
};

/* ERING HOST: Object type assignement */
static struct ubus_object_type ering_obj_type =
	UBUS_OBJECT_TYPE("ering_uobj", ering_methods);


/* ERING HOST: ubus object assignment */


/*---------------------------------------------------------------
The egi_ring_host is a uloop function and normally to be run as a
thread.

Params:
@ering_host	host name to be used as ubus object name.

Return:

---------------------------------------------------------------*/
//int main(void)
void egi_ring_host(const char *ering_host)
{
	struct ubus_context *ctx;
	struct blob_buf	bb;

	int ret;
	const char *ubus_socket=NULL; /* use default UNIX sock path: /var/run/ubus.sock */

	struct ubus_object ering_obj=
	{
		.name = ering_host, 	/* with APP name */
		.type = &ering_obj_type,
		.methods = ering_methods,
		.n_methods = ARRAY_SIZE(ering_methods),
		//.path= /* seems useless */
	};

	/* 1. create an epoll instatnce descriptor poll_fd */
	uloop_init();

	/* 2. connect to ubusd and get ctx */
	ctx=ubus_connect(ubus_socket);
	if(ctx==NULL) {
		printf("%s: Fail to connect to ubusd!\n",__func__);
		return;
	}

	/* 3. registger epoll events to uloop, start sock listening */
	ubus_add_uloop(ctx);

	/* 4. register a usb_object to ubusd */
	ret=ubus_add_object(ctx, &ering_obj);
	if(ret!=0) {
	       printf("%s: Fail to register '%s' to ubus, maybe already registered! Error:%s\n",
						__func__, ering_obj.name, egi_ring_strerror(ret));
		goto UBUS_FAIL;

	} else {
		printf("%s: Add '%s' to ubus successfully.\n",__func__,ering_obj.name);
	}

	/* 5. uloop routine: events monitoring and callback provoking */
	uloop_run();


UBUS_FAIL:
	ubus_free(ctx);
	uloop_done();
}



/*------------------------------------------------------------------------
		EGI RING common method_call handler
type: ubus_handler_t
-------------------------------------------------------------------------*/
static int ering_handler( struct ubus_context *ctx, struct ubus_object *obj,
			  struct ubus_request_data *req, const char *method,
			  struct blob_attr *msg )
{
	struct blob_buf	bb=
	{
		.grow=NULL,	/* NULL, or blob_buf_init() will fail! */
	};
	struct ubus_request_data req_data;
	struct blob_attr *tb[__ERING_CMD_MAX]; /* for parsed attr */

	/* parse blob msg */
	blobmsg_parse(ering_cmd_policy, ARRAY_SIZE(ering_cmd_policy), tb, blob_data(msg), blob_len(msg));

	/*	    -----  APP command/ERING parsing -----
	 * handle ERING policy according to the right order
	 * TODO: here insert APP command parsing function
	 *		APP_parse_ering(id, data,msg)
	 */
	if(tb[ERING_CMD_ID])
	     printf("%s: From @%u, ERING_CMD_ID: %u \n", __func__, obj->id, blobmsg_get_u16(tb[ERING_CMD_ID]));

	if(tb[ERING_CMD_DATA])
	     printf("%s: From @%u, ERING_CMD_DATA: %u \n", __func__, obj->id, blobmsg_get_u32(tb[ERING_CMD_DATA]));

	if(tb[ERING_CMD_MSG])
	     printf("%s: From @%u, ERING_CMD_MSG: '%s' \n", __func__, obj->id, blobmsg_data(tb[ERING_CMD_MSG]));


	/* ering return msg */
	EGI_RING_RET ering_ret =
	{
	      .host="ering.APP",
	      .ret_id=888,
	      .ret_data=55555,
	      .ret_msg="Mission completed!",
	};

	printf("blob_buf_init...\n");
	/* send a reply msg to the caller for information */
	blob_buf_init(&bb, 0);
//	blobmsg_add_string(&bb,"Ering reply", "Request is being proceeded!");

	printf("blobmsg_add...\n");
//        if(ering_ret.ret_id)
                blobmsg_add_u16(&bb, "id", ering_ret.ret_id);
//        if(ering_ret.ret_data)
                blobmsg_add_u32(&bb, "data", ering_ret.ret_data);
//        if(ering_ret.ret_msg)
                blobmsg_add_string(&bb, "msg", ering_ret.ret_msg);

	printf("ubus_send_reply...\n");
	ubus_send_reply(ctx, req, bb.head);


	/* 	-----  reply proceed results  -----
	 * NOTE: we may put proceeding job in a timeout task, just to speed up service response.
	 */
	printf("ubus_defer_request...\n");
	ubus_defer_request(ctx, req, &req_data);
	printf("ubus_complete_request...\n");
	ubus_complete_deferred_request(ctx, req, UBUS_STATUS_OK);


/*
enum ubus_msg_status {
        UBUS_STATUS_OK,
        UBUS_STATUS_INVALID_COMMAND,
        UBUS_STATUS_INVALID_ARGUMENT,
        UBUS_STATUS_METHOD_NOT_FOUND,
        UBUS_STATUS_NOT_FOUND,
        UBUS_STATUS_NO_DATA,
        UBUS_STATUS_PERMISSION_DENIED,
        UBUS_STATUS_TIMEOUT,
        UBUS_STATUS_NOT_SUPPORTED,
        UBUS_STATUS_UNKNOWN_ERROR,
        UBUS_STATUS_CONNECTION_FAILED,
        __UBUS_STATUS_LAST
};
*/


}



////////////////////////////////////////

void main(void)
{

	egi_ring_host("ering.APP");
}
