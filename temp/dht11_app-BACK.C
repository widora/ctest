#include <stdio.h>
//#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
	int fd;
	unsigned int dht11 = 0;
	unsigned int humi,temp;

	//--------OPEN DRIVER--------
	fd = open("/dev/dht11", O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		printf("can't open /dev/dht11\n");
		return -1;
	}
    while(1)
    {
	read(fd, &dht11, sizeof(dht11));
	temp = dht11>>8;
	humi = dht11&0x000000ff;
        if(temp!=0 && humi!=0)
        {
         printf("Current temperature: %d deg\n",temp);
         printf("Current humidity:    %d %%\n",humi);
        }
         sleep(1); //--wait at least 1s as per DHT11 datasheet

     }
	close(fd);
	
	return 0;
}
