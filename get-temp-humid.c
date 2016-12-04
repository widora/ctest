#include <stdio.h>
//#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define SET_GPIO39 0
#define SET_GPIO40 1
#define SET_GPIO41 2
#define SET_GPIO42 3

int main(int argc, char **argv)
{
	int fd;
	unsigned int dht11_data = 0;
        unsigned int int_tmp = 0;
	float humi,temp;
        unsigned int pin_num;         

        //------- check input syntax  DEFAUL GPIO40	
        if((argc>1 && (atoi(argv[1])<39 || atoi(argv[1])>42)) || (argv[1]=="-h"))
         {
          printf("Syntax:%s pin_number   defaul:GPIO40\n",argv[0]);
          printf("Example:  %s 40 \n",argv[0]); 
          printf("please select one of GPIO pin_number from 39-42 for device connection! \n");
          return 1;
         }

	//------- open driver--------
	fd = open("/dev/DHT11", O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		printf("can't open /dev/DHT11\n");
		return -1;
	}
     //------------- set GPIO pin number for device connection ---------
     if(argc==1)
         pin_num=40; //----default pin number 40
     else
         pin_num=atoi(argv[1]);       
     switch(pin_num)
     {
       case 39:
         ioctl(fd,SET_GPIO39);
         break;
       case 40:
         ioctl(fd,SET_GPIO40);
         break;    
       case 41:
         ioctl(fd,SET_GPIO41);
         break;
       case 42:
         ioctl(fd,SET_GPIO42);
         break;

       default:
         printf("Please enter valid GPIO number: 39-42\n");
         return 1;
         break;

      }        

    //------------------------------------- read  Temperature and humidity -------
        while(dht11_data==0)
        {
            read(fd, &dht11_data, sizeof(dht11_data));
            //printf("dht11_data = 0x%08x\n",dht11_data);
        }
	int_tmp = (dht11_data>>16);
        temp = (int_tmp>>8) + (int_tmp&0x000000ff)/256.0;
        int_tmp = dht11_data&0x0000ffff;
	humi = (int_tmp>>8) + (int_tmp&0x000000ff)/256.0;
        printf("%4.1f\n",temp);
        printf("%4.1f\n",humi);
        //sleep(1); //--wait at least 1s as per DHT11 datasheet

	close(fd);
	return 0;
}
