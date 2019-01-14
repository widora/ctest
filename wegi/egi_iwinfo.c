#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>

#ifndef IW_NAME
#define IW_NAME "apcli0"
#endif

int get_iw_rssi(void)
{
	int ret=-1000;
	int rssi;
	int sockfd;
	struct iw_statistics stats;
	struct iwreq req;

	memset(&stats,0,sizeof(struct iw_statistics));
	memset(&req,0,sizeof(struct iwreq));
	sprintf(req.ifr_name,"apcli0");
	req.u.data.pointer=&stats;
	req.u.data.length=sizeof(struct iw_statistics);
	#ifdef CLEAR_UPDATED
	req.u.data.flags=1;
	#endif

//	while(1)
//	{
		if((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1)
		{
			perror("Could not create simple datagram socket");
			return ret;
		}

		if(ioctl(sockfd,SIOCGIWSTATS,&req)==-1)
		{
			perror("Error performing SIOCGIWSTATS");
			close(sockfd);
			return ret;
		}

		close(sockfd);

		rssi=(int)(signed char)stats.qual.level;
//		printf("egi_iw_rssi(): rssi=%d\n",rssi);
/*
		printf("Signal level %s is %d%s.\n",
		(stats.qual.updated & IW_QUAL_DBM ? "(in dBm)":""),(signed char)stats.qual.level,
		(stats.qual.updated & IW_QUAL_LEVEL_UPDATED ? "(updated)":"")
		);

		sleep(1);
	}
*/
	return rssi;
}
