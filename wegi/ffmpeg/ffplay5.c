/*----------------------------------------------------------------------------------------------------------
Based on: dranger.com/ffmpeg/tutorialxx.c
 				       ---  by Martin Bohme
	  muroa.org/?q=node/11
				       ---  by Martin Runge
	  www.xuebuyuan.com/1624253.html
				       ---  by niwenxian

TODO:
1. Convert audio format AV_SAMPLE_FMT_FLTP(fltp) to AV_SAMPLE_FMT_S16.
   It dosen't work for ACC_LC decoding, lots of float point operation.

NOTE:
1. A simpley example of opening a video file then decode frames and send RGB data to LCD for display.
   Files without audio stream can also be played.
2. Decode audio frames and playback by alsa PCM.
3. DivX(DV50) is better than Xdiv. Especially in respect to decoded audio/video synchronization,
   DivX is nearly perfect.
4. It plays video files smoothly with format of 480*320*24bit FP20, encoded by DivX.
   Decoding speed also depends on AVstream data rate of the file.
5. The speed of whole procedure depends on ffmpeg decoding speed, FB wirte speed and LCD display speed.
6. Please also notice the speed limit of your LCD controller, It's 500M bps for ILI9488???
7. Cost_time test codes will slow down processing and cause choppy.
8. Use unstripped ffmpeg libs.
9. Try to play mp3 :)

TODO:
1. TO exit main() from a thread.

The data flow of a 480*320 movie is like this:
  (main)    FFmpeg video decoding (~10-15ms per frame) ----> pPICBuff
  (thread)  pPICBuff ----> FB (~xxxms per frame) ---> Display
  (main)    FFmpeg audio decoding ---> write to PCM ( ~2-4ms per packet?)

Usage:
	ffplay3  video_file

Midas Zhou
-------------------------------------------------------------------------------------------------*/
#include "ffplay.h"
#include "spi.h"
#include "egi_timer.h"
#include "egi_fbgeom.h"
#include <signal.h>
#include <math.h>

int main(int argc, char *argv[])
{
	/* for VIDEO and AUDIO  ::  Initializing these to NULL prevents segfaults! */
	AVFormatContext	*pFormatCtx=NULL;

	/* for VIDEO  */
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

	int Hb,Vb;  /* Horizontal and Veritcal size of a picture */
	/* for Pic Info. */
	struct PicInfo pic;

	/* origin movie size */
	int width;
	int height;

	/* scaled movie size */
	int scwidth;
	int scheight;


	/*  for AUDIO  ::  for audio   */
	int			audioStream;
	AVCodecContext		*aCodecCtxOrig=NULL;
	AVCodecContext		*aCodecCtx=NULL;
	AVCodec			*aCodec=NULL;
	AVFrame			*pAudioFrame=NULL;
	int			frame_size;
	int 			sample_rate;
	int			out_sample_rate; /* after conversion, for ffplaypcm */
	enum AVSampleFormat	sample_fmt;
	int			nb_channels;
	int			bytes_per_sample;
	int64_t			channel_layout;
	int 			bytes_used;
	int			got_frame;
//	SwrContext		**pswr=malloc(sizeof(SwrContext *));
	struct SwrContext		*swr=NULL; /* AV_SAMPLE_FMT_FLTP format conversion */
	uint8_t			*outputBuffer=NULL;/* for converted data */
	int 			outsamples;



	/* time structe */
	struct timeval tm_start, tm_end;

	/* thread */
	pthread_t pthd_displayPic;

	/* check input argc */
	if(argc < 2) {
		printf("Please provide a movie file\n");
		return -1;
	}

/* <<<<<<<    Init SPI and FB, Timer   >>>>>>  */
       /* --- open spi dev --- */
        SPI_Open();

        /* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

        /* ---- set timer for time display ---- */
#if 0
        tm_settimer(500000);/* set timer interval interval */
        signal(SIGALRM, tm_sigroutine);

        tm_tick_settimer(TM_TICK_INTERVAL);/* set global tick timer */
        signal(SIGALRM, tm_tick_sigroutine);
#endif


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
	/* Register all formats and codecs */
	av_register_all();

	avformat_network_init();

	/* Open video file */
	printf("open video file... \n");
	if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
	{
		printf("Fail to open video file!\n");
		return -1;
	}

	/* Retrieve stream information */
	printf("retrieve stream information... \n");
	if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
		printf(" Fail to find stream information!\n");
		return -1;
	}

	/* Dump information about file onto standard error */
	printf("try to dump file information... \n");
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	/* Find the first video stream and audio stream */
	printf("try to find the first video stream... \n");
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
	printf("videoStream=%d, audioStream=%d \n",videoStream,audioStream);

	if(videoStream == -1) {
		printf("Didn't find a video stream!\n");
//		return -1;
	}
	if(audioStream == -1) {
		printf("Didn't find an audio stream!\n");
//go on   	return -1;
	}
	if(videoStream == -1 && audioStream == -1) {
		printf("No stream found for video or audio!\n");
		return -1;
	}


	/* proceed --- audio --- stream */
	if(audioStream != -1) /* only if audioStream exists */
  	{
		printf("Prepare for audio stream processing ...\n");
		/* Get a pointer to the codec context for the audio stream */
		aCodecCtxOrig=pFormatCtx->streams[audioStream]->codec;
		/* Find the decoder for the audio stream */
		printf("try to find the decoder for the audio stream... \n");
		aCodec=avcodec_find_decoder(aCodecCtxOrig->codec_id);
		if(aCodec == NULL) {
			fprintf(stderr, "Unsupported audio codec!\n");
			return -1;
		}
		/* copy audio codec context */
		aCodecCtx=avcodec_alloc_context3(aCodec);
		if(avcodec_copy_context(aCodecCtx, aCodecCtxOrig) != 0) {
			fprintf(stderr, "Couldn't copy audio code context!\n");
			return -1;
		}
		/* open audio codec */
		if(avcodec_open2(aCodecCtx, aCodec, NULL) <0 ) {
			fprintf(stderr, "Cound not open audio codec!\n");
			return -1;
		}
		/* get audio stream parameters */
		frame_size = aCodecCtx->frame_size;
		sample_rate = aCodecCtx->sample_rate;
		sample_fmt  = aCodecCtx->sample_fmt;
		bytes_per_sample = av_get_bytes_per_sample(sample_fmt);
		nb_channels = aCodecCtx->channels;
		channel_layout = aCodecCtx->channel_layout;
		printf("	frame_size=%d\n",frame_size);//=nb_samples, nb of samples per frame.
		printf("	channel_layout=%lld\n",channel_layout);//long long int type
		printf("	nb_channels=%d\n",nb_channels);
		printf("	sample format: %s\n",av_get_sample_fmt_name(sample_fmt) );
		printf("	bytes_per_sample: %d\n",bytes_per_sample);
		printf("	sample_rate=%d\n",sample_rate);

		/* prepare SWR context for FLTP format conversion */
		if(sample_fmt == AV_SAMPLE_FMT_FLTP) {
			/* set out sample rate for ffplaypcm */
			out_sample_rate=44100;

			printf("alloc swr and set_opts for converting AV_SAMPLE_FMT_FLTP to S16 ...\n");
			swr=swr_alloc();
#if 0 /*----- to be replaced by swr_alloc_set_opts() */
			//*pswr=swr;
			av_opt_set_channel_layout(swr, "in_channel_layout",  channel_layout, 0);
			av_opt_set_channel_layout(swr, "out_channel_layout", channel_layout, 0);
			av_opt_set_int(swr, "in_sample_rate", 	sample_rate, 0); // for FLTP sample_rate = 24000
			av_opt_set_int(swr, "out_sample_rate", 	out_sample_rate, 0);
			av_opt_set_sample_fmt(swr, "in_sample_fmt",   AV_SAMPLE_FMT_FLTP, 0);
			av_opt_set_sample_fmt(swr, "out_sample_fmt",   AV_SAMPLE_FMT_S16, 0);
#endif

/*
struct SwrContext *swr_alloc_set_opts( swr ,
                                      int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                                      int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                                      int log_offset, void *log_ctx);
*/

			/* allocate and set opts for swr */
			swr=swr_alloc_set_opts( swr,
						channel_layout,AV_SAMPLE_FMT_S16, out_sample_rate,
						channel_layout,AV_SAMPLE_FMT_FLTP, sample_rate,
						0, NULL );
			av_opt_set(swr,"dither_method",SWR_DITHER_RECTANGULAR,0);

			swr_init(swr);

			/* alloc outputBuffer */
			outputBuffer=malloc(2*frame_size * bytes_per_sample);
			if(outputBuffer == NULL)
	       	 	{
				printf("malloc() outputBuffer failed!\n");
				return -1;
			}
			/* open pcm play device and set parameters */
 			prepare_ffpcm_device(nb_channels,out_sample_rate,true); //true for interleaved access
		}
		else
		{
			/* open pcm play device and set parameters */
 			prepare_ffpcm_device(nb_channels,sample_rate,false); //false for noninterleaved access
		}

		/* allocate frame for audio */
		pAudioFrame=av_frame_alloc();
		if(pAudioFrame==NULL) {
			fprintf(stderr, "Fail to allocate pAudioFrame!\n");
			return -1;
		}


	} /* end of if(audioStream =! -1) */


	/* proceed --- video --- stream */
    if(videoStream != -1) /* only if videoStream exists */
    {
	printf("Prepare for video stream processing ...\n");
	/* Get a pointer to the codec context for the video stream */
	pCodecCtxOrig=pFormatCtx->streams[videoStream]->codec;
	/* Find the decoder for the video stream */
	printf("try to find the decoder for the video stream... \n");
	pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if(pCodec == NULL) {
		fprintf(stderr, "Unsupported video codec! try to continue to decode audio...\n");
		videoStream=-1;
		//return -1;
	}
    }

    if(videoStream != -1 && pCodec != NULL)
    {
	/* copy video codec context */
	pCodecCtx=avcodec_alloc_context3(pCodec);
	if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		fprintf(stderr, "Couldn't copy video code context!\n");
		return -1;
	}
	/* open video codec */
	if(avcodec_open2(pCodecCtx, pCodec, NULL) <0 ) {
		fprintf(stderr, "Cound not open video codec!\n");
		return -1;
	}

	/* allocate frame for audio */
/*
	pAudioFrame=av_frame_alloc();
	if(pAudioFrame==NULL) {
		fprintf(stderr, "Fail to allocate pAudioFrame!\n");
		return -1;
	}
*/

	/* Allocate video frame */
	pFrame=av_frame_alloc();
	if(pFrame==NULL) {
		fprintf(stderr, "Fail to allocate pFrame!\n");
		return -1;
	}
	/* allocate an AVFrame structure */
	printf("try to allocate an AVFrame structure...\n");
	pFrameRGB=av_frame_alloc();
	if(pFrameRGB==NULL) {
		fprintf(stderr, "Fail to allocate AVFrame structure!\n");
		return -1;
	}

	/* max. scaled movie size to fit for the screen */
	width=pCodecCtx->width;
	height=pCodecCtx->height;
        if( width > PIC_MAX_WIDTH || height > PIC_MAX_HEIGHT ) {
		if( (width/height) > (PIC_MAX_WIDTH/PIC_MAX_HEIGHT) )
		{
			/* fit for width, only if video width > screen width */
			if(width>PIC_MAX_WIDTH) {
				scwidth=PIC_MAX_WIDTH;
				scheight=height*scwidth/width;
			}
		}
		else if ( (height/width) > (PIC_MAX_HEIGHT/PIC_MAX_WIDTH) )
		{
			/* fit for height, only if video height > screen height */
			if(height>PIC_MAX_HEIGHT) {
				scheight=PIC_MAX_HEIGHT;
				scwidth=width*scheight/height;
			}
		}
	}
	else {
	 	scwidth=width;
	 	scheight=height;
	}

	if(scwidth>PIC_MAX_WIDTH || scheight>PIC_MAX_HEIGHT)
		printf("----- WARNING !!! -----\n	scwidth or scheight out of limit!\n");
	printf("original video size: width=%d, height=%d\nscaled video size: scwidth=%d, scheight=%d \n",width,height,scwidth,scheight);

	/* Determine required buffer size and allocate buffer for scaled picture size */
	numBytes=avpicture_get_size(PIX_FMT_RGB565LE, scwidth, scheight);//pCodecCtx->width, pCodecCtx->height);
	pic.numBytes=numBytes;
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<    allocate mem. for PIC buffers   >>>>>>>>>>>>>>>>>>>>>>>>>>
	if(malloc_PICbuffs(pCodecCtx->width,pCodecCtx->height) == NULL) {
		fprintf(stderr,"Fail to allocate memory for PICbuffs!\n");
		return -1;
	}
	else
		printf("----- finish allocate memory for uint8_t *PICbuffs[%d]\n",PIC_BUFF_NUM);

	/* ---  PICBuff TEST....  --- */
/*
	printf("----- test to get ALL free PICbuff \n");
	for(i=0;i<PIC_BUFF_NUM;i++){
		printf("	get_FreePicBuff()=%d\n",get_FreePicBuff());
		IsFree_PICbuff[i]=false;
	}
*/

/*<<<<<<<<<<<<<     Hs He Vs Ve for IMAGE to LCD layout    >>>>>>>>>>>>>>>>*/

	 /* in order to put displaying window in center of the screen */
/*
	 Hb=(PIC_MAX_WIDTH-pCodecCtx->width+1)/2;
	 Vb=(PIC_MAX_HEIGHT-pCodecCtx->height+1)/2;
	 pic.Hs=Hb; pic.He=Hb+pCodecCtx->width-1;
	 pic.Vs=Vb; pic.Ve=Vb+pCodecCtx->height-1;
*/
	 Hb=(PIC_MAX_WIDTH-scwidth+1)/2; /* horizontal offset */
	 Vb=(PIC_MAX_HEIGHT-scheight+1)/2; /* vertical offset */
	 pic.Hs=Hb; pic.He=Hb+scwidth-1;
	 pic.Vs=Vb; pic.Ve=Vb+scheight-1;



	/* Assign appropriate parts of buffer to image planes in pFrameRGB
	Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture */
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB565LE, scwidth, scheight); //pCodecCtx->width, pCodecCtx->height);

	/* Initialize SWS context for software scaling */
	printf(" initialize SWS context for software scaling... \n");
	sws_ctx = sws_getContext( pCodecCtx->width,
				  pCodecCtx->height,
				  pCodecCtx->pix_fmt,
				  scwidth,//pCodecCtx->width,
				  scheight,//pCodecCtx->height,
				  PIX_FMT_RGB565LE,
				  SWS_BILINEAR,
				  NULL,
				  NULL,
				  NULL
				);

	av_opt_set(sws_ctx,"dither_method",SWR_DITHER_RECTANGULAR,0);

/* <<<<<<<<<<<<     create a thread to display picture to LCD    >>>>>>>>>>>>>>> */
	if(pthread_create(&pthd_displayPic,NULL,thdf_Display_Pic,(void *)&pic) != 0) {
		printf("----- Fails to create the thread for displaying pictures! \n");
		return -1;
	}

  }/* end of (videoStream != -1) */


/*  --------  LOOP  ::  Read packets and process data  --------   */
	printf("----- start loop of reading AV frames and decoding:\n");
	printf("	 converting video frame to RGB and then send to display...\n");
	printf("	 sending audio frame data to playback ... \n");
	i=0;
	while( av_read_frame(pFormatCtx, &packet) >= 0) {

		/* -----   process Video Stream   ----- */
		if( videoStream != -1 && packet.stream_index==videoStream)
		{
			//decode video frame
			//printf("...decoding video frame\n");
			//gettimeofday(&tm_start,NULL);
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			//gettimeofday(&tm_end,NULL);
			//printf(" avcode_decode_video2() cost time: %d ms\n",get_costtime(tm_start,tm_end) );
			/* if we get a complete video frame */
			if(frameFinished) {
				//convert the image from its native format to RG
				//printf("...converting image to RGB\n");
				sws_scale( sws_ctx,
					   (uint8_t const * const *)pFrame->data,
					   pFrame->linesize, 0, pCodecCtx->height,
					   pFrameRGB->data, pFrameRGB->linesize
					);

				/* push data to pic buff for SPI LCD displaying */
				//gettimeofday(&tm_start,NULL);
				//printf(" start Load_Pic2Buff()....\n");
				if( Load_Pic2Buff(&pic,pFrameRGB->data[0],numBytes) <0 )
					printf("PICBuffs are full! The video frame is dropped!\n");
				//---- get play time
				printf("\r		 Elapsed time: %ds  ---  Duration: %ds  ",
					atoi(av_ts2timestr(packet.pts,&pFormatCtx->streams[videoStream]->time_base)),
					atoi(av_ts2timestr(pFormatCtx->streams[videoStream]->duration,&pFormatCtx->streams[videoStream]->time_base))
				 );
//				printf("\r ------ Time stamp: %llds  ------", packet.pts/ );
				fflush(stdout);
				//gettimeofday(&tm_end,NULL);
				//printf(" LCD_Write_Block() for one frame cost time: %d ms\n",get_costtime(tm_start,tm_end) );				//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			}
		}//----- end  of vidoStream process  ------

	//----------------//////   process audio stream   \\\\\\\-----------------
		else if( audioStream != -1 && packet.stream_index==audioStream) { //only if audioStream exists
			//---bytes_used: indicates how many bytes of the data was consumed for decoding. when provided
			//with a self contained packet, it should be used completely.
			//---sb_size: hold the sample buffer size, on return the number of produced samples is stored.
			while(packet.size > 0) {
				//gettimeofday(&tm_start,NULL);
				bytes_used=avcodec_decode_audio4(aCodecCtx, pAudioFrame, &got_frame, &packet);
				//gettimeofday(&tm_end,NULL);
				//printf(" avcode_decode_audio4() cost time: %d ms\n",get_costtime(tm_start,tm_end) );
				if(bytes_used<0)
				{
					printf(" Error while decoding audio!\n");
					packet.size=0;
					packet.data=NULL;
					//av_free_packet(&packet);
					//break;
					continue;
				}
				//----- if decoded data size >0
				if(got_frame)
				{
					//gettimeofday(&tm_start,NULL);
					//---- playback audio data
					if(pAudioFrame->data[0] && pAudioFrame->data[1]) {
						// pAuioFrame->nb_sample = aCodecCtx->frame_size !!!!
						// Number of samples per channel in an audio frame
						if(sample_fmt == AV_SAMPLE_FMT_FLTP) {
							outsamples=swr_convert(swr,&outputBuffer, pAudioFrame->nb_samples, pAudioFrame->data, aCodecCtx->frame_size);
							printf("outsamples=%d, frame_size=%d \n",outsamples,aCodecCtx->frame_size);
							play_ffpcm_buff( &outputBuffer,outsamples);
						}
						else
							 play_ffpcm_buff( (void **)pAudioFrame->data, aCodecCtx->frame_size);// 1 frame each time
					}
					else if(pAudioFrame->data[0]) {  //-- one channel only
						 play_ffpcm_buff( (void **)(&pAudioFrame->data[0]), aCodecCtx->frame_size);// 1 frame each time
					}
					//gettimeofday(&tm_end,NULL);
					//printf(" play_ffpcm_buff() cost time: %d ms\n",get_costtime(tm_start,tm_end) );
				}
				packet.size -= bytes_used;
				packet.data += bytes_used;
			}//---end of while(packet.size>0)

		}//----- ///////  end of audioStream process \\\\\\------

		//---- free OLD packet each time,   that was allocated by av_read_frame
		av_free_packet(&packet);

	}//end of while()

	/* wait display_thread */
	tok_QuitFFplay = true;
	pthread_join(pthd_displayPic,NULL);

	/* free PICbuffs */
	printf("free PICbuffs[]...\n");
        free_PicBuffs();

	/* Free the YUV frame */
	if(pFrame != NULL)
		av_frame_free(&pFrame);

	/* Free the RGB image */
	if(buffer != NULL)
		av_free(buffer);
	if(pFrameRGB != NULL)
		av_frame_free(&pFrameRGB);


	/* close pcm device */
	printf("close PCM device...\n");
	close_ffpcm_device();

	/* free outputBuffer */
	if(outputBuffer != NULL)
	{
		printf("free outputBuffer for pcm...\n");
		free(outputBuffer);
	}

	/* Close the codecs */
	printf("close the codecs...\n");
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);
	avcodec_close(aCodecCtx);
	avcodec_close(aCodecCtxOrig);

	/* Close the video file */
	printf("close the video file...\n");
	avformat_close_input(&pFormatCtx);

	printf("free swr at last...\n");
	swr_free(&swr);

/*  <<<<<<<<<   finish: clean up   >>>>>>>>>>>> */
	/* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);

        /* close spi dev */
        SPI_Close();
        return 0;
}
