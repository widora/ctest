/*------------------------------------------------

Try to decode ADS-B messages

including  -lm  to compile the program

midaszhou@qq.com
-------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> //-strlen()
#include <math.h> //-floor() pow() cos() acos()

#define CODE_BIN_LENGTH 112 
#define CODE_HEX_LENGTH 28
char LOOKUP_TABLE[]="#ABCDEFGHIJKLMNOPQRSTUVWXYZ#####_###############0123456789######";
#define EVEN_FRAME 0
#define ODD_FRAME 1
#define NZ 15  //--Number of geographic latitude zones between equator and a pole. NZ=15 for Mode-S CPR encoding.

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

/*-----------------------    function get_NL()   -------------------------
calcuate number of longitude zones by given value of latitude angle 
--------------------------------------------------------------------*/
static int get_NL(double lat)
{
 double pi=3.1415926535897932;  //--double: 16 digital precision exclude '.'
 //printf("pi=%.15f \n",pi);
 if(lat==0) return 59;
 else if(lat==87) return 2;
 else if(lat==-87) return 2;
 else if(lat > 87) return 1;
 else if(lat < -87) return 1;
 else
   return floor(2*pi/acos(1-(1-cos(pi/2.0/NZ))/pow(cos(pi/180.0*lat),2)));
}

void main(int argc, char* argv[])
{

//char* str_HEX_CODE="8D4840D6202CC371C32CE0576098";
char* str_HEX_CODE="8D40621D58C382D690C8AC2863A7";

char str_ICAO24[6+1]=""; //--4*6=24bits
int  int_ICAO24;
int  int_FRAME; // 1 or 0
char str_BIN_CODE[CODE_BIN_LENGTH-1];
char str_Temp[8];
char str_CALL_SIGN[8+1]="";
int n_div=CODE_HEX_LENGTH/8+1; // =4
int i,j,k,tmp;
int  int_CODE_DF; 
int  int_CODE_TC;
int  int_NL; //number of longitude zones

double  dbl_lat_cpr_even,dbl_lon_cpr_even;  //--even frame latitude and longitude value
double  dbl_lat_cpr_odd,dbl_lon_cpr_odd; //--odd frame latitude and longitude value

unsigned long bin32_code[4]; //store binary ADS-B CODE in 4 groups of 32bit array
char str_32BIT[8*4];  

//--------------  convert hex code to bin code in string -----------
for(i=0;i<n_div;i++)
 {
    strmid(str_Temp,str_HEX_CODE,8,i*8);
    bin32_code[i]=strtoul(str_Temp,NULL,16);
    printf("%x",bin32_code[i]); 
 }

//-------------------------- get DF and TC ---------------------------- 
int_CODE_DF=(bin32_code[0]>>(32-5))&(0b11111);
int_CODE_TC=(bin32_code[1])>>(32-5)&(0b11111);

//-------------------------- get int_ICAO24 ----------------------------
int_ICAO24=(bin32_code[0]&0xffffff);

//-------------------------  get int_FRAME  ----------------------------
int_FRAME=(bin32_code[1]>>(32-(54-32)))&(0b1);


if(int_CODE_TC>0 && int_CODE_TC<5)  //-------DF=17, TC=1to4  Aircraft identification
{
//-------------------------  get CALL-SIGN  ----------------------------
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
}

if((int_CODE_TC>8 && int_CODE_TC<19) || (int_CODE_TC>19 && int_CODE_TC<23))//-----Airborne position
//---TC=9to18 Airborne position (Baro Alt)  TC=20to22 Airborne position (GNSS Height)
{
 double lat=atof(argv[1]);  

 int_NL=get_NL(lat);
 printf("int_NL=%d \n",int_NL);


//-------------------------   calculate Latitude and Longitude  -----------------------
if(int_FRAME==EVEN_FRAME)
{
dbl_lat_cpr_even= (((bin32_code[1] & 0x3ff)<<7) + (bin32_code[2]>>(32-7)))/131072.0; // 55-41 bit  2^17=131072
dbl_lon_cpr_even= ((bin32_code[2]>>8) & 0x1ffff)/131072.0; //72-88 bit
printf("LAT_CPR_EVEN=%.8f   LON_CPR_EVEN=%.8f \n",dbl_lat_cpr_even,dbl_lon_cpr_even);
}
else if(int_FRAME==ODD_FRAME)
{
dbl_lat_cpr_odd= (((bin32_code[1] & 0x3ff)<<7) + (bin32_code[2]>>(32-7)))/131072; // 55-41 bit
dbl_lon_cpr_odd= ((bin32_code[2]>>8) & 0x1ffff)/131072; //72-88 bit
printf("LAT_CPR_ODD=%.8f   LON_CPR_ODD=%.8f \n",dbl_lat_cpr_odd,dbl_lon_cpr_odd);
}



}



printf("\n");
printf("DF=%d \n",int_CODE_DF);
printf("TC=%d \n",int_CODE_TC);
printf("ICAO24=%x \n",int_ICAO24);
printf("FRAME: %s \n",(int_FRAME>0)?"Odd":"Even");
printf("CALL SIGN: %s \n",str_CALL_SIGN);


}
