/*--------------------------------------------------
ALSA auto. record and play test
Quote from: http://blog.csdn.net/ljclx1748/article/details/8606831

 !!!---- Since mt7688 has no FPU,mp3lame encoding will cost most of CPU processing time ---!!!
 !!! Sample rate=8k is OK, while sample rate=48k will make it too sensitive !!!

Usage: ./autorecord
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
3. lib: lasound lmp3lame
4. IIR filter will produce a sharp noise when input sound is too lound!


Make for Widora-neo
midas-zhou
--------------------------------------------------*/
#include <stdio.h>
#include <getopt.h>
#include <asoundlib.h>
#include <stdbool.h>
#include "lame.h"


//----- for PCM record 
#define NON_BLOCK 0 // 1 - ture ,0 -false  !!! non_block mode not good !!!
#define CHECK_FREQ 125 //-- use average energy in 1/CHECK_FREQ (s) to indicate noise level
#define SAMPLE_RATE 8000 //--4k also OK
#define CHECK_AVERG 1800 //--threshold value of wave amplitude to trigger record
#define KEEP_AVERG 1500 //--threshold value of wave amplitude for keeping recording
#define DELAY_TIME 3 //seconds -- recording time after one trigger
#define MAX_RECORD_TIME 60 //seconds --max. record time in seconds
#define MIN_SAVE_TIME 15 //seconds --min. recording time for saving, short time recording will be discarded.
bool SAVE_RAW_FILE=false; // save raw file or not(default)
bool IIR_FILTER_ON=false; // enable IIR filter or not
//------ for MP3
#define MP3_CHUNK_SIZE 1024 //bytes, chunk buffer size for lame_encode_buffer(), to be big enough!! at least 128?
#define MP3_SAMPLE_RATE 8000 // sample rate for output mp3 file,  MIN.8K for lame
bool SAVE_MP3_FILE=false; //save mp3 file or not(default)

lame_t lame;
FILE *fmp3; // file for mp3 output =fopen("record.mp","wb");
unsigned char mp3_chunk[MP3_CHUNK_SIZE]; // for chunk mp3 encode buffer
unsigned char *mp3_buf=NULL; //---- pointer to final mp3 data,
unsigned char *p_mp3_buf; //-- pointer to mp3_buf position,start from mp3_buffer
int mp3_buf_len; //=0.5*wave_buf_len ---mp3 buffer length in bytes, to be half of wave_buf_len



snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;
snd_pcm_format_t format_val;
char *wave_buf=NULL; //---pointer to wave buffer
int wave_buf_len; //---wave buffer length in bytes
int wave_buf_used=0; //---used wave buf length in bytes
int bit_per_sample;
snd_pcm_uframes_t frames;
snd_pcm_uframes_t period_size; //length of period (max. numbers of frames that hw can handle each time)
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
bool init_lame_mono(int Nchannel,int in_rate,int out_rate);
static int IIR_filter(int16_t *p_in_data, int16_t *p_out_data, int count);

bool device_open(int mode);
bool device_setparams(int nchanl,int rate);
bool device_capture();
bool device_play();
bool device_check_voice();

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
int ret=0;
char str_file[50]={0}; //---directory of save_file 
chanl_val=1; // 1 channel


//--------- parse options --------
while((opt=getopt_long(argc,argv,"rmfh",longopts,NULL)) !=-1){
	switch(opt){
		case 'r':
			SAVE_RAW_FILE = true;
			break;
		case 'm':
			SAVE_MP3_FILE = true;
			break;
		case 'f':
			IIR_FILTER_ON = true;
			break;
		case 'h':
			printf("Program usage:\n");
			printf("   The programe will automatically start recording if lound sound is detected.\n");
			printf("   It will continue recording if there is voice detected within every 3 seconds.\n");
			printf("   Max. recording time is 60s, it will stop automatically then.\n");
			printf("   The data may be saved only if recoding time exceeds 30s in one session.\n");
			printf("   It will playback recoding data in memory after each session.\n\n");
			printf("Use -r (raw) to save a RAW file in /tmp, with time stamp as its name.\n");
			printf("Use -m (mp3) to save a mp3 file in /tmp, with time stamp as its name.\n");
			printf("Use -f (filter) to enable IIR filter.\n");
			printf("Use combined options such as: autorecord -rmf \n\n");

			return -1;
		case '?':
			printf("Unkown option!\n");
			return -1;
			
	}
}



//-------- set recording volume -------
system("amixer set Capture 57");
system("amixer set 'ADC PCM' 246"); // adjust sensitivity, or your can use alsamxier to adjust in realtime.

//-------- INIT LAME ----------
if(SAVE_MP3_FILE)
	init_lame_mono(chanl_val,SAMPLE_RATE,MP3_SAMPLE_RATE);


while(1)
{
//------- for test -----------
printf("capture sample rate:%d, MP3 sample rate:%d\n",SAMPLE_RATE,MP3_SAMPLE_RATE);


//-------- INIT LAME ----------
// init_lame_mono(chanl_val,SAMPLE_RATE,MP3_SAMPLE_RATE);


//--------录音   beware of if...if...if...if...expressions
if (!device_open(SND_PCM_STREAM_CAPTURE )){
	ret=1;
	goto OPEN_STREAM_CAPTURE_ERR;
   }
//printf("---device_open()\n");
if (!device_setparams(chanl_val,SAMPLE_RATE)){
	ret=2;
	goto SET_CAPTURE_PARAMS_ERR;
  }
//printf("---device_setparams()\n");

//---------- calcuate mem. buffer size for RAW and MP3 ----------------------
//---------- The values of rate_val,chanl_val and bit_per_sample are set in device_setparams() function 
//---- rate_val=CAPTURE_RATE and chanl_val are predetermined, bit_per_sample derived from snd_pcm_hw_params_set_format( pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE)
// if wave_buf and mp3_buf both empty,then re-calculate mem. length, otherwise skip.
if(wave_buf == NULL){
	 printf("rate_val=%d, chanl_val=%d, bit_per_sample=%d\n",rate_val,chanl_val,bit_per_sample);
	 wave_buf_len=MAX_RECORD_TIME*rate_val*bit_per_sample*chanl_val/8;
  }
if(mp3_buf == NULL){
	mp3_buf_len=(wave_buf_len>>1);
  }
//----- test only -----
if(wave_buf == NULL)printf("wave_buf=NULL\n");
if(mp3_buf == NULL)printf("mp3_buf=NULL\n");


 //-----checking voice wave amplitude, and start to record if it exceeds preset threshold value,or it will loop checking ...
 if(!device_check_voice()){  //--device_check_voice() has a loop inside, it will jump out and return -1 only if there is an error.
	goto LOOPEND;
 }

//-------- allocate mem for wave_buf and mp3_buf -------------
// ------- if mem. have already been allocated, then skip ----
if(wave_buf == NULL)
	wave_buf=(char *)malloc(wave_buf_len); 
if(mp3_buf == NULL)
	mp3_buf=(unsigned char *)malloc(mp3_buf_len); 


printf("start recording...\n");
if (!device_capture()){
	ret=3;
	goto DEVICE_CAPTURE_ERR;
}
	//printf("-----device_capture()\n");
snd_pcm_close( pcm_handle ); 
	printf("record finish!\n");


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

	//------- save to mp3 file   ---------
	if(SAVE_MP3_FILE){
		sprintf(str_file,"/tmp/%s.mp3",str_time);
		fmp3=fopen(str_file,"wb");
		if(fmp3 != NULL){
			printf("Succeed to open file for saving mp3!\n");
			nb=p_mp3_buf-mp3_buf; //how many bytes to write
			rc=fwrite(mp3_buf,nb,1,fmp3);
			printf("write to %s  %d bytes\n",str_file,rc*nb);
		 }
		else
			printf("fail to open file for saving mp3!\n");
		fclose(fmp3);
	 }
}


//--------播放
if (!device_open(SND_PCM_STREAM_PLAYBACK)){
	ret=4;
	goto OPEN_STREAM_PLAYBACK_ERR;
    }
//printf("-----PLAY: device_open() finish\n");
if (!device_setparams(1,SAMPLE_RATE)){
	ret=5;
	goto SET_PLAYBACK_PARAMS_ERR;
    } 
//printf("-----PLAY: device_setarams() finish\n");
printf("start playback...\n");
if (!device_play()){
	ret=6;
	goto DEVICE_PLAYBACK_ERR; //... contiue to loop
}
//if (!device_play()) goto LOOPEND;

printf("finish playback.\n\n\n");
//snd_pcm_drain( pcm_handle );//PALYBACK pcm_handle!!  to allow any pending sound samples to be transferred.


LOOPEND:
	//------------------------- pcm hanle  ------------------------
	snd_pcm_close( pcm_handle );//CAPTURE or PLAYBACK pcm_handle!!
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

OPEN_STREAM_CAPTURE_ERR:
	printf("Open PCM stream CAPTURE error!\n");
	return ret;
SET_CAPTURE_PARAMS_ERR:
	printf("Set CAPTURE parameters error!\n");
	return ret;
DEVICE_CAPTURE_ERR:
	printf("Set CAPTURE parameters error!\n");
	return ret;
OPEN_STREAM_PLAYBACK_ERR:
	printf("Open PCM stream PLAYBACK error!\n");
	return ret;
SET_PLAYBACK_PARAMS_ERR:
	printf("Set PLAYBACK parameters error!\n");
	return ret;
DEVICE_PLAYBACK_ERR:
	printf("device_play() error! start a new loop...\n");
	goto LOOPEND;

}//while()

lame_close(lame); //--close lame

return ret;


}


//首先让我们封装一个打开音频设备的函数：
//snd_pcm_t *pcm_handle;

bool device_open(int mode){
if(snd_pcm_open (&pcm_handle,"default",mode,0) < 0)
 {
	printf("snd_pcm_open() fail!\n");
	return false; 
 }
 else printf("snd_pcm_open() succeed!\n");

//------ set as non_block mode -------
//---In nonblock mode, readi() writei() will return a negative if device unavailable.
if(snd_pcm_nonblock(pcm_handle,NON_BLOCK)<0){
	printf("snd_pcm_nonblock_mode() fail!\n");
	return false;
  }
else
	printf("snd pcm set nonblock mode successfully!\n");
return true;
}

/*-------------------- set and prepare parameters  ------------------*/
bool device_setparams(int nchanl,int rate)
 {
unsigned int val;
int dir;
int rc;
snd_pcm_hw_params_t *hw_params;

//------ beware of following if..if...if..if...if...expressions ----------
if(snd_pcm_hw_params_malloc (&hw_params) < 0)return false; //为参数变量分配空间
//	printf("---- snd_pcm_hw_params_malloc(&hw_params) ----\n");
if(snd_pcm_hw_params_malloc (&params) < 0)return false;
//	printf("----snd_pcm_hw_params_malloc(&params) ----\n");
if(snd_pcm_hw_params_any (pcm_handle, hw_params) < 0)return false; //参数初始化
//	printf("----snd_pcm_hw_params_any(pcm_handle,hw_params)----\n");
if(snd_pcm_hw_params_set_access (pcm_handle, hw_params,SND_PCM_ACCESS_RW_INTERLEAVED) < 0)return false; //设置为交错模式
//	printf("----snd_pcm_hw_params_set_access()----\n");
if(snd_pcm_hw_params_set_format( pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE) < 0)return false; //使用用16位样本
//	printf("----snd_pmc_hw_params_set_format()-----\n");
val=rate;//8000;
if(snd_pcm_hw_params_set_rate_near( pcm_handle, hw_params,&val,0) < 0)return false; //设置采样率
//	printf("----snd_pcm_hw_params_set_rate_near() val=%d----\n",val);
if(snd_pcm_hw_params_set_channels( pcm_handle, hw_params, nchanl) < 0)return false; //设置为立体声or Mono.
//	printf("----snd_pcm_hw_params_set_channels()-----\n");
frames=32;
if(snd_pcm_hw_params_set_period_size_near(pcm_handle,hw_params,&chunk_size,&dir) < 0 )return false;
//	printf("----snd_pcm_hw_params_set_period_size_near() chunk_size=%d----\n",chunk_size);
if(snd_pcm_hw_params_get_period_size( hw_params, &period_size,0) < 0)return false; //获取周期长度
	printf("----snd_pcm_hw_get_period_size(): %d frames----\n",(int)period_size);
if(snd_pcm_hw_params_get_format(hw_params,&format_val) < 0)return false;
//	printf("----snd_pcm_hw_params_get_format()----\n");
bit_per_sample = snd_pcm_format_width((snd_pcm_format_t)format_val);
//printf("---bit_per_sample=%d  snd_pcm_format_width()----\n",bit_per_sample);
//获取样本长度
snd_pcm_hw_params_get_channels(hw_params,&chanl_val);
//printf("----snd_pcm_hw_params_get_channels %d---\n",chanl_val);
chunk_byte = period_size*bit_per_sample*chanl_val/8; // this is the Max chunk byte size
//chunk_size = frames;//period_size; //frames
//计算周期长度（字节数(bytes) = 每周期的桢数 * 样本长度(bit) * 通道数 / 8 ）
snd_pcm_hw_params_get_rate(hw_params,&rate_val,&dir); 
snd_pcm_hw_params_get_channels(hw_params,&chanl_val);

rc=snd_pcm_hw_params( pcm_handle, hw_params); //设置参数
if(rc<0){
printf("unable to set hw parameters:%s\n",snd_strerror(rc));
exit(1);
}
printf("finish setting sound hw parameters\n");
params = hw_params; //保存参数，方便以后使用
snd_pcm_hw_params_free( hw_params); //释放参数变量空间
//printf("----snd_pcm_hw_params_free()----\n");
return true;

}
//这里先使用了Alsa提供的一系列snd_pcm_hw_params_set_函数为参数变量赋值。
//最后才通过snd_pcm_hw_params将参数传递给设备。
//需要说明的是正式的开发中需要处理参数设置失败的情况，这里仅做为示例程序而未作考虑。
//设置好参数后便可以开始录音了。录音过程实际上就是从音频设备中读取数据信息并保存。


//------------------- record sound ------------------------------------//
 bool device_capture( ){
  int i;
  int r = 0;
  int rc_mp3=0;
  int total=0;
  int averg=0;
  char *data; // pointer to wave_buf position, of which all raw sound data will be stored.
  int16_t *pv; //pointer to current data 

  chunk_size= SAMPLE_RATE/CHECK_FREQ; //--how many frames to be checked for specified CHECK_FREQ,one channel
  chunk_byte=chunk_size*bit_per_sample*chanl_val/8;
  //printf("chunk_byte=%d\n",chunk_byte);

  //-------------- init pointer -----------
  data=wave_buf;
  p_mp3_buf=mp3_buf;   
  
  //------------------  get start time ------------------
  gettimeofday(&t_start,NULL);
  printf("Start Time: %lds + %ldus \n",t_start.tv_sec,t_start.tv_usec);

  while ( (data-wave_buf) <= (wave_buf_len-chunk_byte) ){ //chunk_size*bit_per_sample*chanl_val)){
	r = snd_pcm_readi( pcm_handle,data,chunk_size);  //chunk_size*bit_per_sample*read interleaved frames from a PCM
	//---------- In nonblock mode,it will return a negativer. -----------
	if(r == -EPIPE){
		/* EPIPE means overrun */
		fprintf(stderr,"overrun occurred! try to recover...\n");
		snd_pcm_prepare(pcm_handle);//try to recover.  to put the stream in PREPARED state so it can start again next time.
		continue;
	   }
	if (r <0){
		fprintf(stderr,"error from read:%s\n",snd_strerror(r));
		continue; //--whatever, let's continue.
 	  }

 	 //---------------- to proceed data  ------------------
	 if (r!=chunk_size){  // short read may cause trouble!!! Give a caution only and let's keep on !!!
		fprintf(stderr,"short read ocurrs, read %d of %d frames \n",r,chunk_size);
	 }
	 if ( r>0 ) {
		//------------ filter the raw sound -----------------
		// !!!! WARNING !!!! FOR ONE CHANNEL ONLY, interleaved frame data not valid for filter operation !!!!!!!!!!
		if(IIR_FILTER_ON)
			IIR_filter((int16_t *)data,(int16_t *)data,r); // r=frames(one channel,mono),1 frame =16bits,
			//----- IIR_filter(int16_t *p_in_data, int16_t *p_out_data, int count)

		//------------  encode raw sound to mp3_buffer  ---------------
		if(SAVE_MP3_FILE){
			rc_mp3=lame_encode_buffer(lame,(short int*)data,NULL,r,mp3_chunk,MP3_CHUNK_SIZE);
			//---  data: PCM data for left channel, r---number of samples per channel, all for one channel 
			//--   p_mp2_buf: current position pointer in mp3_buf
 			//---  mp3_chunk is for encode buffer only, if data is insufficient for encoding,it will remain in mp3_chunk, thus rc_mp3 will be zero 
			//printf("rc_mp3=%d\n",rc_mp3);
			memcpy(p_mp3_buf,mp3_chunk,rc_mp3); //--copy to mp3_buf for final file
			p_mp3_buf+=rc_mp3; //--shift pointer in mp3_buffer accordingly
		}

		//------------ adjust indicating pointer --------------------------
		pv=(int16_t *)data; //--get pointer for chunk data, will be used to calculate average value .
		data += chunk_byte;//--move current buffer position pointer, short run is NOT considered!!!

		//------------ checker timer, return when DELAY_TIME used up ----------------
		gettimeofday(&t_end,NULL);
		cost_times=t_end.tv_sec-t_start.tv_sec;
		if(cost_times >= DELAY_TIME){ //------end roecording
			wave_buf_used=data-wave_buf;
			//------------ flush and encode remaind PCM buffer to mp3 ---------
			rc_mp3=lame_encode_flush(lame,mp3_chunk,MP3_CHUNK_SIZE);
			memcpy(p_mp3_buf,mp3_chunk,rc_mp3); //--copy to mp3_buf for final file

			return true;
		 }
      		//----------- check sound wave amplitude  -----------------
		averg=0;total=0;
		for(i=0;i<r;i++){  //r -- equals to chunk_size,16bits each frame, only if no shor-run occurs!
			total+=abs(*pv); // !!!!!!
			pv+=1;
		 }
		//printf("total=%d\n",total);
		//averg=(total>>CN);
		 averg=(total/chunk_size);
		//printf("averg=%d\n",averg);
		if(averg >= KEEP_AVERG){
			  gettimeofday(&t_start,NULL); // reset timer, add one more DELAY_TIME for recording.
	 		  printf("averg=%d\n",averg);
		   	  printf("loud noise sensed!\n");
		 }

	}
/*
	else  //if(r<0)
	{

 		wave_buf_used=data-wave_buf;
		return false;
	}
*/
     } // end of while() ------------- all wave_buf used up, end recording    --------------------
	printf("MAX_RECORD_TIME %d is run out,end recording.\n",MAX_RECORD_TIME);
	wave_buf_used=data-wave_buf; //--short run is not considered!!!
	//------------ flush and encode remaind PCM buffer to mp3 ---------
	rc_mp3=lame_encode_flush(lame,mp3_chunk,MP3_CHUNK_SIZE);
	memcpy(p_mp3_buf,mp3_chunk,rc_mp3); //--copy to mp3_buf for final file

 return true;

}

//形参dtime用来确定录音时间，根据录音时间分配数据空间，再调用snd_pcm_readi从音频设备读取音频数据，存放到wave_buf中。
//同样的原理，我们再添加一个播放函数，向音频设备写入数据：

bool device_play(){
  char *data = wave_buf;
  int r = 0;
  chunk_size=32;
  chunk_byte=chunk_size*bit_per_sample*chanl_val/8;
  while ( (data-wave_buf) <= (wave_buf_used-chunk_byte)){
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


bool device_check_voice(void )
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

/* ------- init lame and set parameters ------------- */
//   Nchannle: number of channels  
//   srate: sample rate of input or output file
bool init_lame_mono(int Nchannel,int in_rate,int out_rate)
{
 int tmp;
 if(!(lame=lame_init()))return false; //return NULL if fail
	printf("lame_init finish!\n");
 lame_set_in_samplerate(lame,in_rate); //--sample rate to be same as raw file
//	printf("lame_set_in_samplerate()=%d\n");
 lame_set_out_samplerate(lame,out_rate);
	printf("lame_set_out_samplerate() finish\n");
 lame_set_num_channels(lame,Nchannel);
	printf("lame_set_num_channels() finish\n");
 lame_set_brate(lame,11);//set brate compression ration,default 11
	printf("lame_set_brate() finish\n");
 lame_set_mode(lame,MONO); // 0-stereo 1=jstereo,2=dual channel(not supported),3=mono
	printf("lame_set_mode() finish\n");
 lame_set_quality(lame,5);//0~9 recommended 2=high 5=medium 7=low
	printf("lame_set_quality() finish\n");
 //lame_set_VBR(lame,vbr_default);//set Variable Bit Rate,  defualt use CBR
 //------!!! VBR file is bigger than CBR ......
 if(lame_init_params(lame) != 0)return false;
	printf("lame_init_params() finish\n");

 return true;
}



/*---------------------- IIR_FILTER -----------------------------------
4 order IIR filter Fl=300Hz,Fh=3400Hz,Fs=8kHz
p_in_data:   pointer to start of input data
p_out_data:  pointer to start of output data
count:       count of data number in unit of int16_t
!!!! p_ind_data and p_out_data maybe the same address !!!!!
----------------------------------------------------------------------*/
static int IIR_filter(int16_t *p_in_data, int16_t *p_out_data, int count)
{
int ret=0;
  //----factors for 4 order IIR filter Fl=300Hz,Fh=3400Hz,Fs=8kHz
static  double IIR_B[5]={ 0.6031972438993, 0, -1.206394487799, 0, 0.6031972438993 };
static  double IIR_A[5]={ 1,-0.325257157029, -1.004332872001, 0.1022259821442, 0.3705866844043 };
static  double w[5]={0.0, 0.0, 0.0, 0.0, 0.0};
int k;

for(k=0;k<count;k++)
 {
        w[0]=(*p_in_data)-IIR_A[1]*w[1]-IIR_A[2]*w[2]-IIR_A[3]*w[3]-IIR_A[4]*w[4];
        (*p_out_data)=IIR_B[0]*w[0]+IIR_B[1]*w[1]+IIR_B[2]*w[2]+IIR_B[3]*w[3]+IIR_B[4]*w[4];

        w[4]=w[3];
        w[3]=w[2];
        w[2]=w[1];
        w[1]=w[0];

        p_in_data++;
        p_out_data++;
 }

return ret;
}

