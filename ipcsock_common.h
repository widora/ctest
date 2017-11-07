#ifndef __IPCSOCK_COMMON_H__
#define __IPCSOCK_COMMON_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> //unix std socket

#define  IPC_SOCK_PATH "/tmp/ipc_socket"

//----- define IPC MESSAGE CODE -----
#define IPCMSG_NONE 0 //msg cleared token, ignore this msg dat, no need to send or parse
#define IPCMSG_PWM_THRESHOLD 1  // 1 for pwm threshold
#define IPCMSG_MOTOR_DIRECTION 2 // dat=0 run, dat=1 reverse
#define IPCMSG_MOTOR_STATU 3  // dat=0 stop, dat=1 run

//----- IPC Message Data struct -----
struct struct_msg_dat{
int msg_id; //--type of dat
int dat;
};

static struct struct_msg_dat msg_dat;


/*---------------------------------------------------------
1. create IPC server socket,
2. bind it with UNIX local addr(IPC_SOCK_PATH)
3. listen to the server sock.
4. accept client connection.
5. loop in receiving and update msg_dat.
return <0 if fail.
-----------------------------------------------------------*/
static int create_IPCSock_Server(struct struct_msg_dat *pmsg_dat)
{
	struct sockaddr_un svr_unaddr;//UNIX type socket address
	struct sockaddr_un clt_unaddr;
	int svr_fd;//server IPC sock fd
	int clt_fd;//client IPC sock fd
	int len;
	int nread;
	int ret;

	//------ reset msg_data
	memset(pmsg_dat,0,sizeof(struct struct_msg_dat));

	//----- 1. create ipc socket
	svr_fd = socket(PF_UNIX,SOCK_STREAM,0);
	if(svr_fd < 0){
		perror("Fail to create IPC server socket!");
//		return -1;
	}

	//----- 2. specify ipc socket path name 
	svr_unaddr.sun_family=AF_UNIX; 
	strncpy(svr_unaddr.sun_path,IPC_SOCK_PATH,sizeof(svr_unaddr.sun_path)-1); // ??? why -1, end of cstring?
	unlink(IPC_SOCK_PATH); //if same name path exists, delete it.

	//----- 3. bind socket with addr.
	ret=bind(svr_fd,(struct sockaddr*)&svr_unaddr,sizeof(svr_unaddr));
	if(ret ==-1){
		perror("Fail to bind ipc socket with unix address");
		close(svr_fd);
		unlink(IPC_SOCK_PATH);
//		return -2;
	}

	//----- 4. listen svr_fd
	ret=listen(svr_fd,1);
	if(ret == -1){
		perror("Fail to listen to svr_fd");
		close(svr_fd);
		unlink(IPC_SOCK_PATH);
//		return -3;
	}

	//----- 5. accept request connection
	len=sizeof(clt_unaddr);
	clt_fd=accept(svr_fd,(struct sockaddr*)&clt_unaddr,&len);
	if(clt_fd <0 ){
		perror("Fail to accept connecting client requent!");
		close(svr_fd);
		unlink(IPC_SOCK_PATH);
//		return -4;
	}

	//----- 6. loop: get client messge
	while(1){

		//--lock msg_dat before read.....

		nread = read(clt_fd,pmsg_dat,sizeof(struct struct_msg_dat));

		if( nread>0 && nread != sizeof(struct struct_msg_dat)){
			printf("Received msg_dat is NOT complete!");
			pmsg_dat->msg_id = IPCMSG_NONE; // mark invalid msg_dat received.
		}

		else if (nread == sizeof(struct struct_msg_dat))
			printf("msg_dat from client msg_id: %d  dat: %d \n",pmsg_dat->msg_id,pmsg_dat->dat);

		//--unlock msg_dat before read.....

		usleep(200000);

	}//while
}


/*---------------------------------------------------------
1. create IPC client socket,
2. connect to the server sock.
3. accept client connection.
4. loop in receiving and update msg_dat.
5. close sock fd
return <0 if fail
-----------------------------------------------------------*/
static int create_IPCSock_Client(struct struct_msg_dat *pmsg_dat)
{
        struct sockaddr_un svr_unaddr;
//        struct sockaddr_un clt_unaddr;
//        socklen_t clt_unaddr_len;
        int clt_fd;//server and connecting client FD
//        int len;
        int ret;
        int nwrite;

        //----- 1. create ipc socket
        clt_fd = socket(PF_UNIX,SOCK_STREAM,0);
        if(clt_fd < 0){
                perror("Fail to create IPC client socket!");
                return -1;
        }
	else
		printf("IPCSockClient: Succeed to create IPC Sock FD!\n");

        //----- 2. specify ipc socket path, which should be created by the server.
        svr_unaddr.sun_family=AF_UNIX; 
        strncpy(svr_unaddr.sun_path,IPC_SOCK_PATH,sizeof(svr_unaddr.sun_path)-1); // ??? why -1? end of cstring ??

        //----- 3. connect to ipc socket server
        ret=connect(clt_fd,(struct sockaddr*)&svr_unaddr,sizeof(svr_unaddr));
        if(ret == -1){
                perror("Fail to connect to ipc socket server");
                close(clt_fd);
                return -2;
        }
	else
		printf("IPCSockClient: Succeed to connect to server!\n");

	//---- !!!!! reset msg_dat in application function -------

        //----- 4. loop send message to ipc socket server
	while(1){
		if(pmsg_dat->msg_id != IPCMSG_NONE){ //Only if msg_dat is valid.
        		nwrite=write(clt_fd,pmsg_dat,sizeof(struct struct_msg_dat));
			if(nwrite != sizeof(struct struct_msg_dat)){//if invalid msg_dat received
				printf("IPCSock_Client: nwrite=%d ,while size of strut_msg_dat is %d, NOT complete! \n",nwrite,sizeof(struct struct_msg_dat));

			}
			else{
				//-- set msg_id IPCMSG_NONE after finish nwrite
				pmsg_dat->msg_id = IPCMSG_NONE;
				printf("IPCSockClient: Succeed to write msg_dat to IPC Socket!\n");
			}
		}

		usleep(20000);

	}//while

        //----- 5. complete session
        close(clt_fd);


}

#endif
