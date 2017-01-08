//This program uses the OSS library.
#include <sys/ioctl.h> //for ioctl()
#include <math.h> //sin(), floor(), and pow()
#include <stdio.h> //perror
#include <fcntl.h> //open, O_WRONLY
//#include <soundcard.h> //SOUND_PCM*
#include <alsa/asoundlib.h>
//#include "/home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/include/alsa/asoundlib.h"
//#include "/home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/include/alsa/asoundef.h"
//#include "/home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29/user_headers/include/linux/soundcard.h"
#include <iostream>
#include <unistd.h>
using namespace std;
 
#define TYPE char
#define LENGTH 1 //number of seconds per frequency
#define RATE 48000 //sampling rate
#define SIZE sizeof(TYPE) //size of sample, in bytes
#define CHANNELS 1 //number of audio channels
#define PI 3.14159
#define NUM_FREQS 3 //total number of frequencies
#define BUFFSIZE (int) (NUM_FREQS*LENGTH*RATE*SIZE*CHANNELS) //bytes sent to audio device
#define ARRAYSIZE (int) (NUM_FREQS*LENGTH*RATE*CHANNELS) //total number of samples
#define SAMPLE_MAX (pow(2,SIZE*8 - 1) - 1) 
 
void writeToSoundDevice(TYPE buf[], int deviceID) {
      int status;
      status = write(deviceID, buf, BUFFSIZE);
      if (status != BUFFSIZE)
            perror("Wrote wrong number of bytes\n");
      status = ioctl(deviceID, SNDCTL_DSP_SYNC, 0);
      if (status == -1)
            perror("SNDCTL_DSP_SYNC failed\n");
}
 
int main() {
      int deviceID, arg, status, f, t, a, i;
      TYPE buf[ARRAYSIZE];
      deviceID = open("/dev/audio", O_WRONLY, 0);
      if (deviceID < 0)
            perror("Opening /dev/dsp failed\n");
// working
      arg = SIZE * 8;
      status = ioctl(deviceID, SNDCTL_DSP_SETFMT, &arg);
      if (status == -1)
            perror("Unable to set sample size\n");
      arg = CHANNELS;
      status = ioctl(deviceID, SNDCTL_DSP_CHANNELS, &arg);
      if (status == -1)
            perror("Unable to set number of channels\n");
   
      arg = RATE;
      status = ioctl(deviceID, SNDCTL_DSP_SPEED, &arg);
      if (status == -1)
            perror("Unable to set sampling rate\n");
 
      a = SAMPLE_MAX;
      for (i = 0; i < NUM_FREQS; ++i) {
            switch (i) {
                  case 0:
                        f = 262;
                        break;
                  case 1:
                        f = 330;
                        break;
                  case 2:
                        f = 392;
                        break;
            }
            for (t = 0; t < ARRAYSIZE/NUM_FREQS; ++t) {
                  buf[t + ((ARRAYSIZE / NUM_FREQS) * i)] = floor(a * sin(2*PI*f*t/RATE));
            }
      }
      writeToSoundDevice(buf, deviceID);
     // fflush(deviceID);
      close(deviceID);
}
