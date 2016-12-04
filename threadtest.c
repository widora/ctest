#include <stdio.h>
#include <pthread.h>
#include <stdlib.h> // for exit()

#define Thread_Num 10 

void thread()
{
 int i;
 
 for(i=0;i<5;i++)
  {
   printf("This is from thread: %d \n",i);
   usleep(1000);
  }
}

int main()
{
 pthread_t id[Thread_Num];
 int j,i,ret;
 
 for(j=0;j<Thread_Num;j++) 
   ret=pthread_create(&id[j],NULL,(void *)thread,NULL);

 if(ret!=0)
   {
    printf("Create thread error!\n");
    exit(1);
    }

 for(i=0;i<5;i++)
  {
   printf("This is from main: %d \n",i);
   usleep(1000);
  }

for(j=0;j<Thread_Num;j++) 
 pthread_join(id[j],NULL);

 return(0);

}



