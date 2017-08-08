/*-------------------------------------------------------------------
ADS-B data is broadcasted by an aircraft every half-second on 1090MHz.
It carrys information such as identity,position,velocity,status etc.

including  -lm  to compile the program

!!! screen will intercept STDIO buffer, so use sh -c "....." to avoid it.

midaszhou@qq.com
-------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> //-strlen()
#include <math.h> //-floor() pow() cos() acos()
#include <sys/time.h> //-gettimeofday(), setitimer()
#include <time.h> // ctime(),time_t,
#include <unistd.h> //-pipe() STDIN_FILENO STDOUT_FILENO
#include <stdint.h> //uint32_t
#include <fcntl.h> //fcntl()
#include <signal.h> //signal()
#include "cstring.h" //strmid(),trim_strfb(),str_findb()
#include "adsb_crc.h" //adsb_crc24( )
#include "ads_hash.h" // hash table and data save

static int PRINT_ON=0;
HASH_TABLE* pHashTbl_CODE; //---hash for ICAO and CALLSIGN data save
static char  str_FILE[]="/tmp/ads.data"; //--file to save data

#define BUFSIZE 40
#define CODE_BIN_LENGTH 112
#define CODE_HEX_LENGTH 28
#define CODE_LIVE_TIME 40 //seconds, data's live time great than the value will be deemed as obsolete.
static char LOOKUP_TABLE[]="#ABCDEFGHIJKLMNOPQRSTUVWXYZ#####_###############0123456789######";
#define EVEN_FRAME 0
#define ODD_FRAME 1
#define NZ 15  //--Number of geographic latitude zones between equator and a pole. NZ=15 for Mode-S CPR encoding.
/*
#define max(x,y) ({\
typeof(x) _x=(x);\
typeof(y) _y=(y);\
(void)(&_x==&_y);\
_x>_y?_x:_y;})
*/

//-----------------------    function max(x,y)   -------------------------
static double max(double x, double y)
{
return  x>y?x:y;
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


//------------- INTERRUPT TO EXIT SIGNAL HANDLER ---------------
static void sighandler(int sig)
{
  printf("Signal to exit......\n");
  save_hash_data(str_FILE,pHashTbl_CODE);
  release_hash_table(pHashTbl_CODE);
  exit(0); 
}

//-----------------  set timer -----------------
void set_timer(void)
{
 struct itimerval itv;
 itv.it_interval.tv_sec=1800; //--reload value, it will trigger the timer-handler every 0.5 hour
 itv.it_interval.tv_usec=0;
 itv.it_value.tv_sec=1800;  //--first set value
 itv.it_value.tv_usec=0;
 setitimer(ITIMER_REAL,&itv,NULL);
}

//------------  timer handler: save hash data ------------
void timer_save_data(int sig)
{
  printf("\n   +++++++++++ timer handler: save hash data ++++++++++\n\n"); 
  save_hash_data(str_FILE,pHashTbl_CODE);
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
uint32_t  int_ICAO24;
uint32_t  int_EVEN_ICAO24,int_ODD_ICAO24;// ICAO for even and odd frames
int  int_FRAME; // 1 or 0
char str_BIN_CODE[CODE_BIN_LENGTH-1];
char str_Temp[8];
char str_CALL_SIGN[8+1]="";
int n_div=CODE_HEX_LENGTH/8+1; // =4
int nj,i,j,k,tmp;
int  int_CODE_DF; 
int  int_CODE_TC;
int  int_NL; //number of latitude zones
int  int_EVEN_NL,int_ODD_NL;
int  int_NI; // =MAX(NL(Late),1)
int  int_M;
uint32_t bin32_code[4]; //store binary ADS-B CODE in 4 groups of 32bit array, meaningful data 112bits
uint32_t bin32_message[3]; //88bits meaningful,message data in the 112-bits CODE after ripping 24bits CRC, used as dividend in CRC calculation.
uint32_t bin32_checksum; // checksum in ADS-B  message, last 24bits in bin32_code.
uint32_t bin32_CRC24; //CRC calculated from bin32_message

int int_ret_errorfix; // return value from adsb_fixerror_slow()
int int_ret_opt;// retrun value from getopt()

double  dbl_lat_cpr_even=0,dbl_lon_cpr_even=0;  //--even frame latitude and longitude factor value
double  dbl_lat_cpr_odd=0,dbl_lon_cpr_odd=0; //--odd frame latitude and longitude factor value
int int_lat_index; // latitude index
double dbl_lat_even,dbl_lat_odd; //--relative latitude values
double dbl_lat_val,dbl_lon_val; //--final latitude and longitude valudes
double dbl_DLon;// =360/int_NI
struct timeval tv_even,tv_odd;//---time stamp
time_t tm_record; //--record time 

int stdflag; //STD_FILENO flag
STRUCT_DATA ads_data;

//------------- intterupt to exit signal handle --------
signal(SIGINT,sighandler);


//-------------- get option -----------------
while( (int_ret_opt=getopt(argc,argv,"hd"))!=-1)
{
    switch(int_ret_opt)
    {
       case 'h':
           printf("usage:  rtl_adsb | ads_b \n");   
           printf("         -h   help \n");   
           printf("         -d   printf debug information \n");   
           printf("ICAO and corresponding CALL-SIGN will be saved every 30 minutes.");
           printf("Please check /tmp/ads.data for saved data\n");
           printf("Saved data is reloaded to hash table at start.")
           return;
       case 'd':
           printf("----- Debug information available now! \n");
           PRINT_ON=1;
           break;
       default:
           break;
    }//--end switch
}//--end while()


//---------------------  hash table prepararton  --------------------
pHashTbl_CODE=create_hash_table(); //---init hash table

//--------------------- restore hash data from file -------------
restore_hash_data(str_FILE,pHashTbl_CODE);

//---------------------  set timer alarm signal ----------------
signal(SIGALRM,timer_save_data);
set_timer();

//---------------------    set STDOUT buffer   ----------------------
setvbuf(stdout,NULL,_IONBF,0); //--!!! set stdout no buff,or re-diret will fail
//setvbuf(stdin,NULL,_IOLBF,0); //--!!! ???ineffective for read()
//stdflag=fcntl(STDIN_FILENO,F_GETFL);//--get flag
//fcntl(STDIN_FILENO,F_SETFL,stdflag&~O_NONBLOCK); // set BLOCK 
//=======================  loop for message receiving and decoding =================
//sleep(1);
while(1)
{
/*----------------!!!!!  check your hardware connection   !!!!!-------------------------------------------
 If rtl dongle anntena is not well connected ,or there is too much noise and electronic interference,
 the rtl_adsb data stream may seems very fast, but they are acturally all false signals.
---------------------------------------------------------------------------------------------------------*/

//usleep(100000);// necessary time for buf filling,  !!!put usleep here to ensure each continue loop will excute once.
//memset(str_hexcode,'\0',sizeof(str_hexcode)); //--clear buffer str
//--!!!!!!---- STDIN_FILENO  is no buffer I/O ----!!!!!!
//int_ret=read(STDIN_FILENO,str_hexcode,32); //-32bits including *#/r/n,  read()--readin from stdin with NO BUFFER!, it will stop at '\0'!!!!.  set iobuff=0 by the sender  the size must be 32 bytes.
//str_hexcode[int_ret]=0; //---add a NULL for string end
fgets(str_hexcode,33,stdin); // fget will get defined length of chars or stop at '\n'(after '\n' is copied), whichever condition gets first, and put a string end '\0' .
str_hexcode[32]='\0'; //-- add a string end token, NOT necessary??
int_ret=strlen(str_hexcode); //---and /n from rtl_adsb also included
////////// try to fflush stdin; the data will remaind in buffer until new data enter to reflash   ///////////
//fflush(stdin);
trim_strfb(str_hexcode); //--trim first char "*" in the string 
if(PRINT_ON)printf("str_hexcode :%s   int_ret=%d \n",str_hexcode,int_ret); //--strlen(str) str must have a '/0' end
if(int_ret<32)continue; //---not a valid code string, continue to loop.
//printf("bin32_code: ");
for(i=0;i<4;i++) //---convert CODE to bin type
 {
    strmid(str_Temp,str_hexcode,8,i*8);
    bin32_code[i]=strtoul(str_Temp,NULL,16);
    if(i==3)
          bin32_code[i]=(bin32_code[i]<<16);  //--shift/adjust the last 16bits to left significent order 
   // printf("%x",bin32_code[i]); 
 }

//------- try to fix 1bit error in original code ---------
int_ret_errorfix=adsb_fixerror_slow(bin32_code);
if(int_ret_errorfix<0)continue;  // !!!!!!!! WARNING !!!!!!!! temporarily suspending until CALLSIGN printed --if can't fix the error then drop it.

//------- get checksum at last 24bits of codes -----------
bin32_checksum=((bin32_code[2]&0xff)<<16) | (bin32_code[3]>>16); 

/*
//---------- extract message data, get rid of 24bits CRC 
  bin32_message[0]=bin32_code[0];
  bin32_message[1]=bin32_code[1];
  bin32_message[2]=bin32_code[2]&0xffffff00;
 // printf("Message Data for CRC check: %08x%08x%08x \n",bin32_message[0],bin32_message[1],bin32_message[2]);
*/

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

//=======================   AIRCRAFT  IDENTIFICATION  CALCUALTION  ========================
if(int_CODE_DF==17 && (int_CODE_TC>0 && int_CODE_TC<5))  //-------DF=17, TC=1to4  Aircraft identification
{
//-----------  get CALL-SIGN  ----------------------
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

  if(PRINT_ON) printf("CALL SIGN: %s \n",str_CALL_SIGN);
  if(!str_findb(str_CALL_SIGN,'#')) //--It's valid only there is no '#' in the CALLSIGN
     {
	 //printf("str_hexcode :%s   len=%d\n",str_hexcode,strlen(str_hexcode));
         bin32_CRC24=adsb_crc(bin32_code,88); //calculate CRC 24
	 printf("\nReceived ADS-B CODES: %x%x%x%04x \n",bin32_code[0],bin32_code[1],bin32_code[2],bin32_code[3]>>16);
	 printf("TC=%d    CRC24 =%06x    CHECKSUM =%06x\n",int_CODE_TC,bin32_CRC24,bin32_checksum);
	 time(&tm_record); 
	 printf("-----------------------------------------       CALL SIGN: %s       %s \n",str_CALL_SIGN,ctime(&tm_record));//ctime() will cause a line return 

         //-----------------    hash ICAO code and corresponding CALL-SIGN    ------------------
         ads_data.int_ICAO24=int_ICAO24;
         strcpy(ads_data.str_CALL_SIGN,str_CALL_SIGN);
         if(insert_data_into_hash(pHashTbl_CODE,&ads_data))
              printf("#########   ICAO CODE and CALL-SIGN push into hash table successfully.  ##########\n");
         else
              printf("#########   ICAO CODE and CALL-SIGN already exist in hash table.  ##########\n");
     }
} ///----- Aircraft identification decode end

//---- !!!!! WARNING !!!!!    only temporarily use----   --or  any error in int_FRMAE,TC etc.  will pass on and affect later position decoding  ---------
//if(int_ret_errorfix<0)continue;  //--After CALL-SIGN decoding. If can't fix CRC error, then drop the codes and continue a new loop.


//===========================     AIRBOREN POSITION  CALCUALTION    =======================
//
if(int_CODE_DF==17 && ((int_CODE_TC>8 && int_CODE_TC<19) || (int_CODE_TC>19 && int_CODE_TC<23)))//$$$$$$$-- Airborne position --$$$$$$$$$
//---TC=9to18 Airborne position (Baro Alt)  TC=20to22 Airborne position (GNSS Height)
{
// double lat=atof(argv[1]);  
//int_NL=get_NL(lat);
// printf("int_NL=%d \n",int_NL);

//-----------------   calculate Latitude and Longitude factors ------------------
if(int_FRAME==EVEN_FRAME)
{
 int_EVEN_ICAO24=int_ICAO24;
 gettimeofday(&tv_even,0);
 //printf("Time stamp EVEN : %lds %ldus \n",tv_even.tv_sec,tv_even.tv_usec);
 dbl_lat_cpr_even= (((bin32_code[1] & 0x3ff)<<7) + (bin32_code[2]>>(32-7)))/131072.0; // 55-41 bit  2^17=131072 
 dbl_lon_cpr_even= ((bin32_code[2]>>8) & 0x1ffff)/131072.0; //72-88 bit
 if(PRINT_ON)printf("LAT_CPR_EVEN=%.16f \n  LON_CPR_EVEN=%.16f \n",dbl_lat_cpr_even,dbl_lon_cpr_even); 

 //-----check data live time----------
  if(abs(tv_even.tv_sec-tv_odd.tv_sec)>=CODE_LIVE_TIME) //--obsolete ODD data 
  {
     int_ODD_ICAO24=0; // reset ODD ICAO, mark it as obsolete
     continue;
  }
}
else if(int_FRAME==ODD_FRAME)
{
 int_ODD_ICAO24=int_ICAO24;
 gettimeofday(&tv_odd,0);
 //printf("Time stamp ODD : %lds %ldus \n",tv_odd.tv_sec,tv_odd.tv_usec);
 dbl_lat_cpr_odd= (((bin32_code[1] & 0x3ff)<<7) + (bin32_code[2]>>(32-7)))/131072.0; // 55-41 bit
 dbl_lon_cpr_odd= ((bin32_code[2]>>8) & 0x1ffff)/131072.0; //72-88 bit
 if(PRINT_ON)printf("LAT_CPR_ODD=%.16f \n  LON_CPR_ODD=%.16f \n",dbl_lat_cpr_odd,dbl_lon_cpr_odd);

 //-----check data live time----------
 if(abs(tv_odd.tv_sec-tv_even.tv_sec)>=CODE_LIVE_TIME) //--obsolete EVEN data 
 {
    int_EVEN_ICAO24=0; // reset EVEN ICAO,mark it as obsolete 
    continue;
 }
}

if(int_ODD_ICAO24==int_EVEN_ICAO24) //--ensure thery are from same flight
  {
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
        int_EVEN_NL=get_NL(dbl_lat_even);
        int_ODD_NL=get_NL(dbl_lat_odd);

	//------- compare time value of even and odd frame, get final Latitude and Longitutde -----
	if((tv_even.tv_sec > tv_odd.tv_sec) || ((tv_even.tv_sec==tv_odd.tv_sec) && (tv_even.tv_usec >= tv_odd.tv_usec)))
	//--- if tv_even > tv_odd
	{
 	  dbl_lat_val=dbl_lat_even;  //---final Lat. value 
  
	  //-----------------   calculate Longitude Teven >Todd --------------------------
	  if(int_EVEN_NL==int_ODD_NL) //--ensure they are in the same lat zone.
	  {
	     int_NI=max(int_EVEN_NL,1);
	     dbl_DLon=360.0/int_NI;
	     int_M=floor(dbl_lon_cpr_even*(int_EVEN_NL-1)-dbl_lon_cpr_odd*int_EVEN_NL+0.5);
	     dbl_lon_val=dbl_DLon*(mod(int_M,int_NI)+dbl_lon_cpr_even);
	//      printf(" tv_even>tv_odd final Longitude =%.14f \n",dbl_lon_val);
	  }
          else continue;
	}
	else  //----tv_odd > tv_even
	{
	  dbl_lat_val=dbl_lat_odd;  //----final Lat. value
	  //-----------------   calculate Longitude Teven < Todd -------------------------
	  if(int_EVEN_NL==int_ODD_NL) //--ensure they are in the same lat zone.
	   {
	      int_NI=max((int_ODD_NL-1),1);
	//      printf("NL_odd=%d\n",(get_NL(dbl_lat_odd)));
	 //     printf("int_NI=%d\n",int_NI);
	      	dbl_DLon=360.0/int_NI;
	 //     printf("DLon=%.14f\n",dbl_DLon);
	 //     printf("LoncprE=%.14f  LoncprO=%.14f \n",dbl_lon_cpr_even,dbl_lon_cpr_odd);
	      int_M=floor(dbl_lon_cpr_even*(int_ODD_NL-1)-dbl_lon_cpr_odd*int_ODD_NL+0.5);
	 //     printf("int_M=%d\n",int_M);
	      dbl_lon_val=dbl_DLon*(mod(int_M,int_NI)+dbl_lon_cpr_odd);
	   }
          else continue; 
	}

       if(dbl_lon_val>=180)dbl_lon_val-=360.0;//--convert to [-180 180]
       //printf("Teven %lds ---  Todd %lds ---  Tgap %d \n",tv_even.tv_sec,tv_odd.tv_sec,abs(tv_even.tv_sec-tv_odd.tv_sec));
      // printf("Teven %lds:%ldus          Todd %lds:%ldus \n",tv_even.tv_sec,tv_even.tv_usec,tv_odd.tv_sec,tv_odd.tv_usec);
       printf("TC=%d     NL=%d     Received ADS-B CODES: %x%x%x%04x \n",int_CODE_TC,int_EVEN_NL,bin32_code[0],bin32_code[1],bin32_code[2],bin32_code[3]>>16);
       printf("--- ICAO: %6X  Fix_Error:%d  Lat: %.14fN Long: %.14fE ---\n",int_ICAO24,int_ret_errorfix,dbl_lat_val,dbl_lon_val);

 }//-----end of  if(int_ODD_ICAO24==int_EVEN_ICAO24)
	

}//$$$$$$----- Airebore Position decode end ------$$$$$

/*
printf("int_FRAME=%d \n",int_FRAME);
printf("DF=%d \n",int_CODE_DF);
printf("TC=%d \n",int_CODE_TC);
printf("ICAO24=%x \n",int_ICAO24);
printf("FRAME: %s \n",(int_FRAME>0)?"Odd":"Even");
//printf("CALL SIGN: %s \n",str_CALL_SIGN);
printf("----------------  end of message --------------\n");
*/

//usleep(50000); // put usleep in the head of loop to avoid any leap of continue operation
 } /// end of for() or while()

} //// end of main()
