/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


                        <<   Glossary  >>

Sample:         A digital value representing a sound amplitude,
                sample data width usually to be 8bits or 16bits.
Channels:       1-Mono. 2-Stereo, ...
Channel_layout: For FFMPEG: It's a 64bits integer,with each bit set for one channel:
		0x00000001(1<<0) for AV_CH_FRONT_LEFT
		0x00000002(1<<1) for AV_CH_FRONT_RIGHT
		0x00000004(1<<2) for AV_CH_FRONT_CENTER
			 ... ...
Frame_Size:     Sizeof(one sample)*channels, in bytes.
		(For ffmpeg: frame_size=number of frames, may refer to encoded/decoded data size.)
Rate:           Frames per second played/captured. in HZ.
Data_Rate:      Frame_size * Rate, usually in kBytes/s(kb/s), or kbits/s.
Period:         Period of time that a hard ware spends in processing one pass data/
                certain numbers of frames, as capable of the HW.
Period Size:    Representing Max. frame numbers a hard ware can be handled for each
                write/read(playe/capture), in HZ, NOT in bytes??
                (different value for PLAYBACK and CAPTURE!!)
		Also can be set in asound.conf ?
		WM8960 running show:
   		1536 frames for CAPTURE(default); 278 frames for PLAYBACK(default), depends on HW?
Chunk(period):  frames from snd_pcm_readi() each time for shine enconder.
Buffer:         Usually in N*periods

Interleaved mode:       record period data frame by frame, such as
                        frame1(Left sample,Right sample), frame2(), ......
Uninterleaved mode:     record period data channel by channel, such as
                        period(Left sample,Left ,left...), period(right,right...),
                        period()...
snd_pcm_uframes_t       unsigned long
snd_pcm_sframes_t       signed long


		--- FFMPEG SAMPLE FORMAT ---

enum AVSampleFormat {
    AV_SAMPLE_FMT_NONE = -1,
    AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    AV_SAMPLE_FMT_S16,         ///< signed 16 bits
    AV_SAMPLE_FMT_S32,         ///< signed 32 bits
    AV_SAMPLE_FMT_FLT,         ///< float
    AV_SAMPLE_FMT_DBL,         ///< double

    AV_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    AV_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    AV_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    AV_SAMPLE_FMT_FLTP,        ///< float, planar
    AV_SAMPLE_FMT_DBLP,        ///< double, planar

    AV_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
};



Note:
1. Mplayer will take the pcm device exclusively?? ---NOPE.
2. ctrl_c also send interrupt signal to MPlayer to cause it stuck.
   Mplayer interrrupted by signal 2 in Module: enable_cache
   Mplayer interrrupted by signal 2 in Module: play_audio

TODO:
1. To play PCM data with format other than SND_PCM_FORMAT_S16_LE.

Midas Zhou
-------------------------------------------------------------------*/
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <math.h>
#include "egi_pcm.h"
#include "egi_log.h"
#include "egi_debug.h"
#include "egi_timer.h"
#include "egi_cstring.h"

static snd_pcm_t *g_ffpcm_handle;	/* for PCM playback */
static snd_mixer_t *g_volmix_handle; 	/* for volume control */
static bool g_blInterleaved;		/* Interleaved or Noninterleaved */
static char g_snd_device[256]; 		/* Pending, use "default" now. */

/*-------------------------------------------------------------------------------
 Open an PCM device and set following parameters:

 1.  access mode:		(SND_PCM_ACCESS_RW_INTERLEAVED)
 2.  PCM format:		(SND_PCM_FORMAT_S16_LE)
 3.  number of channels:	unsigned int nchan
 4.  sampling rate:		unsigned int srate
 5.  bl_interleaved:		TRUE for INTERLEAVED access,
				FALSE for NONINTERLEAVED access
Return:
a	0  :  OK
	<0 :  fails
-----------------------------------------------------------------------------------*/
int egi_prepare_pcm_device(unsigned int nchan, unsigned int srate, bool bl_interleaved)
{
	int rc;
        snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
        int dir=0;

	/* save interleave mode */
	g_blInterleaved=bl_interleaved;

	/* set PCM device */
	sprintf(g_snd_device,"default");

	/* open PCM device for playblack */
	rc=snd_pcm_open(&g_ffpcm_handle,g_snd_device,SND_PCM_STREAM_PLAYBACK,0);
	if(rc<0)
	{
		EGI_PLOG(LOGLV_ERROR,"%s(): unable to open pcm device '%s': %s\n",
							__func__, g_snd_device, snd_strerror(rc) );
		//exit(-1);
		return rc;
	}

	/* allocate a hardware parameters object */
	snd_pcm_hw_params_alloca(&params);

	/* fill it in with default values */
	snd_pcm_hw_params_any(g_ffpcm_handle, params);

	/* <<<<<<<<<<<<<<<<<<<       set hardware parameters     >>>>>>>>>>>>>>>>>>>>>> */
	/* if interleaved mode */
	if(bl_interleaved)  {
		snd_pcm_hw_params_set_access(g_ffpcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	}
	/* otherwise noninterleaved mode */
	else {
		/* !!!! use noninterleaved mode to play ffmpeg decoded data !!!!! */
		snd_pcm_hw_params_set_access(g_ffpcm_handle, params, SND_PCM_ACCESS_RW_NONINTERLEAVED);
	}

	/* signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(g_ffpcm_handle, params, SND_PCM_FORMAT_S16_LE);

	/* set channel	*/
	snd_pcm_hw_params_set_channels(g_ffpcm_handle, params, nchan);

	/* sampling rate */
	snd_pcm_hw_params_set_rate_near(g_ffpcm_handle, params, &srate, &dir);
	if(dir != 0)
		printf("%s: Actual sampling rate is set to %d HZ!\n",__func__, srate);

	/* set HW params */
	rc=snd_pcm_hw_params(g_ffpcm_handle,params);
	if(rc<0) /* rc=0 on success */
	{
		EGI_PLOG(LOGLV_ERROR,"unable to set hw parameter: %s\n",snd_strerror(rc));
		return rc;
	}

	/* get period size */
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
//	EGI_PDEBUG(DBG_NONE, "%s: snd pcm period size = %d frames\n",__func__, (int)frames);
	printf("%s: snd pcm period size = %d frames\n",__func__, (int)frames);

	return rc;
}

/*----------------------------------------------
  close pcm device and free resources
  together with volmix.
----------------------------------------------*/
void egi_close_pcm_device(void)
{
	if(g_ffpcm_handle != NULL) {
		snd_pcm_drain(g_ffpcm_handle);
		snd_pcm_close(g_ffpcm_handle);
		g_ffpcm_handle=NULL;
	}

	if(g_volmix_handle !=NULL) {
		snd_mixer_close(g_volmix_handle);
		g_volmix_handle=NULL;
	}
}


/*-----------------------------------------------------------------------
send buffer data to pcm play device with NONINTERLEAVED frame format !!!!
PCM access mode MUST have been set properly in open_pcm_device().

buffer ---  point to pcm data buffer
nf     ---  number of frames

Return:
#	>0    OK
#	<0   fails
------------------------------------------------------------------------*/
void  egi_play_pcm_buff(void ** buffer, int nf)
{
	int rc;

	/* write interleaved frame data */
	if(g_blInterleaved)
	        rc=snd_pcm_writei(g_ffpcm_handle,buffer,(snd_pcm_uframes_t)nf );
	/* write noninterleaved frame data */
	else
       	        rc=snd_pcm_writen(g_ffpcm_handle,buffer,(snd_pcm_uframes_t)nf ); //write to hw to playback
        if (rc == -EPIPE)
        {
            /* EPIPE means underrun */
            //fprintf(stderr,"snd_pcm_writen() or snd_pcm_writei(): underrun occurred\n");
            EGI_PDEBUG(DBG_PCM,"[%lld]: snd_pcm_writen() or snd_pcm_writei(): underrun occurred\n",
            						tm_get_tmstampms() );
	    snd_pcm_prepare(g_ffpcm_handle);
        }
	else if(rc<0)
        {
        	//fprintf(stderr,"error from writen():%s\n",snd_strerror(rc));
		EGI_PLOG(LOGLV_ERROR,"%s: error from writen():%s\n",__func__, snd_strerror(rc));
        }
        else if (rc != nf)
        {
                //fprintf(stderr,"short write, write %d of total %d frames\n",rc, nf);
		EGI_PLOG(LOGLV_ERROR,"%s: short write, write %d of total %d frames\n", __func__,rc,nf);
        }
}

/*-------------------------------------------------------------------------------
Get current volume value from the first available channel, and then set all
volume to the given value.

!!! --- WARNING: Static variabls applied, for one running thread only --- !!!

pgetvol: 	[0-100], pointer to a value to pass the volume percentage value.
		If NULL, ignore.
		Forced to [0-100] if out of range.

psetvol: 	[0-100], pointer to a volume value of percentage*100, which
		is about to set to the mixer.
		If NULL, ignore.
		Forced to [0-100] if out of range.

Return:
	0	OK
	<0	Fails
------------------------------------------------------------------------------*/
int egi_getset_pcm_volume(int *pgetvol, int *psetvol)
{
	int ret;
	static long  min=-1; /* selem volume value limit */
	static long  max=-2;
	static long  vrange;
	long int     vol;

	static long  dBmin=-50; /* selem volume value limit */
	static long  dBmax=-50;
	static long  dBvrange;
	long int     dBvol;

	static snd_mixer_selem_id_t *sid;
	static snd_mixer_elem_t* elem;
	static snd_mixer_selem_channel_id_t chn;
	const char *card="default";
	static char selem_name[128]={0};
	static bool has_selem=false;
	static bool finish_setup=false;

    /* First time init */
    if( !finish_setup || g_volmix_handle==NULL )
    {
	if( g_volmix_handle==NULL)
		EGI_PLOG(LOGLV_CRITICAL,"%s: A g_volmix_handle is about to open...",__func__);

	/* must reset token first */
	has_selem=false;
	finish_setup=false;

        /* read egi.conf and get selem_name */
        if ( !has_selem ) {
		if(egi_get_config_value("EGI_SOUND","selem_name",selem_name) != 0) {
			EGI_PLOG(LOGLV_ERROR,"%s: Fail to get config value 'selem_name'.",__func__);
			has_selem=false;
	                return -1;
		}
		else {
			EGI_PLOG(LOGLV_INFO,"%s: Succeed to get config value selem_name='%s'",
										__func__, selem_name);
			has_selem=true;
		}
	}

	//printf(" --- Selem: %s --- \n", selem_name);

	/* open an empty mixer for volume control */
	ret=snd_mixer_open(&g_volmix_handle,0); /* 0 unused param*/
	if(ret!=0){
		EGI_PLOG(LOGLV_ERROR, "%s: Open mixer fails: %s",__func__, snd_strerror(ret));
		ret=-1;
		goto FAILS;
	}

	/* Attach an HCTL specified with the CTL device name to an opened mixer */
	ret=snd_mixer_attach(g_volmix_handle,card);
	if(ret!=0){
		EGI_PLOG(LOGLV_ERROR, "%s: Mixer attach fails: %s", __func__, snd_strerror(ret));
		ret=-2;
		goto FAILS;
	}

	/* Register mixer simple element class */
	ret=snd_mixer_selem_register(g_volmix_handle,NULL,NULL);
	if(ret!=0){
		EGI_PLOG(LOGLV_ERROR,"%s: snd_mixer_selem_register(): Mixer simple element class register fails: %s",
									__func__, snd_strerror(ret));
		ret=-3;
		goto FAILS;
	}

	/* Load a mixer element	*/
	ret=snd_mixer_load(g_volmix_handle);
	if(ret!=0){
		EGI_PLOG(LOGLV_ERROR,"%s: Load mixer element fails: %s",__func__, snd_strerror(ret));
		ret=-4;
		goto FAILS;
	}

	/* alloc snd_mixer_selem_id_t */
	snd_mixer_selem_id_alloca(&sid);
	/* Set index part of a mixer simple element identifier */
	snd_mixer_selem_id_set_index(sid,0);
	/* Set name part of a mixer simple element identifier */
	snd_mixer_selem_id_set_name(sid,selem_name);

	/* Find a mixer simple element */
	elem=snd_mixer_find_selem(g_volmix_handle,sid);
	if(elem==NULL){
		EGI_PLOG(LOGLV_ERROR, "%s: Fail to find mixer simple element '%s'.",__func__, selem_name);
		ret=-5;
		goto FAILS;
	}

	/* Get range for playback volume of a mixer simple element */
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	if( min<0 || max<0 ){
		EGI_PLOG(LOGLV_ERROR,"%s: Get range of volume fails.!",__func__);
		ret=-6;
		goto FAILS;
	}
	vrange=max-min+1;
	EGI_PLOG(LOGLV_CRITICAL,"%s: Get playback volume range Min.%ld - Max.%ld.", __func__, min, max);

	/* Get dB range for playback volume of a mixer simple element
	 * dBmin and dBmax in dB*100
	 */
	ret=snd_mixer_selem_get_playback_dB_range(elem, &dBmin, &dBmax);
	if( ret !=0 ){
		EGI_PLOG(LOGLV_ERROR,"%s: Get range of playback dB range fails.!",__func__);
		ret=-6;
		goto FAILS;
	}
	dBvrange=dBmax-dBmin+1;
	EGI_PLOG(LOGLV_CRITICAL,"%s: Get playback volume dB range Min.dB%ld - Max.dB%ld.",
											__func__, dBmin, dBmax);


	/* get volume , ret=0 Ok */
        for (chn = 0; chn < 32; chn++) {
		/* Master control channel */
                if (chn == 0 && snd_mixer_selem_has_playback_volume_joined(elem)) {
			EGI_PLOG(LOGLV_CRITICAL,"%s: '%s' channle 0, Playback volume joined!",
										__func__, selem_name);
			finish_setup=true;
                        break;
		}
		/* Other control channel */
                if (!snd_mixer_selem_has_playback_channel(elem, chn))
                        continue;
		else {
			finish_setup=true;
			break;
		}
        }

	/* Fail to find a control channel */
	if(finish_setup==false) {
		ret=-8;
		goto FAILS;
	}

     }	/*  ---------   Now we finish first setup  ---------  */


	/* try to get playback volume value on the channel */
	snd_mixer_handle_events(g_volmix_handle); /* handle events first */

	ret=snd_mixer_selem_get_playback_volume(elem, chn, &vol); /* suppose that each channel has same volume value */
	if(ret<0) {
		EGI_PLOG(LOGLV_ERROR,"%s: Get playback volume error on channle %d.",
										__func__, chn);
		ret=-7;
		goto FAILS;
	}

	/* try to get playback dB value on the channel */
	ret=snd_mixer_selem_get_playback_dB(elem, chn, &dBvol); /* suppose that each channel has same volume value */
	if(ret<0) {
		EGI_PLOG(LOGLV_ERROR,"%s: Get playback dB error on channle %d.",
										__func__, chn);
		ret=-7;
		goto FAILS;
	}


	if( pgetvol!=NULL ) {
		//printf("%s: Get volume: %ld[%d] on channle %d.\n",__func__,vol,*pgetvol,chn);

		/*** Convert vol back to percentage value.
		 *  	--- NOT GOOD! ---
		 *      pv=percentage*100; v=volume value in vrange.
		 *	pv/100=lg(v^2)/lg(vrange^2)
		 *	pv=lg(v^2)*100/lg(vrange^2)
		 */
		 //printf("get alsa vol=%ld\n",vol);
		 // *pgetvol=log10(vol)*100.0/log10(vrange);  /* in percent*100 */
		*pgetvol=vol*100/vrange;  /* actual volume value to percent. value */
	}

	/* set volume, ret=0 OK */
	//snd_mixer_selem_set_playback_volume_all(elem, val);
	//snd_mixer_selem_set_playback_volume(elem, chn, val);
	//snd_mixer_selem_set_playback_volume_range(elem,0,32);
	// set_volume_mute_value()

	/* set volume, ret=0 OK */
	if(psetvol != NULL) {
		/* limit input to [0-100] */
		printf("%s: min=%ld, max=%ld vrange=%ld\n",__func__, min,max, vrange);
		if(*psetvol > 100)
			*psetvol=100;

		/*** Covert percentage value to lg(pv^2) related value, so to sound dB relative.
		 *  	--- NOT GOOD! ---
		 *      pv=percentage*100; v=volume value in vrange.
		 *	pv/100=lg(v^2)/lg(vrange^2)
		 *	v=10^(pv*lg(vrange)/100)
		 */
		vol=*psetvol;  /* vol: in percent*100 */
		if(vol<1)vol=1; /* to avoid 0 */

		#if 0 /* Call snd_mixer_selem_set_playback_volume_all() */
		//vol=pow(10, vol*log10(vrange)/100.0);
		vol=vol*vrange/100;  /* percent. vol to actual volume value  */
		printf("%s: Percent. vol=%d%%, dB related vol=%ld\n", __func__, *psetvol, vol);

		/* normalize vol, Not necessay now?  */
		if(vol > max) vol=max;
		else if(vol < min) vol=min;
	        snd_mixer_selem_set_playback_volume_all(elem, vol);
		//printf("Set playback all volume to: %ld.\n",vol);

		#else
		dBvol=dBvrange*vol/100;
		/* dB_all(elem, vol, dir)  vol: dB*100 dir>0 round up, otherwise down */
	        snd_mixer_selem_set_playback_dB_all(elem, dBvol, 1);
		#endif

	}

	return 0;

 FAILS:
	snd_mixer_close(g_volmix_handle);
	g_volmix_handle=NULL;

	return ret;
}


