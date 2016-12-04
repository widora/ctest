#include <stdio.h>
//#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

char str_dev[]="/dev/gpio_int_dev";

int main(int argc, char **argv)
{
    int fd;
    unsigned int INT_STATUS = 0;
    //------- open driver--------
    fd = open(str_dev, O_RDWR | O_NONBLOCK);
    if (fd < 0)
     {
	printf("can't open %s\n",str_dev);
	return -1;
      }

    //------------- read INT_STATUS -------
    while(1)
    {
	read(fd, &INT_STATUS, sizeof(INT_STATUS));
        if(INT_STATUS==1)
             printf("GPIO Interrupt Triggered!\n");
        usleep(200000); //--wait 
     }

    close(fd);
    return 0;
}
