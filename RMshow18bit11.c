/*-----------------------------------------------
usage: RMshow bmp-dir/

libs:  lrt  lpthread 

midaszhou@yahoo.com
-------------------------------------------------*/
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <sys/mman.h>
//#include <unistd.h>

#include <stdio.h>
#include <stdint.h> // data type
#include <signal.h>
#include "./RM68140.h"
#include <sys/time.h>
#include <dirent.h> //--directory and file operation
#include <pthread.h>
#include <sched.h> //-- for scheduler set

#define BUFFSIZE 1024 // --- to be times of SPIBUFF
#define SPIBUFF 32   //-- spi write buff-size for SPI_Write()
#define STRBUFF 256   // --- size of file name
#define PROCESS_NUM 1// --total number of sub_processes
#define MAX_WIDTH 320
#define MAX_HEIGHT 480

static int PRINT_ON=1; //--to set printf on or off

uint8_t tmp;
uint8_t *pmap,*oftemp;
int fp; //file handle
int MapLen; // file size,mmap size

char file_path[256];
char BMP_file_name[256][STRBUFF]; //---BMP file directory and name
int BMP_file_num; 
int BMP_file_total=0;

/*------------- time struct -------------*/
struct timeval t_start,t_end;
long cost_timeus=0;
long cost_times=0;

/*--------  functions declaration --------*/
static void ExitClean();
static int Find_BMP_files(char* path);
static int showpic(char* str_bmpf_path);


/*-------------- for child process ---------*/
uint16_t nbuff; //total number of SPI write-buff,total =nbuff*SPIBUFF+residual
int *nbuff_count; //counter for nbuff 
int process_nbuff; //buff number for each process, process_nbuff=nbuff/PROCESS_NUM
uint8_t *p_BGR_start; // pointer to  start of BGR data in  MMAP


pid_t rt_id; //--ID of child procesS
int rt_i;
int ret; //--for  ret=wait(NLL) of child process
int shm_id, shm_mutex_ID; //--ID of shared memory file
char *shm_name="mySHM"; //-name of shared memory file
char *shm_mutex="mutex";
char *ptr_shm; //--mmap pointer for shared memory 
char *ptr_shm_mutex;

/*---------- for mutual exclusion lock ----*/
pthread_mutex_t mutex; 
pthread_mutex_t *pmutex;
pthread_mutexattr_t mutexattr;
int ret_mutex,ret_mutexattr;



///////////////////////////////////////// ----  main  ---- //////////////////////////////////////////
int main(int argc,char* argv[])
{
 int ret;
 pthread_t pid;

 //-----for current thread sheduler ---------
 pthread_attr_t pattr; //thread attribute object
 struct sched_param sch_param; //sheduler parameter

//--------------  set schedurler policy as real-time FIFO -----------
 pthread_attr_init(&pattr);
 pthread_attr_setinheritsched(&pattr,PTHREAD_EXPLICIT_SCHED); //must set before setschedpolicy()
 pthread_attr_setschedpolicy(&pattr,SCHED_FIFO); // adopt FIFO real time scheduler
 pthread_attr_setscope(&pattr,PTHREAD_SCOPE_SYSTEM); // compete with all threads in system
 pthread_attr_setdetachstate(&pattr,PTHREAD_CREATE_DETACHED); // detach with other threads and can't pthread_join()
 sch_param.sched_priority=sched_get_priority_max(SCHED_FIFO); // get max priority of FIFO policy
 pthread_attr_setschedparam(&pattr,&sch_param);

/*
//---------- use sched_setscheduler() function ----------------
 if(sched_setscheduler(0,SCHED_FIFO,&sch_param) == -1)
//-------- 0 and getpid() both OK, if the process contains sub_processes, they will all be infected.
//--- since current parent process and its sub_proesses are all at the highest FIFO level, only ONE process will be active actually!!!!!!!
 {
	printf("-----------sched_setscheduler() fail!\n");
 }
 else 
	printf("-----------sched_setscheduler() successfully!\n");
*/


 ret=pthread_create(&pid,&pattr,showpic,(void *)(argv[1]));
 if(ret!=0){
	printf("Create showpic pthread error!\n");
	return -1;
	}
 else
	printf("-------create showpic() pthread successfully!\n");

//showpic(argv[1]);

 while(1); // hold on for sub_processes.

}



/*-----------------------------------showpic() function -------------------------------*/
//int main(int argc, char* argv[])
static int showpic(char* str_bmpf_path)
{
 int i,j,k,nn;
 int Ncount; //--counter for pic file number
 uint32_t total; // --total bytes of pic file
 uint16_t residual; // --residual after total divided by BUFFSIZE
// uint16_t nbuff;
 uint16_t Hs,He,Vs,Ve; //--GRAM area difinition parameters
 uint16_t Hb,Vb; //--GRAM area corner gap distance from origin

 char strf[STRBUFF]; //--for file name

 uint8_t buff[8]; //--for buffering  data temporarily
 long offp; //offset position
 uint16_t picWidth, picHeight;
 offp=18; // file offset  position for picture Width and Height data


 /* ----------- find out all BMP files in specified path -------- */
   Find_BMP_files(str_bmpf_path);
   if(BMP_file_total == 0){
     printf("\n No BMP file found! \n");
     return 1; }
    Ncount=BMP_file_total-1; //--[Nount] from 0   

 /*---------------- Exit Signal Process ----------------*/
   signal(SIGINT,ExitClean);

 /*---------- initiate mutual exclusion attribute for multi-porcess------*/
    //------ create mutex in shared memory  ------
   shm_mutex_ID=shm_open(shm_mutex,O_RDWR|O_CREAT,0755); //---create and open shared memory for mutex
   ftruncate(shm_mutex_ID,sizeof(pthread_mutex_t));
   ptr_shm_mutex=mmap(NULL,sizeof(pthread_mutex_t),PROT_READ|PROT_WRITE,MAP_SHARED,shm_mutex_ID,0);
   pmutex=(pthread_mutex_t*)ptr_shm_mutex;

   pthread_mutexattr_init(&mutexattr);
   ret_mutexattr=pthread_mutexattr_setpshared(&mutexattr,PTHREAD_PROCESS_SHARED);
   ret_mutex=pthread_mutex_init(pmutex,&mutexattr);
   if(ret_mutex==0 && ret_mutexattr==0)
       printf("Pthread Mutex init. success!\n");
   else
       printf("Pthread Mutex Init. fail!\n");
 /*-------- init shared memory file map and nbuff_count ---------*/
   shm_id=shm_open(shm_name,O_RDWR|O_CREAT,0777); //0644--parent to create shared memory file
   ftruncate(shm_id,sizeof(int)); 
   ptr_shm=mmap(NULL,sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED,shm_id,0); //--map to shared memory 
   nbuff_count=(int*)ptr_shm;
   *nbuff_count=0;
   printf("--------Parent process set nbuff=%d-----------\n",*nbuff_count); 

 /* -------------------    SPI  and PIN control initiation ------------------*/
 //------ !!! Warning SPI mode shoul be set in spi.h properly --------- 
   SPI_Open();
   printf("\nstart mmap for pin...");
   setPinMmap();
   //---!!!! widora will crash here if lirc_mplay has been running already !!!!!!!!!
   //---however, if run RMshow first, then lirc_mplay can start later peacefully !!!

/* --------------------   Init LCD   ---------------------------- */
   LCD_HD_reset();
   LCD_INIT_RM68140();

/*------------------- Pixel Format ---- Brightness ------- BGR order ----------------*/
  WriteComm(0x3a); WriteData(0x66);  //set pixel format 18bits pixel
  WriteComm(0x36); WriteData(0x00);   // 0 RGB ???--set color order as GBR,same as BMP file 
  //set entry mode,flip mode here if necessary
  //brightness control if necessary


/*---------------------  loop load BMP file and  write data to LCD through SPI -------------------*/  
while(1)
{
     //-------------- reload total_numbe after one round show ---------    
       if( Ncount < 1)
          Ncount=BMP_file_total-1;    
     
     //----- load BMP file path -------------
        sprintf(strf,"%s%s",str_bmpf_path,BMP_file_name[Ncount]);
        printf("str = %s\n",strf);         
        Ncount--;        
//        continue;//------------------------------------------------debug-------------------
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
          printf("\n %s opened successfully!",strf);

    //------------    seek position and readin picWidth and picHeight   ----------
        if(lseek(fp,offp,SEEK_SET)<0)
           printf("\n Fail to offset seek position!");
        read(fp,buff,8);
        picWidth=buff[3]<<24|buff[2]<<16|buff[1]<<8|buff[0];
        picHeight=buff[7]<<24|buff[6]<<16|buff[5]<<8|buff[4];
        printf("\n picWidth=%d    picHeight=%d \n",picWidth,picHeight);
 
      /*--------------------- MMAP for BMP file -----------------------*/
        MapLen=picWidth*picHeight*3+54; 
        pmap=(uint8_t*)mmap(NULL,MapLen,PROT_READ,MAP_SHARED,fp,0);
        if(pmap == MAP_FAILED)
         {  printf("\n pmap mmap failed!");
            return 1; }
         else
           printf("\n pmap mmap successfully!");
        
        //------- to trigger page refresh -------- 
        for(j=0;j<MapLen;j++)  
               tmp=*(pmap+j);                                   
        //printf("\n tmp=%0x2",*(pmap+100));
      /*-------------- get start time ---------------------*/
       gettimeofday(&t_start,NULL);
       printf("\n Start Time: %lds + %ldus\n",t_start.tv_sec,t_start.tv_usec);

     // ----------------  calculate GRAM area ------------------
        /*****   WARNING: picWidth must be an 4xtimes number!! ******/
        Hb=(320-picWidth+1)/2;
        Vb=(480-picHeight+1)/2;
        Hs=Hb; He=Hb+picWidth-1;
        Vs=Vb; Ve=Vb+picHeight-1;
//        printf("Hs=%d,  He=%d \n",Hs,He);
//        printf("Vs=%d,  Ve=%d \n",Vs,Ve);

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
         process_nbuff=nbuff/PROCESS_NUM;

         if(process_nbuff==0)
           {
            printf("too small picture!\n");
            break;
           }

         offp=54; //--- start point where BGR data begins in BMP file
         p_BGR_start=pmap+offp; //--start point to BMP data in mmap
         *(nbuff_count)=0; // reset nbuff_count

         for(rt_i=0;rt_i<PROCESS_NUM;rt_i++)
          {
            //-----WARNING !!!!!!  mutex-lock  must NOT put here before FORK, otherwise muter-lock in child process will FAIL
 /*           pthread_mutex_lock(pmutex);
              ret_mutex=pthread_mutex_lock(pmutex);
              if(ret_mutex==0)
                    printf("pthread mutex lock success!\n");
              else
              printf("pthread mutex lock fail!\n");
*/
                rt_id=fork();
             //pthread_mutex_unlock(pmutex);
                if(rt_id==0 || rt_id==-1)break;
//=========                else
//=========                 printf("Create child pid=%d\n",rt_id);   
          }

          if(rt_id==-1)
           {
             printf("fork error!\n");
             break;
           }
         /*--------------------------------- jobs for child process --------------------------*/ 
         else if(rt_id==0)
          {
             for(k=0;k<process_nbuff;k++) //----each child process write nbuff/PROCESS_NUM blocks of SPIBUFF
                 {
                   //---mutex lock----
                   pthread_mutex_lock(pmutex);

                   //------ write data to SPI           
                   WriteNData(p_BGR_start+(*nbuff_count)*SPIBUFF,SPIBUFF);
                   (*nbuff_count)++;
                   
                   //---- mutex unlock----
                   pthread_mutex_unlock(pmutex);
                 }
              //pthread_mutex_destroy(pmutex);
              //----------clean before exit child process --------
              munmap(pmap,MapLen);
              close(fp); //--fp will not set to NULL however close
              //SPI_Close(); //----called by other sub-process, must NOT close!
              //resPinMmap();
              //exit(0); //---for sub-process exit(0) and return 0 is not different.
              return 0;
           }

         /*------------------------------- jobs for  parent process -----------------------*/
        else
       {
          /*------------- wait for child process to finish -------------*/
//          while((*nbuff_count)<(PROCESS_NUM*process_nbuff))
//                         usleep(100);
          for(j=0;j<PROCESS_NUM;j++)
          {
              ret=wait(NULL); //--wait for child processes to terminate
//=========         printf("Child pid=%d finish!\n",ret);
           }

//=========          printf("*******end nbuff_count=%d ********* process_nbuff*PROCESS_NUM=%d\n",*nbuff_count,process_nbuff*PROCESS_NUM);         
          /*------------ draw residual BGR data ------------*/
          for(k=*nbuff_count;k<nbuff;k++)   //---for nbuff%PTHREAD_NUM residual nbuff data
               WriteNData(p_BGR_start+k*SPIBUFF,SPIBUFF);
          if(residual!=0)  //---for  total/SPIBUFF residual
               WriteNData(p_BGR_start+nbuff*SPIBUFF,residual);

         /*---------------- get end time ------------------*/
         gettimeofday(&t_end,NULL);
         printf("End time: %ld s+%ld us\n",t_end.tv_sec,t_end.tv_usec);
         /*-------------- calculate time slot ------------*/ 
         cost_times=t_end.tv_sec-t_start.tv_sec;
         cost_timeus=t_end.tv_usec-t_start.tv_usec;
         printf("Cost time: %ld s + %ld us\n",cost_times,cost_timeus);
         
         printf("---------------------   Finish drawing the picture -------------\n\n");
//==============         sleep(5);
         //WriteComm(0x29);WriteData(0x00); //--display on
         //WriteComm(0x28);WriteData(0x00);  //--disply off
       }

  /*-------------------jobs for both parent and survived child porcesses -----------------*/
  /*-------- No child process shall be survived to carry out following jobs ------------------*/
  munmap(pmap,MapLen);
  printf(" before close: fp=%d\n",fp);
  close(fp); //--fp will not set to NULL however close
  printf("after close: fp=%d\n",fp);
 
} //--while loop end 

/* =========================  close files and release mem   ======================== */
 SPI_Close();
 resPinMmap();
 return 0;

}



/*--------------- clean work when forced to exit -----------*/
static void ExitClean()
{
 printf("\n ----Exit signal received!  start clean work.....\n");
 close(fp);
 SPI_Close();
 printf("SPI close finish!\n");
 resPinMmap();
 printf("PinMmap clean finish!\n");
 shm_unlink(shm_name);
 munmap(ptr_shm,sizeof(int));
 printf("Shared memory and its mmap clean finish!\n");
 printf("ExitClean finish!");
 exit(0);
}

/* --------- find out all BMP files in specified directroy ----------*/
static int Find_BMP_files(char* path)
{
DIR *d;
struct dirent *file;
int fn_len;

//----------- if open dir error ------
if(!(d=opendir(path)))
{
  printf("error open dir: %s !\n",path);
  return 1;
}

while((file=readdir(d))!=NULL)
{
  // ------- find out *.bmp files  -------- 
   fn_len=strlen(file->d_name);
   if(strncmp(file->d_name+fn_len-4,".bmp",4)!=0 )
       continue;
   strncpy(BMP_file_name[BMP_file_num++],file->d_name,fn_len);
   BMP_file_total++;   
 }
 closedir(d);
 return 0;
}


