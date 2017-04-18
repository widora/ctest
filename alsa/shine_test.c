#include <stdio.h>
#include <stdlib.h> //exit()
#include <string.h>
#include "layer3.h"

/*---------------------- IIR_FILTER -----------------------------------
----------------------------------------------------------------------*/
/* Print some info about what we're going to encode */
static void check_config(shine_config_t *config)
{
  static char *version_names[4] = { "2.5", "reserved", "II", "I" };
  static char *mode_names[4]    = { "stereo", "joint-stereo", "dual-channel", "mono" };
  static char *demp_names[4]    = { "none", "50/15us", "", "CITT" };

  printf("MPEG-%s layer III, %s  Psychoacoustic Model: Shine\n",
    version_names[shine_check_config(config->wave.samplerate, config->mpeg.bitr)],
    mode_names[config->mpeg.mode]);
  printf("Bitrate: %d kbps  ", config->mpeg.bitr);
  printf("De-emphasis: %s   %s %s\n",
    demp_names[config->mpeg.emph],
    ((config->mpeg.original) ? "Original" : ""),
    ((config->mpeg.copyright) ? "(C)" : ""));
}


/*================================================================================
                              ---- ( MAIN ) ----
================================================================================*/
int main(int argc,char* argv[])
{
FILE *fin,*fout;
int16_t pcm_buff[2*SHINE_MAX_SAMPLES];//NULL; //data for readin buffer
int16_t *ppbuff=pcm_buff;
unsigned char* data_out=NULL;
int count=0;
int chanl_samples_per_pass; // samples per channel to feed the encoder each time.
int samples_per_pass; //????????bytes in one pass =chanl_samples_per_pass*nchanl
shine_t sh_shine;
shine_config_t sh_config;
int nchanl=1; //MONO 
int nread=0;
int nwrite=0;

char str_fin[]="/tmp/8k.raw";
char str_fout[]="/tmp/8k.mp3";

//--!!!!! sample depth supposed to be 16bits !!!!!!!!------
shine_set_config_mpeg_defaults(&(sh_config.mpeg)); //--init sh_config struct

sh_config.wave.channels=PCM_MONO; //PCM_STEREO;//PCM_MONO;
sh_config.wave.samplerate=8000;//32000;//44100;//8000; // valid samplerates: 44100,48000,32000,22050,24000,16000,11025,12000,8000
sh_config.mpeg.copyright=1;
sh_config.mpeg.original=1;
sh_config.mpeg.mode=MONO;//STEREO;//MONO;
sh_config.mpeg.bitr=8; //64kbps for 16k //32kbps for 8k samplerate

if(shine_check_config(sh_config.wave.samplerate,sh_config.mpeg.bitr)<0)
	printf("Unsupported samplerate/bitrate configuration.\n");

sh_shine=shine_initialise(&sh_config);
if(sh_shine == NULL){
	printf("Fail to init shine!\n");
	exit(-1);
  }
check_config(&sh_config);

chanl_samples_per_pass=shine_samples_per_pass(sh_shine);//numbers of samples(per channel) to feed the encoder with
printf("---- shine one channle samples per pass %d\n",chanl_samples_per_pass);
printf("---- shine bitrate:%d kbps\n",sh_config.mpeg.bitr);

//-----allocate mem for shine encoding buff, assume 16bits sample dapth ---------
samples_per_pass=chanl_samples_per_pass*nchanl;
printf("samples_per_pass=%d\n",samples_per_pass);
//pcm_buff=(int16_t *)malloc(samples_per_pass*sizeof(int16_t));//16bits sample depth assumed!!!

fin=fopen(str_fin,"rb");
if(fin == NULL){
	printf("fail to open input file!\n");
	exit(-1);
 }
fout=fopen(str_fout,"wb");
if(fout == NULL){
	printf("fail to open output file!\n");
	exit(-1);
 }

printf("sizeof(int16_t)=%d\n",sizeof(int16_t));
printf("sizeof(unsigned char)=%d\n",sizeof(unsigned char));
printf("start encoding ...\n");
while(nread=fread(pcm_buff,sizeof(int16_t),samples_per_pass,fin)) //-- binary read !!!  16bits sample depth assumed !!!!
{
	if(nread < samples_per_pass){
		printf("nread < samples_per_pass! nread = %d\n",nread);
		//break;
		memset(pcm_buff+nread,0,(samples_per_pass-nread)*sizeof(int16_t)); //pad remaining buff space with zero
	 }
	//------ unsigned char *shine_encode_buffer(shine_t s,int16_t **data, int *written);
	//------ unsigned cahr *shine_encode_buffer_interleaved(shinet_t s, int16_t *data, int *written);
	data_out=shine_encode_buffer(sh_shine,&ppbuff,&count);
	//data_out=shine_encode_buffer_interleaved(sh_shine,pcm_buff,&count);
	//printf("finish to enconde %d bytes of mp3 data!\n",count);
	nwrite=fwrite(data_out,sizeof(unsigned char),count,fout);// count=number of data_chunk been written, here is 1  (not bytes!)
	//nwrite=fwrite(data_out,sizeof(int16_t),count/2,fout);// OK for 44.1k count=number of data_chunk been written, here is 1  (not bytes!)
	//nwrite=fwrite(data_out,4,count/4,fout);
/*
	if(nwrite != count){
		printf("nwrite != count \n");
		exit(-1);
	 }
*/
	//printf("count=%d\n",count);
}
//---flush and write remaining data 
//unsigned char *shine_flush(shine_t s, int *written)
data_out=shine_flush(sh_shine,&count);
//fwrite(data_out,sizeof(unsigned char),count,fout);// count=number of data_chunk been written, here is 1  (not bytes!)
fwrite(data_out,sizeof(int16_t),count/2,fout);// count=number of data_chunk been written, here is 1  (not bytes!)

//if(pcm_buff != NULL)  // free shine data buff
//	free(pcm_buff);

shine_close(sh_shine);// close encoder

//----close file -----
close(fin);
close(fout);
return 1;

}
