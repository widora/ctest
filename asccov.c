#include "ascii.h"
#include <stdio.h>

void main()
{
 int i,j,k,m;
 unsigned char tmp[16]; //new char data 8x16
  

 for(i=0;i<96;i++) //total 96 ascii chars
 {
	  for(m=0;m<8;m++) // each 8 bits 
          {
		tmp[2*m]=0;
		tmp[2*m]+=((ascii_8x16[i*16+0]&(0x80>>m))>>(7-m)); //--low bit
		tmp[2*m]+=((ascii_8x16[i*16+1]&(0x80>>m))>>(7-m))*2; //-- bit
		tmp[2*m]+=((ascii_8x16[i*16+2]&(0x80>>m))>>(7-m))*4; //-- bit
		tmp[2*m]+=((ascii_8x16[i*16+3]&(0x80>>m))>>(7-m))*8; //-- bit
		tmp[2*m]+=((ascii_8x16[i*16+4]&(0x80>>m))>>(7-m))*16; //--bit
		tmp[2*m]+=((ascii_8x16[i*16+5]&(0x80>>m))>>(7-m))*32; //-- bit
		tmp[2*m]+=((ascii_8x16[i*16+6]&(0x80>>m))>>(7-m))*64; //--
		tmp[2*m]+=((ascii_8x16[i*16+7]&(0x80>>m))>>(7-m))*128; //--high bit
		tmp[2*m+1]=0;
		tmp[2*m+1]+=((ascii_8x16[i*16+8]&(0x80>>m))>>(7-m)); //--low bit
		tmp[2*m+1]+=((ascii_8x16[i*16+9]&(0x80>>m))>>(7-m))*2; //-- bit
		tmp[2*m+1]+=((ascii_8x16[i*16+10]&(0x80>>m))>>(7-m))*4; //-- bit
		tmp[2*m+1]+=((ascii_8x16[i*16+11]&(0x80>>m))>>(7-m))*8; //-- bit
		tmp[2*m+1]+=((ascii_8x16[i*16+12]&(0x80>>m))>>(7-m))*16; //--bit
		tmp[2*m+1]+=((ascii_8x16[i*16+13]&(0x80>>m))>>(7-m))*32; //-- bit
		tmp[2*m+1]+=((ascii_8x16[i*16+14]&(0x80>>m))>>(7-m))*64; //--
		tmp[2*m+1]+=((ascii_8x16[i*16+15]&(0x80>>m))>>(7-m))*128; //--high bit

/*
		tmp[2*m+1]+=(ascii_8x16[i*16+8]&(0x80>>m))>>7; //--low bit
		tmp[2*m+1]+=(ascii_8x16[i*16+9]&(0x80>>m))>>6; //-- bit
		tmp[2*m+1]+=(ascii_8x16[i*16+10]&(0x80>>m))>>5; //-- bit
		tmp[2*m+1]+=(ascii_8x16[i*16+11]&(0x80>>m))>>4; //-- bit
		tmp[2*m+1]+=(ascii_8x16[i*16+12]&(0x80>>m))>>3; //--bit
		tmp[2*m+1]+=(ascii_8x16[i*16+13]&(0x80>>m))>>2; //-- bit
		tmp[2*m+1]+=(ascii_8x16[i*16+14]&(0x80>>m))>>1; //--
		tmp[2*m+1]+=(ascii_8x16[i*16+15]&(0x80>>m))>>0; //--high bit
*/
	    }

             for(k=0;k<16;k++)
  		printf("0x%02x,",tmp[k]);
              printf("\n");

 	}


}
