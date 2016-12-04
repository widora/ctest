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
   unsigned int LIRC_DATA = 0; //---raw data from LIRC module 
   unsigned int LIRC_CODE =0;
   unsigned int volume_val=105;

//------------ socket --------- 
   int sock_fd;
   char recvline[MAXLINE];
   char sendline[MAXLINE];
   struct sockaddr_in server_addr; //--sokect 

 if(argc<2) //---default server-addr and port
   {
      argv[1]="192.168.3.13";
      argv[2]="55555";
   }   
  
 printf("Please enter IP addr. and PORT, default is  192.168.3.13  55555\n"); 
/*  
 if(argc!=3)
   {
    printf("error:please enter IP and PORT!\n");
    return -1;
   }
*/

//------------------------ open LIRC driver -------------------
    fd = open(str_dev, O_RDWR | O_NONBLOCK);
    if (fd < 0)
     {
        printf("can't open %sn",str_dev);
        return -1;
      }

//-----------  get socket file descriptor ----------
 if((sock_fd=socket(AF_INET,SOCK_STREAM,0))<0) 
  {
    printf("socket error\n");
    return -1;
  }
 
 //------socket definition and port binding -----
 memset((void*)&server_addr,0,sizeof(server_addr));
 server_addr.sin_family =AF_INET;
 server_addr.sin_port=htons(atoi(argv[2])); //--host to network short
 //-------- transform char type IP to server_add.sin_addr-----------
 if(inet_pton(AF_INET,argv[1],(void*)&(server_addr.sin_addr))<=0)//--pton  presentation to network
  {
    printf("inet_pton error for %s\n",argv[1]); 
    return -1;
  }

 //-------- connect to server ------
  if(connect(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)//---use file-handler and erver_addrr to connect
   {
     printf("Can not connet to %s : %s, exit!\n",argv[1],argv[2]);
     printf("%s\n",strerror(errno));
     exit(0);
   }
   else
     printf("Connect to the server successfully!\n");

   //------ recevie data first -------
   if(read(sock_fd,recvline,MAXLINE)==0) //-- use socket-file-descriptor to read, it will take some time to read data from socket buffer
    {
      printf("The server  disconnect!\n");
      exit(1);
    }
   
   //printf("Response:%s\n",recvline);
   fputs(recvline,stdout);
   memset(recvline,0,MAXLINE);
   printf("\nNow you can send command to the Mplayer Server.\n");
   printf("fx - Forward x channels, bx - Backward x channels\n");
   printf("lx - Load playlist x.list  vu,vd - volume up & dowm\n"); 

//------------------------- loop for input command and receive response ---------------------
while(1)
  {
     memset(recvline,0,MAXLINE);
     //--------------- receive LIRC data ---------
     read(fd, &LIRC_DATA, sizeof(LIRC_DATA));
     if(LIRC_DATA!=0)
         {
           printf("LIRC_DATA: 0x%0x\n",LIRC_DATA);  
           LIRC_CODE=(LIRC_DATA>>16)&0x000000ff;
           printf("LIRC_CODE: %d\n",LIRC_CODE);

           switch(LIRC_CODE)
           {
/*  following switch structrue will no run, in switch structure, it will jump to case: or default: clause.
              switch(LIRC_CODE)
              {
                 case CODE_NUM_1:PLAY_LIST_NUM=1;break;
                 case CODE_NUM_2:PLAY_LIST_NUM=2;break;
                 case CODE_NUM_3:PLAY_LIST_NUM=3;break;
                 case CODE_NUM_4:PLAY_LIST_NUM=4;break;
                 case CODE_NUM_5:PLAY_LIST_NUM=5;break;
                 case CODE_NUM_6:PLAY_LIST_NUM=6;break;
               }
               sprintf(sendline,"l%d",PLAY_LIST_NUM);break;
               printf("PLAY_LIST_NUM =%s\n",PLAY_LIST_NUM);        
*/
               case CODE_NEXT: strcpy(sendline,"f1");break;
               case CODE_PREV: strcpy(sendline,"b1");break;
               case CODE_PLAY_PAUSE: strcpy(sendline,"ps");break;
               case CODE_VOLUME_UP: strcpy(sendline,"vu");break;
               case CODE_VOLUME_DOWN: strcpy(sendline,"vd");break; 
               case CODE_MUTE: 
                    system("echo 'mute' > /home/slave");
                    printf("echo mute > /home/slave\n");
                    usleep(100000);continue;
               default: 
                 switch(LIRC_CODE)
                  {
                     case CODE_NUM_1:PLAY_LIST_NUM=1;break;
                     case CODE_NUM_2:PLAY_LIST_NUM=2;break;
                     case CODE_NUM_3:PLAY_LIST_NUM=3;break;
                     case CODE_NUM_4:PLAY_LIST_NUM=4;break;
                     case CODE_NUM_5:PLAY_LIST_NUM=5;break;
                     case CODE_NUM_6:PLAY_LIST_NUM=6;break;
                   }
                  sprintf(sendline,"l%d",PLAY_LIST_NUM);break;
                  printf("PLAY_LIST_NUM =%s\n",PLAY_LIST_NUM);          
                  usleep(100000);continue;
           } 
     }
     else
          {
              usleep(100000);
              continue;  //---- no LIRC data received
           }

    //---------- send command line to mplayer server ------------
     printf("sending command %s to mplayer-server...",sendline);
     write(sock_fd,sendline,strlen(sendline));
     //sleep(3); //--wait for soket to send out data in buffer, and for Server to send 2 messages,  
     //---------- receive data --------
     if(read(sock_fd,recvline,MAXLINE)==0)
      {
          printf("The server  disconnect!\n");
          exit(1);
      }
     fputs(recvline,stdout);
     memset(recvline,0,MAXLINE);
     printf("\n");

  usleep(100000); //---sleep

  } //--end while()

  close(sock_fd);
  return 0;

}
