#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> //unix std socket

#define  IPC_SOCK_PATH "/tmp/ipc_socket"

struct struct_msg_dat{
int dat;
char name[50];
} msg_dat;

int main(void)
{
	struct sockaddr_un svr_unaddr;
	struct sockaddr_un clt_unaddr;
	socklen_t clt_unaddr_len;
	char msg_buf[1024];
	int clt_fd;//server and connecting client FD
	int len;
	int ret;
	int nread;

	//----- 1. create ipc socket
	clt_fd = socket(PF_UNIX,SOCK_STREAM,0);
	if(clt_fd < 0){
		perror("Fail to create IPC client socket!");
		return -1;
	}

	//----- 2. specify ipc socket path, which should be created by the server.
	svr_unaddr.sun_family=AF_UNIX; 
	strncpy(svr_unaddr.sun_path,IPC_SOCK_PATH,sizeof(svr_unaddr.sun_path)-1); // ??? why -1

	//----- 3. connect to ipc socket server
	ret=connect(clt_fd,(struct sockaddr*)&svr_unaddr,sizeof(svr_unaddr));
	if(ret == -1){
		perror("Fail to connect to ipc socket server");
		close(clt_fd);
		return -2;
	}

	//----- 5. send message to ipc socket server
//	memset(msg_buf,0,sizeof(msg_buf));
//	strcpy(msg_buf,"Hello message from IPC socket lient!");
//	write(clt_fd,msg_buf,sizeof(msg_buf));
	msg_dat.dat=123456;
	strcpy(msg_dat.name,"Hello World!");
	write(clt_fd,&msg_dat,sizeof(msg_dat));

	//----- 7. complete session
	close(clt_fd);
	return 0;
}

