/*-------------------------------------------------------------------

midaszhou@qq.com
-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //-strlen()
#include <stdint.h> //uint32_t


uint32_t adsb_crc24(uint32_t *bin32_divid)
{

/*****  *bin32_divid: message data, totally will be 88-bits  ******/
/*       *bin32_divid   *(bin32_divid+1) *(bin32_divid+2)        */

int j;
uint32_t crc32_GP=0xFFFA0480;//0x04C11DB7; // CRC Generator Polynomial alread inlucdes the highest first bit, while standard CRC usually omitted. 

 printf("INPUT CRC DIVIDEND (22x4bits) : %08x%08x%06x \n",*bin32_divid,*(bin32_divid+1),*(bin32_divid+2)>>8);

for(j=0;j<88;j++)
 {
    if(*bin32_divid & 0x80000000)
        (*bin32_divid)^=crc32_GP; // xor CRC generator polynomial
    *bin32_divid=*bin32_divid<<1;
    if(*(bin32_divid+1) & 0x80000000)  *(bin32_divid)|=1; // shift code[1] to code[0]
    *(bin32_divid+1)=*(bin32_divid+1)<<1;
    if(*(bin32_divid+2) & 0x80000000)  *(bin32_divid+1)|=1; // shift code[2] to code[1]
    *(bin32_divid+2)=*(bin32_divid+2)<<1;
 }
//  printf("CRC: %08x \n",*bin32_divid); 
//  printf("CRC24: %06x \n",(*bin32_divid)>>8);

  return (*bin32_divid)>>8;

} 
