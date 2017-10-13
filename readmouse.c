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

#define  LEFT_KEY 9
#define  RIGHT_KEY 10
#define  MID_KEY 12
#define  WH_UP 255
#define  WH_DOWN 1




int main(int argc,char **argv)
{
   int fd,retval;
   int nread,nwrite;
   int i;
   unsigned char buf[4]={0};
   unsigned char setbuf[6]={0xf3,200,0xf3,100,0xf3,80};
   fd_set readfds;
   struct timeval tv;
   //------
   unsigned int volume_val=50;
   char strCMD[50];


   if(argc<2)
   {
	printf(" Usage: %s  dev_path \n",argv[0]);
	exit(-1);
   }
   if( (fd=open(argv[1],O_RDWR))<0)
   {
	printf(" Fail to open %s !\n",argv[1]);
	exit(-1);
   }
   else
   {
	printf("Open %s successfuly!\n",argv[1]);
   }

   //-- set mouse type to Miscrosft Intellimouse
   nwrite=write(fd,setbuf,6);
   printf("%d bytes written to mousedev\n",nwrite);
   //------ set volume
   sprintf(strCMD,"amixer -c 1 set Speaker %d%%",volume_val);
   system(strCMD);


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
		nread=read(fd,buf,4);
		if(nread<0)
		{
		    continue;
		}
		//printf("%d bytes data: 0x%02x, X:%d, Y:%d, Z:%d  buf[4]=%d  buf[7]=%d \n",nread,buf[0],buf[1],buf[2],buf[3],buf[4],buf[7]);
		printf("%d bytes data: ",nread);
		for(i=0;i<4;i++)
			printf("buf[%d]=%d, ",i,buf[i]);
		printf("\n");

//------------------ control mplayer ---------------
                 if( buf[0] == LEFT_KEY )//left key press
                 {
                         printf("--left key--\n");
                         system("mprev");
                 }
                 else if( buf[0] == RIGHT_KEY )//right key press
                         system("mnext");
                 else if( buf[0] == MID_KEY )//middle key press
		 {
			 printf("--mid key--\n");
			 system("mpause");
//			 usleep(100000);
		 }
		 else if ( buf[3] == WH_UP ){
//                       system("vup");
                         if(volume_val<125){
                                 volume_val+=4;
                                 sprintf(strCMD,"amixer -c 1 set Speaker %d%%",volume_val);
                                 system(strCMD);
			}
                  }
		 else if ( buf[3] == WH_DOWN ){
//                       system("vdown");
                         if(volume_val>5){
                                 volume_val-=4;
                                 sprintf(strCMD,"amixer -c 1 set Speaker %d%%",volume_val);
                                 system(strCMD);
			}
                  }


	}
   }
   close(fd);

   return 0;
}
