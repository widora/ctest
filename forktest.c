#include <stdio.h>
#include <sys/types.h> //--for ??
#include <unistd.h> //--for ??
#include <stdlib.h> 
#include <fcntl.h>
#include <sys/wait.h>

int glob1=1;

int main(int argc,char *argv[])
{
          int fd,pid,vpid,status,test;
          int glob2=2;
          printf("before fork glob1=%d\n",++glob1);
          printf("before fork glob2=%d\n",++glob2);
          printf("current pid=%d\n",getpid());
          printf("current parent ppid=%d\n",getppid());
           
          if((pid=fork())<0)
          {
               perror("fork");
               exit(-1);
           }
          
           else if (pid==0)
           {
              printf("fork created successfully\n");
              printf("current parent ppid=%d\n",getppid());
              printf("fork child pid  =%d\n",getpid());  
              printf("fork glob1=%d\n",++glob1);
              printf("fork glob2=%d\n",++glob2);
           }

          else
          {
               if((vpid=vfork())<0)
                 {
                    perror("vfork");
                 }
                else if(vpid==0)
                 {
                    glob1++;
                    glob2++;
                    printf("vfork succeed!\n");
                    printf("current ppid=%d\n",getppid());
                    printf("vfork glob1=%d\n",++glob1);
                    printf("vfork glob2=%d\n",++glob2);
                 }
                else
                 {
                     wait(&vpid);
                     wait(&pid);
                     printf("after child pid created  glob1=%d\n",glob1);
                     printf("after child pid created  glob2=%d\n",glob2);
                  }
            }

           printf(" Finish pid %d\n",getpid());
           exit(0);


}
