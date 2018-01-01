/*-------------------------------------------------------------
Based on: dranger.com/ffmpeg/tutorialxx.c
 				       ---  by Martin Bohme
	  muroa.org/?q=node/11
				       ---  by Martin Runge
	  www.xuebuyuan.com/1624253.html
				       ---  by niwenxian

NOTE:
1. A simpley example of opening a video file then decode frames and send RGB data to LCD for display.
2. Decode audio frames and playback.
3. DivX(DV50) is better than Xdiv. Especially in respect to decoded audio/video synchronization,DivX is nearly perfect.
4. fp=15 is OK for playing video files in a USB stick.
5. if you adjust baudrate to >2000000 in ft232.h, FP can reach 18. However the color will be distorted.
6. It depends on ffmpeg decoding speed, USB transfer speed and FT232 fanout(baudrate) speed.
   USB control is improtant.
7. Also notice the speed limit of your LCD controller, It's 500M bps for ILI9488.
   Adjust baudrate for FT232 accordingly.
   
The data chain is like this:
   FFmpge decoding speed ----> USB transfer speed ----> FT232 baudrate ----> ILI9488 Write Speed Limit ---> Display

Usage:
	ffplay3  video_file

Midas
---------------------------------------------------------------*/

#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include <stdio.h>
#include "include/ftdi.h"
#include "ft232.h"
#include "ILI9488.h"
#include "play_ffpcm.h"


#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48KHz 32bit audio


int get_costtime(struct timeval tm_start, struct timeval tm_end) {
	int time_cost;
	time_cost=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
	return time_cost;
}


int main(int argc, char *argv[]) {
	//Initializing these to NULL prevents segfaults!
	AVFormatContext	*pFormatCtx=NULL;
	int			i;
	int			videoStream;
	AVCodecContext		*pCodecCtxOrig=NULL;
	AVCodecContext		*pCodecCtx=NULL;
	AVCodec			*pCodec=NULL;
	AVFrame			*pFrame=NULL;
	AVFrame			*pFrameRGB=NULL;
	AVPacket		packet;
	int			frameFinished;
	int			numBytes;
	uint8_t			*buffer=NULL;
	struct SwsContext	*sws_ctx=NULL;

	int Hb,Vb,Hs,He,Vs,Ve;  //---for LCD image layout

	//------ for audio -------
	int			audioStream;
	AVCodecContext		*aCodecCtxOrig=NULL;
	AVCodecContext		*aCodecCtx=NULL;
	AVCodec			*aCodec=NULL;
	AVFrame			*pAudioFrame=NULL;
	int 			sample_rate;
	enum AVSampleFormat	sample_fmt;
	int			nb_channels;
	int			bytes_per_sample;
	int64_t			channel_layout;
	int 			bytes_used;
	int			got_frame;

	//------- time structe ------
	struct timeval tm_start, tm_end;


	//----- check input argc ----
	if(argc < 2) {
		printf("Please provide a movie file\n");
		return -1;
	}


//<<<<<<<<<<<<<      prepare FT232 and ILI9488    >>>>>>>>>>>>>>>>
	//----- initialize ft232
	if(usb_init_ft232()<0){
		printf("Fail to initialize ft232!\n");
		close_ft232();
		return -1;
	}
	//----- init ILI9488
	LCD_INIT_ILI9488();
        //----- adjust RGB order and layout here ------
        LCD_Write_Cmd(0x36); //memory data access control
	LCD_Write_Data(0x28); // oder: RGB.

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

	//----- Register all formats and codecs
	av_register_all();

	//-----Open video file
	printf("---- open video file...\n");
	if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0) {
		printf("Fail to open video file!\n");
		return -1; 
	}

	//----Retrieve stream information
	printf("---- retrieve stream information...\n");
	if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
		printf(" Fail to find stream information!\n");
		return -1;
	}

	//----Dump information about file onto standard error
	printf("----- try to dump file information ...\n");
	av_dump_format(pFormatCtx, 0, argv[1], 0);


	//-----Find the first video stream and audio stream
	printf("----- try to find the first video stream... \n");
	videoStream=-1;
	audioStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
		   videoStream < 0) {
			videoStream=i;
		}
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
		   audioStream < 0) {
			audioStream=i;
		}
	}

	if(videoStream == -1) {
		printf("Didn't find a video stream!\n");
		return -1;
	}
	if(audioStream == -1) {
		printf("Didn't find an audio stream!\n");
		return -1;
	}

	//-----Get a pointer to the codec context for the audio stream
	aCodecCtxOrig=pFormatCtx->streams[audioStream]->codec;
	//-----Find the decoder for the audio stream
	printf("----- try to find the decoder for the audio stream... \n");
	aCodec=avcodec_find_decoder(aCodecCtxOrig->codec_id);
	if(aCodec == NULL) {
		fprintf(stderr, "Unsupported audio codec!\n");
		return -1;
	}
	//----copy audio codec context
	aCodecCtx=avcodec_alloc_context3(aCodec);
	if(avcodec_copy_context(aCodecCtx, aCodecCtxOrig) != 0) {
		fprintf(stderr, "Couldn't copy audio code context!\n");
		return -1;
	}
	//----open audio codec
	if(avcodec_open2(aCodecCtx, aCodec, NULL) <0 ) {
		fprintf(stderr, "Cound not open audio codec!\n");
		return -1;
	}
	//----- get audio stream parameters -----------------------------
	sample_rate = aCodecCtx->sample_rate;
	sample_fmt  = aCodecCtx->sample_fmt;
	bytes_per_sample = av_get_bytes_per_sample(sample_fmt);
	nb_channels = aCodecCtx->channels;
	channel_layout = aCodecCtx->channel_layout;

	printf("	channel_layout=%lld\n",channel_layout);//long long int type
	printf("	nb_channels=%d\n",nb_channels);
	printf("	sample format: %s\n",av_get_sample_fmt_name(sample_fmt) );
	printf("	bytes_per_sample: %d\n",bytes_per_sample);
	printf("	sample_rate=%d\n",sample_rate);

	//----- open pcm play device and set parameters ----
 	prepare_ffpcm_device(nb_channels,sample_rate,false); //false for noninterleaved access

	//-----Get a pointer to the codec context for the video stream
	pCodecCtxOrig=pFormatCtx->streams[videoStream]->codec;
	//-----Find the decoder for the video stream
	printf("----- try to find the decoder for the video stream... \n");
	pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if(pCodec == NULL) {
		fprintf(stderr, "Unsupported video codec!\n");
		return -1;
	}
	//----copy video codec context
	pCodecCtx=avcodec_alloc_context3(pCodec);
	if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		fprintf(stderr, "Couldn't copy video code context!\n");
		return -1;
	}
	//----open video codec
	if(avcodec_open2(pCodecCtx, pCodec, NULL) <0 ) {
		fprintf(stderr, "Cound not open video codec!\n");
		return -1;
	}

	//----allocate frame for audio
	pAudioFrame=av_frame_alloc();
	//----Allocate video frame
	pFrame=av_frame_alloc();
	//----allocate an AVFrame structure
	printf("----- try to allocate an AVFrame structure...\n");
	pFrameRGB=av_frame_alloc();
	if(pFrameRGB==NULL) {
		fprintf(stderr, "Fail to allocate AVFrame structure!\n");
		return -1;
	}

	//----Determine required buffer size and allocate buffer
	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

//<<<<<<<<<<<<<     Hs He Vs Ve for IMAGE to LCD layout    >>>>>>>>>>>>>>>>
	 Hb=(PIC_MAX_WIDTH-pCodecCtx->width+1)/2;
	 Vb=(PIC_MAX_HEIGHT-pCodecCtx->height+1)/2;
	 Hs=Hb; He=Hb+pCodecCtx->width-1;
	 Vs=Vb; Ve=Vb+pCodecCtx->height-1;
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


	//----Assign appropriate parts of buffer to image planes in pFrameRGB
	//Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

	//----Initialize SWS context for software scaling
	printf("----- initialize SWS context for software scaling... \n");
	sws_ctx = sws_getContext( pCodecCtx->width,
				  pCodecCtx->height,
				  pCodecCtx->pix_fmt,
				  pCodecCtx->width,
				  pCodecCtx->height,
				  PIX_FMT_RGB24,
				  SWS_BILINEAR,
				  NULL,
				  NULL,
				  NULL
				);

//===========================     Read packets and process data     =============================
	printf("----- start loop of reading AV frames and decoding:\n");
	printf("	 converting video frame to RGB and then send to display...\n");
	printf("	 sending audio frame data to playback ... \n");
	i=0;
	while( av_read_frame(pFormatCtx, &packet) >= 0) {

	//----------------//////   process video stream   \\\\\\\-----------------
		if(packet.stream_index==videoStream) {
			//decode video frame
//			printf("...decoding video frame\n");
			gettimeofday(&tm_start,NULL);
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			gettimeofday(&tm_end,NULL);
			printf(" avcode_decode_video2() cost time: %d ms\n",get_costtime(tm_start,tm_end) );
			//did we get a complete video frame?
			if(frameFinished) {
				//convert the image from its native format to RGB
//				printf("...converting image to RGB\n");
				sws_scale( sws_ctx,
					   (uint8_t const * const *)pFrame->data,
					   pFrame->linesize, 0, pCodecCtx->height,
					   pFrameRGB->data, pFrameRGB->linesize
					);
				//<<<<<<<<<<<<<<<<<<<<<<<<    send data to LCD      >>>>>>>>>>>>>>>>>>>>>>>>
				gettimeofday(&tm_start,NULL);
				LCD_Write_Block(Hs,He,Vs,Ve,pFrameRGB->data[0],numBytes);
				gettimeofday(&tm_end,NULL);
				printf(" LCD_Write_Block() for one frame cost time: %d ms\n",get_costtime(tm_start,tm_end) );				//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

			}
		}//----- end  of vidoStream process  ------

	//----------------//////   process audio stream   \\\\\\\-----------------
		else if(packet.stream_index==audioStream) {
			//---bytes_used: indicates how many bytes of the data was consumed for decoding. when provided
			//with a self contained packet, it should be used completely.
			//---sb_size: hold the sample buffer size, on return the number of produced samples is stored.
			while(packet.size > 0) {
				gettimeofday(&tm_start,NULL);
				bytes_used=avcodec_decode_audio4(aCodecCtx, pAudioFrame, &got_frame, &packet);
				gettimeofday(&tm_end,NULL);
				printf(" avcode_decode_audio4() cost time: %d ms\n",get_costtime(tm_start,tm_end) );
				if(bytes_used<0)
				{
					printf(" Error while decoding audio!\n");
					//break;
					continue;
				}
				//----- if decoded data size >0
				if(got_frame)
				{
					gettimeofday(&tm_start,NULL);
					//---- playback audio data
					if(pAudioFrame->data[0] && pAudioFrame->data[1]) {
						// aCodecCtx->frame_size: Number of samples per channel in an audio frame
						 play_ffpcm_buff( (void **)pAudioFrame->data, aCodecCtx->frame_size);// 1 frame each time
					}
					else if(pAudioFrame->data[0]) {  //-- one channel only
						 play_ffpcm_buff( (void **)(&pAudioFrame->data[0]), aCodecCtx->frame_size);// 1 frame each time
					}
					gettimeofday(&tm_end,NULL);
					printf(" play_ffpcm_buff() cost time: %d ms\n",get_costtime(tm_start,tm_end) );
				}
				packet.size -= bytes_used;
				packet.data += bytes_used;
			}//---end of while(packet.size>0)

		}//----- ///////  end of audioStream process \\\\\\------


		//---- free OLD packet each time,   that was allocated by av_read_frame
		av_free_packet(&packet);

	}//end of while()


	//----Freee the RGB image
	av_free(buffer);
	av_frame_free(&pFrameRGB);

	//-----Free the YUV frame
	av_frame_free(&pFrame);

	//-----close pcm device
	close_ffpcm_device();

	//----Close the codecs
	printf("----- close the codecs...\n");
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);
	avcodec_close(aCodecCtx);
	avcodec_close(aCodecCtxOrig);

	//----Close the video file
	printf("----- close the video file...\n");
	avformat_close_input(&pFormatCtx);

//<<<<<<<<<<<<<<<     close FT232 and ILI9488    >>>>>>>>>>>>>>>>
	printf("----- close ft232...\n");
	close_ft232();
	printf("---- close_ili9488...\n");
	close_ili9488();
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


	return 0;
}
