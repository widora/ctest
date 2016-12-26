#include <unistd.h> // STDIN_FILENO,STDOUT_FILENO
#include <stdio.h>

int main(void)
{
 char buf[32];//="";
 int n;

while(1)
{
         usleep(500000);
	 n=read(STDIN_FILENO,&buf[0],32);
         //buf[n]=0;
         printf("n=%d\n",n);
 	 write(STDOUT_FILENO,&buf[0],n);
         printf("aft stdout---\n");
}


return 0;
}
