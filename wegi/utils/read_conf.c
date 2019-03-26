#include <stdio.h>
#include "egi_cstring.h"

int main(void)
{
	char value[64]={0};

	if( egi_get_config_value("IOT_CLIENT", "server_ip", value)==0)
		printf("Found value: %s\n",value);
	else
		printf("Key value is NOT found in config file!\n");

	return 0;

}
