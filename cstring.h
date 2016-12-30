/*-------------------------------------------------------------------
simple string handle fuction for ADS-B program

midaszhou@qq.com
-------------------------------------------------------------------*/

#ifndef _CSTRING_H
#define _CSTRING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h> //-strlen()


/*------------------   function  strmid()   ------------------------------
to copy 'n' chars from 'src' to 'dst', starting from 'm'_th char in 'src'
------------------------------------------------------------------------*/
 char* strmid(char *dst,char *src,int n,int m)  
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


/*-----------------------    function trim_strfb()   -------------------------
 trim first byte of a string 
--------------------------------------------------------------------*/
static int trim_strfb(char* str)
{
int i;
int len=strlen(str);
if(str[0]=='*')
 {
  for(i=0;i<len-1;i++)
    str[i]=str[i+1];
  return 0;
  }
return 1;
}

/*---------------------- function str_findb()   ---------------------------
if find tg[0] in src, return 1; else return 0; 
---------------------------------------------------------------------------*/
 int str_findb(char* src,char tg)
{
  int len=strlen(src);
  int i,ret=0;
  for(i=0;i<len;i++)
     if(src[i]==tg)
         { ret=1; break; }
  return ret;
}

#endif
