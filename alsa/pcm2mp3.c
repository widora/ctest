
/*-------------------------------------------------------------
gcc pcm2mp3.c -lmp3lame

------------------------------------------------------------*/
#include <stdio.h>
#include "lame.h"

#define IN_SAMPLE_RATE 8000 // 4k also OK
#define OUT_SAMPLE_RATE 8000 // MPEG only allows: 88,11.025k,12k,16k,22.05k,24k,32k,44.1k,48k

int main(void)
{
    int nread, nwrite;

    FILE *pcm = fopen("test.raw", "rb");
    FILE *mp3 = fopen("test.mp3", "wb");

    //-------buffer size for each read/write --------
    const int PCM_SIZE = 64;//128;//8192; //frames, of 16bits sample
    const int MP3_SIZE = 8192;//at leat 128 at 8k rate 1 channel ;//8192; //bytes

    int Nchannel=1; //number of channels
    short int pcm_buffer[PCM_SIZE*Nchannel]; //for 16bits sample
    unsigned char mp3_buffer[MP3_SIZE];

    if(pcm ==NULL){ printf("Fail to open input file!\n");
	return -1;
     }


    lame_t lame=lame_init();
	printf("lame_init() finish\n");
    lame_set_in_samplerate(lame,IN_SAMPLE_RATE); //--sample rate to be same as raw/pcm file
	printf("lame_set_in_samplerate() finish\n");
    lame_set_out_samplerate(lame,OUT_SAMPLE_RATE); //must not less than input samplerate.
    lame_set_num_channels(lame,1);
    lame_set_brate(lame,11);//set brate compression ratio,default 11
    lame_set_mode(lame,MONO);//0=stereo,1=jstereo,2=dual channel(not supported) 3=mono
    lame_set_quality(lame,5);//0~9   recommended: 2=high 5=medium 7=low
    //lame_set_VBR(lame, vbr_default); //Variable Bit Rate,  default is CBR
     //--!!!! VBR file is bigger than CBR at 8K sample rate .
	printf("lame_set_VBR() finish\n");
    lame_init_params(lame);
	printf("lame_init_params() finish\n");

    do {
        nread = fread(pcm_buffer, Nchannel*sizeof(short int), PCM_SIZE, pcm);// read one frame, 16bits sample
	printf("nread=%d\n",nread);
        if (nread == 0)
            nwrite = lame_encode_flush(lame, mp3_buffer, MP3_SIZE); //--end of mp3
        else
	    if(Nchannel == 1)
	            nwrite = lame_encode_buffer(lame, pcm_buffer,NULL,nread, mp3_buffer, MP3_SIZE);
            else if(Nchannel == 2)
                    nwrite = lame_encode_buffer_interleaved(lame, pcm_buffer, nread, mp3_buffer, MP3_SIZE);
	printf("nwrite=%d\n",nread);
        fwrite(mp3_buffer, nwrite, 1, mp3);
    } while (nread != 0);

    lame_close(lame);
    fclose(mp3);
    fclose(pcm);

    return 0;
}

/*
int main()
{
//...
//...
// extraction of PCM audio data file .wav in file.pcm
short read, write;

FILE *pcm = fopen("file.pcm", "rb");
FILE *mp3 = fopen("file.mp3", "wb");

const int PCM_SIZE = 18424;
const int MP3_SIZE = 13424;

short int pcm_buffer[PCM_SIZE];
unsigned char mp3_buffer[MP3_SIZE];

lame_t lame = lame_init();

if ( lame == NULL ) {
    cout<<"Unable to initialize MP3"<<endl;
    return -1;
}

lame_set_num_channels(lame, 1);
lame_set_in_samplerate(lame, 8000);
lame_set_out_samplerate(lame, 8000);
lame_set_brate(lame, 128);
lame_set_mode(lame, MONO);
lame_set_quality(lame, 2);
lame_set_bWriteVbrTag(lame, 0);
if (( lame_init_params(lame)) < 0) {
    cout << "Unable to initialize MP3 parameters"<<endl;
    return -1;
}

do {
    read = fread(pcm_buffer,sizeof(short), PCM_SIZE, pcm);
    if (read == 0)
        write = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
    else
        write = lame_encode_buffer_interleaved(lame, pcm_buffer, read, mp3_buffer, MP3_SIZE);
    fwrite(mp3_buffer, write, 1, mp3);
} while (read != 0);

lame_close(lame);
fclose(mp3);
fclose(pcm);

return 0;
}
*/
