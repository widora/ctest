#include "egi_pcm.h"


#define AVG_PCMBUFF_SIZE	4096

static unsigned char pcm_buff[AVG_PCMBUFF_SIZE];	/* PCM data buffer */
static bool	     effect_launch;			/* Sound of launching a bullet/missle */

