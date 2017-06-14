#ifndef _ADSB_CRC_H
#define _ADSB_CRC_H

/*-------------------------------------------------------------------
 CRC and Fix-1bit-erro functions 

midaszhou@qq.com
-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //-strlen()
#include <stdint.h> //uint32_t


/********************************************************** 
//-------------- using POINTER call is DANGEROUS !!!! and will contaminate original datas  --------------------

uint32_t adsb_crc24(uint32_t *bin32_divid)
{
//bin32_divid: message data, totally will be 88-bits  
//bin32_divid   *(bin32_divid+1) *(bin32_divid+2)        
int j;
uint32_t crc32_GP=0xFFFA0480;//0x04C11DB7; // CRC Generator Polynomial alread inlucdes the highest first bit, while standard CRC usually omitted. 
//printf("INPUT CRC DIVIDEND (22x4bits) : %08x%08x%06x \n",*bin32_divid,*(bin32_divid+1),*(bin32_divid+2)>>8);
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
***********************************************************/



/******************************************************
//------------  passing array to the function(), it's also a POINTER call!!!!!! -------------------
uint32_t adsb_crc24( uint32_t tbin32_divid[])
{
//  bin32_divid[0-2]: message data, totally will be 88-bits 
//uint32_t tbin32_divid[3];
int j;
uint32_t crc32_GP=0xFFFA0480;//0x04C11DB7; // CRC Generator Polynomial alread inlucdes the highest first bit, while standard CRC usually omitted. 

//---copy message data to a temp. array, so following proceeding  will not contaminate original datas 
//tbin32_divid[0]=bin32_divid[0];tbin32_divid[1]=bin32_divid[1];tbin32_divid[2]=bin32_divid[2];
printf("INPUT CRC DIVIDEND (22x4bits) : %08x%08x%06x \n",tbin32_divid[0],tbin32_divid[1],tbin32_divid[2]>>8);
for(j=0;j<88;j++)
 {
    if(*tbin32_divid & 0x80000000)
        (*tbin32_divid)^=crc32_GP; // xor CRC generator polynomial
    *tbin32_divid=*tbin32_divid<<1;
    if(*(tbin32_divid+1) & 0x80000000)  *(tbin32_divid)|=1; // shift code[1] to code[0]
    *(tbin32_divid+1)=*(tbin32_divid+1)<<1;
    if(*(tbin32_divid+2) & 0x80000000)  *(tbin32_divid+1)|=1; // shift code[2] to code[1]
    *(tbin32_divid+2)=*(tbin32_divid+2)<<1;
 }
  printf("CRC24: %06x \n",(*tbin32_divid)>>8);
  printf("after calculation tbin32_divid[0]=%06x",tbin32_divid[0]>>8);
  return (*tbin32_divid)>>8;
} 
******************************************************/



//------------  passing VALUE instead of POINTER to function(), it's safer,but cost time.!!!!!! -----------------
//---  return 24bits CRC in uint32_t 
uint32_t adsb_crc_88bits( uint32_t *bin32_divid) // for 88bits message
{
//--- bin32_divid[0-2]: message data, totally will be 88-bits 

uint32_t tbin32_divid[3];
int j;
uint32_t crc32_GP=0xFFFA0480;//0x04C11DB7; // CRC Generator Polynomial alread inlucdes the highest first bit, while standard CRC usually omitted. 

//---copy message data to a temp. array, so following proceeding  will not contaminate original datas 
tbin32_divid[0]=bin32_divid[0];tbin32_divid[1]=bin32_divid[1];tbin32_divid[2]=bin32_divid[2];
printf("INPUT CRC DIVIDEND (22x4bits) : %08x%08x%06x \n",tbin32_divid[0],tbin32_divid[1],tbin32_divid[2]>>8);

for(j=0;j<88;j++)
 {
    if(*tbin32_divid & 0x80000000)
        (*tbin32_divid)^=crc32_GP; // xor CRC generator polynomial
    *tbin32_divid=*tbin32_divid<<1;
    if(*(tbin32_divid+1) & 0x80000000)  *(tbin32_divid)|=1; // shift code[1] to code[0]
    *(tbin32_divid+1)=*(tbin32_divid+1)<<1;
    if(*(tbin32_divid+2) & 0x80000000)  *(tbin32_divid+1)|=1; // shift code[2] to code[1]
    *(tbin32_divid+2)=*(tbin32_divid+2)<<1;
 }
  printf("CRC24: %06x \n",(*tbin32_divid)>>8);
  printf("after calculation tbin32_divid[0]=%06x \n",bin32_divid[0]>>8);
  return (*tbin32_divid)>>8;
}


/*========================================================================================
                            CRC CALCULATION FOR VARIABLE WIDTH 
 adsb_crc(uint32_t *bin32_divid, int nbits)
 *bin32_divid :  pointer to full message
 nbits:     width of messsage for CRC check
 Return:  24bits CRC
 Note: original data uncontaminated.
========================================================================================*/
uint32_t adsb_crc(uint32_t *bin32_divid, int nbits)
{
//--- bin32_divid[0-3]: message data, Max. to be 112-bits 
uint32_t tbin32_divid[4]={0,0,0,0};
int32_t mask=0xffffffff;
int i,j,k,m;
uint32_t crc32_GP=0xFFFA0480;//0x04C11DB7; // CRC Generator Polynomial alread inlucdes the highest first bit, while standard CRC usually omitted. 

//---copy message data to a temp. array tbin32_divid[], so following proceeding  will not contaminate original data
if(nbits>112) //---nbits Max 112
{
    printf("Message length great than 28*4=112bits, use 112 as Max. value.\n");
    nbits=112;
}
k=nbits/32;
m=nbits%32;
//printf("k=%d, m=%d \n",k,m);
if(k>0)
{
  for(i=0;i<k;i++)
     tbin32_divid[i]=bin32_divid[i];
}  
mask=mask<<(32-m);
tbin32_divid[k]=bin32_divid[k] & mask;
//printf("INPUT CRC DIVIDEND : %08x%08x%08x%04x \n",tbin32_divid[0],tbin32_divid[1],tbin32_divid[2],tbin32_divid[3]>>16);
for(j=0;j<nbits;j++) //------CRC calculation
 {
    if(*tbin32_divid & 0x80000000)
        (*tbin32_divid)^=crc32_GP; // xor CRC generator polynomial
    *tbin32_divid=*tbin32_divid<<1;
    if(*(tbin32_divid+1) & 0x80000000)  *(tbin32_divid)|=1; // shift code[1] to code[0]
    *(tbin32_divid+1)=*(tbin32_divid+1)<<1;
    if(*(tbin32_divid+2) & 0x80000000)  *(tbin32_divid+1)|=1; // shift code[2] to code[1]
    *(tbin32_divid+2)=*(tbin32_divid+2)<<1;
    if(*(tbin32_divid+3) & 0x80000000)  *(tbin32_divid+2)|=1; // shift code[2] to code[1]
    *(tbin32_divid+3)=*(tbin32_divid+3)<<1;
 }
 // printf("***  CRC24: %06x \n",(*tbin32_divid)>>8);
  return (tbin32_divid[0])>>8; //---EXTRACT 24bits CRC, Signed-	Int will affect right-shift result. 
}


/*========================================================================== 
    -------  FIX 1BIT ERROR IN MESSAGE --------- 
    return position of the error bit starting from 0
    *bin32_code : full message 
     nbits:   width of messsage for CRC check
     Return:   -1:error unfixable;     0: no error;     1: error fixed;
     Note: original data will be modified! !!! ensure pointer safe !!!!
===========================================================================*/
int adsb_fixerror_slow(uint32_t *bin32_code)
{
   int ret=-1;
   int i,j,k,m;  
   uint32_t mask=0x80000000;
   if(!adsb_crc(bin32_code,112))
   {
//      printf("CRC check OK.  No error in original message.\n");
      return 0;
   }

   for(i=0;i<112;i++)
   {
      k=i/32;
      m=i%32;
      mask=(0x80000000>>m);
      bin32_code[k]^=mask; // exchange 1 and 0 
      if(!adsb_crc(bin32_code,112))
      {  
 //        printf("1bit error in bin32_code[%d] fixed! \n",32*k+m);
         return 1;
      } 
      bin32_code[k]^=mask; // exchange 1 and 0, resume data
    }

//    printf("More than 1 bit error, can't fix the error!\n");
    return -1;  
}


#endif
