#include <stdio.h>
#include <stdint.h> // data type
#include <signal.h>
#include "./RM68140.h"
#include <sys/time.h>
#include <dirent.h>
#include <pthread.h>

#define BUFFSIZE 1024 // --- to be times of SPIBUFF
#define SPIBUFF 32   //-- spi write buff-size for SPI_Write()
#define STRBUFF 256   // --- size of file name
#define PTHREAD_NUM 10 // --total number of pthread
#define MAX_WIDTH 320
#define MAX_HEIGHT 480

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

void ExitClean();
int Find_BMP_files(char* path);


/*-------------- for pthread ---------*/
uint16_t nbuff; //total number of SPI write-buff,total =nbuff*SPIBUFF+residual
uint16_t nbuff_count; //counter for nbuff 
uint8_t *p_BGR_start; // pointer to  start of BGR data in  MMAP
pthread_mutex_t mutex_lock;
void pfunc_SPI_WriteNData();

///////////////////////////////////////// ----  main  ---- //////////////////////////////////////////
int main(int argc, char* argv[])
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

 pthread_t pthread_ID[PTHREAD_NUM];

 /* ----------- find out all BMP files in specified path -------- */
   //Find_BMP_files(file_path);
   Find_BMP_files(argv[1]);
   if(BMP_file_total == 0){
     printf("\n No BMP file found! \n");
     return 1; }

    Ncount=BMP_file_total-1; //--[Nount] from 0   

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
       if( Ncount < 1)
          Ncount=BMP_file_total-1;    
     
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
          printf("\n %s opened successfully!",strf);

    //------------    seek position and readin picWidth and picHeight   ----------
        if(lseek(fp,offp,SEEK_SET)<0)
           printf("\n Fail to offset seek position!");
        read(fp,buff,8);
        //for(i=0;i<8;i++){
        //    printf("buff[%d]=%0x2  ",i,buff[i]);
        //    printf("\n");}
        picWidth=buff[3]<<24|buff[2]<<16|buff[1]<<8|buff[0];
        picHeight=buff[7]<<24|buff[6]<<16|buff[5]<<8|buff[4];
        printf("\n picWidth=%d    picHeight=%d \n",picWidth,picHeight);
 
      /*--------------------- MMAP -----------------------*/
        MapLen=picWidth*picHeight*3+54; 
        pmap=(uint8_t*)mmap(NULL,MapLen,PROT_READ,MAP_PRIVATE,fp,0);
        if(pmap == MAP_FAILED)
         {  printf("\n pmap mmap failed!");
            return 1; }
         else
           printf("\n pmap mmap successfully!");
        
        //------- to triger page refresh -------- 
        for(j=0;j<MapLen;j++)  
               tmp=*(pmap+j);                                   
        printf("\n tmp=%0x2",*(pmap+100));
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
         p_BGR_start=pmap+offp;
         
         pthread_mutex_init(&mutex_lock,NULL); //---init of mutex lock
         nbuff_count=0; //--counter for total nbuff
         for(k=0;k<PTHREAD_NUM;k++) //---init pthread
            if(pthread_create(&pthread_ID[k],NULL,(void*)pfunc_SPI_WriteNData,NULL)<0)     
              { 
                printf("Create thread error!\n");
                exit(1);
               }
     
         for(k=0;k<PTHREAD_NUM;k++)
            pthread_join(pthread_ID[k],NULL);          

         pthread_mutex_destroy(&mutex_lock); //--destroy mutex_lock
              
         for(k=nbuff_count;k<nbuff;k++)   //---for nbuff%PTHREAD_NUM residual nbuff data
               WriteNData(p_BGR_start+nbuff_count*SPIBUFF,SPIBUFF);
     
         if(residual!=0)  //---for  total/SPIBUFF residual
               WriteNData(p_BGR_start+nbuff*SPIBUFF,residual);
  
    printf("---------------------   Finish drawing the picture -------------\n\n");
   
      /*---------------- get end time ------------------*/
       gettimeofday(&t_end,NULL);
       printf("End time: %ld s+%ld us\n",t_end.tv_sec,t_end.tv_usec);

      /*-------------- calculate time slot ------------*/ 
       cost_times=t_end.tv_sec-t_start.tv_sec;
       cost_timeus=t_end.tv_usec-t_start.tv_usec;
       printf("Cost time: %ld s + %ld us\n",cost_times,cost_timeus);

       sleep(10);

//WriteComm(0x29);WriteData(0x00); //--display on
//delayms(5000); //---put delay in python show prograam
//WriteComm(0x28);WriteData(0x00);  //--disply off

munmap(pmap,MapLen);
printf(" before close: fp=%d\n",fp);
close(fp); //--fp will not set to NULL however close
printf("after close: fp=%d\n",fp);
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

/*--------------- pthread  function -------*/
void  pfunc_SPI_WriteNData()
{
  int  pk;
  for(pk=0;pk<(nbuff/PTHREAD_NUM);pk++)
  {
   pthread_mutex_lock(&mutex_lock);
     WriteNData(p_BGR_start+nbuff_count*SPIBUFF,SPIBUFF);
     nbuff_count++;
   pthread_mutex_unlock(&mutex_lock);
  }
}


