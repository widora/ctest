/*---------------------------------------------------------------------
Based on: blog.csdn.net/dlutbrucezhang/article/details/8880131

---------------------------------------------------------------------*/

#include <netinet/in.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_SERVER_PORT 5555
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512


int main(int argc, char **argv)
{
int socket_desc;
struct sockaddr_in client_addr;
struct sockaddr_in server_addr;
FILE *fp;
int rec_len;
int wrt_len;
char file_name[FILE_NAME_MAX_SIZE+1];
char buffer[BUFFER_SIZE];

bzero(&client_addr,sizeof(client_addr));
/*
client_addr.sin_family = AF_INET; //Address Family IP version 4
client_addr.sin_addr.s_addr = htons(INADDR_ANY); //host to net short
client_addr.sin_port = htons(0); // auto. port number
*/

//------ check input arguments ------
if(argc !=2)
{
	printf("Usage: ./%s server_IP \n",argv[0]);
	exit(-1);
}

//------ get client socket desc -----
socket_desc = socket(AF_INET,SOCK_STREAM,0); //SOCK_STREAM -- connection oriented TCP protocol,  0--or IPPROTO_IP is IP protocol
if(socket_desc < 0)
{
	printf("create client socket failed!\n");
	exit(-1);
}
/*
//-----  int bind(int sock, struct sockaddr *addr, int addrLen) -----
if( bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0 )
{
	printf("Failed to bind server socket with port %d!\n",FILE_SERVER_PORT);
	exit(-1);
}
*/

//----- server addr prepare -----
bzero(&server_addr,sizeof(server_addr));
server_addr.sin_family = AF_INET;

//------  get server_addr -------
if(inet_aton(argv[1],&server_addr.sin_addr) ==0 )
{
	printf("Server IP address error!\n");
	exit(-1);
}
server_addr.sin_port = htons(FILE_SERVER_PORT);
socklen_t server_addr_length = sizeof(server_addr);


//---- connect to server, int connect( int sock, (struct sockaddr *)servaddr, int addrLen) -----
if( connect(socket_desc,(struct sockaddr*)&server_addr, server_addr_length) < 0)
{
	printf("fail to connect to file server IP %s!\n",argv[1]);
	exit(-2);
}

bzero(file_name,FILE_NAME_MAX_SIZE+1);
printf("please input file name:\t");
scanf("%s",file_name);

bzero(buffer,BUFFER_SIZE);
strncpy(buffer,file_name,strlen(file_name) > BUFFER_SIZE?BUFFER_SIZE:strlen(file_name));
//---send buffer data to server ----
send(socket_desc,buffer,BUFFER_SIZE,0);

fp = fopen(file_name,"w");
if(fp==NULL)
{
	printf("Cann't open file %s to write!\n",file_name);
	exit(-3);
}

//----- receive data from server -------
bzero(buffer,BUFFER_SIZE);
rec_len=0;
//------ receive data from server and write to the file ----------
while( rec_len = recv(socket_desc,buffer,BUFFER_SIZE,0))
{
	if(rec_len <0 )
	{
		printf("Fail to receive file data from the server!\n");
		break;
	}
	//------ write data to file -----
	wrt_len = fwrite(buffer,sizeof(char),rec_len,fp);
	if(wrt_len < rec_len)
	{
		printf("Fail to write data to %s!",file_name);
		break;
	}
	bzero(buffer,BUFFER_SIZE);
}// end of while()

printf("Finish receiving file %s from the server!\n",file_name);

close(fp);
close(socket_desc);

return 0;

}

