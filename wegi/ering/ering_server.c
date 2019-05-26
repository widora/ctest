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


int count=1;

void msg_parser(EGI_RING_CMD *ering_cmd, EGI_RING_RET *ering_ret)
{
//	printf("start to parse request cmd .. \n");
	/* already clear ering_cmd in EGI_RING module */
	if(ering_cmd->cmd_valid[ERING_CMD_ID])
		printf("cmd id: %d,  ",ering_cmd->cmd_id);
	if(ering_cmd->cmd_valid[ERING_CMD_DATA])
		printf("data: %d,  ",ering_cmd->cmd_data);
	if(ering_cmd->cmd_valid[ERING_CMD_MSG])
		printf("msg: %s \n",ering_cmd->cmd_msg);


//	printf("start to prepare return msg ... \n");
	/* already clear ering_ret in EGI_RING module */
	ering_ret->ret_valid[ERING_RET_CODE]=true;
	ering_ret->ret_code=count++;
	ering_ret->ret_valid[ERING_RET_DATA]=true;
	ering_ret->ret_data=555555;
	ering_ret->ret_valid[ERING_RET_MSG]=true;
	ering_ret->ret_msg=strdup("Request deferred!");

}

void main(void)
{
//	EGI_RING_CMD

	/* ERING uloop */
	ering_run_host("ering.APP", msg_parser);
}
