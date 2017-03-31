
 /*--------------------------------------------------------------------------
 Quote from :  http://blog.csdn.net/leixiaohua1020/article/details/50534316

 Convert PCM16LE raw data to WAVE format

 ----------------------------------------------------------------------------*/
 /**
 * Convert PCM16LE raw data to WAVE format
 * @param pcmpath      Input PCM file.
 * @param channels     Channel number of PCM file.
 * @param sample_rate  Sample rate of PCM file.
 * @param wavepath     Output WAVE file.
 */

#include <stdio.h>
#include <string.h>

int simplest_pcm16le_to_wave(const char *pcmpath,int channels,int sample_rate,const char *wavepath)
{

    typedef struct WAVE_HEADER{
        char         fccID[4];
        unsigned   long    dwSize;
        char         fccType[4];
    }WAVE_HEADER;

    typedef struct WAVE_FMT{
        char         fccID[4];
        unsigned   long       dwSize;
        unsigned   short     wFormatTag;
        unsigned   short     wChannels;
        unsigned   long       dwSamplesPerSec;
        unsigned   long       dwAvgBytesPerSec;
        unsigned   short     wBlockAlign;
        unsigned   short     uiBitsPerSample;
    }WAVE_FMT;

    typedef struct WAVE_DATA{
        char       fccID[4];
        unsigned long dwSize;
    }WAVE_DATA;


    if(channels==0||sample_rate==0){
    channels = 2;
    sample_rate = 44100;
    }
    int bits = 16;


    WAVE_HEADER   pcmHEADER;
    WAVE_FMT   pcmFMT;
    WAVE_DATA   pcmDATA;

    unsigned   short   m_pcmData;
    FILE   *fp,*fpout;

    fp=fopen(pcmpath, "rb");
    if(fp == NULL) {
        printf("open pcm file error\n");
        return -1;
    }
    fpout=fopen(wavepath,   "wb+");
    if(fpout == NULL) {
        printf("create wav file error\n");
        return -1;
    }

    //WAVE_HEADER
    memcpy(pcmHEADER.fccID,"RIFF",strlen("RIFF"));
    memcpy(pcmHEADER.fccType,"WAVE",strlen("WAVE"));
    fseek(fpout,sizeof(WAVE_HEADER),1); // 1--SEEK_CUR
    //fwrite to file later till we get total size.

    //WAVE_FMT
    pcmFMT.dwSamplesPerSec=sample_rate;
    pcmFMT.dwAvgBytesPerSec=pcmFMT.dwSamplesPerSec*sizeof(m_pcmData);
    pcmFMT.uiBitsPerSample=bits;
    memcpy(pcmFMT.fccID,"fmt ",strlen("fmt "));
    pcmFMT.dwSize=16;
    pcmFMT.wBlockAlign=2;
    pcmFMT.wChannels=channels;
    pcmFMT.wFormatTag=1;

    fwrite(&pcmFMT,sizeof(WAVE_FMT),1,fpout);

    //WAVE_DATA;
    memcpy(pcmDATA.fccID,"data",strlen("data"));
    pcmDATA.dwSize=0;
    fseek(fpout,sizeof(WAVE_DATA),SEEK_CUR);
    //fwrite to file later till we get total size.

    //read pcm file and write to wav file
    fread(&m_pcmData,sizeof(unsigned short),1,fp);
    while(!feof(fp)){
        pcmDATA.dwSize+=2;
        fwrite(&m_pcmData,sizeof(unsigned short),1,fpout);
        fread(&m_pcmData,sizeof(unsigned short),1,fp);
    }

    //total size
    pcmHEADER.dwSize=44+pcmDATA.dwSize;

    rewind(fpout);
    fwrite(&pcmHEADER,sizeof(WAVE_HEADER),1,fpout);
    fseek(fpout,sizeof(WAVE_FMT),SEEK_CUR);
    fwrite(&pcmDATA,sizeof(WAVE_DATA),1,fpout);

    fclose(fp);
    fclose(fpout);

    return 0;
}

int main(int argc, char* argv[])
{
 char* str_fpcm;
 char* str_fwav;

 printf("sizeof(unsigned short)=%d\n",sizeof(unsigned short));
 printf("sizeof(unsigned long)=%d\n",sizeof(unsigned long));

 if(argc < 3){
	printf("Not enough input arguments!\n");
	printf("Example: pcm2wav test.pcm test.wav\n");
	return -1;
  }

 str_fpcm=argv[1];
 str_fwav=argv[2];

simplest_pcm16le_to_wave(str_fpcm,1,8000,str_fwav);


}
