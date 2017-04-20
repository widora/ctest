/*--------------------------------------------------
ALSA auto. loopback record and play test
Quote from: http://blog.csdn.net/ljclx1748/article/details/8606831

 !!!---- Since mt7688 has no FPU, using Shine fixed_point mp3 encoder is a better choice! while mp3lame encoding will cost most of CPU processing time ---!!!
 !!! Sample rate=8k is OK, while sample rate=48k will make it too sensitive !!!

Usage: autorecord [options]

1.It will monitor surrounding sound wave and trigger 60s recording if loud voice is sensed,
  then it will playback. The sound will also be saved to a raw file and a mp3 file.

2. Ensure there are no other active/pausing applications which may use ALSA simutaneously when you run the program,
   sometimes it will make noise to the CAPUTRUE.
   If PLAYBACK starts while Mplayer is playing, then sounds from Mplayer will totally disappear. 
   However if Mplayer starts later than PLAYBACK, two streams of sounds will be mixed.

3. use alsamixer to adjust Capture and ADC PCM value
	ADC PCM 0-255 
	Capture 0-63  
	Low down headphone voice will reduce noise.

4. some explanation:
	sample: usually 8bits or 16bits, one sample data width.
	channel: 1-Mono. 2-Stereo
	frame: sizeof(one sample)*channels
	rate: frames per second
	period: Max. frame numbers hard ware can be handled each time. (different value for PLAYBACK and CAPTURE!!)
		running show: 1536 frames for CAPTURE; 278 frames for PLAYBACK
	chunk: frames receive from/send to hard ware each time.
	buffer: N*periods
	interleaved mode:record period data frame by frame, such as  frame1(Left sample,Right sample),frame2(), ......
	uninterleaved mode: record period data channel by channel, such as period(Left sample,Left ,left...),period(right,right...),period()...
5. lib: lasound  -lshine //--lmp3lame
6. IIR filter will produce a sharp noise when input sound is too lound!
7. For loopback record/playback, you may adjust volume in asound.conf.

Make for Widora-neo
midas-zhou
--------------------------------------------------*/
#include <stdio.h>
#include <getopt.h>
#include <asoundlib.h>
#include <stdbool.h>
#include "layer3.h" //for shine encoder
#include "filter.h" // for digital filter
#include "pcm2wav.h"//save as .wav

//----- for PCM record 
#define NON_BLOCK 0 // 1 - ture ,0 -false  !!! non_block mode not good !!!
#define CHECK_FREQ 125 //-- use average energy in 1/CHECK_FREQ (s) to indicate noise level
#define SAMPLE_RATE 8000 //--PCM sample rate, 4k also OK
#define CHECK_AVERG 200 //--threshold value of wave amplitude to trigger record
#define KEEP_AVERG 200 //--threshold value of wave amplitude for keeping recording
#define DELAY_TIME 3 //seconds -- recording time after one trigger
#define MAX_RECORD_TIME 60 //seconds --max. record time in seconds
#define MIN_SAVE_TIME 15 //seconds --min. recording time for saving, short time recording will be discarded.
bool SAVE_RAW_FILE=false; // save raw file or not(default)
bool SAVE_WAVE_FILE=false; // save to .wav file
bool IIR_FILTER_ON=false; // enable IIR filter or not
//------ for MP3
//#define MP3_CHUNK_SIZE 1024 //bytes, chunk buffer size for lame_encode_buffer(), to be big enough!! at least 128?
#define MP3_SAMPLE_RATE 8000 // sample rate for output mp3 file,  MIN.8K for lame
bool SAVE_MP3_FILE=false; //save mp3 file or not(default)

FILE *fmp3; // file for mp3 output =fopen("record.mp","wb");

//-------- for lame mp3 encoder ---------------
unsigned char *mp3_buf=NULL; //---- pointer to final mp3 data,
int mp3_buf_len; //=0.5*wave_buf_len ---mp3 buffer length in bytes, to be half of wave_buf_len

int16_t sh_pcm_buff[2*SHINE_MAX_SAMPLES]; // for shine_encoder PCM buffer
int chanl_samples_per_pass; //samples per channle to feed to the shine encoder each session
int samples_per_pass; //=chanl_samples_per_pass*nchanl
shine_t sh_shine; //handle to shine encoder
shine_config_t sh_config;//config structure for shine
int mp3_buf_used=0;


//------------------- for sound device ----------- 
char str_record_device[]="loopback";
char str_play_device[]="in1out2";//This is a mixer
snd_pcm_t *pcm_handle;// for record 
snd_pcm_t *pcm_handle_play; // for play
snd_pcm_t *pcm_handle_loopback; // for loopback record ONLY
snd_pcm_hw_params_t *params;
snd_pcm_format_t format_val;
unsigned char *wave_buf=NULL; //---pointer to loopback  wave buffer
unsigned char *wave_buf2=NULL; //--- capture wave buffer
int wave_buf_len; //---wave buffer length in bytes, for wave_buf and wave_buf2
int wave_buf_used=0; //---used wave buf length in bytes, for wave_buf and wave_buf2
int bit_per_sample;
snd_pcm_uframes_t frames;
snd_pcm_uframes_t period_size;//length of period (max. numbers of frames that hw can handle each time)
//for CAPTURE period_size=1536 frame, and for PLAYBACK period_size=278 frames
snd_pcm_uframes_t chunk_size=32;//numbers of frames read/write to hard ware each time
int chunk_byte; //length of chunk (period)  (in bytes)
unsigned int chanl_val,rate_val;
int dir;

//------------ time struct ---------------
struct timeval t_start,t_end;
long cost_timeus=0;
long cost_times=0;
char str_time[20]={0};
time_t timep;
struct tm *p_tm;

//------------------- functions declaration ----------------------
int init_shine_mono(int samplerate,int bitrate);//--input and  output sample rate is the same.
bool device_open(snd_pcm_t** pcm_handle,char* device,int mode);
bool device_setparams(snd_pcm_t* pcm_handle,int nchanl,int rate,int token_resample);
bool device_capture(snd_pcm_t* pcm_handle_loopback,snd_pcm_t* pcm_handle_mic);
bool device_play(snd_pcm_t* pcm_handle);
bool device_check_voice(snd_pcm_t* pcm_hanle);

/*-----------------  output file option --------------*/
static struct option longopts[]={    //---long opts seems not applicable !!!!!!!
{"raw",no_argument,NULL,'r'},
{"mp3",no_argument,NULL,'m'},
{ NULL,0,NULL,0}
};


/*========================= MAIN ====================================*/
int main(int argc,char* argv[])
{
int fd;
int rc;
int nb;
int opt;
int token_resample;//1 or 0
int ret=0;
char str_file[50]={0}; //---directory of save_file 
chanl_val=1; // 1 channel


//--------- parse options --------
while((opt=getopt_long(argc,argv,"rmwfh",longopts,NULL)) !=-1){
	switch(opt){
		case 'r':
			SAVE_RAW_FILE = true;
			break;
		case 'm':
			SAVE_MP3_FILE = true;
			break;
		case 'w':
			SAVE_WAVE_FILE = true;
			break;
		case 'f':
			IIR_FILTER_ON = true;
			break;
		case 'h':
			printf("Program usage:   autorecord [options] \n");
			printf("-r (raw)   save to a RAW file in /tmp, with time stamp as its name.\n");
			printf("-m (mp3)   save to a mp3 file in /tmp, with time stamp as its name.\n");
			printf("-w (wav)   save to a wav file in /tmp, with time stamp as its name.\n");
			printf("-f (filter)  enable IIR filter.\n");
			printf("use combined options such as: autorecord -rmf \n\n");
			printf("The programe will automatically start recording if lound sound is detected.\n");
			printf("It will continue recording if there is voice detected within every 3 seconds.\n");
			printf("Max. recording time is 60s, it will stop automatically then.\n");
			printf("The data may be saved only if recoding time exceeds 30s in one session.\n");
			printf("It will playback recoding data in memory after each session.\n\n");

			return -1;
		case '?':
			printf("Unkown option!\n");
			return -1;
	}
}


//-------- set recording volume -------
system("amixer set Capture 57");
system("amixer set 'ADC PCM' 246"); // adjust sensitivity, or your can use alsamxier to adjust in realtime.


//-------- INIT SHINE ---------
if(SAVE_MP3_FILE)
	init_shine_mono(SAMPLE_RATE,8);//16,32--input and  output sample rate is the same.

while(1)
{
//------- for test -----------
printf("capture sample rate:%d, MP3 sample rate:%d\n",SAMPLE_RATE,SAMPLE_RATE); //shine  input and output samplerate is the same


//--------录音   beware of if...if...if...if...expressions
if (!device_open(&pcm_handle_loopback,str_record_device,SND_PCM_STREAM_CAPTURE)){
	ret=1;
	goto OPEN_STREAM_CAPTURE_ERR;
   }
//---- set param for LOOPBACK device-----
 if(!device_setparams(pcm_handle_loopback,chanl_val,SAMPLE_RATE,0)){ // resample not allowed!!!
	printf(" set params for LOOP-BACK record error!\n");
	ret=2;
	goto SET_LOOPBACK_DEVICE_PARAMS_ERR;
	}
 printf("Finish setting params for LOOP-BACK record!\n");
 
/*
if (!device_setparams(chanl_val,SAMPLE_RATE)){
	ret=2;
	goto SET_CAPTURE_PARAMS_ERR;
  }
//printf("---device_setparams()\n");
*/


//------ Opend device for MIC_CAPTURE  beware of if...if...if...if...expressions
if (!device_open(&pcm_handle,"default",SND_PCM_STREAM_CAPTURE)){
	ret=4;
	goto OPEN_PLAY_RECORD_DEVICE_ERR;
   }
//---- set param for MIC_CAPTURE device-----
if(!device_setparams(pcm_handle,chanl_val,SAMPLE_RATE,1)){
	ret=5;
	goto SET_PLAYBACK_PARAMS_ERR;
	}
 printf("Finish setting params for MIC_CAPTURE !\n");


//---------- calcuate mem. buffer size for RAW and MP3 ----------------------
//---------- The values of rate_val,chanl_val and bit_per_sample are set in device_setparams() function 
//---- rate_val=CAPTURE_RATE and chanl_val are predetermined, bit_per_sample derived from snd_pcm_hw_params_set_format( pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE)
// if wave_buf and mp3_buf both empty,then re-calculate mem. length, otherwise skip.

bit_per_sample=16;

if(wave_buf == NULL){
	 printf("rate_val=%d, chanl_val=%d, bit_per_sample=%d\n",rate_val,chanl_val,bit_per_sample);
	 wave_buf_len=MAX_RECORD_TIME*SAMPLE_RATE*bit_per_sample*chanl_val/8;
  }
if(mp3_buf == NULL){  //--assume half length of wave_buf 
	mp3_buf_len=(wave_buf_len>>1);
  }
//----- test only -----
if(wave_buf == NULL)printf("wave_buf=NULL\n");
if(wave_buf2 == NULL)printf("wave_buf2=NULL\n");
if(mp3_buf == NULL)printf("mp3_buf=NULL\n");


 //-----checking voice wave amplitude, and start to record if it exceeds preset threshold value,or it will loop checking ...
 if(!device_check_voice(pcm_handle_loopback)){  //--device_check_voice() has a loop inside, it will jump out and return -1 only if there is an error.
	goto LOOPEND;
 }

//-------- allocate mem for wave_buf and mp3_buf -------------
// ------- if mem. have already been allocated, then skip ----
if(wave_buf == NULL)
	wave_buf=(unsigned char *)malloc(wave_buf_len); 
if(wave_buf2 == NULL)
	wave_buf2=(unsigned char *)malloc(wave_buf_len); 
if(SAVE_MP3_FILE){
	if(mp3_buf == NULL)
		mp3_buf=(unsigned char *)malloc(mp3_buf_len); 
   }


printf("start recording...\n");
if (!device_capture(pcm_handle_loopback,pcm_handle)){
	ret=3;
	goto DEVICE_CAPTURE_ERR;
}

snd_pcm_close( pcm_handle_loopback ); 
printf("LOOPBACK record finish!\n");
snd_pcm_close( pcm_handle);
printf("MICPHONE record finish!\n");

//------------save to file
timep=time(NULL);// get CUT time,seconds from Epoch, long type indeed
p_tm=localtime(&timep);// convert to local time in struct tm
strftime(str_time,sizeof(str_time),"%Y-%m-%d-%H:%M:%S",p_tm);
printf("record at: %s\n",str_time);

//----------------- save to RAW and MP3 files ------------------
if(wave_buf_used >= (MIN_SAVE_TIME*rate_val*bit_per_sample*chanl_val/8)) // save to file only if recording time is great than 20s.
{
	//----- save to raw file ------------
	if(SAVE_RAW_FILE){
		sprintf(str_file,"/tmp/%s.raw",str_time);
		fd=open(str_file,O_WRONLY|O_CREAT|O_TRUNC);
		rc=write(fd,wave_buf,wave_buf_used);
		printf("write to %s  %d bytes\n",str_file,rc);
		close(fd); //though kernel will close it automatically
	 }

	// ----------  save to wav file ------------
	if(SAVE_WAVE_FILE){
		sprintf(str_file,"/tmp/%s.wav",str_time);
		simplest_pcm16le_to_wave(wave_buf,str_file,chanl_val,SAMPLE_RATE, wave_buf_used);
		printf("save to %s \n",str_file);
	 }

	//------- save to mp3 file   ---------
	if(SAVE_MP3_FILE){
		sprintf(str_file,"/tmp/%s.mp3",str_time);
		fmp3=fopen(str_file,"wb");
		if(fmp3 != NULL){
			printf("Succeed to open file for saving mp3!\n");
			rc=fwrite(mp3_buf,mp3_buf_used,1,fmp3);
			printf("write to %s  %d bytes\n",str_file,rc*nb);
		 }
		else
			printf("fail to open file for saving mp3!\n");
		fclose(fmp3);
	 }
}


//------ Opend device for PLAYBACK  beware of if...if...if...if...expressions
if (!device_open(&pcm_handle_play,str_play_device,SND_PCM_STREAM_PLAYBACK)){
	ret=4;
	goto OPEN_PLAY_RECORD_DEVICE_ERR;
   }
//---- set param for PLAYBACK device-----
if(!device_setparams(pcm_handle_play,chanl_val,SAMPLE_RATE,1)){
	ret=5;
	goto SET_PLAYBACK_PARAMS_ERR;
	}
 printf("Finish setting params for PLAYBACK !\n");


printf("start playback...\n");
if (!device_play(pcm_handle_play)){
	ret=6;
	goto DEVICE_PLAYBACK_ERR; //... contiue to loop
}

//if (!device_play()) goto LOOPEND;
printf("Finish playback!\n");

//snd_pcm_drain( pcm_handle_play );//PALYBACK pcm_handle!!  to allow any pending sound samples to be transferred.



LOOPEND:
	//------------------------- pcm hanle  ------------------------
	snd_pcm_close( pcm_handle_play );//CAPTURE or PLAYBACK pcm_handle!!
	//printf("-----PLAY: snd_pcm_close()  ----\n");
	wave_buf_used=0;
/*-----------  no need to free and re-allocate mem. every loop -------------
	if(wave_buf != NULL){   //--dynamically allocated every loop, need to free and re_allocate at new loop.
		free(wave_buf); //--wave_buf mem. to be allocated in device_capture() and played in device_play();
		wave_buf=NULL; } //-- remeber to the pointer to NULL after free
	//------------------------- clear lame  ------------------------
	//lame_close(lame); Need not to exit every loop.
	if(mp3_buf != NULL){   //--dynam
		free(mp3_buf);
		mp3_buf=NULL; }
*/
	continue;

OPEN_STREAM_CAPTURE_ERR: //ret=1
	printf("Open PCM stream CAPTURE error!\n");
	return ret;
SET_LOOPBACK_DEVICE_PARAMS_ERR: //ret=2
	printf("Set LOOPBACK device parameters error!\n");
	return ret;
DEVICE_CAPTURE_ERR: //ret=3
	printf("Set CAPTURE parameters error!\n");
	return ret;
OPEN_PLAY_RECORD_DEVICE_ERR: //ret=4
	printf("Open PCM stream PLAYBACK error!\n");
	return ret;
SET_PLAYBACK_PARAMS_ERR: //ret=5
	printf("Set PLAYBACK parameters error!\n");
	return ret;
DEVICE_PLAYBACK_ERR: //ret=6
	printf("device_play() error! start a new loop...\n");
	goto LOOPEND;

}//while()

return ret;


}


//首先让我们封装一个打开音频设备的函数：
//snd_pcm_t *pcm_handle;

bool device_open(snd_pcm_t** pcm_handle,char* device,int mode){
if(snd_pcm_open (pcm_handle,device,mode,0) < 0)
 {
	printf("snd_pcm_open() fail!\n");
	return false; 
 }
 else printf("snd_pcm_open() succeed!\n");

//------ set as non_block mode -------
//---In nonblock mode, readi() writei() will return a negative if device unavailable.
/*
if(snd_pcm_nonblock(pcm_handle,NON_BLOCK)<0){
	printf("snd_pcm_nonblock_mode() fail!\n");
	return false;
  }
else
	printf("snd pcm set nonblock mode successfully!\n");
*/

return true;
}


//-------------------- set and prepare parameters  ------------------
bool device_setparams(snd_pcm_t* pcm_handle,int nchanl,int rate,int token_resample)
{
	if(snd_pcm_set_params(pcm_handle,
			SND_PCM_FORMAT_S16_LE, //formate
			SND_PCM_ACCESS_RW_INTERLEAVED,//SND_PCM_ACCESS_RW_INTERLEAVED, //access
			nchanl, //channels !!! if chanl_val=1, You must also set 'salve.channles 1' for Loopback in asound.conf
			rate,//rate
			token_resample,//0 -disallow, 1 -allow resampling !!!! Here you must disallow resampling, value of 'rate' will be passed to asound.conf as the input rate for rate-plugin, where final resample will be implemented.
			500000)<0){  //0.5s  required overall latency in us
	printf("fail to set params for pcm handle!\n");
	return false;
	}

	return true;
}


//------------------- record sound ------------------------------------//
//  pcm_handle_loopback:  pcm device handle to loopback
//  pcm_handle_mic: pcm device handle to micphone 
 bool device_capture(snd_pcm_t* pcm_handle_loopback, snd_pcm_t* pcm_handle_mic )
{
  int i;
  int r = 0;
  int r_mic=0;
  int rc_mp3=0;
  int total=0;
  int averg=0;
  unsigned char *data; //init. data=wave_buf, pointer to wave_buf position, of which all raw sound data will be stored.
  unsigned char *mic_data;// init. mic_data=wave_buf2..
  unsigned char *p_mp3_buf; //init. p_mp3_buf=mp3_buf, pointer to mp3_buf current write/read position
  int16_t *pv; //pointer to current data 

  int16_t *ppbuff=sh_pcm_buff; //pass ** to shine encoder 
  unsigned char* sh_data_out=NULL; // pointer to encoded mp3 data from shine
  int  sh_count=0;
  int sh_chunk_size;//---frames,data chunk for each session of MP3 encoding.
  int sh_chunk_byte; //---bytes of sh_chunk_size

  if(!SAVE_MP3_FILE) //--While shine is not initialized, chanl_samples_per_pass=0, so init with a value here. !!!!! 
	chanl_samples_per_pass=128; // or SAMPLE_RATE/CHECK_FREQ

  sh_chunk_size= chanl_samples_per_pass*chanl_val; // data chunk needed by shine_encoder each encoding session
  sh_chunk_byte= sh_chunk_size*bit_per_sample/8;

  //-------------- init pointer -----------
  //------ASSERT wave_buf -------
  if((wave_buf != NULL) && (wave_buf2 != NULL))
  {
	  data=wave_buf;
	  mic_data=wave_buf2;
    }
  else{
	 printf("wave_buf or wave_buf2 is NULL!\n");
	 return false;
   }
 //-------- ASSERT mp3_buf --------
 if(SAVE_MP3_FILE){
	 if(mp3_buf != NULL){
  		  p_mp3_buf=mp3_buf;  
	    }
	  else{
		 printf("mp3_buf is NULL!\n");
		 return false;
	   }
  }
  
  //------------------  get start time ------------------
  gettimeofday(&t_start,NULL);
  printf("Start Time: %lds + %ldus \n",t_start.tv_sec,t_start.tv_usec);

  //printf("data=%d; wave_buf=%d; wave_buf_len=%d; sh_chunk_byte=%d\n",data,wave_buf,wave_buf_len,sh_chunk_byte);
  while ((data-wave_buf) <= (wave_buf_len-sh_chunk_byte) ){ //chunk_size*bit_per_sample*chanl_val)){
	r = snd_pcm_readi(pcm_handle_loopback,data,sh_chunk_size);  //chunk_size: frames to be read
	r_mic=snd_pcm_readi(pcm_handle_mic,mic_data,sh_chunk_size);
	//---------- In nonblock mode,it will return a negativer. -----------
	if(r == -EPIPE || r_mic == -EPIPE){
		/* EPIPE means overrun */
		fprintf(stderr,"overrun occurred! try to recover...\n");
		snd_pcm_prepare(pcm_handle_loopback);//try to recover.  to put the stream in PREPARED state so it can start again next time.
		snd_pcm_prepare(pcm_handle_mic);
		continue;
	   }

	if (r <0 || r_mic <0){
		fprintf(stderr,"error from read:%s\n",snd_strerror(r));
		continue; //--whatever, let's continue.
 	  }

 	 //---------------- to proceed data  ------------------
	 if (r!=sh_chunk_size){  // !!!!!WARNING!!!!! short read may cause trouble!!! Give a caution only and let's keep on !!!
		fprintf(stderr,"LOOPBACK:Read-end or short read ocurrs, read %d of %d frames \n",r,sh_chunk_size);
		//continue;//discard it anyway,let's continue.
		memset(data+r,0,(sh_chunk_size-r)*sizeof(int16_t)); //pad remaining chunk space with 0.
		r=sh_chunk_size;
	 }
	 if (r_mic!=sh_chunk_size){  // !!!!!WARNING!!!!! short read may cause trouble!!! Give a caution only and let's keep on !!!
		fprintf(stderr,"Micphone:Read-end or short read ocurrs, read %d of %d frames \n",r,sh_chunk_size);
		//continue;//discard it anyway,let's continue.
		memset(mic_data+r_mic,0,(sh_chunk_size-r_mic)*sizeof(int16_t)); //pad remaining chunk space with 0.
		r_mic=sh_chunk_size;
	 }


	 if ( r==sh_chunk_size && r_mic==sh_chunk_size ) {
		//------------ filter the raw sound -----------------
		// !!!! WARNING !!!! FOR ONE CHANNEL ONLY, interleaved frame data not valid for filter operation !!!!!!!!!!

		if(IIR_FILTER_ON) //--for MICPHONE record only
			//IIR_freq_trapper((int16_t *)data, (int16_t *)data, r, 10, SAMPLE_RATE);//10Hz trapper 
			IIR_freq_trapper((int16_t *)data, (int16_t *)data, r_mic, 50, SAMPLE_RATE);//50Hz trapper 
			IIR_bandpass_filter((int16_t *)data,(int16_t *)data,r_mic); // r=frames(one channel,mono),1 frame =16bits,
			//----- IIR_filter(int16_t *p_in_data, int16_t *p_out_data, int count)

		//------------  encode raw sound to mp3_buffer  ---------------

		if(SAVE_MP3_FILE){ //for LOOPBACK only
			//---unsigned char* shine_encode_buffer(shine_t s, int16_t **data, int *written);
			//---unsigned char* shine_encode_buffer_interleaved(shinet_t s, int16_t *data, int *written);
			//---- ONLY 16bit depth sample is accepted by shine_encoder
			sh_data_out=shine_encode_buffer(sh_shine,(int16_t **)(&data),&sh_count);//--chanl_samples_per_pass*chanl_val samples encoded
			memcpy(p_mp3_buf,sh_data_out,sh_count); //--copy to mp3_buf for final MP3 file
			p_mp3_buf+=sh_count; //--shift pointer to current position in mp3_buffer accordingly
		}

		//------------ adjust indicating pointer --------------------------
		pv=(int16_t *)data; //--get pointer to current chunk data, will be used to calculate average value .
		data += sh_chunk_byte;//--move current buffer position pointer, short run is NOT considered!!!
		mic_data += sh_chunk_byte;

		//------------ checker timer, return when DELAY_TIME used up ----------------
		gettimeofday(&t_end,NULL);
		cost_times=t_end.tv_sec-t_start.tv_sec;

		if(cost_times >= DELAY_TIME){ //------ end recording
			wave_buf_used=data-wave_buf; //--cal meaningful wave_buf length
			if(SAVE_MP3_FILE){
				mp3_buf_used=p_mp3_buf-mp3_buf; //--cal meaningful mp3_buf length

			 }
			return true;
		 }
      		//----------- !!!!! check sound wave amplitude !!!!-----------------
		averg=0;total=0;
		for(i=0;i<SAMPLE_RATE/CHECK_FREQ;i++){  //r -- equals to chunk_size,16bits each frame, only if no shor-run occurs!
			total+=abs(*pv); // !!!!!!
			pv+=1;
		  }
		//printf("total=%d\n",total);
		//averg=(total>>CN);
		 averg=(total/(SAMPLE_RATE/CHECK_FREQ));
		//printf("averg=%d\n",averg);
		if(averg >= KEEP_AVERG){
			  gettimeofday(&t_start,NULL); // reset timer, add one more DELAY_TIME for recording.
	 		//  printf("averg=%d\n",averg);
		   	// printf("loud noise sensed!\n");
		 }

	} // if(r>0) finish

     } // end of while() ------------- all wave_buf used up, end recording    --------------------
	printf("MAX_RECORD_TIME %d is run out,end recording.\n",MAX_RECORD_TIME);
	wave_buf_used=data-wave_buf; //--short run is not considered!!!
	if(SAVE_MP3_FILE){
		mp3_buf_used=p_mp3_buf-mp3_buf; //--cal meaningful mp3_buf length
/*
		//------------ flush and encode remaind PCM buffer to mp3 ---------
		rc_mp3=lame_encode_flush(lame,mp3_chunk,MP3_CHUNK_SIZE);
		memcpy(p_mp3_buf,mp3_chunk,rc_mp3); //--copy to mp3_buf for final file
*/
	 }
 return true;

}

//形参dtime用来确定录音时间，根据录音时间分配数据空间，再调用snd_pcm_readi从音频设备读取音频数据，存放到wave_buf中。
//同样的原理，我们再添加一个播放函数，向音频设备写入数据：

bool device_play(snd_pcm_t* pcm_handle)
{
  unsigned char *data = wave_buf2;
  int r = 0;
  chunk_size=32; //beware of the type
  chunk_byte=chunk_size*bit_per_sample*chanl_val/8;
  while ( (data-wave_buf2) <= (wave_buf_used-chunk_byte)){
  r = snd_pcm_writei( pcm_handle, data , chunk_size); //chunk_size = frames
  if(r == -EAGAIN)continue;
  if(r < 0){
	printf("write error: %s\n",snd_strerror(r));
	//exit(EXIT_FAILURE); //ocassionally, it will exit here!
	return false;
   }
  //printf("----- writei()  r=%d -----\n ",r);
  if ( r>0 ) data += chunk_byte;
  else
  return false;
  }
  return true;

}


bool device_check_voice(snd_pcm_t* pcm_handle)
{
 int i;
 int r = 0;
 int count=0;
 int total=0;
 int averg=0;//average of sample values in one chunk.
 int CN=5;
 chunk_size= SAMPLE_RATE/CHECK_FREQ; //--how many frames to be checked for specified CHECK_FREQ,one channel
 //chunk_size=(2<<CN); //--frames each time
 chunk_byte=chunk_size*bit_per_sample*chanl_val/8; //---bytes
 int16_t *buf=(int16_t *)malloc(chunk_byte); //--sample width 16bits
 int16_t *data=buf;

 printf("listening and checking any voice......\n");
 while(1)
  {
	r = snd_pcm_readi( pcm_handle,(char *)buf,chunk_size);  //chunk_size*bit_per_sample*read interleaved rames from a PCM
        if(r == -EAGAIN)continue;
	if ( r>=0 ) {
		    //printf(" r= %d \n ",r);
		    data=buf;
		    averg=0;total=0;
		    for(i=0;i<r;i++){
			total+=abs(*data); // !!!!!!
			data+=1;
		       }
		    //printf("total=%d\n",total);
		    //averg=(total>>CN);
		    averg=(total/chunk_size);
		    //printf("averg=%d\n",averg);
		    if(averg >= CHECK_AVERG){
			    printf("loud noise sensed!  averg =%d  chunk_size=%d\n",averg,chunk_size);
			    free(buf);
			    return true;
			}

//		usleep(20000); //
//		snd_pcm_prepare(pcm_handle);
		//usleep(10000); //---you cann't sleep here,
	}//if
	else
	{
	        printf(" check voice readi() erro!  r= %d \n ",r);
		free(buf);
		return false;
	}
   } //while(1)

free(buf);
return true;

}


int init_shine_mono(int samplerate,int bitrate)//--input and  output sample rate is the same.
{

shine_set_config_mpeg_defaults(&(sh_config.mpeg));//--init sh_config struct
sh_config.wave.channels=1;// PCM_STEREO=2,PCM_MONO=1
sh_config.wave.samplerate=samplerate;
sh_config.mpeg.copyright=1;
sh_config.mpeg.original=1;
sh_config.mpeg.mode=3;//STEREO=0,MONO=3
sh_config.mpeg.bitr=bitrate;

if(shine_check_config(sh_config.wave.samplerate,sh_config.mpeg.bitr)<0)
	printf("Unsupported sample rate and bit rate configuration.\n");
sh_shine=shine_initialise(&sh_config);
if(sh_shine == NULL){
	printf("Fail to initialize Shine!\n");
	return -1;
 }

chanl_samples_per_pass=shine_samples_per_pass(sh_shine); //numbers of samples per channel, to feed to the encoder
samples_per_pass=chanl_samples_per_pass*1; //---for MONO, 1 channel
printf("----- MONO samples_per_pass=%d  -----\n",samples_per_pass);

return 0;
}
