#include <string.h>
#include <signal.h>
#include "i2c_oled.h"


//======================== MAIN ==================

void main()
{
 int k;
 char strtime[10]={0};
 time_t timep;
 struct tm *p_tm;

 //---init oled ----
 initOledDefault();
 //---init timer----
 initOledTimer();
 signal(SIGALRM,sigHndlOledTimer);
 //----clear OLED display----FAIL !!!!!!!
 clearOled();


//---- clear OLED display-----
/*
 for(k=0;k<8;k++)
	drawOledStr16x8(k,0,"                ");
*/


 drawOledStr16x8(0,0,"  Widora-NEO! ");
 drawOledStr16x8(6,0," Test for Ting");

  while(1)
  {
	//---get time string---
	timep=time(NULL);
	p_tm=localtime(&timep);
	strftime(strtime,sizeof(strtime),"%H:%M:%S",p_tm);
	printf("strtime=%s\n",strtime);
	drawOledStr16x8(2,0,"    ");
	drawOledStr16x8(2,31,strtime);
	drawOledStr16x8(2,95,"    ");

	usleep(500000); //--200k ~1000k same cpu load
  }


  //---unlock i2c fd ----
  intFcntlOp(g_fdoled, F_SETLK, F_UNLCK, 0, SEEK_SET,10);

}
