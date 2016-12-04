#include <stdio.h>
#include <stdlib.h> //--for exit()
//#include <unistd.h>

int main()
{
pid_t status;
int i;

for(i=0;i<10;i++)
{
  status=fork();
  if(status==0 || status==-1)break;
}

if(status==-1) //--parent  
{
   printf("Fork i=%d error!\n",i);
   exit(0);
}

else if(status==0) //--child
{
  printf("Fork i=%d successfully! pid=%d\n",i,getpid());

  exit(0);
}

else //--parent
{
  
 printf("This is parent! final status=%d\n",status);
}

 return 0;
 exit(0);

}

