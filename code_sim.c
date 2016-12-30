#include <stdio.h> // stdout stdin 
#include <stdlib.h>
#include <unistd.h>
#include <string.h> // strcpy

void main(void)
{
int i,j,k;
char strcode[5][32];
//------!!! \0  end of a string  \n  return !!!------- 
memset(strcode,'*',5*32);
strcpy(strcode[0],"*8D40621D58C382D690C8AC2863A7  \0");
strcpy(strcode[1],"*8D40621D58C386435CC412692AD6  \0");
//strcpy(strcode[2],"--\0");
strcode[2][15]='\0';
//strcpy(strcode[3],"--\0");
strcode[3][15]='\0';
//strcpy(strcode[4],"--\0");
strcode[4][31]='\0';

 setvbuf(stdout,NULL,_IONBF,0); //--!!! set stdout no buff
// char str[]="123456789;\r\n";

 for(i=0;i<5;i++)
 {
   usleep(500000);
   //printf("----%s----\n",strcode[i]);
   fprintf(stdout,"%s\n",strcode[i]);
   //fprintf(stdout,"*12345678;\r\n"); //must give an end, or it will never end entering.
   //fwrite(str,sizeof(char),12,stdout); 
  }
}
