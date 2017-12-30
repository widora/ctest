/*------------------------------------------------------------------
 Based on:
	   www.itwendao.com/article/detail/420944.html

some explanation:
        sample: usually 8bits or 16bits, one sample data width.
        channel: 1-Mono. 2-Stereo
        frame: sizeof(one sample)*channels
        rate: frames per second
        period: Max. frame numbers hardware can be handled each time. (different value for PLAYBACK and CAPTURE!!)
                running show: 1536 frames for CAPTURE; 278 frames for PLAYBACK
        chunk: frames receive from/send to hardware each time.
        buffer: N*periods
        interleaved mode:record period data frame by frame, such as  frame1(Left sample,Right sample),frame2(), ......
        uninterleaved mode: record period data channel by channel, such as period(Left sample,Left ,left...),period(right,right..$
-----------------------------------------------------------------------------------------------------*/
#ifndef __PLAY_PCM_H__
#define __PLAY_PCM_H__

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <stdint.h>
#include <alsa/asoundlib.h>

snd_pcm_t *g_ffpcm_handle=NULL;

int prepare_pcm_device(unsigned int nchan, unsigned int srate);
void close_pcm_device(void);
void play_pcm_buff(uint8_t * buffer, int nf);

#endif
