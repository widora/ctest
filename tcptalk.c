#include <stdio.h>
//#include <sys/socket.h>  //connect,send,recv,setsockopt
//#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>  //sockaddr_in,htons..
//#include <netinet/tcp.h>
#include <unistd.h> //read,write
#include <stdlib.h> //--exit()
#include <errno.h>

#define MAXLINE 1000

int main(int argc, char** argv)
{
 int sock_fd;
 char recvline[MAXLINE];
 char sendline[MAXLINE];
 struct sockaddr_in server_addr; 

 if(argc<2) //---default server-addr and port
   {
      argv[1]="192.168.3.13";
      argv[2]="55556";
   }   
  
 printf("Please enter IP addr. and PORT, default is  192.168.3.13  55556\n"); 
/*  
 if(argc!=3)
   {
    printf("error:please enter IP and PORT!\n");
    return -1;
   }
*/
 
 if((sock_fd=socket(AF_INET,SOCK_STREAM,0))<0)
  {
    printf("socket error\n");
    return -1;
  }

 memset((void*)&server_addr,0,sizeof(struct sockaddr_in));
 server_addr.sin_family =AF_INET;
 server_addr.sin_port=htons(atoi(argv[2])); //--host to network short

  //-------- transform char type IP to server_add.sin_addr-----------
 if(inet_pton(AF_INET,argv[1],(void*)&(server_addr.sin_addr))<=0)//--pton  presentation to network
  {
   printf("inet_pton error for %s\n",argv[1]); 
   return -1;
  }

   //-------- connect to server ------
  if(connect(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
   {
     printf("can not connet to %s : %s, exit!\n",argv[1],argv[2]);
     printf("%s\n",strerror(errno));
     exit(0);
   }
   else
     printf("Connect to the server successfully!\n");

   //------ recevie data first -------
   if(read(sock_fd,recvline,MAXLINE)==0) //-- it will take some time to read data from socket buffer
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

  //------------------------- loop for input command and receive response ----
  while(fgets(sendline,MAXLINE,stdin))  //-- WARNING!: fgets will read in line-break, and that will be parsed by Server also.
  {
   
   //--------- send data ----------
   sendline[strlen(sendline)-1]=0; //--- get rid of line-break
   if(sendline[0]=='0')
     {
      close(sock_fd);
      printf("user exit.\n");
      return 0;
     }

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

  }

  close(sock_fd);
  return 0;

}
