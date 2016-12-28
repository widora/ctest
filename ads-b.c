/*-------------------------------------------------------------------
ADS-B data is broadcasted by an aircraft every half-second on 1090MHz.
It carrys information such as identity,position,velocity,status etc.

including  -lm  to compile the program

midaszhou@qq.com
-------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> //-strlen()
#include <math.h> //-floor() pow() cos() acos()
#include <sys/time.h> //-gettimeofday()
#include <time.h> // ctime(),time_t,
#include <unistd.h> //-pipe() STDIN_FILENO STDOUT_FILENO

#define BUFSIZE 40
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


/*-----------------------    function trim_strfb()   -------------------------
 trim first byte of a string 
--------------------------------------------------------------------*/
static int trim_strfb(char* str)
{
int i;
int len=strlen(str);
for(i=0;i<len-1;i++)
  str[i]=str[i+1];

return 0;
}

/*---------------------- function str_findb()   ---------------------------
if find tg[0] in src, return 1; else return 0; 
---------------------------------------------------------------------------*/
static int str_findb(char* src,char tg)
{
  int len=strlen(src);
  int i,ret=0;
  for(i=0;i<len;i++)
     if(src[i]==tg)
         { ret=1; break; }
  return ret;
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

/*-----------------------    function mod(x,y)   -------------------------
  modulus function 
--------------------------------------------------------------------*/
static double mod(double x,double y)
{
  return x-y*floor(x/y);
}


/*=====================================================================
                              MAIN 
=====================================================================*/
void main(int argc, char* argv[])
{

//------------  test code -----------
char str_hexcode[BUFSIZE]=""; //-init string

char str_HEX_CODE[3][30];
strcpy(str_HEX_CODE[0],"8D40621D58C382D690C8AC2863A7");
strcpy(str_HEX_CODE[1],"8D40621D58C386435CC412692AD6");
strcpy(str_HEX_CODE[2],"8D4840D6202CC371C32CE0576098");

int int_ret; //-read STDIN_FILENO  return;

char str_ICAO24[6+1]=""; //--4*6=24bits
int  int_ICAO24;
int  int_FRAME; // 1 or 0
char str_BIN_CODE[CODE_BIN_LENGTH-1];
char str_Temp[8];
char str_CALL_SIGN[8+1]="";
int n_div=CODE_HEX_LENGTH/8+1; // =4
int nj,i,j,k,tmp;
int  int_CODE_DF; 
int  int_CODE_TC;
int  int_NL; //number of longitude zones

unsigned long bin32_code[4]; //store binary ADS-B CODE in 4 groups of 32bit array

double  dbl_lat_cpr_even=0,dbl_lon_cpr_even=0;  //--even frame latitude and longitude factor value
double  dbl_lat_cpr_odd=0,dbl_lon_cpr_odd=0; //--odd frame latitude and longitude factor value
int int_lat_index; // latitude index
double dbl_lat_even,dbl_lat_odd; //--relative latitude values
struct timeval tv_even,tv_odd;//---time stamp
time_t tm_record; //--record time 

//=======================  loop for message receiving and decoding =================
/*
for(nj=0;nj<3;nj++)
{


//--------------  convert hex code to bin code in string -----------
for(i=0;i<n_div;i++)
 {
    strmid(str_Temp,str_HEX_CODE[nj],8,i*8);
    bin32_code[i]=strtoul(str_Temp,NULL,16);
    printf("%x",bin32_code[i]); 
 }
 printf("\n");
*/

setvbuf(stdout,NULL,_IONBF,0); //--!!! set stdout no buff,or re-diret will fail

sleep(1);
while(1)
{

usleep(500000); //--!!!!!! wait rtl_adbs to finish printing to STDOUT
int_ret=read(STDIN_FILENO,str_hexcode,32); //--readin from stdin, set iobuff=0 by the sender  the size must be 32 bytes.
//str_hexcode[int_ret]=0; //---add a NULL for string end
trim_strfb(str_hexcode); //--trim "*" in the string
//printf("str_hexcode :%s   int_ret=%d \n",str_hexcode,int_ret); //--strlen(str) str must have a '/0' end
//printf("bin32_code: ");
for(i=0;i<n_div;i++)
 {
    strmid(str_Temp,str_hexcode,8,i*8);
    bin32_code[i]=strtoul(str_Temp,NULL,16);
   // printf("%x",bin32_code[i]); 
 }
//  printf("\n");


//-------------------------- get DF and TC ---------------------------- 
int_CODE_DF=(bin32_code[0]>>(32-5))&(0b11111);
/*
if(int_CODE_DF != 17)   //----????????? cause input message krupt ?????????????
{
   printf("DF=%d \n",int_CODE_DF);
   continue; //-------------- decode only  DF=17 message
}
*/
int_CODE_TC=(bin32_code[1])>>(32-5)&(0b11111);

//-------------------------- get int_ICAO24 ----------------------------
int_ICAO24=(bin32_code[0]&0xffffff);

//-------------------------  get int_FRAME  ----------------------------
int_FRAME=(bin32_code[1]>>(32-(54-32)))&(0b1);

if(int_CODE_DF==17 && (int_CODE_TC>0 && int_CODE_TC<5))  //-------DF=17, TC=1to4  Aircraft identification
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

  //printf("CALL SIGN: %s \n",str_CALL_SIGN);
  if(!str_findb(str_CALL_SIGN,'#')) //--It's valid only there is no '#' in the CALLSIGN
     {
	 //printf("str_hexcode :%s   len=%d\n",str_hexcode,strlen(str_hexcode));
	 printf("bin32_code: %x%x%x%x \n",bin32_code[0],bin32_code[1],bin32_code[2],bin32_code[3]);
	 printf("TC=%d \n",int_CODE_TC);
	 time(&tm_record);
	 printf("----------------------------------       CALL SIGN: %s       %s ",str_CALL_SIGN,ctime(&tm_record));//ctime() will cause a line return 
     }
} ///----- Aircraft identification decode end


if(int_CODE_DF==17 && ((int_CODE_TC>8 && int_CODE_TC<19) || (int_CODE_TC>19 && int_CODE_TC<23)))//-----Airborne position
//---TC=9to18 Airborne position (Baro Alt)  TC=20to22 Airborne position (GNSS Height)
{

// double lat=atof(argv[1]);  
//int_NL=get_NL(lat);
// printf("int_NL=%d \n",int_NL);

//-------------------------   calculate Latitude and Longitude factors -----------------------
if(int_FRAME==EVEN_FRAME)
{
gettimeofday(&tv_even,0);
//printf("Time stamp EVEN : %lds %ldus \n",tv_even.tv_sec,tv_even.tv_usec);
dbl_lat_cpr_even= (((bin32_code[1] & 0x3ff)<<7) + (bin32_code[2]>>(32-7)))/131072.0; // 55-41 bit  2^17=131072
dbl_lon_cpr_even= ((bin32_code[2]>>8) & 0x1ffff)/131072.0; //72-88 bit
//printf("LAT_CPR_EVEN=%.16f \n  LON_CPR_EVEN=%.16f \n",dbl_lat_cpr_even,dbl_lon_cpr_even);
}
else if(int_FRAME==ODD_FRAME)
{
gettimeofday(&tv_odd,0);
//printf("Time stamp ODD : %lds %ldus \n",tv_odd.tv_sec,tv_odd.tv_usec);
dbl_lat_cpr_odd= (((bin32_code[1] & 0x3ff)<<7) + (bin32_code[2]>>(32-7)))/131072.0; // 55-41 bit
dbl_lon_cpr_odd= ((bin32_code[2]>>8) & 0x1ffff)/131072.0; //72-88 bit
//printf("LAT_CPR_ODD=%.16f \n  LON_CPR_ODD=%.16f \n",dbl_lat_cpr_odd,dbl_lon_cpr_odd);
}

//-------------------------   calculate Latitude Index  ----------------------------
//-- !!!!!!!!!!!! to ensure that lat_cpr_even and odd are both valid !!!!!!!!!!!
int_lat_index=floor(59.0*dbl_lat_cpr_even-60.0*dbl_lat_cpr_odd+1/2.0);
//printf("Latitude Index = %d \n",int_lat_index);

//-------------------------   calculate Latitude Value  ----------------------------
dbl_lat_even=360.0/60.0*(mod(int_lat_index,60)+dbl_lat_cpr_even);
dbl_lat_odd=360.0/59.0*(mod(int_lat_index,59)+dbl_lat_cpr_odd);
//--convert value to within [-90,+90]
if(dbl_lat_even >=270.0)dbl_lat_even-=360.0;
if(dbl_lat_odd >=270.0)dbl_lat_odd-=360.0;
//printf("Latitude-Even = %.14f \n",dbl_lat_even);
//printf("Latitude-Odd = %.14f \n",dbl_lat_odd);


}///---- Airebore Position decode end

/*
printf("int_FRAME=%d \n",int_FRAME);
printf("DF=%d \n",int_CODE_DF);
printf("TC=%d \n",int_CODE_TC);
printf("ICAO24=%x \n",int_ICAO24);
printf("FRAME: %s \n",(int_FRAME>0)?"Odd":"Even");
//printf("CALL SIGN: %s \n",str_CALL_SIGN);
printf("----------------  end of message --------------\n");
*/

usleep(200000);
 } /// end of for() or while()

} //// end of main()
