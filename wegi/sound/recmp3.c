/*--------------------------------------------------------------------------------------------------

		<< Golossry >>

Sample:		usually 8bits or 16bits, one sample data width.
Channel: 	1-Mono. 2-Stereo
Frame: 		sizeof(one sample)*channels
Rate: 		frames per second
Period: 	Max. frame numbers hard ware can be handled each time.
		(different value for PLAYBACK and CAPTURE!!)
Running show:   1536 frames for CAPTURE; 278 frames for PLAYBACK
Chunk: 		frames receive from/send to hard ware each time.
Buffer: 	N*periods
Interleaved mode:	record period data frame by frame, such as
	 		frame1(Left sample,Right sample), frame2(), ......
Uninterleaved mode:	record period data channel by channel, such as
			period(Left sample,Left ,left...), period(right,right...), period()...


Midas Zhou
-----------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include "layer3.h"
#include "pcm2wav.h"


int init_shine_mono(shine_t *pshine, shine_config_t *psh_config, int sample_rate,int bitrate,
		    int *psamples_per_chanl, int *psamples_per_pass);


int main(int argc, char** argv)
{
	int ret;
	/* for pcm capture */
snd_pcm_t *pcm_handle;
int nchanl=1;	/* number of channles */
int format=SND_PCM_FORMAT_S16_LE;
int frame_size=2;		/*bytes, for 1 channel, format S16_LE */
int sample_rate=8000; 		/* HZ,record wav sample rate */
bool enable_resample=true;	/* whether to enable resample */
int latency=500000;		/* required overall latency in us */
snd_pcm_uframes_t period_size;  /* max numbers of frames that HW can hanle each time */
/* for CAPTURE, period_size=1536 frames, while for PLAYBACK: 278 frames */
snd_pcm_uframes_t chunk_size=32; /* frames for readi/writei each time */
int buf_size=64*1024;
unsigned char pcm_buff[64*1024]; /* buff to hold pcm raw data */
unsigned char *pbuf=pcm_buff;	 /* pointer to current position in the pcm buff */
int count;

/* for shine encoder */
shine_t	sh_shine;		/* handle to shine encoder */
shine_config_t	sh_config; 	/* config structure for shine */
int bitrate=8; 			/* kbit */
int samples_per_chanl; /* expected samples per channel to feed to the shine encoder each session */
int samples_per_pass;	/* nchanl*samples_per_chanl, samples per frame for each session */


	/* adjust record volume */
	system("amixer -D hw:0 set Capture 57");
	system("amixer -D hw:0 set 'ADC PCM' 246");

	/* init shine */
	init_shine_mono(&sh_shine, &sh_config, sample_rate, bitrate, &samples_per_chanl, &samples_per_pass);

	/* open pcm captrue device */
	if( snd_pcm_open(&pcm_handle, "plughw:0,0", SND_PCM_STREAM_CAPTURE, 0) <0 ) {
		printf("Fail to open pcm captrue device!\n");
		exit(-1);
	}

	/* set params for pcm capture handle */
	if( snd_pcm_set_params( pcm_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED,
				nchanl, sample_rate, enable_resample, latency )  <0 ) {
		printf("Fail to set params for pcm capture handle.!\n");
		exit(-1);
	}


	/* capture pcm */
	count=0;
	pbuf=pcm_buff;
	while(count<buf_size)
	{
		ret=snd_pcm_readi(pcm_handle, pbuf, chunk_size);  /* return number of frames, 1 chan, 1 frame=size of a sample (S16_Le)  */
		if(ret == -EPIPE ) {
			/* EPIPE for overrun */
			printf("Overrun occurs during capture, try to recover...\n");
			/* try to recover, to put the stream in PREPARED state so it can start again next time */
			snd_pcm_prepare(pcm_handle);
			continue;
		}
		else if(ret<0) {
			printf("snd_pcm_readi error: %s\n",snd_strerror(ret));
			continue; /* carry on anyway */
		}
		/* CAUTION: short read may cause trouble! Let it carry on though. */
		else if(ret != chunk_size) {
			printf("snd_pcm_readi: read end or short read ocuurs! get only %d of %d expected data.\n",
										ret, chunk_size);
		}

		//count += ret*2; /* 1 frame = 2bytes:  1 (chan) * 2 bytes (as for S16_LE) */
		//pbuf=pcm_buff+count;
		pbuf += ret*frame_size; /* 1 frame = 2bytes:  1 (chan) * 2 bytes (as for S16_LE) */
		if( (long)(pbuf-pcm_buff) > buf_size - chunk_size*frame_size )
			break;

	} /* end of capture while() */


	/* save as wav */
	simplest_pcm16le_to_wave(pcm_buff,"/tmp/rec.wav", nchanl, sample_rate, pbuf-pcm_buff);

}



/*---------------------------------------------
Initiliaze shine for mono channel
@sample_rate:	input wav sample rate
@bitrate:	bitrate for mp3 file
--------------------------------------------*/
int init_shine_mono(shine_t *pshine, shine_config_t *psh_config, int sample_rate,int bitrate,
		    int *psamples_per_chanl, int *psamples_per_pass)
{
	shine_set_config_mpeg_defaults(&(psh_config->mpeg));
	psh_config->wave.channels=1;// PCM_STEREO=2,PCM_MONO=1
	psh_config->wave.samplerate=sample_rate;
	psh_config->mpeg.copyright=1;
	psh_config->mpeg.original=1;
	psh_config->mpeg.mode=3;//STEREO=0,MONO=3
	psh_config->mpeg.bitr=bitrate;

	if(shine_check_config(psh_config->wave.samplerate,psh_config->mpeg.bitr)<0) {
        	printf("%s: unsupported sample rate and bit rate configuration.\n",__func__);
	}

	*pshine=shine_initialise(psh_config);
	if(*pshine == NULL){
        	printf("%s: Fail to initialize Shine!\n",__func__);
	        return -1;
	 }

	/* number of samples per channel, to feed to the encoder */
	*psamples_per_chanl=shine_samples_per_pass(*pshine); /* expected samples per channel for one encoding session */
	*psamples_per_pass=(*psamples_per_chanl)*1; //---for MONO, 1 channel
	printf("%s: MONO samples_per_pass=%d for Shine encoder.\n",__func__, *psamples_per_pass);

	return 0;
}

