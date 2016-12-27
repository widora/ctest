#include  <stdio.h> // stdout stdin 
#include <stdlib.h>
#include  <unistd.h>

void main(void)
{
/*  
while(1) //------Can not be accepted by | pipe operation
{
  usleep(800000);
//  printf("123\n");
  write(STDOUT_FILENO,"123\n",4);
  usleep(800000);
  printf("1234\n");
  usleep(800000);
  printf("12345\n");
  usleep(800000);
  printf("123456\n");
  usleep(500000);
  printf("1234567\n");
  usleep(500000);
  printf("12345678\n");
  usleep(500000);
  printf("123456789\n");
  usleep(800000);
  write(STDIN_FILENO,"1234567890\n",11);
//  printf("1234567890\n");
}
*/
  setvbuf(stdout,NULL,_IONBF,0); //--!!! set stdout no buff
 char str[]="123456789;\r\n";
 while(1)
 {
   usleep(800000);
   fprintf(stdout,"*12345678;\r\n"); //must give an end, or it will never end entering.
   //fwrite(str,sizeof(char),12,stdout); //---can NOT be accepted by fread(STDIN_OUT...
 }

}
