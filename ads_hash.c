#include <stdlib.h>  //malloc()
#include <string.h>  //memset()
#include <stdio.h>
#include <stdbool.h>
#include <signal.h> //signal()
#include <stdint.h> //uint32_t

#define HASH_TABLE_SIZE 10

char  str_FILE[]="/tmp/ads.data";

typedef struct _STRUCT_DATA
{
  uint32_t int_ICAO24; //int data;             //-----------------!!!! int_ICAO24 is a key word  !!!!!!!
  char str_CALL_SIGN[9];  
}STRUCT_DATA;

typedef struct _NODE  //--------- define a data link, memset to 0 during init.
{
  STRUCT_DATA ads_data;  //---must no be a Pointer!!!
  struct _NODE* next;
}NODE;               //-----!!!!!!!!  end of struct token ;;;;;;;;;  !!!!!

typedef struct _HASH_TABLE
{
  NODE* HashTbl_item[HASH_TABLE_SIZE];   //--------- hash table with total HASH_TABLE_SIZE HashTbl_items (links)
}HASH_TABLE;


HASH_TABLE* create_hash_table()  //---- create a hash table
{
  HASH_TABLE* pHashTbl=(HASH_TABLE*)malloc(sizeof(HASH_TABLE));
  memset(pHashTbl,0,sizeof(HASH_TABLE));
  return pHashTbl;
}

//---- find data in which link node    -----
NODE* find_data_in_hash(HASH_TABLE* pHashTbl,int int_ICAO24)
{
  NODE* pNode;
  if(NULL == pHashTbl) //--hash table is null
     return NULL;
  
  if(NULL == (pNode=pHashTbl->HashTbl_item[int_ICAO24 % HASH_TABLE_SIZE])) //--HashTbl_item(link node) is null
     return NULL;

  while(pNode) //---while pNode is not null, search each link node for the data
   {
     if(int_ICAO24 == pNode->ads_data.int_ICAO24)
       return pNode;
     pNode=pNode->next;
   }

  return NULL;
}

//==================   insert data into hash   ===================
bool insert_data_into_hash(HASH_TABLE* pHashTbl,const STRUCT_DATA* _ads_data)
{
   NODE* pNode;

   if(NULL == pHashTbl)  //---hash table is null
     return false;

   if(NULL == pHashTbl->HashTbl_item[(_ads_data->int_ICAO24) % HASH_TABLE_SIZE]) //--corresponding  HashTbl_item (link node) is null
    {
      pNode=(NODE*)malloc(sizeof(NODE)); 
      memset(pNode,0,sizeof(NODE));
//    printf("memset finish\n");
      //------push in ICAO and Call-sign
      memcpy(&(pNode->ads_data),_ads_data,sizeof(STRUCT_DATA));
//    printf("CALL SIGN push into Node finish.\n");
      pHashTbl->HashTbl_item[(_ads_data->int_ICAO24) % HASH_TABLE_SIZE]=pNode;
//     printf("hash data in the head of table item link!\n");
      return true;
    }

    if(NULL!= find_data_in_hash(pHashTbl,_ads_data->int_ICAO24)) //--if data exist
       return false;

    //-----if corresponding hash-table already existes, then put pnode to the end of the link
    pNode=pHashTbl->HashTbl_item[(_ads_data->int_ICAO24) % HASH_TABLE_SIZE]; //-- get pnode head 
    while(NULL!=pNode->next)
          pNode=pNode->next;       //---move to end of the link

   //---add a link node and put data into 
    pNode->next=(NODE*)malloc(sizeof(NODE));
    memset(pNode->next,0,sizeof(NODE));
    //----push ICAO and CALL-SIGN 
    pNode->next->ads_data.int_ICAO24=_ads_data->int_ICAO24;
    strcpy(&(pNode->next->ads_data.str_CALL_SIGN[0]),&(_ads_data->str_CALL_SIGN[0]));

    return true;
}

//=====================  delete data from hash   ======================
bool delete_data_from_hash(HASH_TABLE* pHashTbl,int int_ICAO24)
{
   NODE* pHead;
   NODE* pNode;
   if(NULL==pHashTbl || NULL==pHashTbl->HashTbl_item[int_ICAO24%HASH_TABLE_SIZE]) //--hash table or HashTbl_item doesn't exist
        return false;

   if(NULL==(pNode=find_data_in_hash(pHashTbl,int_ICAO24))) //--the data haven't been hashed in before
        return false;

   if(pNode==pHashTbl->HashTbl_item[int_ICAO24%HASH_TABLE_SIZE])  //--if it's the first node in found hash table HashTbl_item
    {
       pHashTbl->HashTbl_item[int_ICAO24%HASH_TABLE_SIZE]=pNode->next; //--adjust the head node for hash HashTbl_item, prepare for deleting.
       goto final;
    }

    pHead=pHashTbl->HashTbl_item[int_ICAO24%HASH_TABLE_SIZE];
    while(pNode!=pHead->next)
         pHead=pHead->next;
         pHead->next=pNode->next;  //--bridge two pnodes beside the wanted pnode,preapare for deleting .

final:
    free(pNode);  //--delete the pnode
    return true;     
}

int  count_hash_data(HASH_TABLE* pHashTbl)
{
 int i,cnt=0;
 NODE *pTemp;
 for(i=0;i<HASH_TABLE_SIZE;i++)
 {
   if(pHashTbl->HashTbl_item[i]!=NULL) //
   { 
     cnt++;
     pTemp=pHashTbl->HashTbl_item[i];
     while(pTemp->next!=NULL)
       {
           pTemp=pTemp->next;
           cnt++;
       }//-while
    }//-if
 }//-for
 
 return cnt;
} 

//-----------------save hash data to  file -------------------
void save_hash_data(HASH_TABLE* pHashTbl)
{
 FILE *fp;
 int i;
 int ncount=0;
 NODE* pHead;

 if((fp=fopen(str_FILE,"w+"))==NULL) //--to overwrite original file
 {
    printf("fail to open %s\n",str_FILE);
    return;
 }
 ncount=count_hash_data(pHashTbl);
 fwrite(&ncount,sizeof(int), 1,fp);

  for(i=0;i<HASH_TABLE_SIZE;i++)
 {
   if(pHashTbl->HashTbl_item[i]!=NULL) //
   {
     pHead=pHashTbl->HashTbl_item[i];
     while(pHead)
     {
       fwrite((char*)&(pHead->ads_data),sizeof(STRUCT_DATA),1,fp); // Is it necessary to convert to char* ??
       pHead=pHead->next;
     }//-end of while
   }//-end of if
 }//-end of for

 fclose(fp);
}

//----------------- restore hash data ----------------------
void restore_hash_data(HASH_TABLE* pHashTbl)
{
 FILE *fp;
 int i;
 int ncount=0;
 STRUCT_DATA  sdata;

 if((fp=fopen(str_FILE,"r"))==NULL) //--to overwrite original file
 {
    printf("fail to open %s\n",str_FILE);
    return;
 }
 fread(&ncount,sizeof(int), 1,fp);
 printf("restore ncount=%d\n",ncount);
 for(i=0;i<ncount;i++)
 {
    fread((char*)&sdata,sizeof(STRUCT_DATA),1,fp);
    printf("restore DATA[%d]: %d   %s  \n",i,sdata.int_ICAO24,sdata.str_CALL_SIGN);
 }

 fclose(fp);
}


//-------------------release hash and free memory ------------------
void release_hash_table(HASH_TABLE* pHashTbl)
{
 int i;
 NODE *pHead,*pTemp;
 
 for(i=0;i<HASH_TABLE_SIZE;i++)
 {
   if(pHashTbl->HashTbl_item[i]!=NULL) //
   {
     printf("------- Free hash table item[%d] -------\n",i); 
     pHead=pHashTbl->HashTbl_item[i];
     while(pHead)
     {
       pTemp=pHead;       //---two pnodes, one for deleting and one for pointing.
       pHead=pHead->next;
       if(pTemp!=NULL)
       {
         printf("free a node with int_ICAO24=%d  CALLSIGN:%s \n",pTemp->ads_data.int_ICAO24,pTemp->ads_data.str_CALL_SIGN);
         free(pTemp);
       }
     }//-end of while
   }//-end of if
 }//-end of for

}

//-------- global variables for signal handler --------
 HASH_TABLE* pHashTbl_CODE;

 void sighandler(int sig)
{
  printf("Signal to exit......\n");
  save_hash_data(pHashTbl_CODE);
  release_hash_table(pHashTbl_CODE);
  exit(0); 
}


#define  DATA_NUM 99
//=========================== main =================================
void main(void)
{
  int i,j,k;
  //HASH_TABLE* pHashTbl_CODE  is GLOBAL!!!	`
  pHashTbl_CODE=create_hash_table(); //--INIT HASH TABLE
  printf("hash table create finish!\n");
  //signal handle
  signal(SIGINT,sighandler);

  STRUCT_DATA  ads_data[DATA_NUM];
  printf("STRUCT_DATA difinition finish.\n");

//-------- restore hash data
  restore_hash_data(pHashTbl_CODE);
  
//-------- init STRUCT_DATA
 for(k=0;k<DATA_NUM;k++)
 {
     ads_data[k].int_ICAO24=k;
     strcpy((&ads_data[k])->str_CALL_SIGN,"hello");
 }

   strcpy((&ads_data[95])->str_CALL_SIGN,"thisd");
   strcpy((&ads_data[89])->str_CALL_SIGN,"hgggo");
   strcpy((&ads_data[88])->str_CALL_SIGN,"midas zo");

  printf("ads_data[] init finish\n");
  printf("ICAO24[1]=%d\n",(ads_data+1)->int_ICAO24);

//------------------- insert data into hash
 for(i=0;i<DATA_NUM;i++)
   insert_data_into_hash(pHashTbl_CODE,ads_data+i); 
  printf("insert data to hash finish.\n");

//-------------------- delete hash data 
   delete_data_from_hash(pHashTbl_CODE,29);
   delete_data_from_hash(pHashTbl_CODE,39);
   delete_data_from_hash(pHashTbl_CODE,59);
   delete_data_from_hash(pHashTbl_CODE,69);


//-------------------- count hash data nodes
  printf("total hashed data number:%d \n",count_hash_data(pHashTbl_CODE));
  while(1)sleep(200000);

 printf("----- HASH TABLE EXAMPLE -----\n");

//------------------- release hash
  release_hash_table(pHashTbl_CODE);

}
