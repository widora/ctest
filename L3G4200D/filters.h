#ifndef __FILTER__H__
#define __FILTER__H__

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#define PI 3.1415926535898
#define INT16MAF_MAX_GRADE 10  //max grade limit for int16MA filter, 2^10 points moving average.


//---- int16_t  moving average filter Data Base ----
struct  int16MAFilterDB {
	uint16_t f_ng; //grade number,   point number = 2^ng
	//----allocate 2^ng+2 elements actually, 2 more expanded slot fors limit filter check !!!
	int16_t* f_buff;//buffer for averaging data,size=(2^ng+2) !!!
        int32_t f_sum; //sum of the buff data
        int16_t f_limit;//=0x7fff, //limit value for each data of *source.
};


//------------------------     FUNCTION DECLARATION     ---------------------------
inline int16_t int16_MAfilter(struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov);
inline int Init_int16MAFilterDB(struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit);
inline void Release_int16MAFilterDB(struct int16MAFilterDB *fdb);
inline void int16_MAfilter_NG(uint8_t m, struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov);
inline int Init_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit);
inline void Release_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb);
static void IIR_Lowpass_int16Filter(int16_t *p_in_data, int16_t *p_out_data, int nmov);
static void IIR_Lowpass_dblFilter(double *p_in_data, double *p_out_data, int nmov);


#endif
