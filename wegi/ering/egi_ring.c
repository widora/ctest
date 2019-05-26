/*---------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Create an ERING mechanism for process communication betwee EGI UI and APP.

NOTE:
1. !!!! There should be only ONE host and/or ONE caller instance running, for
   ering_cmd and ering_ret is commonly shared. Need to be imporved....

2. Through ERING/UBUS/LPC you may avoid contanimation from GPL.


Midas Zhou
midaszhou@yahoo.com
----------------------------------------------------------------------*/
#include <stdio.h>
#include <libubus.h>
#include <libubox/blobmsg_json.h>
#include "egi_ring.h"


/* ERING CALLER: ering command  */
// To be provided by the caller: static EGI_RING_CMD ering_cmd;

/* ERING HOST:
 *  wait for caller's msg ---> ering_cmd ---> wait for host_thread respond --> ering_ret ---> feedback to caller
 */
//static EGI_RING_CMD ering_cmd;	/* host recevied ERING command */
static EGI_RING_RET ering_ret;	/* host process prepared ERING return message, after process ering_cmd */
static EGI_RING_CMD ering_cmd;
static ering_cmdret_handler_t	host_handler;	/* ering_cmd/ret handler for the ering host
						 * parse ering_cmd received and and prepare ering_ret
						 * for feedback to the caller.
						 */
static ering_cmdret_handler_t	caller_handler;	/* ering_cmd/ret handler for the ering caller
						 * normally ONLY check ering_ret from the host!!!
						 */

static int count; /* for test.... */

/* ERING HOST: ering  handler */
static int ering_handler( struct ubus_context *ctx, struct ubus_object *obj,
                          struct ubus_request_data *req, const char *method,
                          struct blob_attr *msg );

/* ERING CALLER: returned data handler */
static void result_data_handler(struct ubus_request *req, int type, struct blob_attr *msg);

/* ERING CALLER: Policy assignment */
static const struct blobmsg_policy ering_ret_policy[] =
{
        [ERING_RET_CODE]  = { .name="code", .type=BLOBMSG_TYPE_INT16},   /* APP returned code */
        [ERING_RET_DATA]  = { .name="data", .type=BLOBMSG_TYPE_INT32 },  /* APP returned data */
        [ERING_RET_MSG]   = { .name="msg", .type=BLOBMSG_TYPE_STRING },  /* APP returned msg */
};

/* ERING HOST: Policy assignment */
static const struct blobmsg_policy ering_cmd_policy[] =
{
        [ERING_CMD_ID]   = { .name="id", .type=BLOBMSG_TYPE_INT16},     /* APP command id, try TYPE_ARRAY */
        [ERING_CMD_DATA] = { .name="data", .type=BLOBMSG_TYPE_INT32 },  /* APP command data */
        [ERING_CMD_MSG]  = { .name="msg", .type=BLOBMSG_TYPE_STRING },  /* APP command msg */
};

/* ERING HOST:  Methods assignment, Currently only one method! */
static const struct ubus_method ering_methods[] =
{
        UBUS_METHOD("ering_cmd", ering_handler, ering_cmd_policy),
};


/* ERING HOST: Ubus object type */
static struct ubus_object_type ering_obj_type =
	UBUS_OBJECT_TYPE("ering_uobj", ering_methods);


/*------------------------------------
	Return ubus strerror
-------------------------------------*/
inline const char *ering_strerror(int ret)
{
	return ubus_strerror(ret);
}

/*------------------------------  ERING HOST  ------------------------------
The egi_run_host is a uloop function and normally to be run
as a thread.
Start a ubus host.

Params:
@ering_host     host name to be used as ubus object name.
@cmd_to_host	a pointer to EGI_RING_CMD, that will pass command
                from ering_caller to the function caller.

Return:
----------------------------=-------------------------------------------------*/
void ering_run_host(const char *ering_host, ering_cmdret_handler_t ehandler)
{
        struct ubus_context *ctx;
        struct blob_buf bb;

        int ret;
        const char *ubus_socket=NULL; /* use default UNIX sock path: /var/run/ubus.sock */

	if( ering_host==NULL ) {
                printf("%s: Ering host name invalid!\n",__func__);
		return;
	}
	if( ering_handler==NULL ) {
                printf("%s: ering_handler is invalid!\n",__func__);
		return;
	}

	/* pass cmd_to_host to module's param host_handler */
	host_handler=ehandler;

        struct ubus_object ering_obj=
        {
                .name = ering_host,     /* with APP name, though NULL is also ok For UBUS */
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
                                                __func__, ering_obj.name, ering_strerror(ret));
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


/*-----------------------  ERING HOST  ------------------------
EGI RING common method_call handler, once host receive ering msg
from the caller this function will be trigged.

type: ubus_handler_t
--------------------------------------------------------------*/
static int ering_handler( struct ubus_context *ctx, struct ubus_object *obj,
                          struct ubus_request_data *req, const char *method,
                          struct blob_attr *msg )
{
        struct blob_buf bb=
        {
                .grow=NULL,     /* NULL, or blob_buf_init() will fail! */
        };
        struct ubus_request_data req_data;
        struct blob_attr *tb[__ERING_CMD_MAX]; /* for parsed attr */

        /* parse blob msg */
        blobmsg_parse(ering_cmd_policy, ARRAY_SIZE(ering_cmd_policy), tb, blob_data(msg), blob_len(msg));

        /*          -----  	APP command/ERING parsing 	-----
         * handle ERING policy according to the right order
         * TODO: here insert APP command parsing function
         *              APP_parse_ering(id, data,msg)
         */

	ering_clear_cmd(&ering_cmd); /* clear ering_cmd before input new data */
	ering_clear_ret(&ering_ret); /* clear ering_ret before host_handler */

	/* put received data to ering_cmd */
        if(tb[ERING_CMD_ID]) {
	     ering_cmd.cmd_valid[ERING_CMD_ID]=true;
	     ering_cmd.cmd_id=blobmsg_get_u16(tb[ERING_CMD_ID]);
             printf("%s: From @%u, ERING_CMD_ID: %u \n",
						__func__, obj->id, blobmsg_get_u16(tb[ERING_CMD_ID]));
	}
        if(tb[ERING_CMD_DATA]) {
	     ering_cmd.cmd_valid[ERING_CMD_DATA]=true;
	     ering_cmd.cmd_data=blobmsg_get_u32(tb[ERING_CMD_DATA]);
             printf("%s: From @%u, ERING_CMD_DATA: %u \n",
						__func__, obj->id, blobmsg_get_u32(tb[ERING_CMD_DATA]));
	}
        if(tb[ERING_CMD_MSG]) {
	     ering_cmd.cmd_valid[ERING_CMD_MSG]=true;
	     ering_cmd.cmd_msg=strdup(blobmsg_data(tb[ERING_CMD_MSG]) );
             printf("%s: From @%u, ERING_CMD_MSG: '%s' \n",
						__func__, obj->id, blobmsg_data(tb[ERING_CMD_MSG]));
	}


	/* run host defined command/return msg handler */
	(*host_handler)(&ering_cmd, &ering_ret);

////// following to be done by the host_handler ......
        /* send back result of method_call to the caller,  with data in ering_ret
	 * as per ering_ret_policy[ ]
         */
        blob_buf_init(&bb, 1);

	/* pust prepared ering_ret to blobmsg */
	if(ering_ret.ret_valid[ERING_RET_CODE]) {
	        blobmsg_add_u16(&bb, "code", ering_ret.ret_code);	/* put return code */
	}
	if(ering_ret.ret_valid[ERING_RET_DATA]) {
	        blobmsg_add_u32(&bb, "data", ering_ret.ret_data);	/* put return data */
	}
	if(ering_ret.ret_valid[ERING_RET_MSG]) {
		blobmsg_add_string(&bb, "msg",ering_ret.ret_msg);
	}
#if 0
	memset(&ering_ret,0,sizeof(ering_ret));
	printf("------ put id ------\n");
	ering_ret.ret_code=ering_cmd.cmd_id;
        blobmsg_add_u16(&bb, "code", ering_ret.ret_code);		/* put return code */

	printf("------ put data ------\n");
	count++;
	ering_ret.ret_data=count; /* for test......*/
        blobmsg_add_u32(&bb, "data", ering_ret.ret_data);		/* put return data */

	printf("------ put msg ------\n");
        if(ering_ret.ret_msg)
              blobmsg_add_string(&bb, "msg", ering_ret.ret_msg);	/* put return msg */
#endif


        ubus_send_reply(ctx, req, bb.head);


       	/*      -----  reply proceed results  -----
         * NOTE: we may put proceeding job in a timeout task, just to speed up service response.
         */
        ubus_defer_request(ctx, req, &req_data);

        ubus_complete_deferred_request(ctx, req, UBUS_STATUS_OK);

	blob_buf_free(&bb);
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


/*----------------  ERING CALLER  --------------------
Send ering command to an APP and get its reply.
params:
@ering_host	ering host (APP) name
@ering_cmd	ering command
TODO @ering_ret	ering returned result

return:
	0	OK, and host reply OK.
	<0	Function fails
	>0	Ering host reply fails.
--------------------------------------------------------*/
int ering_call_host(const char *ering_host, EGI_RING_CMD *ering_cmd) /* EGI_RING_RET *ering_ret */
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
		printf("%s: Fail to register ering_obj, Error: %s\n",__func__, ering_strerror(ret));
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

	/* 6. pack data to blob_buf
	 *    as per ering_cmd[]::ering_cmd_policy[]
	 */
	blob_buf_init(&bb,0);

	if(ering_cmd->cmd_valid[ERING_CMD_ID])
		blobmsg_add_u16(&bb, "id", ering_cmd->cmd_id);
	if(ering_cmd->cmd_valid[ERING_CMD_DATA])
		blobmsg_add_u32(&bb, "data", ering_cmd->cmd_data);
	if( ering_cmd->cmd_valid[ERING_CMD_MSG]  &&  ering_cmd->cmd_msg != NULL )
		blobmsg_add_string(&bb, "msg", ering_cmd->cmd_msg);

	/* 7. invoke a ering command session
	 *    call the ubus host object with a specified method.
 	 */
	ret=ubus_invoke(ctx, host_id, "ering_cmd", bb.head, result_data_handler, 0, 0);
	if(ret!=0)
		printf("%s Fail to call ering host '%s': %s\n",__func__, ubus_strerror(ret));

	/* DO NOT DO uloop: 	uloop_run(); */

	blob_buf_free(&bb);
	uloop_done();

UBUS_FAIL:
	ubus_free(ctx);

	return ret;
}


/*-----------  ERING CALLER  ------------
TYPE: ubus_data_handler_t
Host feedback data handler

---------------------------------------*/
static void result_data_handler(struct ubus_request *req, int type, struct blob_attr *msg)
{
	int i;
	char *strmsg;
	struct blob_attr *tb[__ERING_RET_MAX]; /* for parsed attr */

	if(!msg)
		return;

	/* print in format of json string */
	strmsg=blobmsg_format_json_indent(msg,true, 0); /* 0 type of format */
	printf("%s\n", strmsg);
	free(strmsg); /* need to free strmsg */

        /* parse returned msg */
        blobmsg_parse(ering_ret_policy, ARRAY_SIZE(ering_ret_policy), tb, blob_data(msg), blob_len(msg));

	/* reset ering_ret->ret_valid first */
	for(i=0; i<__ERING_RET_MAX; i++) {
		ering_ret.ret_valid[i]=false;
	}

	/* put returned data to ering_ret for caller to read */
        if(tb[ERING_RET_CODE]) {
		ering_ret.ret_valid[ERING_RET_CODE]=true;
		ering_ret.ret_code=blobmsg_get_u16(tb[ERING_RET_CODE]);
             	printf("%s: ERING_RET_CODE: %u \n", __func__, blobmsg_get_u16(tb[ERING_RET_CODE]));
	}
        if(tb[ERING_RET_DATA]) {
		ering_ret.ret_valid[ERING_RET_DATA]=true;
		ering_ret.ret_data=blobmsg_get_u32(tb[ERING_RET_DATA]);
             	printf("%s: ERING_RET_DATA: %u \n", __func__, blobmsg_get_u32(tb[ERING_RET_DATA]));
	}
        if(tb[ERING_RET_MSG]) {
		ering_ret.ret_valid[ERING_RET_MSG]=true;
		ering_ret.ret_msg=strdup(blobmsg_data(tb[ERING_RET_MSG]));
           	printf("%s: ERING_RET_MSG: '%s' \n", __func__, blobmsg_data(tb[ERING_RET_MSG]));
	}

	/* TODO better way to pass EGI_RING_RET to the caller ???? */


}


/*-----------  ERING CALLER  ---------------
Read out ering_ret, and reset ering_ret!

@eret		pointer to pass ering_ret

Return:
	0	OK, host reply msg received.
	<0	There's no host reply msg.
--------------------------------------------*/
int ering_read_ret(EGI_RING_RET *eret)
{
	int i;
	int ret=-1;

	if(eret==NULL)
		return -1;

	/* reset ret_valid */
	for(i=0; i<__ERING_RET_MAX; i++) {
		eret->ret_valid[i]=false;
	}

	/* read data */
	if( ering_ret.ret_valid[ERING_RET_CODE] ) {
		eret->ret_valid[ERING_RET_CODE]=true;
		eret->ret_code=ering_ret.ret_code;
		ret=0;
	}

	if( ering_ret.ret_valid[ERING_RET_DATA] ) {
		eret->ret_valid[ERING_RET_DATA]=true;
		eret->ret_data=ering_ret.ret_data;
		ret=0;
	}

	if( ering_ret.ret_valid[ERING_RET_MSG] ) {
		eret->ret_valid[ERING_RET_MSG]=true;
		eret->ret_msg=strdup(ering_ret.ret_msg);	/* Do not forget to free */
		ret=0;
	}

	/* reset ering_ret->ret_valid and free msg, as we already read out data */
	ering_clear_ret(&ering_ret);

	return ret;
}


/*------------------------------------------------------
Clear a ering_ret struct: reset ret_valid and free msg.
Do not clear
-------------------------------------------------------*/
void ering_clear_ret(EGI_RING_RET *eret)
{
	int i;

	if(eret==NULL)
		return;

        /* reset ret_valid */
        for(i=0; i<__ERING_RET_MAX; i++) {
                eret->ret_valid[i]=false;
        }

	/* free msg */
	if(eret->ret_msg != NULL) {
		free(eret->ret_msg);		/* !!!! */
		eret->ret_msg=NULL;
	}
}


/*------------------------------------------------------
Clear a ering_cmd struct: reset cmd_valid and free msg.
Do not clear cmd_handler
-------------------------------------------------------*/
void ering_clear_cmd(EGI_RING_CMD *ecmd)
{
        int i;

        if(ecmd==NULL)
                return;

        /* reset cmd_valid */
        for(i=0; i<__ERING_CMD_MAX; i++) {
                ecmd->cmd_valid[i]=false;
        }

        /* free msg */
        if(ecmd->cmd_msg != NULL) {
                free(ecmd->cmd_msg);            /* !!!! */
                ecmd->cmd_msg=NULL;
        }
}

