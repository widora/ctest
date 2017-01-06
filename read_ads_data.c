#include "ads_hash.h"
#include <stdio.h>

// char str_FILE[]="/tmp/ads.data"; ---defined in ads_hash.h

void main(void)
{
 FILE *fp;
 int i;
 int ncount=0;
 STRUCT_DATA  sdata;

 if((fp=fopen(str_FILE,"r"))==NULL) 
 {
    printf("fail to open %s\n",str_FILE);
    return;
 }

 fread(&ncount,sizeof(int), 1,fp);
 printf("restore ncount=%d\n",ncount);
 for(i=0;i<ncount;i++)
 {
    fread((char*)&sdata,sizeof(STRUCT_DATA),1,fp);
    printf("restore DATA[%d]:   %06X  %s  \n",i,sdata.int_ICAO24,sdata.str_CALL_SIGN);
 }

 fclose(fp);
}


