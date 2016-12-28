/*-------------------------------------------------------------------


midaszhou@qq.com
-------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> //-strlen()
#include <stdint.h> //uint32_t
#include "adsb-crc.h"



/*------------------   function  strmid()   ------------------------------
to copy 'n' chars from 'src' to 'dst', starting from 'm'_th char in 'src'
------------------------------------------------------------------------*/
static char* strmid(char *dst,char *src,int n,int m)  
{
 char *p=src;
 char *q=dst;
 unsigned int len=strlen(src);
 if(n>len) n=len-m;
 if(m<0) m=0;
 if(m>len) return NULL;
 p+=m;
 while(n--) *(q++)=*(p++);
 *(q++)='\0';
 return dst;
}




/*=====================================================================
                              MAIN 
=====================================================================*/
void main(int argc, char* argv[])
{

//------------  test code -----------

char str_HEX_CODE[32]=""; //4*8=32 > 28
strcpy(str_HEX_CODE,"8D40621D58C382D690C8AC2863A7"); //--28 chars for str_HEX code
//strcpy(str_HEX_CODE,"8D40621D58C386435CC412692AD6");
//strcpy(str_HEX_CODE,"8D4840D6202CC371C32CE0576098");

int i,j,k;
char str_Temp[8]; 
//int n_div=4;
uint32_t crc32_GP=0xFFFA0480;//0x04C11DB7; // CRC Generator Polynomial alread inlucdes the highest first bit, while standard CRC usually omitted. 
char str_ICAO24[6+1]=""; //--4*6=24bits
int  int_ICAO24;
uint32_t bin32_code[4]; //store full binary ADS-B CODE in 4 groups of 32bit array
uint32_t bin32_divid[4]={0,0,0,0};//dividend = message(88)+crc(24)=112bits
uint32_t bin32_CRC24;

//--------------  convert hex code to bin code in string -----------
for(i=0;i<4;i++)
 {
    strmid(str_Temp,str_HEX_CODE,8,i*8);
    bin32_code[i]=strtoul(str_Temp,NULL,16);
    if(i==3)
      bin32_code[i]=(bin32_code[i]<<16);  //--shift the last 16bits to left
    printf("%08x",bin32_code[i]); 
 }
 printf("\n");

//------------------------- CRC calculation -------------------------------
  //------- prepare dividend for CRC calculation 
  bin32_divid[0]=bin32_code[0];
  bin32_divid[1]=bin32_code[1];
  bin32_divid[2]=bin32_code[2]&0xffffff00;

  printf("--- CRC24: %06x \n",adsb_crc24(bin32_divid));
  

} //// end of main()
