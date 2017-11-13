#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ipcsock_common.h"


int main(void)
{
	int i;
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

	//------ set a low speed for test  ------
	msg_dat.msg_id=IPCMSG_PWM_THRESHOLD; // msg_dat for pwm threshold control
	msg_dat.dat=350;
	usleep(300000);
	//------ loop sending motor direction msg to IPCSockServer ------
	while(1){
		//-- pwm threshold range [0 high_speed - 400 low_speed]

		//--- pthread_mute_lock here msg_dat here.....
		//msg_dat.msg_id=IPCMSG_MOTOR_DIRECTION; // msg_dat for motor direction
		//msg_dat.dat=IPCDAT_MOTOR_FORWARD; // run forward
		msg_dat.msg_id=IPCMSG_PWM_THRESHOLD; // must set msg_id each time!
		msg_dat.dat=350;
		printf("set smg_dat.msg_id=%d, msg_dat.dat=%d \n",msg_dat.msg_id,msg_dat.dat);
		//--- pthread_mute_unlock  msg_dat here .....
		for(i=0;i<6;i++)
			usleep(500000);

		//--- pthread_mute_lock here msg_dat here.....
		//msg_dat.msg_id=IPCMSG_MOTOR_DIRECTION; // msg_dat for motor direction
		//msg_dat.dat=IPCDAT_MOTOR_REVERSE; // run forward
		msg_dat.msg_id=IPCMSG_PWM_THRESHOLD; // set msg_id each time!
		msg_dat.dat=-350;
		printf("set smg_dat.msg_id=%d, msg_dat.dat=%d \n",msg_dat.msg_id,msg_dat.dat);
		for(i=0;i<6;i++)
			usleep(500000);

	}

	//------- end thread -----
	pthread_join(thread_IPCSockClient,NULL);

	return 0;
}

