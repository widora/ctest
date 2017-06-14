#include <stdlib.h>  //malloc()
#include <string.h>  //memset()
#include <stdio.h>
#include <stdbool.h>
#include <signal.h> //signal()


#define HASH_TABLE_SIZE 10

typedef struct _NODE  //--------- define a data link
{
  int data;             //-------------------- !!!!!!! data is a key word  !!!!!!!
  //--- put more data here ---//
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
NODE* find_data_in_hash(HASH_TABLE* pHashTbl,int data)
{
  NODE* pNode;
  if(NULL == pHashTbl) //--hash table is null
     return NULL;
  
  if(NULL == (pNode=pHashTbl->HashTbl_item[data % HASH_TABLE_SIZE])) //--HashTbl_item(link node) is null
     return NULL;

  while(pNode) //---while pNode is not null, search each link node for the data
   {
     if(data == pNode->data)
       return pNode;
     pNode=pNode->next;
   }

  return NULL;
}

//==================   insert data into hash   ===================
bool insert_data_into_hash(HASH_TABLE* pHashTbl,int data)
{
   NODE* pNode;
   if(NULL == pHashTbl)  //---hash table is null
     return false;

   if(NULL == pHashTbl->HashTbl_item[data % HASH_TABLE_SIZE]) //--corresponding  HashTbl_item (link node) is null
    {
      pNode=(NODE*)malloc(sizeof(NODE)); 
      memset(pNode,0,sizeof(NODE));
      pNode->data = data;
      pHashTbl->HashTbl_item[data % HASH_TABLE_SIZE]=pNode;
      return true;
    }

    if(NULL!= find_data_in_hash(pHashTbl,data)) //--if data exist
       return false;

    pNode=pHashTbl->HashTbl_item[data % HASH_TABLE_SIZE]; //-- data hash to an alread existed HashTbl_item
    while(NULL!=pNode->next)
          pNode=pNode->next;       //---move to end of the link

   //---add a link node and put data into
    pNode->next=(NODE*)malloc(sizeof(NODE));
    memset(pNode->next,0,sizeof(NODE));
    pNode->next->data =data;
    return true;

}

//=====================  delete data from hash   ======================
bool delete_data_from_hash(HASH_TABLE* pHashTbl,int data)
{
   NODE* pHead;
   NODE* pNode;
   if(NULL==pHashTbl || NULL==pHashTbl->HashTbl_item[data%HASH_TABLE_SIZE]) //--hash table or HashTbl_item doesn't exist
        return false;

   if(NULL==(pNode=find_data_in_hash(pHashTbl,data))) //--the data haven't been hashed in before
        return false;

   if(pNode==pHashTbl->HashTbl_item[data%HASH_TABLE_SIZE])  //--if it's the first node in found hash table HashTbl_item
    {
       pHashTbl->HashTbl_item[data%HASH_TABLE_SIZE]=pNode->next; //--adjust the head node for hash HashTbl_item, prepare for deleting.
       goto final;
    }

    pHead=pHashTbl->HashTbl_item[data%HASH_TABLE_SIZE];
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
         printf("free a node with data=%d! \n",pTemp->data);
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
  release_hash_table(pHashTbl_CODE);
  exit(0); 
}



//=========================== main =================================
void main(void)
{
  int i;
  //HASH_TABLE* pHashTbl_CODE;
  pHashTbl_CODE=create_hash_table(); //--INIT HASH TABLE
  //signal handle
  signal(SIGINT,sighandler);

 for(i=0;i<999;i++)
  insert_data_into_hash(pHashTbl_CODE,i); 

  printf("total hashed data number:%d \n",count_hash_data(pHashTbl_CODE));
  while(1)sleep(200000);

  printf("----- HASH TABLE EXAMPLE -----\n");

  release_hash_table(pHashTbl_CODE);
  //--------free mem-------

}
