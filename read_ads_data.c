#include "ads_hash.h"
#include <stdio.h>


void main(int argc, char* argv[])
{
 FILE *fp;
 int i;
 int ncount=0;
 STRUCT_DATA  sdata;
 char str_FILE[30]="/tmp/ads.data";
 char* pstrf; //="/tmp/ads.data";

 if(argc>1)
    pstrf=argv[1];
 else 
    pstrf=str_FILE;
 
 if((fp=fopen(pstrf,"r"))==NULL) 
 {
    printf("fail to open %s\n",pstrf);
    return;
 }

 fread(&ncount,sizeof(int), 1,fp);
 printf("restore ncount=%d\n",ncount);
 for(i=0;i<ncount;i++)
 {
    fread((char*)&sdata,sizeof(STRUCT_DATA),1,fp);
    printf("restore DATA[%03d]:   %06X  %s  \n",i,sdata.int_ICAO24,sdata.str_CALL_SIGN);
 }

 fclose(fp);
}


