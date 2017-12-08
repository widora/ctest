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
#define SERVER_LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512


int main(int argc, char **argv)
{
int socket_desc;
struct sockaddr_in server_addr;
int rec_len;
FILE * fp;
char file_name[FILE_NAME_MAX_SIZE+1];
char buffer[BUFFER_SIZE];

bzero(&server_addr,sizeof(server_addr));
server_addr.sin_family = AF_INET; //Address Family IP version 4
server_addr.sin_addr.s_addr = htons(INADDR_ANY); //host to net short
server_addr.sin_port = htons(FILE_SERVER_PORT);

socket_desc = socket(AF_INET,SOCK_STREAM,0); //SOCK_STREAM -- connection oriented TCP protocol,  0--or IPPROTO_IP is IP protocol
if(socket_desc < 0)
{
	printf("create server socket failed!\n");
	exit(-1);
}

//-----  int bind(int sock, struct sockaddr *addr, int addrLen) -----
if( bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0 )
{
	printf("Failed to bind server socket with port %d!\n",FILE_SERVER_PORT);
	exit(-1);
}

//----- int listen(int sock, int backlog) -----
if( listen(socket_desc,SERVER_LISTEN_BACKLOG) != 0)
{
	printf("listen to prot %d failed!\n",FILE_SERVER_PORT);
	exit(-1);
}

//-----loop server-------
while(1){
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	//---- int accept( int sock, struct sockaddr *addr, int *addrLen ) ------
	int new_socket_desc = accept(socket_desc, (struct sockaddr*)&client_addr, &client_addr_len);
	if (new_socket_desc < 0 )
	{
		printf("accept new socket failed");
		break;
	}

	//-----  receive file name  ---------
	bzero(buffer,BUFFER_SIZE);
	rec_len = recv(new_socket_desc, buffer, BUFFER_SIZE,0);
	if(rec_len < 0)
	{
		printf(" receive data failed!\n");
		break;
	}
	bzero(file_name,FILE_NAME_MAX_SIZE+1);
	strncpy(file_name,buffer, strlen(buffer)>FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE:strlen(buffer));

	//-------  send file data  -------
	fp = fopen(file_name, "r");
	if(fp == NULL)
	{
		printf("fail to open file %s!\n", file_name);
	}
	else
	{
		bzero(buffer, BUFFER_SIZE);
		int file_block_length=0;
		//----- read file and send()  ------
		while( (file_block_length = fread(buffer,sizeof(char),BUFFER_SIZE,fp)) >0 )
		{
			printf("file_block_length = %d \n",file_block_length);
			//----- int send( int sock, const void *msg, int len, unsigned int flag ) --------
			if(send(new_socket_desc, buffer, file_block_length,0)<0)
			{
				printf("send file: %s failed!\n",file_name);
			}
			bzero(buffer, BUFFER_SIZE);
		}//end while()
		fclose(fp);
		printf("Finish sending file: %s!\n",file_name);
	}//end else{}

	close(new_socket_desc);

}//end while()

return 0;
}
