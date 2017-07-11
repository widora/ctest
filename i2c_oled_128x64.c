#include <string.h>
#include <signal.h>
#include "i2c_oled_128x64.h"
#include <sys/msg.h>
#include "msg_common.h"

//===================== MAIN ==================

void main()
{
 int k;
 char strtime[10]={0};
 char strRSSI[5]={0};
 char strOledBuf[16]={0}; //--for oled line show
 //----for message IPC ----
 int msg_id=-1;
 struct st_msg msg_data; //message data
 long int msg_type=0; //message type
 struct msqid_ds msg_conf;//msg configuration set

 //----for time string ----
 time_t timep;
 struct tm *p_tm;


 //---init oled ----
 initOledDefault();
 //---init timer----
 initOledTimer();
 signal(SIGALRM,sigHndlOledTimer);

 //----clear OLED display--- !!!!!!!
 clearOledV();
 usleep(300000);

 //----- create message IPC ------
  msg_id=msgget((key_t)5678,0666 | IPC_CREAT);
  if(msg_id<0)
  {
	perror("msgget error");
 	exit(-1);
   }
  else
	printf("msgget: IPC create successfully!\n");

  //----!!!! Minimize msg queue size to accumulation of msg -------
  if(msgctl(msg_id,IPC_STAT,&msg_conf)==0)
  {
	msg_conf.msg_qbytes=65;
	if(msgctl(msg_id,IPC_SET,&msg_conf)==0) 
		printf("msgctl: reset msg_conf successfully!\n");
  }
/*
 //--- delete obselete data in IPC if any
  if(msgctl(msg_id,IPC_RMID,0)<0)
	printf("msgctl IPC_RMID failed!\n");
  else
	printf("Old data in IPC deleted!\n");
*/

  drawOledStr16x8(0,0,"  Widora-NEO! ");

  while(1)
  {
	//---- get time string ----
	timep=time(NULL);
	p_tm=localtime(&timep);
	strftime(strtime,8,"%H:%M:%S",p_tm);
	printf("strtime=%s\n",strtime);

	//---- oled show ----
//	drawOledStr16x8(2,0,"    ");
	drawOledStr16x8(2,31,strtime);
//	drawOledStr16x8(2,95,"    ");

	//-----get IPC message-----
	if(msgrcv(msg_id,(void *)&msg_data,MSG_BUFSIZE,msg_type,0) < 0)
	{
		printf("No IPC message received!\n");
		//---put IPC msg to oled ---
		drawOledStr16x8(4,0," Ting: No data! ");
	}
	else
	{
		printf("IPC Message:%s\n",msg_data.text);
		//---get RSSI--
		strncpy(strRSSI,msg_data.text+3,4);
		//---put IPC msg to oled ---
		sprintf(strOledBuf," Ting: %d dbm ",atoi(strRSSI));
		drawOledStr16x8(4,0,strOledBuf);//msg_data.text);
	}

	//----- sleep a while ----
	usleep(300000); // to short time may blur oled --200k ~1000k same cpu load
  }
  //---delete IPC message----

  //---unlock i2c fd ----
  intFcntlOp(g_fdoled, F_SETLK, F_UNLCK, 0, SEEK_SET,10);

}
