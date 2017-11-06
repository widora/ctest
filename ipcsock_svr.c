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
	struct sockaddr_un svr_unaddr;//UNIX type socket address
	struct sockaddr_un clt_unaddr;
	socklen_t clt_unaddr_len;
	char msg_buf[1024];
	int svr_fd,clt_fd;//server and connecting client FD
	int len;
	int ret;
	int nread;

	//----- 1. create ipc socket
	svr_fd = socket(PF_UNIX,SOCK_STREAM,0);
	if(svr_fd < 0){
		perror("Fail to create IPC server socket!");
		return -1;
	}
	//----- 2. specify ipc socket path name 
	svr_unaddr.sun_family=AF_UNIX; 
	strncpy(svr_unaddr.sun_path,IPC_SOCK_PATH,sizeof(svr_unaddr.sun_path)-1); // ??? why -1
	unlink(IPC_SOCK_PATH); //if same name path exists, delete it.

	//----- 3. bind socket with addr.
	ret=bind(svr_fd,(struct sockaddr*)&svr_unaddr,sizeof(svr_unaddr));
	if(ret ==-1){
		perror("Fail to bind ipc socket with unix address");
		close(svr_fd);
		unlink(IPC_SOCK_PATH);
		return -2;
	}

	//----- 4. listen svr_fd
	ret=listen(svr_fd,1);
	if(ret == -1){
		perror("Fail to listen to svr_fd");
		close(svr_fd);
		unlink(IPC_SOCK_PATH);
		return -3;
	}

	//----- 5. accept request connection
	len=sizeof(clt_unaddr);
	clt_fd=accept(svr_fd,(struct sockaddr*)&clt_unaddr,&len);
	if(clt_fd <0 ){
		perror("Fail to accept connecting client requent!");
		close(svr_fd);
		unlink(IPC_SOCK_PATH);
		return -4;
	}

	//----- 6. get client messge
//	memset(msg_buf,0,1024);
//	nread=read(clt_fd,msg_buf,sizeof(msg_buf));
//	printf("Message from client: %s \n",msg_buf);
	memset(&msg_dat,0,sizeof(msg_dat));
	nread = read(clt_fd,&msg_dat,sizeof(msg_dat));
	printf("msg_dat from client dat: %d  name: %s \n",msg_dat.dat,msg_dat.name);

	//----- 7. complete session
	close(clt_fd);
	close(svr_fd);
	unlink(IPC_SOCK_PATH);
	return 0;
}

