
/*-------------------------------------------------------------
gcc pcm2mp3.c -lmp3lame

------------------------------------------------------------*/
#include <stdio.h>
#include "lame.h"

int main(void)
{
    int nread, nwrite;

    FILE *pcm = fopen("test.raw", "rb");
    FILE *mp3 = fopen("test.mp3", "wb");

    const int PCM_SIZE = 8192;
    const int MP3_SIZE = 8192;

    short int pcm_buffer[PCM_SIZE*2]; //S16_LE
    unsigned char mp3_buffer[MP3_SIZE];

    lame_t lame=lame_init();
	printf("lame_init() finish\n");
    lame_set_in_samplerate(lame, 8000);
	printf("lame_set_in_samplerate() finish\n");
    lame_set_VBR(lame, vbr_default);
	printf("lame_set_VBR() finish\n");
    lame_init_params(lame);
	printf("lame_init_params() finish\n");

    do {
        nread = fread(pcm_buffer, 2*sizeof(short int), PCM_SIZE, pcm);
	printf("nread=%d\n",nread);
        if (nread == 0)
            nwrite = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
        else
            nwrite = lame_encode_buffer_interleaved(lame, pcm_buffer, nread, mp3_buffer, MP3_SIZE);
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
