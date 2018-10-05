#include <stdio.h>
//#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

char str_dev[]="/dev/LIRC_dev";

int main(int argc, char **argv)
{
    int fd;
    unsigned int LIRC_DATA = 0;
    //------- open driver--------
    fd = open(str_dev, O_RDWR | O_NONBLOCK);
    if (fd < 0)
     {
	printf("can't open %s\n",str_dev);
	return -1;
      }

    //------------- read LIRC_DATA -------
    while(1)
    {
	read(fd, &LIRC_DATA, sizeof(LIRC_DATA));
        if(LIRC_DATA!=0)
             printf("LIRC receive data: 0x%0x\n",LIRC_DATA);
        usleep(100000); //--wait 
     }

    close(fd);
    return 0;
}
