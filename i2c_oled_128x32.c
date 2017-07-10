#include <string.h>
#include <signal.h>
#include "i2c_oled_128x32.h"
#include "my_ip.h"

//======================== MAIN ==================
void main()
{
 int k,len;
 char strtime[10]={0};
 char* pstrIP;
 time_t timep;
 struct tm *p_tm;


 //---init oled ----
 initOledDefault();
 //---init timer----
// initOledTimer();
// signal(SIGALRM,sigHndlOledTimer);
 //----clear OLED display----FAIL !!!!!!!
 clearOled();


//---- clear OLED display-----

 for(k=0;k<8;k++)
	drawOledStr16x8(k,0,"                ");


 drawOledStr16x8(0,0,"   NanoPi-NEO   ");

//  drawOledDat(0xf0);
//  sleep(5);


//    setOledHScroll(0,1); // set scroll page
//  actOledScroll(); // enact scroll
  sleep(1);


 while(1)
  {
      //----- stop scrolling before wrting to GRAM
     // actOledScroll();
      usleep(200000);
      //deactOledScroll();

      //--------IP address--------
      pstrIP=getMyIP();
      len=strlen(pstrIP);
      if(len>16)len=16;
      drawOledStr16x8(0,0,"  ");
      drawOledStr16x8(0,(16-len)*4,pstrIP); //4=8/2
      drawOledStr16x8(0,0,"  ");

      //--------CPU TEMP.---------
      drawOledStr16x8(2,88,pstrGetCpuTemp());
      drawOledStr16x8(2,120,"C");

      //--------TIME-------------
      timep=time(NULL);
      p_tm=localtime(&timep);
      strftime(strtime,sizeof(strtime),"%H:%M:%S",p_tm);
      printf("strtime=%s\n",strtime);
      drawOledStr16x8(2,8,strtime);
//	usleep(200000); //--200k ~1000k same cpu load
  }


  //---unlock i2c fd ----
       intFcntlOp(g_fdOled, F_SETLK, F_UNLCK, 0, SEEK_SET,10);

}
