/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#ifndef __EGI_RING_H__
#define __EGI_RING_H__

#include <stdio.h>
#include <stdint.h>

/* ERING APP METHOD ID */
// to be defined in APP header file. ???


/* Enum for ERING command policy order */
enum {
        ERING_CMD_ID,
        ERING_CMD_DATA,
        ERING_CMD_MSG,
        __ERING_CMD_MAX,
};

/* Enum for ERING returned data handling policy order */
enum {
        ERING_RET_ID,
        ERING_RET_DATA,
        ERING_RET_MSG,
        __ERING_RET_MAX,
};


#define ERING_MAX_STRMSG 	1024

typedef struct egi_ring_command EGI_RING_CMD; 	/* ering caller command */
typedef struct egi_ring_return  EGI_RING_RET;	/* ering host reply */

struct egi_ring_command
{
	char 	*caller;		/* name of the ering caller, for client_obj.name */

	/***
	 * Complying with 'id','data' and 'msg' of ering_cmd_policy[]
	 * in ering_method['ering_cmd']
	 */
	int16_t cmd_id;			/* command ID, APP_ID + Method_ID */
	int32_t cmd_data;		/* command param */
//	char msg[ERING_MAX_STRMSG];	/* string msg if necessary, or NONE. depends on command */
	char 	*cmd_msg;
};

struct egi_ring_return
{
	char 	*host;			/* name of the ering host (APP) */

	/***
	 * Complying with 'id','data' and 'msg' of ering_cmd_policy[]
	 * in ering_method['ering_cmd']
	 */
	int16_t  ret_id;
	int32_t  ret_data; /* or void* ret_data  ....TBD */
	char 	 *ret_msg;
};


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




#endif
