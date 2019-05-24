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
#include <libubus.h>

static struct ubus_context *ctx;
static struct ubus_request_data req_data;
static struct blob_buf	bb;

static int ering_handler( struct ubus_context *ctx, struct ubus_object *obj,
			  struct ubus_request_data *req, const char *method,
			  struct blob_attr *msg );



/* Enum for EGI policy order */
enum {
	ERING_ID,
	ERING_DATA,
	ERING_MSG,
	__ERING_MAX,
};

/* Policy assignment */
static const struct blobmsg_policy ering_policy[] =
{
	[ERING_ID]  = { .name="id", .type=BLOBMSG_TYPE_INT16},	   /* APP command id, try TYPE_ARRAY */
	[ERING_DATA] = { .name="data", .type=BLOBMSG_TYPE_INT32 }, /* APP command data */
	[ERING_MSG] = { .name="msg", .type=BLOBMSG_TYPE_STRING },  /* APP command msg */
};

/* Methods assignment */
static const struct ubus_method ering_methods[] =
{
	UBUS_METHOD("ering_cmd", ering_handler, ering_policy),
};

/* Object type assignement */
static struct ubus_object_type ering_obj_type =
	UBUS_OBJECT_TYPE("ering_uobj", ering_methods);

/* ubus object assignment */
static struct ubus_object ering_obj=
{
	.name = "ering.APP", 	/* with APP name */
	.type = &ering_obj_type,
	.methods = ering_methods,
	.n_methods = ARRAY_SIZE(ering_methods),
	//.path= /* useless */
};

/*---------------------------------------------------
Params:
@ering_obj.name		APP name
@APP_parse_ering()	APP command parsing function

---------------------------------------------------*/
int main(void)
{
	int ret;

	const char *ubus_socket=NULL; /* use default UNIX sock path: /var/run/ubus.sock */

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
		printf("%s: Fail to register an object to ubus.\n",__func__);
		ret=-2;
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



/*-----------------------------------------------------------------------
A callback handler function for method 'tune'
-------------------------------------------------------------------------*/
static int ering_handler( struct ubus_context *ctx, struct ubus_object *obj,
			  struct ubus_request_data *req, const char *method,
			  struct blob_attr *msg )
{
	struct blob_attr *tb[__ERING_MAX]; /* for parsed attr */
//	struct ubus_request_data req;

	/* parse blob msg */
	blobmsg_parse(ering_policy, ARRAY_SIZE(ering_policy), tb, blob_data(msg), blob_len(msg));

	/*	    -----  APP command/ERING parsing -----
	 * handle ERING policy according to the right order
	 * TODO: here insert APP command parsing function
	 *		APP_parse_ering(id, data,msg)
	 */
	if(tb[ERING_ID])
	     printf("%s: From @%u, ERING_ID: %u \n", __func__, obj->id, blobmsg_get_u16(tb[ERING_ID]));

	if(tb[ERING_DATA])
	     printf("%s: From @%u, ERING_DATA: %u \n", __func__, obj->id, blobmsg_get_u32(tb[ERING_DATA]));

	if(tb[ERING_MSG])
	     printf("%s: From @%u, ERING_MSG: '%s' \n", __func__, obj->id, blobmsg_data(tb[ERING_MSG]));

	/* send a reply msg to the caller for information */
	blob_buf_init(&bb, 0);
	blobmsg_add_string(&bb,"Ering reply", "Request is being proceeded!");
	ubus_send_reply(ctx, req, bb.head);

	/* 	-----  reply proceed results  -----
	 * NOTE: we may put proceeding job in a timeout task, just to speed up service response.
	 */
	ubus_defer_request(ctx, req, &req_data);
	ubus_complete_deferred_request(ctx, req, UBUS_STATUS_PERMISSION_DENIED);

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
