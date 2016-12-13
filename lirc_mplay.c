#include <stdio.h>
//#include <sys/socket.h>  //connect,send,recv,setsockopt
//#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>  //sockaddr_in,htons..
//#include <netinet/tcp.h>
#include <unistd.h> //read,write
#include <stdlib.h> //--exit()
#include <errno.h>
#include <fcntl.h> //---open()
#include <string.h> //---strcpy()
#include <stdbool.h> // true,false

#define CODE_NUM_0 22
#define CODE_NUM_1 12
#define CODE_NUM_2 24
#define CODE_NUM_3 94
#define CODE_NUM_4 8
#define CODE_NUM_5 28
#define CODE_NUM_6 90
#define CODE_NUM_7 66
#define CODE_NUM_8 82
#define CODE_NUM_9 74

#define CODE_MODE 70
#define CODE_MUTE 71
#define CODE_NEXT 67
#define CODE_PREV 64
#define CODE_PLAY_PAUSE 68
#define CODE_VOLUME_UP 9
#define CODE_VOLUME_DOWN 21

#define MAXLINE 100

char str_dev[]="/dev/LIRC_dev";
unsigned int PLAY_LIST_NUM=2; //---default playlist

int main(int argc, char** argv)
{

//----------- LIRC ------------
   int fd;
   bool flag_3D=false;
   unsigned int LIRC_DATA = 0; //---raw data from LIRC module 
   unsigned int LIRC_CODE =0;
   unsigned int volume_val=100;
   char strCMD[50];

//---------------  OPEN LIRC DEVICE   --------------
    fd = open(str_dev, O_RDWR | O_NONBLOCK);
    if (fd < 0)
     {
        printf("can't open %sn",str_dev);
        return -1;
      }

//------------------------- loop for input command and receive response ---------------------
while(1)
  {
     //--------------- receive LIRC data ---------
     read(fd, &LIRC_DATA, sizeof(LIRC_DATA));
     if(LIRC_DATA!=0)
         {
           printf("LIRC_DATA: 0x%0x\n",LIRC_DATA);  
           LIRC_CODE=(LIRC_DATA>>16)&0x000000ff;
           printf("LIRC_CODE: %d\n",LIRC_CODE);

           switch(LIRC_CODE)
           {
               case CODE_NEXT:
 		    system("echo 'pt_step 1'>/mplayer/slave");
 		    printf("echo 'pt_step 1'>/mplayer/slave \n");
                    break;
               case CODE_PREV:
 		    system("echo 'pt_step -1'>/mplayer/slave");
 		    printf("echo 'pt_step -1'>/mplayer/slave \n");
                    break;
               case CODE_PLAY_PAUSE:
                     break;
               case CODE_VOLUME_UP: 
                    if(volume_val<125)
                        volume_val+=2;
                    sprintf(strCMD,"amixer set Speaker %d",volume_val);
                    system(strCMD);
                    printf("%s \n",strCMD);
                    break;
               case CODE_VOLUME_DOWN: 
                    if(volume_val>20)
                        volume_val-=2;
                    sprintf(strCMD,"amixer set Speaker %d",volume_val);
                    system(strCMD);
                    printf("%s \n",strCMD);
                    break; 
      	       case CODE_MODE:
		    if(flag_3D)
                    {
         		system("amixer set 3D 0");
                        printf("amixer set 3D 0 \n");
                        flag_3D=false;
                    }
                    else
                    {
                        system("amixer set 3D 15");
                        printf("amixer set 3D 15 \n");
                        flag_3D=true;	
                     }
                     break;
               case CODE_MUTE: 
                    system("echo 'mute'>/mplayer/slave");
                    printf("echo 'mute'>/mplayer/slave \n");
                    break;
               default: 

                 switch(LIRC_CODE)
                  {
                     case CODE_NUM_1:PLAY_LIST_NUM=1;break;
                     case CODE_NUM_2:PLAY_LIST_NUM=2;break;
                     case CODE_NUM_3:PLAY_LIST_NUM=3;break;
                     case CODE_NUM_4:PLAY_LIST_NUM=4;break;
                     case CODE_NUM_5:PLAY_LIST_NUM=5;break;
                     case CODE_NUM_6:PLAY_LIST_NUM=6;break;
                     default:
                        printf("Unrecognizable code! \n");
                   }

                printf("PLAY_LIST_NUM =%d \n",PLAY_LIST_NUM);          
                usleep(100000);continue;
           } 
     }
     else //--- ii LIRC_DATA==0
          {
              usleep(100000);
              continue;  //---- no LIRC data received
           }

    usleep(100000); //---sleep

  } //--end while()

  close(fd);
  return 0;

}
