#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include "layer3.h"

/*================================================================================
                              ---- ( MAIN ) ----
================================================================================*/
int main(int argc,char* argv[])
{
FILE *fin,*fout;
int16_t pcm_buff[2*SHINE_MAX_SAMPLES];
int16_t *ppbuff=pcm_buff;
unsigned char* data_out=NULL;
int count=0;
int chanl_samples_per_pass; // samples per channel to feed the encoder each time.
int samples_per_pass; //  =chanl_samples_per_pass*nchanl
shine_t sh_shine;
shine_config_t sh_config;
int nchanl=1; //MONO
int nread=0;
int nwrite=0;

char str_fin[]="/tmp/8k.raw";
char str_fout[]="/tmp/8k.mp3";

//--------初始化config.mpeg结构 ------
shine_set_config_mpeg_defaults(&(sh_config.mpeg)); //--init sh_config struct

//-------- 配置 config 结构 -------------
sh_config.wave.channels=PCM_MONO; //PCM_STEREO;//PCM_MONO;
sh_config.wave.samplerate=8000;//valid samplerates: 44100,48000,32000,22050,24000,16000,11025,12000,8000
sh_config.mpeg.copyright=1;
sh_config.mpeg.original=1;
sh_config.mpeg.mode=MONO;//STEREO;//MONO;
sh_config.mpeg.bitr=8; //64kbps for 16k //32kbps for 8k samplerate

//-------- 检查samplerate和bit rate是否匹配  -----------
if(shine_check_config(sh_config.wave.samplerate,sh_config.mpeg.bitr)<0)
	printf("Unsupported samplerate/bitrate configuration.\n");

//---------  初始化shine  ---------------
sh_shine=shine_initialise(&sh_config);
if(sh_shine == NULL){
	printf("Fail to init shine!\n");
	exit(-1);
  }

//-------  每次需要喂给shine进行压缩的的采样数 ---------------
chanl_samples_per_pass=shine_samples_per_pass(sh_shine);//numbers of samples(per channel) to feed to the encoder for each encoding session
printf("---- shine one channle samples per pass %d\n",chanl_samples_per_pass);
printf("---- shine bitrate:%d kbps\n",sh_config.mpeg.bitr);
samples_per_pass=chanl_samples_per_pass*nchanl;
printf("samples_per_pass=%d\n",samples_per_pass);fin=fopen(str_fin,"rb");

//--------------- 打开文件读写 ------------
if(fin == NULL){
	printf("fail to open input file!\n");
	exit(-1);
 }
fout=fopen(str_fout,"wb");
if(fout == NULL){
	printf("fail to open output file!\n");
	exit(-1);
 }

printf("start encoding ...\n");
while(nread=fread(pcm_buff,sizeof(int16_t),samples_per_pass,fin))
{
	if(nread < samples_per_pass){  //---小于喂给采样数时，剩余用0填充
		printf("nread < samples_per_pass! nread = %d\n",nread);
		memset(pcm_buff+nread,0,(samples_per_pass-nread)*sizeof(int16_t));
	 }
	//------ unsigned char *shine_encode_buffer(shine_t s,int16_t **data, int *written);
	//------ unsigned cahr *shine_encode_buffer_interleaved(shinet_t s, int16_t *data, int *written);
	data_out=shine_encode_buffer(sh_shine,&ppbuff,&count);
	nwrite=fwrite(data_out,sizeof(unsigned char),count,fout);// count=number of data_chunk been written, here is 1  (not bytes!)
}

//-------- 把shine中剩余的数据压缩后读出-----------
data_out=shine_flush(sh_shine,&count);
fwrite(data_out,sizeof(unsigned char),count,fout);// count=number of data_chunk been written, here is 1  (not bytes!)


shine_close(sh_shine);// close encoder

//----close file -----
close(fin);
close(fout);
return 1;

}
