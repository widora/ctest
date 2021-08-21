#include <libubox/blobmsg_json.h>
#include "libubus.h"

//Default UBUS_UNIX_SOCKET "/var/run/ubus.sock"

//"ubus call network.wireless down",
const char *arg_wifi_down[3]={
        "network.wireless",
        "down",
        NULL,
};
const char *arg_wifi_up[3]={
        "network.wireless",
        "up",
        NULL,
};


static bool simple_output=false;
static void receive_call_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
        char *str;
        if (!msg)
                return;

        str = blobmsg_format_json_indent(msg, true, simple_output ? -1 : 0);
        printf("%s\n", str);
        free(str);
}

/*-------------------------------------------------
        Call Ubus Host
Refer to: ubus-2015-05-25/cli.c
Copyright (C) 2011 Felix Fietkau <nbd@openwrt.org>
GNU Lesser General Public License version 2.1
-------------------------------------------------*/
static int ubus_cli_call(const char *ubus_socket, int argc, const char **argv)
{
        struct ubus_context *ctx=NULL;
        struct blob_buf b={0};
        uint32_t id;
        int ret;
        int timeout=30;

        if (argc < 2 || argc > 3)
                return -2;

	printf("ubus_connect...\n");
        ctx = ubus_connect(ubus_socket);
        if (!ctx) {
                if (!simple_output)
                        fprintf(stderr, "Failed to connect to ubus\n");
                return -1;
        }

	printf("blob_buf_init...\n");
        blob_buf_init(&b, 0);
        if (argc == 3 && !blobmsg_add_json_from_string(&b, argv[2])) {
                if (!simple_output)
                        fprintf(stderr, "Failed to parse message data\n");
                return -1;
        }

	printf("ubus_lookup_id...\n");
        ret = ubus_lookup_id(ctx, argv[0], &id);
        if (ret) {
                printf("ubus_lookup_id fails!\n");
                return ret;
        }

        printf("ubus_invoke...\n");
        ret= ubus_invoke(ctx, id, argv[1], b.head, receive_call_result_data, NULL, timeout * 1000);
	ubus_free(ctx);

	return ret;
}


int main(void)
{
	int ret;
	ret=ubus_cli_call(NULL, 2, arg_wifi_down);
	if(ret)
		printf("ubus_cli_call fails!\n");

return 0;
}
