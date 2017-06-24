#include <stdio.h>
#include <errno.h>
#include <stdint.h>

int main(int argc,char *argv[])
{
  int i;

FILE *fp;
char *str;
uint8_t ut=0x35;

for(i=0;i<10;i++)
	printf("dddddddd%02xn ",ut);


fp=fopen(argv[1],"w+");
if(fp==NULL)
	{
	sprintf(str,"fopen %s",argv[1]);
	perror(str);
	}
fclose(fp);
}

