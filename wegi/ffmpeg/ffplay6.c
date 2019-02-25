/*----------------------------------------------------------------------------------------------------------
Based on:
          FFmpeg examples in code sources.
          dranger.com/ffmpeg/tutorialxx.c
 				       ---  by Martin Bohme
	  muroa.org/?q=node/11
				       ---  by Martin Runge
	  www.xuebuyuan.com/1624253.html
				       ---  by niwenxian


TODO:
1. Convert audio format AV_SAMPLE_FMT_FLTP(fltp) to AV_SAMPLE_FMT_S16.
   It dosen't work for ACC_LC decoding, lots of float point operations.
2. Can not decode png picture in a mp3 file. ...OK, with some tricks.


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


		 (((  --- Glossary ---  )))

PAR	 --- Pixel Aspect Ratio
SAR	 --- Sample Aspect Ratio
	     BUT!!!! in avcodec.h,  struct AVCodecContext->sample_aspect_ratio is "width of a pixel divided by the height of the pixel"
DAR	 --- Display Aspect Ratio
PIX_FMT  --- pixel format defined in libavutil/pixfmt.h


		 (((  --- Data Flow ---  )))

The data flow of a movie is like this:
  (main)    FFmpeg video decoding (~10-15ms per frame) ----> pPICBuff
  (thread)  pPICBuff ----> FB (~xxxms per frame) ---> Display
  (main)    FFmpeg audio decoding ---> write to PCM ( ~2-4ms per packet?)



Usage:
	ffplay  video_file/media_address
	ffplay /music/*.mp3
	ffplay /music/*.mp3  /video/*.avi

Midas Zhou
-------------------------------------------------------------------------------------------------*/
#include "ffplay.h"
#include "spi.h"
#include "egi_timer.h"
#include "egi_fbgeom.h"
#include "egi_debug.h"
#include <signal.h>
#include <math.h>

#define ENABLE_AVFILTER 1 /* enable AVFilter for video */
#define ENABLE_CLIP_TEST 0 /* play the beginning part of a file */
#define ENABLE_SEEK_LOOP 0  /* loop seeking and playing from the start of the same file */
#define ENABLE_MEDIA_LOOP 1 /* loop trying to open and play media stream or file(s), if the file is not recognizable then skip it. */
#define ENABLE_AUDIO 1
#define ENABLE_AUTOFIT_ROTPIC 0 /* auto. rotate picture to fit for the screen ration */

#define FF_LOOP_TIMEGAP 0 //3 /* in second, holdon or idle time after ffplay a file */


int ff_token_skip=0; /* when >0, stop to play next file */

int main(int argc, char *argv[])
{
	/* for input files */
	int fnum; /* number of multimedia files input from shell */
	int ff_sec_Vduration; /* in seconds, multimedia file duration */
	int ff_sec_Aduration; /* in seconds, multimedia file duration */
	int ff_sec_Velapsed;  /* in seconds, playing time elapsed */
	int ff_sec_Aelapsed;  /* in seconds, playing time elapsed */

	/* for VIDEO and AUDIO  ::  Initializing these to NULL prevents segfaults! */
	AVFormatContext	*pFormatCtx=NULL;

	/* for VIDEO  */
	int			i;
	int			videoStream=-1; /* >=0, if stream exists */
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
	AVRational 		time_base; /*get from video stream, pFormatCtx->streams[videoStream]->time_base*/

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
	int			audioStream=-1;/* >=0, if stream exists */
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
	struct SwrContext		*swr=NULL; /* AV_SAMPLE_FMT_FLTP format conversion */
	uint8_t			*outputBuffer=NULL;/* for converted data */
	int 			outsamples;

	/* for AVFilters */
	AVFilterContext *avFltCtx_BufferSink=NULL;
	AVFilterContext *avFltCtx_BufferSrc=NULL;
	AVFilter	*avFlt_BufferSink=NULL; /* to be freed by its holding graph */
	AVFilter	*avFlt_BufferSrc=NULL;
	AVFilterInOut	*avFltIO_InPuts=NULL;/* A linked-list of the inputs/outputs of the filter chain */
	AVFilterInOut	*avFltIO_OutPuts=NULL;
	//AVBufferSinkParams	*BufferSink_params;
	AVFilterGraph	*filter_graph=NULL; /* filter chain graph */
	AVFrame		*filt_pFrame=NULL; /* for filtered frame */
	enum AVPixelFormat outputs_pix_fmts[] = { AV_PIX_FMT_RGB565LE, AV_PIX_FMT_NONE };/* NONE as for end token */
	char args[512];
	/* video filter descr, same as -vf option in ffmpeg */
//	const char *filters_descr = "scale=240:160,transpose=cclock"; /* after cclock -> W160xH240, or clock */;
//	const char *filters_descr = "movie=logo.png[logo];[in][logo]overlay=5:5,scale=240:160,transpose=cclock"; /* after cclock -> W160xH240, or clock */;
	const char *filters_descr = "movie=logo.png[logo];[in][logo]overlay=5:5,scale=320:240,transpose=cclock"; /* after cclock -> W160xH240, or clock */;

	/* time structe */
	struct timeval tm_start, tm_end;

	/* thread */
	pthread_t pthd_displayPic;

	int ret;

	/* check input argc */
	if(argc < 2) {
		printf("File path not found!\n");
		return -1;
	}
	printf("total number of input files: %d\n",argc);

/* <<<<<<<    Init SPI and FB, Timer   >>>>>>  */
       /* --- open spi dev for touch pad --- */
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
#if ENABLE_MEDIA_LOOP
 /* MEDIA_LOOP_START */
printf("enable to loop playing media file or stream ...\n");

while(1) {
#endif

   /* Register all formats and codecs */
   av_register_all();
   avformat_network_init();
   avfilter_register_all(); /* register all default builtin filters */


   /* play all input files, one by one. */
   for(fnum=1;fnum<argc;fnum++)
   {

	/* Open media stream or file */
	printf("%lld(ms):	try to open file %s ...\n", tm_get_tmstampms(), argv[fnum]);
	if(avformat_open_input(&pFormatCtx, argv[fnum], NULL, NULL)!=0)
	{
		printf("Fail to open the file, or file type is not recognizable.\n");
#if ENABLE_MEDIA_LOOP
		avformat_close_input(&pFormatCtx);
        	pFormatCtx=NULL;
		usleep(350000);
		continue;
#else
		return -1;
#endif
	}

	/* Retrieve stream information, !!!!! time consuming !!!! */
	printf("%lld(ms):	retrieve stream information... \n", tm_get_tmstampms());
	// ---- seems no use! ----

	//pFormatCtx->probesize2=128*1024;
	//pFormatCtx->max_analyze_duration2=8*AV_TIME_BASE;
	if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
		printf(" Fail to find stream information!\n");
#if ENABLE_MEDIA_LOOP
		avformat_close_input(&pFormatCtx);
        	pFormatCtx=NULL;
		continue;
#else
		return -1;
#endif
	}

	/* Dump information about file onto standard error */
	printf("%lld(ms):	try to dump file information... \n",tm_get_tmstampms());
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	/* Find the first video stream and audio stream */
	printf("%lld(ms):	try to find the first video stream... \n",tm_get_tmstampms());
	/* reset stream index first */
	videoStream=-1;
	audioStream=-1;

	printf("number of streams: %d \n",pFormatCtx->nb_streams);
	for(i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) {
		   if( videoStream < 0) {
			videoStream=i;
			printf("video stream [%d] is found .\n",i);
		   }
		   else
			printf("video stream [%d] is also available.\n",i);
		}
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
		   audioStream < 0) {
			audioStream=i;
		}
	}
	printf("%lld(ms):	videoStream=%d, audioStream=%d \n",tm_get_tmstampms(),videoStream,audioStream);

	if(videoStream == -1) {
		printf("Didn't find a video stream!\n");
//		return -1;
	}
	if(audioStream == -1) {
		printf("Didn't find an audio stream!\n");
//go on   	return -1;
	}
	if(videoStream == -1 && audioStream == -1) {
		printf("No stream found for video or audio! quit now...\n");
		return -1;
	}

#if(!ENABLE_AUDIO)
	printf(" audio is disabled! \n");
	audioStream=-1;
#endif

	/* proceed --- audio --- stream */
	if(audioStream >= 0) /* only if audioStream exists */
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
#if 1 /*----- or to be replaced by swr_alloc_set_opts() */
			av_opt_set_channel_layout(swr, "in_channel_layout",  channel_layout, 0);
			av_opt_set_channel_layout(swr, "out_channel_layout", channel_layout, 0);
			av_opt_set_int(swr, "in_sample_rate", 	sample_rate, 0); // for FLTP sample_rate = 24000
			av_opt_set_int(swr, "out_sample_rate", 	out_sample_rate, 0);
			av_opt_set_sample_fmt(swr, "in_sample_fmt",   AV_SAMPLE_FMT_FLTP, 0);
			av_opt_set_sample_fmt(swr, "out_sample_fmt",   AV_SAMPLE_FMT_S16, 0);
#endif

#if 0 /* test swr_alloc_set_opts() */

/* function definition:
struct SwrContext *swr_alloc_set_opts( swr ,
                                      int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                                      int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                                      int log_offset, void *log_ctx);		 */

			/* allocate and set opts for swr */
			printf("swr_alloc_set_opts()...\n");
			swr=swr_alloc_set_opts( swr,
						channel_layout,AV_SAMPLE_FMT_S16, out_sample_rate,
						channel_layout,AV_SAMPLE_FMT_FLTP, sample_rate,
						0, NULL );
			//printf("av_opt_set()...\n");
			/* how to dither ...?? */
			//av_opt_set(swr,"dither_method",SWR_DITHER_RECTANGULAR,0);
#endif

			printf("swr_init(swr)...\n");
			swr_init(swr);

			/* alloc outputBuffer */
			printf("malloc outputBuffer ...\n");
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
		printf("alloc pAudioFrame ...\n");
		pAudioFrame=av_frame_alloc();
		if(pAudioFrame==NULL) {
			fprintf(stderr, "Fail to allocate pAudioFrame!\n");
			return -1;
		}

	} /* end of if(audioStream =! -1) */


	/* proceed --- video --- stream */
    if(videoStream >=0 ) /* only if videoStream exists */
    {
	printf("Prepare for video stream processing ...\n");
	clear_screen(&gv_fb_dev, 0);

	/* get time_base */
	time_base = pFormatCtx->streams[videoStream]->time_base;

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

	if(pCodecCtxOrig->width <= 0 || pCodecCtxOrig->height <= 0) {
		fprintf(stderr, "Video width=%d or height=%d illegal! try to continue to decode audio...\n",
				   pCodecCtxOrig->width, pCodecCtxOrig->height);
		videoStream=-1;
	}

    }

    /* videoStream may be reset to -1, so confirm again */
    if(videoStream >=0 && pCodec != NULL)
    {

#if ENABLE_AVFILTER /* AVFilter ON */
  	/*---------<<< START: prepare filters >>>--------*/
   	printf("video stream is found, prepare avfilters ...\n");

	filt_pFrame=av_frame_alloc();/* alloc AVFrame for filtered frame */
   	avFlt_BufferSink=avfilter_get_by_name("buffersink");/* get a registerd builtin filter by name */
   	avFlt_BufferSrc=avfilter_get_by_name("buffer");
   	if(avFlt_BufferSink==NULL || avFlt_BufferSrc==NULL)
   	{
		printf("Fail to get avFlt_BufferSink or avFlt_BufferSrc.\n");
		goto ff_fail;
   	}
   	avFltIO_InPuts=avfilter_inout_alloc();
   	avFltIO_OutPuts=avfilter_inout_alloc();
   	filter_graph=avfilter_graph_alloc();

   	/* input arguments for filter_graph */
   	snprintf(args,sizeof(args),
			"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
			pCodecCtxOrig->width,pCodecCtxOrig->height,pCodecCtxOrig->pix_fmt,
			time_base.num,time_base.den,
			pCodecCtxOrig->sample_aspect_ratio.num,pCodecCtxOrig->sample_aspect_ratio.den);
		//240,320,AV_PIX_FMT_RGB565LE,1,25,1,1
	printf("filter graph args: %s\n",args);

   	/* create source(in) filter in the filter  graph
    	*  int avfilter_graph_create_filter(AVFilterContext **filt_ctx, const AVFilter *filt,
    	*                        const char *name, const char *args, void *opaque,
    	*                        AVFilterGraph *graph_ctx);
   	*/
   	ret=avfilter_graph_create_filter(&avFltCtx_BufferSrc, avFlt_BufferSrc,"in",args,NULL,filter_graph);
   	if(ret<0)
   	{
        	//av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source.\n");
		printf("Fail to call avfilter_graph_create_filter() to create BufferSrc filter...\n");
		goto ff_fail;
   	}

   	/* create sink(out) filter in the filter chain graph */
   	//BufferSink_params=av_buffersink_params_alloc();
   	//BufferSink_params->pixel_fmts=;
   	ret=avfilter_graph_create_filter(&avFltCtx_BufferSink, avFlt_BufferSink,"out",NULL,NULL,filter_graph);
   	//av_free(BufferSink_params);
   	if(ret<0)
   	{
        	//av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink.\n");
		printf("Fail to call avfilter_graph_create_filter() to create BufferSink filter...\n");
		goto ff_fail;
   	}

   	/* set output pix format */
   	/**
   	* Set a binary option to an integer list.
   	*
   	* @param obj    AVClass object to set options on
   	* @param name   name of the binary option
   	* @param val    pointer to an integer list (must have the correct type with
   	*               regard to the contents of the list)
  	* @param term   list terminator (usually 0 or -1)
   	* @param flags  search flags
   	*/
   	ret=av_opt_set_int_list(avFltCtx_BufferSink, "pix_fmts", outputs_pix_fmts,
        	                      AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
   	if (ret < 0) {
        	//av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		printf("Fail to call av_opt_set_int_list() to set pixel format for output filter...\n");
        	goto ff_fail;
    	}
   	/* set the endpoints for the filter graph --- Source */
   	/* in the view of Caller | in the view of Filter */
   	avFltIO_OutPuts->name		=av_strdup("in");
   	avFltIO_OutPuts->filter_ctx	=avFltCtx_BufferSrc;
   	avFltIO_OutPuts->pad_idx	=0;
   	avFltIO_OutPuts->next		=NULL;

   	/* set the endpoints for the filter graph --- Sink */
   	/* in the view of Caller | in the view of the Filter */
   	avFltIO_InPuts->name	=av_strdup("out");
   	avFltIO_InPuts->filter_ctx	=avFltCtx_BufferSink;
   	avFltIO_InPuts->pad_idx	=0;
   	avFltIO_InPuts->next	=NULL;

   	/* parse filter graph
    	* int avfilter_graph_parse_ptr(AVFilterGraph *graph, const char *filters,
    	*                     AVFilterInOut **open_inputs_ptr, AVFilterInOut **open_outputs_ptr,
    	*                     void *log_ctx)
    	*		                  	( I/O in the view of Caller )
    	*/
    	ret=avfilter_graph_parse_ptr(filter_graph,filters_descr,&avFltIO_InPuts,&avFltIO_OutPuts,NULL);
    	if (ret < 0) {
        	//av_log(NULL, AV_LOG_ERROR, "Fail to parse avfilter graph.\n");
		printf("Fail to call avfilter_graph_parse_ptr() to parse fitler graph descriptions.\n");
        	goto ff_fail;
    	}
    	/* configure the filter graph */
    	ret=avfilter_graph_config(filter_graph,NULL);
    	if (ret < 0) {
        	//av_log(NULL, AV_LOG_ERROR, "Fail to parse avfilter graph.\n");
		printf("Fail to call avfilter_graph_config() to configure filter graph.\n");
        	goto ff_fail;
    	}
    	/* free temp. vars */
    	avfilter_inout_free(&avFltIO_InPuts);
    	avfilter_inout_free(&avFltIO_OutPuts);
	printf("Finish prepare avfilter and configure filter graph.\n");
  	/*---------<<< END: prepare filters >>>--------*/

#endif  /* end of AVFilter ON */


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

	/* get original movie size */
	width=pCodecCtx->width;
	height=pCodecCtx->height;
	/* calculate scaled movie size to fit for the screen */
#if ENABLE_AVFILTER  /* to be decided by filters_description string, if frame rotated. */
        scwidth=240;//160;
	scheight=320;//240;


#else if (!ENABLE_AVFILTER) /* to fit for current width and height, no rotation */
        if( width > PIC_MAX_WIDTH || height > PIC_MAX_HEIGHT ) {
		if( (1.0*width/height) >= (1.0*PIC_MAX_WIDTH/PIC_MAX_HEIGHT) )
		{
			//printf("width/height >= PIC_MAX_WIDTH/PIC_MAX_HEIGHT \n");
			/* fit for width, only if video width > screen width */
			if(width>PIC_MAX_WIDTH) {
				scwidth=PIC_MAX_WIDTH;
				scheight=height*scwidth/width;
			}
		}
		else if ( (1.0*height/width) > (1.0*PIC_MAX_HEIGHT/PIC_MAX_WIDTH) )
		{
			//printf("height/width > PIC_MAX_HEIGHT/PIC_MAX_WIDTH \n");
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
#endif /* end of ENABLE_AVFILTER on/off */

	printf("original video size: width=%d, height=%d\nscaled video size: scwidth=%d, scheight=%d \n",width,height,scwidth,scheight);

	if( scwidth>PIC_MAX_WIDTH ||
            scheight>PIC_MAX_HEIGHT ||
	    scwidth <= 0 || scheight <= 0  )
        {
		printf("----- WARNING !!! -----\n	scwidth or scheight out of limit! reset to 240x240. \n");
		scwidth=240;
		scheight=240;
	}

	/* Determine required buffer size and allocate buffer for scaled picture size */
	numBytes=avpicture_get_size(PIX_FMT_RGB565LE, scwidth, scheight);//pCodecCtx->width, pCodecCtx->height);
	pic.numBytes=numBytes;
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));


/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<    allocate mem. for PIC buffers   >>>>>>>>>>>>>>>>>>>>>>>>>> */

//	if(malloc_PICbuffs(pCodecCtx->width,pCodecCtx->height) == NULL) {
	if(malloc_PICbuffs(scwidth,scheight) == NULL) {
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
	 Hb=(PIC_MAX_WIDTH-scwidth+1)/2; /* horizontal offset */
	 Vb=(PIC_MAX_HEIGHT-scheight+1)/2; /* vertical offset */
	 pic.Hs=Hb; pic.He=Hb+scwidth-1;
	 pic.Vs=Vb; pic.Ve=Vb+scheight-1;

	/* Assign appropriate parts of buffer to image planes in pFrameRGB
	Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture */
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB565LE, scwidth, scheight); //pCodecCtx->width, pCodecCtx->height);

	/* Initialize SWS context for software scaling, allocate and return a SwsContext */
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

//	av_opt_set(sws_ctx,"dither_method",SWR_DITHER_RECTANGULAR,0);

/* <<<<<<<<<<<<     create a thread to display picture to LCD    >>>>>>>>>>>>>>> */
	if(pthread_create(&pthd_displayPic,NULL,thdf_Display_Pic,(void *)&pic) != 0) {
		printf("----- Fails to create the thread for displaying pictures! \n");
		return -1;
	}

  }/* end of (videoStream >=0 ) */


/*  --------  LOOP  ::  Read packets and process data  --------   */
	gettimeofday(&tm_start,NULL);
	printf("--- PLAYING ---  loop reading AV frames and decoding... \n");
	//printf("	 converting video frame to RGB and then send to display...\n");
	//printf("	 sending audio frame data to playback ... \n");
	i=0;

#if ENABLE_SEEK_LOOP /* test loop ..... */
 START_SEEK_LOOP:
	/* seek starting point */
        av_seek_frame(pFormatCtx, 0, 0, AVSEEK_FLAG_ANY);
#endif

	while( av_read_frame(pFormatCtx, &packet) >= 0) {
		/* -----   process Video Stream   ----- */
		if( videoStream >=0 && packet.stream_index==videoStream)
		{
			//printf("...decoding video frame\n");
			//gettimeofday(&tm_start,NULL);
			if( avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet)<0 )
				printf("Error decoding video. try to carry on...\n");
			//gettimeofday(&tm_end,NULL);
			//printf(" avcode_decode_video2() cost time: %d ms\n",get_costtime(tm_start,tm_end) );

			/* if we get complete video frame(s) */
			if(frameFinished) {
#if ENABLE_AVFILTER /* AVFilter ON */
				pFrame->pts=av_frame_get_best_effort_timestamp(pFrame);
				/* push decoded frame into the filter graph */
			        if( av_buffersrc_add_frame_flags(avFltCtx_BufferSrc,pFrame,
									AV_BUFFERSRC_FLAG_KEEP_REF) <0 )
				{
				    //av_log(NULL,AV_LOG_ERROR,"Error while feeding pFrame to filter graph through avFltCtx_BufferSrc.\n");
				    printf("Error feeding pFrame to filter graph,try to carry on...\n");
				    //break;
				}
				/* pull filtered frames from the filter graph */
				while(1)
				{
					ret=av_buffersink_get_frame(avFltCtx_BufferSink, filt_pFrame);
					if( ret==AVERROR(EAGAIN) || ret==AVERROR_EOF )
						break;
					else if(ret<0)
					{
						break; /* try to carry on */
						//goto ff_fail;
					}
					/* push data to pic buff for SPI LCD displaying */
					//printf(" start Load_Pic2Buff()....\n");
					if( Load_Pic2Buff(&pic,filt_pFrame->data[0],numBytes) <0 )
						egi_pdebug(DBG_FFPLAY,"PICBuffs are full! video frame is dropped!\n");

					av_frame_unref(filt_pFrame); /* unref it, or it will eat up memory */
				}
				av_frame_unref(filt_pFrame);

#else if (!ENABLE_AVFILTER) /* AVfilter OFF */
				/* convert the image from its native format to RGB */
				//printf("...converting image to RGB\n");
				sws_scale( sws_ctx,
					   (uint8_t const * const *)pFrame->data,
					   pFrame->linesize, 0, pCodecCtx->height,
					   pFrameRGB->data, pFrameRGB->linesize
					);

				/* push data to pic buff for SPI LCD displaying */
				//printf(" start Load_Pic2Buff()....\n");
				if( Load_Pic2Buff(&pic,pFrameRGB->data[0],numBytes) <0 )
					printf("PICBuffs are full! The video frame is dropped!\n");
#endif  /* end of AVFilter ON/OFF */


				/* ---print playing time--- */
				ff_sec_Velapsed=atoi( av_ts2timestr(packet.pts,
				     			&pFormatCtx->streams[videoStream]->time_base) );
				ff_sec_Vduration=atoi( av_ts2timestr(pFormatCtx->streams[videoStream]->duration,
							&pFormatCtx->streams[videoStream]->time_base) );
/*
				printf("\r	     video Elapsed time: %ds  ---  Duration: %ds  ",
									ff_sec_Velapsed, ff_sec_Vduration );

				//printf("\r ------ Time stamp: %llds  ------", packet.pts/ );
				fflush(stdout);
*/
				//gettimeofday(&tm_end,NULL);
				//printf(" LCD_Write_Block() for one frame cost time: %d ms\n",get_costtime(tm_start,tm_end) );				//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			} /* end of if(FrameFinished) */

		}/*  end  of vidoStream process  */


	/*----------------//////   process audio stream   \\\\\\\-----------------*/
		else if( audioStream != -1 && packet.stream_index==audioStream) { //only if audioStream exists
			/* bytes_used: indicates how many bytes of the data was consumed for decoding.
			         when provided with a self contained packet, it should be used completely.
			*  sb_size: hold the sample buffer size, on return the number of produced samples is stored.
			*/
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
				/* if decoded data size >0 */
				if(got_frame)
				{
					//gettimeofday(&tm_start,NULL);
					/* playback audio data */
					if(pAudioFrame->data[0] && pAudioFrame->data[1]) {
						// pAuioFrame->nb_sample = aCodecCtx->frame_size !!!!
						// Number of samples per channel in an audio frame
						if(sample_fmt == AV_SAMPLE_FMT_FLTP) {
							outsamples=swr_convert(swr,&outputBuffer, pAudioFrame->nb_samples, (const uint8_t **)pAudioFrame->data, aCodecCtx->frame_size);
							printf("outsamples=%d, frame_size=%d \n",outsamples,aCodecCtx->frame_size);
							play_ffpcm_buff( (void **)&outputBuffer,outsamples);
						}
						else
							 play_ffpcm_buff( (void **)pAudioFrame->data, aCodecCtx->frame_size);// 1 frame each time
					}
					else if(pAudioFrame->data[0]) {  //-- one channel only
						 play_ffpcm_buff( (void **)(&pAudioFrame->data[0]), aCodecCtx->frame_size);// 1 frame each time
					}

					/* print audio playing time, only if no video stream */
						ff_sec_Aelapsed=atoi( av_ts2timestr(packet.pts,
				     				&pFormatCtx->streams[audioStream]->time_base) );
						ff_sec_Aduration=atoi( av_ts2timestr(pFormatCtx->streams[audioStream]->duration,
								&pFormatCtx->streams[audioStream]->time_base) );
/*
						printf("\r	     audio Elapsed time: %ds  ---  Duration: %ds  ",
									ff_sec_Aelapsed, ff_sec_Aduration );
*/
					//gettimeofday(&tm_end,NULL);
					//printf(" play_ffpcm_buff() cost time: %d ms\n",get_costtime(tm_start,tm_end) );

				}
				packet.size -= bytes_used;
				packet.data += bytes_used;
			}/* end of while(packet.size>0) */


		}/*   end of audioStream process  */

                /* free OLD packet each time, that was allocated by av_read_frame */
                av_free_packet(&packet);

#if ENABLE_CLIP_TEST /* for test only ---------*/
		if( (audioStream >= 0) && (ff_sec_Aelapsed >= 5 || ff_token_skip ) )
		{
			//reset:  ff_token_skip=0;
			ff_sec_Aelapsed=0;
			ff_sec_Aduration=0;
			break;
		}
#endif

	}/*  end of while()   */

#if ENABLE_SEEK_LOOP /* test seek loop  */
goto SEEK_LOOP_START;
#endif


ff_fail:
	if(videoStream >=0) /* only if video stream exists */
	{
		/* wait display_thread */
		printf("joint picture displaying thread ...\n");
		fftok_QuitFFplay = true;
		pthread_join(pthd_displayPic,NULL);
		fftok_QuitFFplay = false;

		/* free PICbuffs */
		printf("free PICbuffs[]...\n");
        	free_PicBuffs();
	}

	/* Free the YUV frame */
	printf("free pFrame...\n");
	if(pFrame != NULL) {
		av_frame_free(&pFrame);
		pFrame=NULL;
	}

	/* Free the RGB image */
	printf("free buffer...\n");
	if(buffer != NULL) {
		av_free(buffer);
		buffer=NULL;
	}
	printf("free pFrameRGB...\n");
	if(pFrameRGB != NULL) {
		av_frame_free(&pFrameRGB);
		pFrameRGB=NULL;
	}

	/* close pcm device */
	printf("close PCM device...\n");
	close_ffpcm_device();

	/* free outputBuffer */
	if(outputBuffer != NULL)
	{
		printf("free outputBuffer for pcm...\n");
		free(outputBuffer);
		outputBuffer=NULL;
	}

#if ENABLE_AVFILTER
	/* free filter items */
	if(filter_graph != NULL)
	{
		printf("avfilter graph free ...\n");
		/* It will also free all AVFilters in the filter graph */
		avfilter_graph_free(&filter_graph);
		filter_graph=NULL;
	}
	/* Free filted frame */
	printf("free filt_pFrame...\n");
	if(filt_pFrame != NULL) {
		av_frame_free(&filt_pFrame);
		filt_pFrame=NULL;
	}
#endif

	/* Close the codecs */
	printf("close the codecs...\n");
	avcodec_close(pCodecCtx);
	pCodecCtx=NULL;
	avcodec_close(pCodecCtxOrig);
	pCodecCtxOrig=NULL;
	avcodec_close(aCodecCtx);
	aCodecCtx=NULL;
	avcodec_close(aCodecCtxOrig);
	aCodecCtxOrig=NULL;

	/* Close the video file */
	printf("close the video file...\n");
	avformat_close_input(&pFormatCtx);
	pFormatCtx=NULL;

	if(audioStream >= 0)
	{
		printf("free swr at last...\n");
		swr_free(&swr);
		swr=NULL;
	}

	if(videoStream >= 0)
	{
		printf("free sws_ctx at last...\n");
		sws_freeContext(sws_ctx);
		sws_ctx=NULL;
	}

	/* print total playing time for the file */
	gettimeofday(&tm_end,NULL);
	printf("Playing %s cost time: %d ms\n", argv[fnum],get_costtime(tm_start,tm_end) );

	if(FF_LOOP_TIMEGAP>0)
	{
		printf("Hold on for a while...\n");
		sleep(FF_LOOP_TIMEGAP);
	}


   } /* end of for(...) play input files*/

#if ENABLE_MEDIA_LOOP

	//usleep(500000);
/* MEDIA_LOOP_END */
}

#endif

/*  <<<<<<<<<   finish: clean up   >>>>>>>>>>>> */
	/* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);

        /* close spi dev */
        SPI_Close();
        return 0;
}
