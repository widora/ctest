#ifndef __FILTER__H__
#define __FILTER__H__

#include <stdio.h>
#include <math.h>
#include <stdint.h>

//---- 16 points moving average filter context ----
struct  MA16_int16_FilterCtx {
	int16_t f_buff[16];//={0}, //buffer for the 16 data
        int32_t f_sum;//=0, //sum of the 16 data
        int16_t f_limit;//=0x7fff, //limit value for each data of *source.
};

/*----------------------------------------------------------------------
            ***  16 points Moving Average Filter   ***

!!!!!! WARNING : There shall be only ONE instance for each filter context gloablly !!!!!!!
Reset the filter context struct every time before call the function, to clear old data in f_buff and f_sum.
Filter a data stream until it ends, then reset the filter context struct before filter another.

*fctx:  Filter context struct
*source:  where you put your raw data
*dest:    where you put your filtered data
	  (source and dest may be the same)
nmov:     nmov_th step of moving average calculation

To filter data you must iterate nmov one by one

Return:
	The last 8 pionts average value.
--------------------------------------------------------------------------*/
static int16_t int16_MA16P_filter(struct MA16_int16_FilterCtx *fctx,int16_t *source, int16_t *dest, int nmov)
{
	int i;

	//----- shift filter buff data
	fctx->f_sum -= fctx->f_buff[0]; // deduce old data from f_sum

	for(i=0;i<16-1;i++)
	{
		fctx->f_buff[i]=fctx->f_buff[i+1];
	}

	//----- get new data and put in
	//----- check limit first
	fctx->f_limit = abs(fctx->f_limit);
	if( source[nmov] > fctx->f_limit )
	{
		fctx->f_buff[16-1]=fctx->f_limit;
	}
	else if (source[nmov] < -1*(fctx->f_limit) )
	{
		fctx->f_buff[16-1]=-1*(fctx->f_limit);
	}
	else
		fctx->f_buff[16-1]=source[nmov];

	fctx->f_sum += source[nmov]; //add new data to f_sum

	//----- update dest with average value
	dest[nmov]=(fctx->f_sum)/16;

	return dest[nmov];
}

/*----------------------------------------------------------------------
Reset int16 MA16P filter strut with given value
----------------------------------------------------------------------*/
void reset_MA16_filterCtx(struct MA16_int16_FilterCtx *f_ctx,int16_t limit)
{
	int i;

	//---- clear sum 
	f_ctx->f_sum=0;
	//---- set limit
	f_ctx->f_limit=abs(limit);
	//---- clear buff data
	for(i=0;i<16;i++)
		f_ctx->f_buff[i]=0;
}


#endif
