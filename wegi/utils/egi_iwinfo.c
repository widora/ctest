#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ether.h>   /* ETH_P_ALL */
#include <unistd.h>
#include <netpacket/packet.h> /* sockaddr_ll */
#include "egi_timer.h"
#include "egi_log.h"
#include "egi_iwinfo.h"

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
		//perror("Could not create simple datagram socket");
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to create SOCK_DGRAM socket: %s \n",
								   __func__, strerror(errno));
		return -1;
	}
	if(ioctl(sockfd,SIOCGIWSTATS,&req)==-1)
	{
		//perror("Error performing SIOCGIWSTATS");
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to call ioctl(socket,SIOCGIWSTATS,...): %s \n",
								   __func__, strerror(errno));
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
	char buf[2048];
	int ret;
	int count;
	int len;
	len=sizeof(addr);

	struct timeval timeout;
	timeout.tv_sec=IW_TRAFFIC_SAMPLE_SEC;
	timeout.tv_usec=0;

	/* reset ws first */
	*ws=0;
	count=0;

	/* create a socket */
	if( (sock=socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL))) == -1 )
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to create SOCK_RAW socket: %s \n",
								   __func__, strerror(errno));
		return -1;
	}
	sll.sll_family=PF_PACKET;
	sll.sll_protocol=htons(ETH_P_ALL);
	strcpy(ifstruct.ifr_name,"apcli0");
	if(ioctl(sock,SIOCGIFINDEX,&ifstruct)==-1)
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to call ioctl(sock, SIOCGIFINDEX, ...): %s \n",
								   __func__, strerror(errno));
		close(sock);
		return -2;
	}
	sll.sll_ifindex=ifstruct.ifr_ifindex;

	/* bind socket with address */
	ret=bind(sock,(struct sockaddr*)&sll, sizeof(struct sockaddr_ll));
	if(ret !=0 ) {
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to call bind(): %s \n", __func__, strerror(errno));
		close(sock);
		return -3;
	}

	/* set timeout option */
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

	/* set port_reuse option */
	int optval=1;/*YES*/
	ret=setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if(ret !=0 ) {
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to call setsockopt(): %s \n", __func__, strerror(errno));
		/* go on anyway */
	}

	printf("----------- start recvfrom() and tm_pulse counting ------------\n");
	while(1)
	{
		/* use pulse timer [0] */
		if(tm_pulseus(IW_TRAFFIC_SAMPLE_SEC*1000000, 0)) {
			EGI_PLOG(LOGLV_INFO,"egi_iwinfo: tm pulse OK!\n");
			break;
		}

		ret=recvfrom(sock,(char *)buf, sizeof(buf), 0, (struct sockaddr *)&addr, (socklen_t *)&len);
		if(ret<=0) {
			if( ret == EWOULDBLOCK ) {
				EGI_PLOG(LOGLV_INFO,"%s: Fail to call recvfrom() ret=EWOULDBLOCK. \n"
												 , __func__);
				continue;
			}
			else if (ret==EAGAIN) {
				EGI_PLOG(LOGLV_INFO,"%s: Fail to call recvfrom() ret=EAGAIN. \n",__func__);
				continue;
			}
			else {
				EGI_PLOG(LOGLV_ERROR,"%s: Fail to call ret=recvfrom()... ret=%d:%s \n",
									__func__, ret, strerror(errno));
				close(sock);
				return -4;
			}
		}

		count+=ret;
	}

	*ws=count;

	close(sock);
	return 0;
}
