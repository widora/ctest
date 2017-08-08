#include <string.h>
#include <signal.h>
#include "i2c_oled_128x32.h"
#include "my_ip.h"

//======================== MAIN ==================
void main()
{
 int i,k,len;
 char strtime[10]={0};
 char* pstrIP;
 time_t timep;
 struct tm *p_tm;

 //---init oled ----
 initOledDefault();
 //---init timer----
// initOledTimer();
// signal(SIGALRM,sigHndlOledTimer);

 //----clear OLED display----!!! refresh data firt before clear !!!!!
  clearOledV(); 

 //--------set scroll------------
//  setOledHScroll(0,1); // set scroll page
//  actOledScroll(); // enact scroll
//  sleep(1);

 //---- set vertical scroll ----
 drawOledStr16x8(4,0,"   NanoPi-NEO   ");
 drawOledStr16x8(6,0,"****************");

 //setOledVeScroll();
 i=0;
/*
 while(1)
 {
	if(i>63)i=0;
	setStartLine(i);
	i++;
	usleep(100000);
 }
*/

 while(1)
  {
      //----- stop scrolling before wrting to GRAM
     // actOledScroll();
      //deactOledScroll();

      //--------- vertical scroll -------
      if(i>63)i=0;
      setStartLine(i);

      //--------IP address--------
     if(i==0)
     {
    	 pstrIP=getMyIP();
      	 len=strlen(pstrIP);
      	 if(len>16)len=16;
      	 //drawOledStr16x8(0,0,"  ");
      	 drawOledStr16x8(0,(16-len)*4,pstrIP); //4=8/2
      	 //drawOledStr16x8(0,0,"  ");
      }

      i++;

      //--------CPU TEMP.---------
      drawOledStr16x8(2,88,pstrGetCpuTemp());
      drawOledStr16x8(2,120,"C");

      //--------TIME-------------

      timep=time(NULL);
      p_tm=localtime(&timep);
      strftime(strtime,sizeof(strtime),"%H:%M:%S",p_tm);
      //printf("strtime=%s\n",strtime);
      drawOledStr16x8(2,8,strtime);


 //     usleep(200000);
  }


  //---unlock i2c fd ----
       intFcntlOp(g_fdOled, F_SETLK, F_UNLCK, 0, SEEK_SET,10);

}
