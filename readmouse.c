/*-------------------------------------------------------------------
with reference to:
https://item.congci.com/-/content/linux-shubiao-shuju-duqu-caozuo
-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc,char **argv)
{
   int fd,retval;
   int nread;
   int i;
   unsigned char buf[8]={0};
   fd_set readfds;
   struct timeval tv;

   if(argc<2)
   {
	printf(" Usage: %s  dev_path \n",argv[0]);
	exit(-1);
   }
   if( (fd=open(argv[1],O_RDONLY))<0)
   {
	printf(" Fail to open %s !\n",argv[1]);
	exit(-1);
   }
   else
   {
	printf("Open %s successfuly!\n",argv[1]);
   }

   while(1)
   {
	tv.tv_sec=5;
	tv.tv_usec=0;

	FD_ZERO(&readfds);
	FD_SET(fd,&readfds);

	retval = select(fd+1, &readfds,NULL,NULL,&tv);
	if(retval==0)
		printf("Time out!\n");
	if(FD_ISSET(fd,&readfds))
	{
		nread=read(fd,buf,8);
		if(nread<0)
		{
		    continue;
		}
		//printf("%d bytes data: 0x%02x, X:%d, Y:%d, Z:%d  buf[4]=%d  buf[7]=%d \n",nread,buf[0],buf[1],buf[2],buf[3],buf[4],buf[7]);
		printf("%d bytes data: ",nread);
		for(i=0;i<8;i++)
			printf("buf[%d]=%d, ",i,buf[i]);
		printf("\n");
	}
   }
   close(fd);

   return 0;
}
