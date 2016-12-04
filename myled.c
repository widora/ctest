#include <stdio.h>  // ---printf
//#include <string.h>
#include <fcntl.h>  //---open
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h> //-----??
//#include <sys/ioctl.h>

#define MYLED_ON 1
#define MYLED_OFF 0

char str_dev[]="/dev/midas_driver";

int main(int argc,char* argv[])
{
 int fd;

 if(argc!=2)
 {
    printf(" %s <on|off> to turn on or off LED on GPIO17\n",argv[0]);
    return -1;
 }

 fd=open(str_dev,O_RDWR|O_NONBLOCK); //--- GPIO17 will set to 0 when open
 if(fd<0)
 {
    printf("can't open %s \n",str_dev);
    return -1;
 }

 if(!strcmp("on",argv[1]))
  {
    while(1)
      {
        ioctl(fd,MYLED_ON);
        usleep(50000);
        ioctl(fd,MYLED_OFF);
        usleep(50000);
       }
  }

 else if(!strcmp("off",argv[1]))
   ioctl(fd,MYLED_OFF);
 else
  {
    printf(" %s <on|off> to turn on or off LED on GPIO17\n",argv[0]);
    return -1;
  }

return 0;

}
