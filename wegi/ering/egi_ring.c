/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Create an ERING icp mechanism for EGI.

Test:
	ubus call ering.ffplay ering "{'msg':'Hello EGI ring'}"


Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdio.h>
#include <libubus.h>

static struct ubus_context *ctx;
static struct blob_buf	bb;

static int ering_handler( struct ubus_context *ctx, struct ubus_object *obj,
			  struct ubus_request_data *req, const char *method,
			  struct blob_attr *msg );


/* Enum for EGI policy order */
enum {
	ERING_ID,
	ERING_MSG,
	__ERING_MAX,
};

/* Policy assignment */
static const struct blobmsg_policy ering_policy[] =
{
	[ERING_ID]  = { .name="id", .type=BLOBMSG_TYPE_INT32 },
	[ERING_MSG] = { .name="msg", .type=BLOBMSG_TYPE_STRING },
};

/* Methods assignment */
static const struct ubus_method ering_methods[] =
{
	UBUS_METHOD("ering", ering_handler, ering_policy),
};

/* Object type assignement */
static struct ubus_object_type egi_uobj_type =
	UBUS_OBJECT_TYPE("ering_uobj", ering_methods);


/* ubus object assignment */
static struct ubus_object egi_uobj=
{
	.name = "ering.ffplay",
	.type = &egi_uobj_type,
	.methods = ering_methods,
	.n_methods = ARRAY_SIZE(ering_methods),
	//.path= /* useless */
};


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
	ret=ubus_add_object(ctx, &egi_uobj);
	if(ret!=0) {
		printf("%s: Fail to register an object to ubus.\n",__func__);
		ret=-2;
		goto UBUS_FAIL;
	} else {
		printf("%s: Add '%s' to ubus successfully.\n",__func__,egi_uobj.name);
	}

	/* 5. uloop routine: events monitoring and callback provoking */
	uloop_run();


UBUS_FAIL:
	ubus_free(ctx);
	uloop_done();
}



/*-----------------------------------------------------------------------
A handler function for method 'ering'
-------------------------------------------------------------------------*/
static int ering_handler( struct ubus_context *ctx, struct ubus_object *obj,
			  struct ubus_request_data *req, const char *method,
			  struct blob_attr *msg )
{
	struct blob_attr *tb[__ERING_MAX]; /* for parsed attr */
//	struct ubus_request_data req;

	/* parse blob msg */
	blobmsg_parse(ering_policy, ARRAY_SIZE(ering_policy), tb, blob_data(msg), blob_len(msg));

	/* handle ering policy according to the right order */
	if(tb[ERING_ID])
		printf("%s: caller's ubus id=%d, ERING_ID: %u \n", __func__, obj->id, blobmsg_get_u32(tb[ERING_ID]));

	if(tb[ERING_MSG])
		printf("%s: caller's ubus id=%d, ERING_MSG: '%s' \n", __func__, obj->id, blobmsg_data(tb[ERING_MSG]));


	/* send a reply msg to the caller for information */
	blob_buf_init(&bb, 0);
	blobmsg_add_string(&bb,"Ering reply", "Ering accepts your request!");

	ubus_send_reply(ctx, req, bb.head);
//	ubus_complete_deferred_request(ctx, &req,0);

}
