#include <stdio.h>
#include <inttypes.h>
#include <sndfile.h>
//#include <ao/ao.h>

#define SF_CONTAINER(x)		((x) & SF_FORMAT_TYPEMASK)
#define SF_CODEC(x)			((x) & SF_FORMAT_SUBMASK)
#define CASE_NAME(x)            case x : return #x ; break ;

const char *
str_of_major_format (int format)
{       switch (SF_CONTAINER (format))
        {       CASE_NAME (SF_FORMAT_WAV) ;
                CASE_NAME (SF_FORMAT_AIFF) ;
                CASE_NAME (SF_FORMAT_AU) ;
                CASE_NAME (SF_FORMAT_RAW) ;
                CASE_NAME (SF_FORMAT_PAF) ;
                CASE_NAME (SF_FORMAT_SVX) ;
                CASE_NAME (SF_FORMAT_NIST) ;
                CASE_NAME (SF_FORMAT_VOC) ;
                CASE_NAME (SF_FORMAT_IRCAM) ;
                CASE_NAME (SF_FORMAT_W64) ;
                CASE_NAME (SF_FORMAT_MAT4) ;
                CASE_NAME (SF_FORMAT_MAT5) ;
                CASE_NAME (SF_FORMAT_PVF) ;
                CASE_NAME (SF_FORMAT_XI) ;
                CASE_NAME (SF_FORMAT_HTK) ;
                CASE_NAME (SF_FORMAT_SDS) ;
                CASE_NAME (SF_FORMAT_AVR) ;
                CASE_NAME (SF_FORMAT_WAVEX) ;
                CASE_NAME (SF_FORMAT_SD2) ;
                CASE_NAME (SF_FORMAT_FLAC) ;
                CASE_NAME (SF_FORMAT_CAF) ;
                CASE_NAME (SF_FORMAT_WVE) ;
                CASE_NAME (SF_FORMAT_OGG) ;
                default :
                        break ;
                } ;

        return "BAD_MAJOR_FORMAT" ;
} /* str_of_major_format */

const char *
str_of_minor_format (int format)
{       switch (SF_CODEC (format))
        {       CASE_NAME (SF_FORMAT_PCM_S8) ;
                CASE_NAME (SF_FORMAT_PCM_16) ;
                CASE_NAME (SF_FORMAT_PCM_24) ;
                CASE_NAME (SF_FORMAT_PCM_32) ;
                CASE_NAME (SF_FORMAT_PCM_U8) ;
                CASE_NAME (SF_FORMAT_FLOAT) ;
                CASE_NAME (SF_FORMAT_DOUBLE) ;
                CASE_NAME (SF_FORMAT_ULAW) ;
                CASE_NAME (SF_FORMAT_ALAW) ;
                CASE_NAME (SF_FORMAT_IMA_ADPCM) ;
                CASE_NAME (SF_FORMAT_MS_ADPCM) ;
                CASE_NAME (SF_FORMAT_GSM610) ;
                CASE_NAME (SF_FORMAT_VOX_ADPCM) ;
                CASE_NAME (SF_FORMAT_G721_32) ;
                CASE_NAME (SF_FORMAT_G723_24) ;
                CASE_NAME (SF_FORMAT_G723_40) ;
                CASE_NAME (SF_FORMAT_DWVW_12) ;
                CASE_NAME (SF_FORMAT_DWVW_16) ;
                CASE_NAME (SF_FORMAT_DWVW_24) ;
                CASE_NAME (SF_FORMAT_DWVW_N) ;
                CASE_NAME (SF_FORMAT_DPCM_8) ;
                CASE_NAME (SF_FORMAT_DPCM_16) ;
                CASE_NAME (SF_FORMAT_VORBIS) ;
                default :
                        break ;
                } ;

        return "BAD_MINOR_FORMAT" ;
} /* str_of_minor_format */



int main(void)
{
	SNDFILE *snf;
	SF_INFO  sinfo={.format=0 };

	snf=sf_open("/tmp/widora.wav", SFM_READ, &sinfo);
	if(snf==NULL) {
		printf("Fail to open wav file!\n");
		return -1;
	}

	printf("--- wav file info --- \n");
	printf("Frames:		%"PRIu64"\n", sinfo.frames);
	printf("Sample Rate:	%d\n", sinfo.samplerate);
	printf("Channels:	%d\n", sinfo.channels);
	printf("Formate:  	%s %s\n", str_of_major_format(sinfo.format),
					  str_of_minor_format(sinfo.format) );
	printf("Sections:	%d\n", sinfo.sections);
	printf("seekable:	%d\n", sinfo.seekable);

	/*
	sf_read(write)_short( SNDFILE *sndfile, short *ptr, sf_count_t items);
	sf_read(write)_int(...)
	sf_read(write)_float(...)
	sf_read(write)_double(...)
	sf_readf(write)_... 		[ sf_count_t frames ]

	*/

	sf_close(snf);

return 0;
}

