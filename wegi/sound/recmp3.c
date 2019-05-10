/*-----------------------------------------------------------------------------

			<< Glossary >>

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
			period(Left sample,Left ,left...), period(right,right...),
			, period()...


Midas Zhou
-------------------------------------------------------------------------------*/
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <shine/layer3.h>
#include "pcm2wav.h"


int init_shine_mono(shine_t *pshine, shine_config_t *psh_config, int sample_rate,int bitrate);

int main(int argc, char** argv)
{

	int ret;

	/* for pcm capture */
	snd_pcm_t *pcm_handle;
	int nchanl=1;	/* number of channles */
	int format=SND_PCM_FORMAT_S16_LE;
	int bits_per_sample=16;
	int frame_size=2;		/*bytes per frame, for 1 channel, format S16_LE */
	int sample_rate=8000; 		/* HZ,sample rate */
	bool enable_resample=true;	/* whether to enable resample */
	int latency=500000;		/* required overall latency in us */
	snd_pcm_uframes_t period_size;  /* max numbers of frames that HW can hanle each time */

	/* for CAPTURE, period_size=1536 frames, while for PLAYBACK: 278 frames */
	snd_pcm_uframes_t chunk_frames=32; /*in frame, expected frames for readi/writei each time, to be modified by ... */

	int record_size=128*1024;	/* total pcm raw size for recording/encoding */

	/* chunk_frames=576, */
	int16_t buff[4*SHINE_MAX_SAMPLES]; /* enough size of buff to hold pcm raw data AND encoding results */
	int16_t *pbuf=buff;
	int count;

	/* for shine encoder */
	shine_t	sh_shine;		/* handle to shine encoder */
	shine_config_t	sh_config; 	/* config structure for shine */
	int sh_bitrate=16; 		/* kbit */
	int ssamples_per_chanl; /* expected shine_samples (16bit depth) per channel to feed to the shine encoder each session */
	int ssamples_per_pass;	/* nchanl*ssamples_per_chanl, for each encoding pass/session */
	unsigned char *mp3_pout; /* pointer to mp3 data after each encoding pass */
	int mp3_count;		/* byte counter for shine mp3 output per pass */

	FILE *fmp3;

	/* adjust record volume */
	system("amixer -D hw:0 set Capture 57");
	system("amixer -D hw:0 set 'ADC PCM' 246");

	/* init shine */
	init_shine_mono(&sh_shine, &sh_config, sample_rate, sh_bitrate);

	/* get number of ssamples per channel, to feed to the encoder */
	ssamples_per_chanl=shine_samples_per_pass(sh_shine); /* return audio ssamples expected  */
	printf("%s: MONO ssamples_per_chanl=%d frames for Shine encoder.\n",__func__, ssamples_per_chanl);
	/* change to frames for snd_pcm_readi() */
	chunk_frames=ssamples_per_chanl*16/bits_per_sample*nchanl; /* 16bits for a shine input sample */
	printf("Expected pcm readi chunk_frames for the shine encoder is %d frames per pass.\n",chunk_frames);
	printf("SHINE_MAX_SAMPLES = %d \n",SHINE_MAX_SAMPLES);
	printf("Shine bitrate:%d kbps\n",sh_config.mpeg.bitr);

	/* open pcm captrue device */
	if( snd_pcm_open(&pcm_handle, "plughw:0,0", SND_PCM_STREAM_CAPTURE, 0) <0 ) {
		printf("Fail to open pcm captrue device!\n");
		return -1;
	}
	printf("open pcm captrue device...OK. \n");


	/* set params for pcm capture handle */
	if( snd_pcm_set_params( pcm_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED,
				nchanl, sample_rate, enable_resample, latency )  <0 ) {
		printf("Fail to set params for pcm capture handle.!\n");
		snd_pcm_close(pcm_handle);
		return -2;
	}
	printf("set params for pcm capture...OK. \n");

	/* open file for mp3 input */
	fmp3=fopen("/tmp/ss.mp3","wb");
	if( fmp3==NULL ) {
		printf("Fail to open file for mp3 input.\n");
		snd_pcm_close(pcm_handle);
		return -3;
	}
	printf("open file to save mp3 ...OK. \n");


	/* capture pcm */
	count=0;
	while(count<record_size)
	{
		/* snd_pcm_sframes_t snd_pcm_readi (snd_pcm_t pcm, void buffer, snd_pcm_uframes_t size) */
		ret=snd_pcm_readi(pcm_handle, buff, chunk_frames);  /* return number of frames, 1 chan, 1 frame=size of a sample (S16_Le)  */
		printf("snd_pcm_readi...Ok, ret=%d \n",ret);
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
		else if(ret != chunk_frames) {
			printf("snd_pcm_readi: read end or short read ocuurs! get only %d of %d expected frame data.\n",
										ret, chunk_frames);
			/* pad 0 to chunk_frames */
			if( ret<chunk_frames ) {  /* >chunk_frames NOT possible? */
				memset( buff+ret*frame_size, 0, (chunk_frames-ret)*frame_size);
			}
		}

		/* pcm raw data count */
		count += chunk_frames*frame_size; /* 1 frame = 2bytes:  1 (chan) * 2 bytes (as for S16_LE) */

                 /* 		------	Shine Start -----
		  * unsigned char* shine_encode_buffer(shine_t s, int16_t **data, int *written);
                  * unsigned char* shine_encode_buffer_interleaved(shinet_t s, int16_t *data, int *written);
                  * ONLY 16bit depth sample is accepted by shine_encoder
                  * chanl_samples_per_pass*chanl_val samples encoded
		  */

		  printf("start shine_encode_buffer...\n");
                  mp3_pout=shine_encode_buffer(sh_shine, &pbuf, &mp3_count);
		  printf("Ok, get mp3_count=%d \n",mp3_count);

		/*	 	----- Shine End -----  	*/

		 /* write to mp3 file */
		 fwrite(mp3_pout, mp3_count, sizeof(unsigned char), fmp3);

	} /* end of capture while() */

	printf("Ok,record enough pcm data.\n");
	/* flush out remain mp3 */
	mp3_pout=shine_flush(sh_shine,&mp3_count);


	/* close files and release soruces */
	fclose(fmp3);
	snd_pcm_close(pcm_handle);
	shine_close(sh_shine);

	return 0;
}



/*---------------------------------------------
Initiliaze shine for mono channel
@sample_rate:	input wav sample rate
@bitrate:	bitrate for mp3 file
Return:
	0	oK
	<0 	fails
--------------------------------------------*/
int init_shine_mono(shine_t *pshine, shine_config_t *psh_config, int sample_rate,int bitrate)
{
	shine_set_config_mpeg_defaults(&(psh_config->mpeg));
	psh_config->wave.channels=PCM_MONO;/* PCM_STEREO=2,PCM_MONO=1 */
	/*valid samplerates: 44100,48000,32000,22050,24000,16000,11025,12000,8000*/
	psh_config->wave.samplerate=sample_rate;
	psh_config->mpeg.copyright=1;
	psh_config->mpeg.original=1;
	psh_config->mpeg.mode=MONO;/* STEREO=0,MONO=3 */
	psh_config->mpeg.bitr=bitrate;

	if(shine_check_config(psh_config->wave.samplerate,psh_config->mpeg.bitr)<0) {
        	printf("%s: unsupported sample rate and bit rate configuration.\n",__func__);
	}

	*pshine=shine_initialise(psh_config);
	if(*pshine == NULL){
        	printf("%s: Fail to initialize Shine!\n",__func__);
	        return -1;
	 }

	return 0;
}

