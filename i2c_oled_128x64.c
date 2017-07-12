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

 //--- for IPC message queque --
 int msg_id=-1;
 key_t msg_key=5678;
 int msg_ret;

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

 //---- create message queue ------
 if((msg_id=createMsgQue(MSG_KEY_OLED_TEST))<0)
 {
	printf("create message queue fails!\n");
	exit(-1);
 }


  drawOledStr16x8(0,0,"  Widora-NEO! ");

  while(1)
  {
	//---- get time string ----
	timep=time(NULL);
	p_tm=localtime(&timep);
	strftime(strtime,8,"%H:%M:%S",p_tm);
	printf("strtime=%s\n",strtime);

	//---- oled show ----
	drawOledStr16x8(0,0,"    ");
	drawOledStr16x8(0,31,strtime);
	drawOledStr16x8(0,95,"    ");

	//------- receive mssage queue from Ting ------
        msg_ret=recvMsgQue(msg_id,MSG_TYPE_TING);
	if(msg_ret >0)
	{
		//---get RSSI--
		strncpy(strRSSI,g_msg_data.text+3,4);
		//---put to oled ---
		sprintf(strOledBuf,"Ting: %ddBm   ",atoi(strRSSI));
		drawOledStr16x8(2,0,strOledBuf);
	}
	else if(msg_ret != EAGAIN) //--bypass EAGAIN
		drawOledStr16x8(2,0," Ting: No data  ");

	//------- receive mssage queue from CC1101 ------
        msg_ret=recvMsgQue(msg_id,MSG_TYPE_CC1101);
	if(msg_ret > 0 )
	{
		//---put to oled ---
		sprintf(strOledBuf,"CC1101: %s  ",g_msg_data.text);
		drawOledStr16x8(4,0,strOledBuf);
	}
	else if(msg_ret != EAGAIN)
		drawOledStr16x8(4,0,"CC1101: No data!");


	//----- sleep a while ----
	usleep(300000); // to short time may blur oled --200k ~1000k same cpu load
  }

 //--- delete message queue -----
/*
  if(msgctl(msg_id,IPC_RMID,0)<0)
	printf("msgctl IPC_RMID failed!\n");
  else
	printf("Old data in IPC deleted!\n");
*/

  //---unlock i2c fd ----
  intFcntlOp(g_fdoled, F_SETLK, F_UNLCK, 0, SEEK_SET,10);

}
