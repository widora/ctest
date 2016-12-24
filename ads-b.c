#include <stdio.h>
#include <stdlib.h>
#include <string.h> //-strlen()

#define CODE_BIN_LENGTH 112 
#define CODE_HEX_LENGTH 28
char LOOKUP_TABLE[]="#ABCDEFGHIJKLMNOPQRSTUVWXYZ#####_###############0123456789######";


/*-------------function  strmid() ----------------------------
to copy 'n' chars from 'src' starting from 'm'_th char to 'dst'
------------------------------------------------------------*/
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



void main(void)
{

char* str_HEX_CODE="8D4840D6202CC371C32CE0576098";
char str_BIN_CODE[CODE_BIN_LENGTH-1];
char str_Temp[8];
char str_CALL_SIGN[8+1]="";
int n_div=CODE_HEX_LENGTH/8+1; // =4
int i,j,k;
int  int_CODE_DF; 
int  int_CODE_TC;

//uint64_t ltmp; //temp. unsigned long type
unsigned long bin32_code[4]; //store binary ADS-B CODE in 4 groups of 32bit array
char str_32BIT[8*4];  

//--------------  convert hex code to bin code in string -----------
for(i=0;i<n_div;i++)
 {
    strmid(str_Temp,str_HEX_CODE,8,i*8);
    bin32_code[i]=strtoul(str_Temp,NULL,16);
    //ltmp=strtoul(str_Temp,NULL,16);
    //ltoa(ltmp,str_32BIT,2);
    printf("%x ",bin32_code[i]); 
    //printf("%s \n",str_32BIT);
       
 }

 
int_CODE_DF=(bin32_code[0]>>(32-5))&(0b11111);
int_CODE_TC=(bin32_code[1])>>(32-5)&(0b11111);

//------ get call-sign -------------
int tmp;

for(j=0;j<4;j++) //--first 4 chars
 {
   tmp=(bin32_code[1]>>(32-(14+6*j)))&(0x3f);
   str_CALL_SIGN[j]=LOOKUP_TABLE[tmp];  
 }
for(j=0;j<4;j++) //--after 4 chars
 {
   tmp=(bin32_code[2]>>(32-(6+6*j)))&(0x3f);
   str_CALL_SIGN[j+4]=LOOKUP_TABLE[tmp];  
 }


printf("DF=%d \n",int_CODE_DF);
printf("TC=%d \n",int_CODE_TC);
printf("CALL SIGN: %s \n",str_CALL_SIGN);


}
