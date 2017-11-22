/*---------------------------------------------------
TODOs and BUGs
1. High CPU usage of the process.

----------------------------------------------------*/

#include <string.h>
#include <signal.h>
#include "i2c_oled_128x64.h"
#include <sys/msg.h>
#include "msg_common.h"

//===================== MAIN ==================

void main()
{
 int i=0,k;
 char strtime[10]={0};

//---test non_ASCII char
 char ascii_127[3];
 ascii_127[0]=129;
 ascii_127[1]=28;
 ascii_127[2]='\0';

 //--- for IPC message queque --
 int msg_ret;

 //----for time string ----
 time_t timep;
 struct tm *p_tm;


 //---init i2c interface -----
 init_I2C_Slave();
 //---init oled ----
 initOledDefault();

 //---init timer----
 initOledTimer();
 signal(SIGALRM,sigHndlOledTimer);//--renew CC1101 and Ting RSSI msg here

 //----clear OLED display--- !!!!!!!
 clearOledV();
 usleep(300000);

 //---- create message queue ------
 if((g_msg_id=createMsgQue(MSG_KEY_OLED_TEST))<0)
 {
	printf("create message queue fails!\n");
	exit(-1);
  }

  drawOledStr16x8(0,0,ascii_127);//test last ASCII symbol
  drawOledStr16x8(6,0,"-- Widora-NEO --");
  drawOledStr16x8(0,120,ascii_127);//test last ASCII symbol

  while(1)
  {
	//---- get time string ----
	timep=time(NULL);
	p_tm=localtime(&timep);
	strftime(strtime,8,"%H:%M:%S",p_tm);
	//---- push strtime to oled frame buff ---
	push_Oled_Ascii32x18_Buff(strtime,0,3);

	//---- oled show time ----
//	drawOledStr16x8(0,31,strtime);


	//------ example of vertical scrolling -----
/*
	if(i<64)
	{
		setStartLine(i);
		i+=2;
		if(i==64) i=0;
	}
*/
	//------- display Ting and CC1101 Msg. ------
//	drawOledStr16x8(2,0,g_strTingBuf);
//	drawOledStr16x8(4,0,g_strCC1101Buf);

	//------- refresh OLED frame buff --------
	refresh_Oled_Ascii32x18_Buff(false);

	//----- sleep a while ----
	usleep(200000); //  --200k ~1000k same cpu load
  }

 //--- delete message queue -----
/*
  if(msgctl(msg_id,IPC_RMID,0)<0)
	printf("msgctl IPC_RMID failed!\n");
  else
	printf("Old data in IPC deleted!\n");
*/

  //---free i2c ioctl data mem---
   free_I2C_IOdata();
  //---unlock i2c fd ----
  intFcntlOp(g_fdOled, F_SETLK, F_UNLCK, 0, SEEK_SET,10);

}
