#include <unistd.h> // STDIN_FILENO,STDOUT_FILENO
#include <stdio.h>
//#include <stdint.h> // type  uint8_t etc.
#include "crc24.h"

#define BUFSIZE 20
int main(void)
{
 char buf[BUFSIZE]="";
 int i,n;
//char str_data[22]="8d780fc5200d33b3d71da0";//error
//char str_data[22]="8d380fc5200d33b3d71da0";
char str_data[30]="";
char str_tmp[3]="";
uint8_t data[11];
uint32_t crc24;


 n=fread(str_data,sizeof(char),29,stdin); // RETURN also will read in
 str_data[n]=0; //--give an end for the string
 printf("n=%d\n",n);
 printf("prinft: %s\n",str_data);

for(i=0;i<11;i++) //--11bits for code
{
  str_tmp[0]=str_data[2*i];
  str_tmp[1]=str_data[2*i+1];
  data[i]=strtoul(str_tmp,NULL,16);
  printf("data[%d]=%02x \n",i,data[i]);

}

printf("data: ");
for(i=0;i<11;i++)
printf("%02x",data[i]); //--2 bits, add 0 
printf("\n");

crc24=crc24_calc(0xffffff,data,11);
printf("CRC24=%0x \n",crc24);
//uint32_t crc24_calc(uint32_t fcs, uint8_t *cp, unsigned int len)
/*
while(1)
{
         usleep(100000);
	 //n=read(STDIN_FILENO,buf,11); //--CAN NOT read from 
         n=fread(buf,sizeof(char),6,stdin); //--CAN read from echo, rtl_adsb
         buf[n]=0; //--give an end for the string
         printf("n=%d\n",n);
         printf("prinft: %s\n",buf);
 	// write(STDOUT_FILENO,buf,n); //--no LINE RETURN for write()
         printf("---aft stdout---\n");
}
*/



return 0;
}
