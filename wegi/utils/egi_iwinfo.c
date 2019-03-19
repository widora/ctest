#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ether.h>   /* ETH_P_ALL */
#include <unistd.h>
#include <netpacket/packet.h> /* sockaddr_ll */
#include "egi_timer.h"

#ifndef IW_NAME
#define IW_NAME "apcli0"
#endif

/*------------------------------------------
Get wifi strength value

*rssi:	strength value

Return
	0	OK
	<0	Fails
-------------------------------------------*/
int iw_get_rssi(int *rssi)
{
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

	if((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1)
	{
		perror("Could not create simple datagram socket");
		return -1;
	}
	if(ioctl(sockfd,SIOCGIWSTATS,&req)==-1)
	{
		perror("Error performing SIOCGIWSTATS");
		close(sockfd);
		return -2;
	}

	*rssi=(int)(signed char)stats.qual.level;

	close(sockfd);
	return 0;
}

/*------------------------------------------
A rough method to get current wifi speed

ws:	speed in (bytes/s)

Return
	0	OK
	<0	Fails
-------------------------------------------*/
int  iw_get_speed(int *ws)
{
	int 			sock;
	struct ifreq 		ifstruct;
	struct sockaddr_ll	sll;
	struct sockaddr_in	addr;
	char buf[2000];
	int ret;
	int count;
	int len;
	len=sizeof(addr);

	if( (sock=socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL))) == -1 )
	{
		perror("Could not SOCK_RAW socket");
		return -1;
	}

	sll.sll_family=PF_PACKET;
	sll.sll_protocol=htons(ETH_P_ALL);
	strcpy(ifstruct.ifr_name,"apcli0");
	ioctl(sock,SIOCGIFINDEX,&ifstruct);
	sll.sll_ifindex=ifstruct.ifr_ifindex;

	bind(sock,(struct sockaddr*)&sll, sizeof(struct sockaddr_ll));
	count=0;

	printf("----------- start tm pulse ------------\n");
	while(1)
	{
		/* 1 second,  use 0 pulse timer */
		if(tm_pulseus(1000000,0)) {
			printf("tm pulse 1 second,OK!\n");
			break;
		}

		ret=recvfrom(sock,(char *)buf, sizeof(buf), 0, (struct sockaddr *)&addr, (socklen_t *)&len);
		if(ret<0)return ret;

		count+=ret;
	}

	*ws=count;
	return 0;
}
