#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ipcsock_common.h"


int main(void)
{
        //-- for IPC Msg Sock ---
        pthread_t thread_IPCSockClient;
	int pret;

	//----- clear msg_dat before IPCSockClient session ----
	memset(&msg_dat,0,sizeof(struct struct_msg_dat));//set msg_dat.msg_id=0, to prevent IPSockClient from sending it.

        //----- create IPC Sock Client thread -----
        pret=pthread_create(&thread_IPCSockClient,NULL,(void *)&create_IPCSock_Client,&msg_dat);
	if(pret != 0){
		printf("Fail to create thread for IPC Sock Client!\n");
		exit -1;
	}

	//------ loop sending pwm threshold to IPCSockServer ------
	msg_dat.msg_id=IPCMSG_PWM_THRESHOLD; // msg_dat for pwm threshold control
	while(1){
		//-- pwm threshold range [0 - 400]
		msg_dat.dat=120;
		usleep(900000);
		msg_dat.dat=350;
		usleep(900000);
	}

	//------- end thread -----
	pthread_join(thread_IPCSockClient,NULL);

	return 0;
}

