/*-------------------------------------------------------------
Based on: dranger.com/ffmpeg/tutorialxx.c
 				       ---  by Martin Bohme
	  muroa.org/?q=node/11
				       ---  by Martin Runge
	  www.xuebuyuan.com/1624253.html
				       ---  by niwenxian

1. A simpley example of opening a video file then decode frames
and send RGB data to LCD for display.
2. Decode audio frames and save to a PCM file.

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

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48KHz 32bit audio


void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame){
	FILE *pFile;
	char szFilename[32];
	int y;

	//Open file
	printf("----- try to save frame to frame%d.ppm \n",iFrame);
	sprintf(szFilename,"frame%d.ppm",iFrame);
	pFile=fopen(szFilename,"wb");
	if(pFile==NULL){
		printf("Fail to open file for write!\n");
		return;
	}

	//write header
	fprintf(pFile,"P6\n%d %d\n255\n",width,height);
	//write pixel data
	for(y=0; y<height; y++)
		fwrite(pFrame->data[0]+y*pFrame->linesize[0],1,width*3,pFile);

	//close file
	fclose(pFile);
}


int main(int argc, char *argv[]) {
	//Initializing these to NULL prevents segfaults!
	AVFormatContext	*pFormatCtx=NULL;
	int			i,j;
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
//	const int 		audio_sample_buf_size=2*MAX_AUDIO_FRAME_SIZE;
//	int16_t			audio_sample_buffer[audio_sample_buf_size];
	int 			sample_rate;
	enum AVSampleFormat	sample_fmt;
	int			nb_channels;
	int			bytes_per_sample;
	int64_t			channel_layout;
	int 			bytes_used;
//	int			sb_size=audio_sample_buf_size;
	int			got_frame;
 
	FILE* faudio; // to save decoded audio data
	faudio=fopen("/tmp/ffaudio.pcm","wb");
	if(faudio==NULL){
		printf("Fail to open file for audio data saving!\n");
		return -1;
	}


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

	//======================  Read packets and process =============================
	printf("----- read frames and convert to RGB and then send to LCD ... \n");
	i=0;
	while( av_read_frame(pFormatCtx, &packet) >= 0) {

		//----------------//////  process of video stream  \\\\\\\-----------------
		if(packet.stream_index==videoStream) {
			//decode video frame
//			printf("...decoding video frame\n");
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
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
				LCD_Write_Block(Hs,He,Vs,Ve,pFrameRGB->data[0],numBytes);
				//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

			}
		}//----- end  of vidoStream process  ------

		//----------------//////  process of audio stream  \\\\\\\-----------------
		else if(packet.stream_index==audioStream) {
			//---bytes_used: indicates how many bytes of the data was consumed for decoding. when provided
			//with a self contained packet, it should be used completely.
			//---sb_size: hold the sample buffer size, on return the number of produced samples is stored.
			while(packet.size > 0) {
				bytes_used=avcodec_decode_audio4(aCodecCtx, pAudioFrame, &got_frame, &packet);
				if(bytes_used<0)
				{
					printf(" Error while decoding audio!\n");
					//break;
					continue;
				}
				//----- if decoded data size >0
				if(got_frame)
				{
					//---- save decoded audio data
					if(pAudioFrame->data[0] && pAudioFrame->data[1]) {
						// aCodecCtx->frame_size: Number of samples per channel in an audio frame
						// write in interleaved format
						for(j=0; j < aCodecCtx->frame_size; j++) {
							fwrite(pAudioFrame->data[0]+j*bytes_per_sample,1,bytes_per_sample,faudio);
							fwrite(pAudioFrame->data[1]+j*bytes_per_sample,1,bytes_per_sample,faudio);
						}
					}
					else if(pAudioFrame->data[0]) {
							fwrite(pAudioFrame->data[0]+i*bytes_per_sample,1,bytes_per_sample,faudio);
					}

					fflush(faudio);
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

	//-----close file
	fclose(faudio);

	//----Close the codecs
	printf("----- close the codecs...\n");
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);
	avcodec_close(aCodecCtx);
	avcodec_close(aCodecCtxOrig);

	//----Close the video file
	printf("----- close the viedo file...\n");
	avformat_close_input(&pFormatCtx);

//<<<<<<<<<<<<<<<     close FT232 and ILI9488    >>>>>>>>>>>>>>>>
	printf("----- close ft232...\n");
	close_ft232();
	printf("---- close_ili9488...\n");
	close_ili9488();
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


	return 0;
}
