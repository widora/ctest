#include <stdio.h>
#include <utils/egi_iwinfo.h>

int main(void)
{
	const char host[]= "free-api.heweather.net";
	const char request[]="/s6/weather/now?location=beijing&key=xxxxxx";
	char strreply[1024]={0};

	if( iw_http_request(host, request, strreply) != 0 )
		printf("Fail to call HTTP request!\n");
	else
		printf("%s\n",strreply);

}
