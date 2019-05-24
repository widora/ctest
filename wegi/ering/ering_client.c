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

static struct ubus_context *ctx;
static struct blob_buf	bb;


#if 0   ///////////////////////////////////////////////////////////////////////
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
	[ERING_ID]  = { .name="id", .type=BLOBMSG_TYPE_INT16 },	   /* APP command id, try TYPE_ARRAY */
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

#endif 	///////////////////////////////////////////////////////////////////



/* ubus object assignment */
static struct ubus_object ering_obj=
{
	.name = "ering_caller",
//	.type = &ering_obj_type,
//	.methods = ering_methods,
//	.n_methods = ARRAY_SIZE(ering_methods),
	//.path= /* useless */
};


/*-------------------------------------------------
Params:
@ering_caller name 	(client ubus obj name)
@ering_host name   	(APP ubus obj name)

NOTE: only one mehtod 'ering_cmd' for all APPs
---------------------------------------------------*/
int main(void)
{
	int ret;
	uint32_t host_id; /* ering host id */
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

	/* 5.1 search a registered object with a given name */
	if( ubus_lookup_id(ctx, "ering.APP", &host_id) ) {
		printf("%s: ering APP host is NOT found in ubus!\n",__func__);
		ret=-3;
		goto UBUS_FAIL;
	}
	printf("%s: get ering host_id %u\n",__func__, host_id);

	/* 5.2 call the ubus host object with a specified method */
	blob_buf_init(&bb,0);
	blobmsg_add_u16(&bb,"id", ering_obj.id); /* ERING_ID */
	blobmsg_add_u32(&bb,"data", 123456); /* ERING_DATA */
	blobmsg_add_string(&bb,"msg", "Hello, ERING!"); /* ERING_DATA */

	/*
	int ubus_invoke(struct ubus_context *ctx, uint32_t obj, const char *method,
                struct blob_attr *msg, ubus_data_handler_t cb, void *priv,
                int timeout)
	{  ...

	   1. ubus_start_request() ---> ubus_send_msg() ---> writev_retry(ctx->sock.fd,,,) ---> writev()
	   NOTE: "The data transfers performed by readv() and writev() are atomic: the data written
		  by writev() is written as a single block that is not intermingled with output from
		  writes in other processes (but see pipe(7) for an exception)"

	   2. ubus_complete_request() ---> uloop_timeout_set() ---> ubus_complete_request_async(),
                  list_add(&req->list, &ctx->requests) ----> single step uloop_run(),waiting for req->status_msg
	       --->hold on uloop/timeout ---> req->complete_cb(req,status)
	    ...
	}
	*/
	ret=ubus_invoke(ctx, host_id, "ering_cmd", bb.head, NULL, 0, 3000);
	printf("%s ering_cmd result: %s\n",__func__, ubus_strerror(ret));

	/* 6 uloop routine: events monitoring and callback provoking
	 *   OR to ignore this loop routine ......
	 */
	uloop_run();


	uloop_done();
UBUS_FAIL:
	ubus_free(ctx);
}


#if 0 //////////////////////////////////////////////////////////////////////
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
	 */
	if(tb[ERING_ID])
	     printf("%s: From @%d, ERING_ID: %u \n", __func__, obj->id, blobmsg_get_u16(tb[ERING_ID]));

	if(tb[ERING_DATA])
	     printf("%s: From @%d, ERING_DATA: %u \n", __func__, obj->id, blobmsg_get_u32(tb[ERING_DATA]));

	if(tb[ERING_MSG])
	     printf("%s: From @%d, ERING_MSG: '%s' \n", __func__, obj->id, blobmsg_data(tb[ERING_MSG]));

	/* send a reply msg to the caller for information */
	blob_buf_init(&bb, 0);
	blobmsg_add_string(&bb,"Ering reply", "Ering accepts your request!");

	ubus_send_reply(ctx, req, bb.head);
//	ubus_complete_deferred_request(ctx, &req,0);

}
#endif  ////////////////////////////////////////////////////////////////
