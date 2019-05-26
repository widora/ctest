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

	EGI_RING_CMD ering_cmd=
	{
		.caller="UI_stock",
		.cmd_valid={0,1,0},	/* switch valid data */
		.cmd_id=5,
		.cmd_data=555,
		.cmd_msg="Hello EGI RING!",
	};

  while(1) {
	i++;

	ering_cmd.cmd_data=i;
	if(ret=ering_call_host(ERING_HOST_APP, &ering_cmd))
		printf("Fail to call '%s', error: %s. \n", ERING_HOST_APP, ering_strerror(ret));
	else
		printf("i=%d, Ering call session completed!'\n", i);

	usleep(100000);
  }

}

