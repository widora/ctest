#include <stdio.h>
#include <stdlib.h> //---system()
//#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h> //--strcat()

#define CODE_NEXT 67
#define CODE_PREV 64
#define CODE_PLAY_PAUSE 68
#define CODE_VOLUME_UP 9
#define CODE_VOLUME_DOWN 21

char str_dev[]="/dev/LIRC_dev";

int main(int argc, char **argv)
{
    int fd;
    int ret;
    unsigned int LIRC_DATA = 0; //---raw data from LIRC module 
    unsigned int LIRC_CODE =0;
    unsigned int volume_val=105;
    char str_tmp[5];
    char str_cmd_vol[50];

    //------- open driver--------
    fd = open(str_dev, O_RDWR | O_NONBLOCK);
    if (fd < 0)
     {
	printf("can't open %sn",str_dev);
	return -1;
      }

    //------------- read LIRC_DATA -------
    while(1)
    {
	ret=read(fd, &LIRC_DATA, sizeof(LIRC_DATA));
	printf("-----  read ret=%d -----\n",ret);
        if(LIRC_DATA!=0)
         {
           printf("LIRC_DATA: 0x%0x\n",LIRC_DATA);  
           LIRC_CODE=(LIRC_DATA>>16)&0x000000ff;
           printf("LIRC_CODE: %d\n",LIRC_CODE);
/*
           switch(LIRC_CODE)
           {
             case CODE_NEXT: system("echo 'pt_step 1' > /home/slave");break;
             case CODE_PREV: system("echo 'pt_step -1' > /home/slave");break;
             case CODE_PLAY_PAUSE: system("echo pause > /home/slave");break;
             //case CODE_VOLUME_UP: system("echo 'volume +10' > /home/slave");break;
             case CODE_VOLUME_UP: 
                 {
                     volume_val+=3;
                     if(volume_val>127)volume_val=127;
                     sprintf(str_cmd_vol,"amixer set Speaker %d",volume_val);
                     system(str_cmd_vol);
                  break;
                 }
             case CODE_VOLUME_DOWN:
                 {
                     volume_val-=3;
                     if(volume_val<50)volume_val=50;
                     sprintf(str_cmd_vol,"amixer set Speaker %d",volume_val);
                     system(str_cmd_vol);
                  break;
                 }


             //default:
           } 
*/

         }            

        usleep(100000); //--wait 
     }

    close(fd);
    return 0;
}
