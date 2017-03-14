/*--------------------------------------------------
ALSA auto. record and play test
Quote from: http://blog.csdn.net/ljclx1748/article/details/8606831

Usage: ./autorecord
It will monitor surrounding sound wave and trigger 10s recording if loud voice is sensed,
then it will playback. The sound will also be saved to a raw file.

1. use alsamixer to adjust Capture and ADC PCM value
	ADC PCM 0-255 (240)
	Capture 0-63  (53)
2. some explanation:
	sample: usually 8bits or 16bits, one sample data width.
	channel: 1-Mono. 2-Stereo
	frame: sizeof(one sample)*channels
	rate: frames per second
	period: Max. frame numbers hard ware can be handled each time.
	chunk: frames receive from/send to hard ware each time.
	buffer: N*periods
	interleaved mode:record period data frame by frame, such as  frame1(Left sample,Right sample),frame2(), ......
	uninterleaved mode: record period data channel by channel, such as period(Left sample,Left ,left...),period(right,right...),period()...
3. lib: lasound 
--------------------------------------------------*/

#include <asoundlib.h>
#include <stdbool.h>
#define CHECK_AVERG 2500 //--threshold value of wave amplitude

snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;
snd_pcm_format_t format_val;
char *wave_buf; //---pointer to wave buffer
int wave_buf_len; //---wave buffer length in bytes
int bit_per_sample;
snd_pcm_uframes_t frames;
snd_pcm_uframes_t period_size; //length of period (in frames)
snd_pcm_uframes_t chunk_size=32;//numbers of frames read/write to hard ware each time
int chunk_byte; //length of chunk (period)  (in bytes)
unsigned int chanl_val,rate_val;
int dir;


//------------------- functions declaration ----------------------
bool device_open( int mode);
bool device_setparams();
bool device_capture( int dtime );
bool device_play();
bool device_check_voice();

/*========================= MAIN ====================================*/
int main(int argc,char* argv[]){
int fd;
int rc;

while(1)
{

//--------录音   beware of if...if...if...if...expressions
system("amixer set Capture 53");
system("amixer set 'ADC PCM' 240");

if (!device_open(SND_PCM_STREAM_CAPTURE ))return 1;
	 //printf("---device_open()\n");
if (!device_setparams(1,8000)) return 2;
	//printf("---device_setparams()\n");
if(!device_check_voice()) //--checking voice wave amplitude, and start to record if it exceeds preset threshold value,or it will loop checking ...
	continue;

printf("start recording...\n");
if (!device_capture( 10 ))return 3;
	//printf("-----device_capture()\n");
snd_pcm_close( pcm_handle ); 
	printf("record finish!\n");

fd=open("/tmp/record.raw",O_WRONLY|O_CREAT|O_TRUNC);
rc=write(fd,wave_buf,wave_buf_len);
printf("write to record.raw  %d bytes\n",rc);
close(fd); //though kernel will close it automatically

//--------播放
printf("start playback...\n");
if (!device_open(SND_PCM_STREAM_PLAYBACK)) return 4;
//printf("-----PLAY: device_open() finish\n");
if (!device_setparams(1,8000)) return 5;
//printf("-----PLAY: device_setarams() finish\n");
if (!device_play()) return 6;
printf("finish playback.\n\n\n");

snd_pcm_close( pcm_handle );
//printf("-----PLAY: snd_pcm_close()  ----\n");

free(wave_buf); //--wave_buf mem. to be allocated in device_capture() and played in device_play();

}//while()

return 0;

}


//首先让我们封装一个打开音频设备的函数：
//snd_pcm_t *pcm_handle;

bool device_open(int mode){
if(snd_pcm_open (&pcm_handle,"default",mode,0) < 0)
 {
	printf("snd_pcm_open() fail!\n");
	return false; 
 }
 	printf("snd_pcm_open() succeed!\n");
	return true;
}


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
//	printf("----snd_pcm_hw_get_period_size(): %d----\n",(int)period_size);
if(snd_pcm_hw_params_get_format(hw_params,&format_val) < 0)return false;
//	printf("----snd_pcm_hw_params_get_format()----\n");
bit_per_sample = snd_pcm_format_width((snd_pcm_format_t)format_val);
//printf("---bit_per_sample=%d  snd_pcm_format_width()----\n",bit_per_sample);
//获取样本长度
snd_pcm_hw_params_get_channels(hw_params,&chanl_val);
//printf("----snd_pcm_hw_params_get_channels %d---\n",chanl_val);
chunk_byte = period_size*bit_per_sample*chanl_val/8;
//chunk_size = frames;//period_size; //frames
//计算周期长度（字节数(bytes) = 每周期的桢数 * 样本长度(bit) * 通道数 / 8 ）

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



 bool device_capture( int dtime ){
  snd_pcm_hw_params_get_rate(params,&rate_val,&dir); //params=hw_params
  wave_buf_len=dtime*rate_val*bit_per_sample*chanl_val/8;
  //printf(" wave_buf_len=%d \n",wave_buf_len);
   //计算音频数据长度（秒数 * 采样率 * 桢长度）
  char *data=wave_buf=(char*)malloc(wave_buf_len);
  int r = 0;
  chunk_size=32;
  chunk_byte=chunk_size*bit_per_sample*chanl_val/8;
  //printf("chunk_byte=%d\n",chunk_byte);
  while ( (data-wave_buf) <= (wave_buf_len-chunk_byte) ){ //chunk_size*bit_per_sample*chanl_val)){
	r = snd_pcm_readi( pcm_handle,data,chunk_size);  //chunk_size*bit_per_sample*read interleaved rames from a PCM
//--- snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm,void* buffer, snd_pcm_uframes_t size)
//--- pcm -PCM handle    buffer-frames containing buffer  size - frames to be read
	if ( r>0 ) {
//		printf(" r= %d\n",r);
//		printf("data=%d  wave_buf=%d\n",data,wave_buf);
		data += chunk_byte;//--move current buffer position pointer
	}
	else
		return false;
}

return true;

}

//形参dtime用来确定录音时间，根据录音时间分配数据空间，再调用snd_pcm_readi从音频设备读取音频数据，存放到wave_buf中。
//同样的原理，我们再添加一个播放函数，向音频设备写入数据：

bool device_play(){
  char *data = wave_buf;
  int r = 0;
  chunk_size=32;
  chunk_byte=chunk_size*bit_per_sample*chanl_val/8;
  while ( (data-wave_buf) <= (wave_buf_len-chunk_byte)){
  r = snd_pcm_writei( pcm_handle, data , chunk_size); //chunk_size = frames
  if(r == -EAGAIN)continue;
  if(r < 0){
	printf("wirte error: %s\n",snd_strerror(r));
	exit(EXIT_FAILURE);
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
 chunk_size=32; //--frames each time
 chunk_byte=chunk_size*bit_per_sample*chanl_val/8; //---bytes
 //printf("chunk_byte=%d\n",chunk_byte);
 int16_t *buf=(int16_t *)malloc(chunk_byte); //32bits
 //printf("size of int16_t = %d\n",sizeof(int16_t));
 int16_t *data=buf;
 //char* tmp;

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
			total+=abs((*data)); // !!!!!!
			data+=1;
		       }
		    //printf("total=%d\n",total);
		    averg=(total>>5);
		    //printf("averg=%d\n",averg);
		    if(averg >= CHECK_AVERG){
			    printf("loud noise sensed!\n");
			    free(buf);
			    return true;
			}

//		usleep(20000); //
//		snd_pcm_prepare(pcm_handle);
		//usleep(10000); //---you cann't sleep here,
	}//if
	else
	{
	        printf(" r= %d \n ",r);
		free(buf);
		return false;
	}
   } //while(1)

free(buf);
return true;

}
