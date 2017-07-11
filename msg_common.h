/*--------------------------------------
Common head for message IPV communication
--------------------------------------*/
#ifndef __MSG_COMMON_H__
#define __MSG_COMMON_H__

#define MSG_BUFSIZE  64

struct st_msg
{
	long int msg_type;
	char text[MSG_BUFSIZE];
};


#endif
