/*--------------------------------------
Common head for message queue IPC
--------------------------------------*/
#ifndef __MSG_COMMON_H__
#define __MSG_COMMON_H__

#include <stdio.h>
#include <sys/msg.h>

#define MSG_BUFSIZE  64
#define MSG_TYPE_TING 1
#define MSG_TYPE_CC1101 2
#define MSG_KEY_OLED_TEST 5678 //--- msg queue identical key

struct g_st_msg
{
	long int msg_type;
	char text[MSG_BUFSIZE];
};

static struct g_st_msg g_msg_data; //message data
static long int msg_type=0; //message type

/*-------------------------------------------------------
 create message queue and return message queue identifier
-------------------------------------------------------*/
static int createMsgQue(key_t key)
{
  int msg_id=-1;
  struct msqid_ds msg_conf;//msg configuration set

  //----- create message IPC ------
  msg_id=msgget(key,0666 | IPC_CREAT);
  if(msg_id<0)
  {
        perror("msgget error");
        return msg_id;
   }

  //----!!!! Minimize msg queue size to eliminate accumulation of msg -------
  if(msgctl(msg_id,IPC_STAT,&msg_conf)==0)
  {
        msg_conf.msg_qbytes=sizeof(g_msg_data)*4;//!!!!-depend on how many clients will open and send data to it simutaneouly !!!!
        if(msgctl(msg_id,IPC_SET,&msg_conf)==0) 
                printf("msgctl: reset msg_conf to allow only one message in the queue succeed!\n");
  }
  else
        printf("msgctl: reset msg_conf to allow only one message in the queue fails!\n");

  return msg_id;
}


/*-------------------------------------------------------
 receive data in message queue with specified message type
-------------------------------------------------------*/
static int recvMsgQue(int msg_id,long msg_type)
{
   int msg_ret=-1;

   //?????? need to initialize msg_data ????? 
   //-----get IPC message-----
   msg_ret=msgrcv(msg_id,(void *)&g_msg_data,MSG_BUFSIZE,msg_type,IPC_NOWAIT);  // return number of bytes copied into msg buf
   if(msg_ret == EAGAIN)
   {
//	printf("No message with specified type now.\n");
	return msg_ret;
   }
   if(msg_ret<0) //??? how about =0?
   {
        perror("msgrcv");
        printf("Receive IPC message fails!\n");
        return msg_ret;
   }
   else
   {
        printf("IPC Message received:%s\n",g_msg_data.text);
        return msg_ret;
   }

}


/*-------------------------------------------------------
 send data to  message queue with specified message type
 return 0 if succeed.
-------------------------------------------------------*/
static int sendMsgQue(int msg_id,long msg_type, char *data)
{
    int msg_ret=-1;

//    memset(g_msg_data.text,sizeof(g_msg_data.text),0);
    g_msg_data.msg_type=msg_type;
    strncpy(g_msg_data.text,data,sizeof(g_msg_data.text));
    printf("start msgsnd()...\n");
    msg_ret=msgsnd(msg_id,(void *)&g_msg_data,MSG_BUFSIZE,IPC_NOWAIT); //non-blocking
    if(msg_ret == EAGAIN)
    {
	printf("Space not available now.\n");
	return msg_ret;
    }
    else if(msg_ret != 0 ) 
    {
         perror("msgsnd failed");
	 return msg_ret;
    }
    return msg_ret;
}



#endif
