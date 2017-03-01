/*--------------------------------------

lib -lpthread
by midaszhou
*--------------------------------------*/

#include <stdio.h>
#include <stdint.h> // data type
#include <signal.h>
#include "./RM68140.h"
#include <sys/time.h>
#include <dirent.h>
#include <sched.h> // scheduler set
#include <pthread.h>

#define BUFFSIZE 1024 // --- to be times of SPIBUFF
#define SPIBUFF 32   //-- spi write buff-size for SPI_Write()
#define STRBUFF 256   // --- size of file name
#define MAX_WIDTH 320
#define MAX_HEIGHT 480
#define SHOW_DELAY 3  //--sleep() delay seconds for showing a pic

uint8_t tmp;
uint8_t *pmap,*oftemp;
int fp; //file handle
int MapLen; // file size,mmap size

char file_path[256];
char BMP_file_name[256][STRBUFF]; //---BMP file directory and name
int BMP_file_num=0;//----BMP file name index
int BMP_file_total=0; //--total number of BMP files

/*------------- time struct -------------*/
struct timeval t_start,t_end;
long cost_timeus=0;
long cost_times=0;

void ExitClean();
int Find_BMP_files(char* path);


int main(int argc, char* argv[])
{
 int i,j,k,nn;
 int Ncount=-1; //--index number of picture displayed
 uint32_t total; // --total bytes of pic file
 uint16_t residual; // --residual after total divided by BUFFSIZE
 uint16_t nbuff;
 uint16_t Hs,He,Vs,Ve; //--GRAM area difinition parameters
 uint16_t Hb,Vb; //--GRAM area corner gap distance from origin

 char strf[STRBUFF]; //--for file name

 uint8_t buff[8]; //--for buffering  data temporarily
 long offp; //offset position
 uint16_t picWidth, picHeight;
 offp=18; // file offset  position for picture Width and Height data


/*------------------------ SET SCHEDULER FIFO  ----------------------*/
 pthread_attr_t pattr; //thread attribute object
 struct sched_param sch_param; //sheduler parameter

 pthread_attr_init(&pattr);
 pthread_attr_setinheritsched(&pattr,PTHREAD_EXPLICIT_SCHED); //must set before setschedpolicy()
 pthread_attr_setschedpolicy(&pattr,SCHED_FIFO); // adopt FIFO real time scheduler
 pthread_attr_setscope(&pattr,PTHREAD_SCOPE_SYSTEM); // compete with all threads in system
 pthread_attr_setdetachstate(&pattr,PTHREAD_CREATE_DETACHED); // detach with other threads and can't pthread_join()

 sch_param.sched_priority=99;//sched_get_priority_max(SCHED_FIFO); // get max priority of FIFO policy
 pthread_attr_setschedparam(&pattr,&sch_param);

//---------- use sched_setscheduler() function ----------------
 if(sched_setscheduler(0,SCHED_FIFO,&sch_param) == -1)
//-------- 0 and getpid() both OK, if the process contains sub_processes, they will all be infected.
//--- since current parent process and its sub_proesses are all at the highest FIFO level, only ONE parent process will be active ac$
 {
        printf("-----------sched_setscheduler() fail!\n");
 }
 else 
        printf("-----------sched_setscheduler() successfully!\n");

//-------------------   check current sheduler type  --------------
int my_policy;
//struct sched_param my_param;
//--------- check SCHEDULER type -----
pthread_getschedparam(pthread_self(),&my_policy,&sch_param);
printf("----------- thread_routine running at %s  %d\n -------------", \
        (my_policy == SCHED_FIFO ? "FIFO" \
        :(my_policy == SCHED_RR ? "RR" \
        :(my_policy == SCHED_OTHER ? "OTHER" \
        : "unknown"))),
        sch_param.sched_priority);





 /*---------------- Exit Signal Process ----------------*/
   signal(SIGINT,ExitClean);

 /* -------------------    SPI  and PIN control initiation ------------------*/
 //------ !!! Warning SPI mode shoul be set in spi.h properly ---------
   SPI_Open();
   printf("\nstart mmap for pin...");
   setPinMmap();

/* --------------------   Init LCD   ---------------------------- */
   LCD_HD_reset();
   LCD_INIT_RM68140();

/*------------------- Pixel Format ---- Brightness ------- BGR order ----------------*/
  WriteComm(0x3a); WriteData(0x66);  //set pixel format 18bits pixel
  WriteComm(0x36); WriteData(0x00);   // 0 RGB ???--set color order as GBR,same as BMP file
  //set entry mode,flip mode here if necessary
  //brightness control if necessary

while(1)
{
     //-------------- reload total_numbe after one round show ---------
      printf("    Ncount =%d  \n",Ncount);
      if( Ncount < 0)
      {
          /* find out all BMP files in specified path  */
          Find_BMP_files(argv[1]);
          printf("\n\n===============  reload BMP file, totally  %d BMP-files found.   =================\n",BMP_file_total);
          if(BMP_file_total == 0){
             printf("\n No BMP file found! \n");
             return 1; }
          Ncount=BMP_file_total-1; //--[Nount] from 0
      }


     //----- load BMP file path -------------
        sprintf(strf,"%s%s",argv[1],BMP_file_name[Ncount]);
        printf("str = %s\n",strf);
        Ncount--;

        offp=18; // reset file offset for pic width and height

     //------------------------------ open file and show picture ------------
         fp=open(strf,O_RDONLY);
         if(fp<0)
         {
           printf("\n Fail to open the file!\n");
          //----------------if loop--------------
          continue;
          }
         else
          printf("%s opened successfully!\n",strf);

    //------------    seek position and readin picWidth and picHeight   ----------
        if(lseek(fp,offp,SEEK_SET)<0)
           printf("Fail to offset seek position!\n");
        read(fp,buff,8);
        //for(i=0;i<8;i++){
        //    printf("buff[%d]=%0x2  ",i,buff[i]);
        //    printf("\n");}
        picWidth=buff[3]<<24|buff[2]<<16|buff[1]<<8|buff[0];
        picHeight=buff[7]<<24|buff[6]<<16|buff[5]<<8|buff[4];
        printf("\n picWidth=%d    picHeight=%d",picWidth,picHeight);

      /*--------------------- MMAP -----------------------*/
        MapLen=picWidth*picHeight*3+54;
        pmap=(uint8_t*)mmap(NULL,MapLen,PROT_READ,MAP_PRIVATE,fp,0);
        if(pmap == MAP_FAILED)
         {  printf("\n pmap mmap failed!");
            return 1; }
         else
           printf("\n pmap mmap successfully!");

        //------- to triger page refresh --------
        //for(j=0;j<MapLen;j++)
        //        tmp=*(pmap+j);

      /*-------------- get start time ---------------------*/
       gettimeofday(&t_start,NULL);
       printf("\n Start Time: %lds + %ldus\n",t_start.tv_sec,t_start.tv_usec);

     // ----------------  calculate GRAM area ------------------
        /*****   WARNING: picWidth must be an 4xtimes number!! ******/
        Hb=(320-picWidth+1)/2;
        Vb=(480-picHeight+1)/2;
        Hs=Hb; He=Hb+picWidth-1;
        Vs=Vb; Ve=Vb+picHeight-1;
        printf("Hs=%d,  He=%d \n",Hs,He);
        printf("Vs=%d,  Ve=%d \n",Vs,Ve);

        GRAM_Block_Set(Hs,He,Vs,Ve);  //--GRAM area set
        //printf("GRAM Block Set finished!\n");
     // -------------- prepare for LCD GRAM fast write ---------
        // WriteComm(0x3a); WriteData(0x66);  //set pixel format 18bits pixel
        //WriteComm(0x36); WriteData(0x00);   // 0 RGB ???--set color order as GBR,same as BMP file
        //GRAM set to start position here if necessary
        WriteComm(0x2c);  // --prepare for continous GRAM write
        //printf("GRAM fast write preparation finished!\n");

     //-------------------- read RGB data from the file  and write to GRAM ----------------
         total=picWidth*picHeight*3; //--total bytes for BGR data
         nbuff=total/SPIBUFF;
         residual=total%SPIBUFF;

         offp=54; //--- start point where BGR data begins
         oftemp=offp+pmap;

         for(i=0;i<nbuff;i++)
         {
                 //-----------write BGR interface to LCD, first 6bits of each 8bits for very color is valid ------------
                 WriteNData(oftemp+i*SPIBUFF,SPIBUFF);
         }
         if(residual!=0)
                 WriteNData(offp+pmap+i*SPIBUFF,residual);

    printf("---------------------   Finish drawing the picture -------------\n");

      /*---------------- get end time ------------------*/
       gettimeofday(&t_end,NULL);
       printf("End time: %ld s+%ld us\n",t_end.tv_sec,t_end.tv_usec);

      /*-------------- calculate time slot ------------*/
       cost_times=t_end.tv_sec-t_start.tv_sec;
       cost_timeus=t_end.tv_usec-t_start.tv_usec;
       printf("Cost time: %ld s + %ld us\n\n\n",cost_times,cost_timeus);

       //sleep(SHOW_DELAY);

//WriteComm(0x29);WriteData(0x00); //--display on
//delayms(5000); //---put delay in python show prograam
//WriteComm(0x28);WriteData(0x00);  //--disply off

munmap(pmap,MapLen);
//printf(" before close: fp=%d\n",fp);
close(fp); //--fp will not set to NULL however close
//printf("after close: fp=%d\n",fp);
}

/* =========================  close files and release mem   ======================== */
 SPI_Close();
 resPinMmap();
 return 0;

}



/*--------------- clean work when forced to exit -----------*/
void ExitClean()
{
 printf("\n ----Exit signal received!  start clean work.....\n");
 close(fp);
 SPI_Close();
 printf("SPI close finish!\n");
 resPinMmap();
 printf("PinMmap clean finish!\n");
 printf("ExitClean finish!");
 exit(0);
}

/* --------- find out all BMP files in specified directroy ----------*/
int Find_BMP_files(char* path)
{
DIR *d;
struct dirent *file;
int fn_len;

BMP_file_total=0; //--reset total  file number
BMP_file_num=0; //--reset file  index
/*----------- if open dir error ------*/
if(!(d=opendir(path)))
{
  printf("error open dir: %s !\n",path);
  return 1;
}

while((file=readdir(d))!=NULL)
{
  /* ------- find out *.bmp files  -------- */
   fn_len=strlen(file->d_name);
   if(strncmp(file->d_name+fn_len-4,".bmp",4)!=0 )
       continue;
   strncpy(BMP_file_name[BMP_file_num++],file->d_name,fn_len);
   BMP_file_total++;
 }


 closedir(d);
 return 0;
}
