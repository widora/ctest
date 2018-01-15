#ifndef __FILTER__H__
#define __FILTER__H__

#include <stdio.h>
#include <math.h>
#include <stdint.h>


#define INT16MAF_MAX_GRADE 10  //max grade limit for int16MA filter, 2^10 points moving average.

//---- int16_t  moving average filter Data Base ----
struct  int16MAFilterDB {
	uint16_t f_ng; //grade number,   point number = 2^ng
	int16_t* f_buff;//buffer for averaging data
        int32_t f_sum; //sum of the buff data
        int16_t f_limit;//=0x7fff, //limit value for each data of *source.
};

/*----------------------------------------------------------------------
            ***  Moving Average Filter   ***
!!!Reset the filter data base struct every time before call the function, to clear old data in f_buff and f_sum.
Filter a data stream until it ends, then reset the filter context struct before filter another.

To filter data you must iterate nmov one by one:
         source[nmov] ----> filter ----> dest[nmov]

*fdb:     int16 MAFilter data base
*source:  raw data input
*dest:    where you put your filtered data
	  (source and dest may be the same)
nmov:     nmov_th step of moving average calculation

Return:
	The last 2^f_ng pionts average value.
	0  if fails
--------------------------------------------------------------------------*/
inline static int16_t int16_MAfilter(struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov)
{
	int i;
	int np=1<<(fdb->f_ng); //2^np points average 

	//----- verify filter data base
	if(fdb->f_buff == NULL)
	{
		fprintf(stderr,"int16_MA16P_filter(): int16MAFilter data base has not been initilized yet!\n");
		*dest=0;
		return 0;
	}

	//----- deduce old data from f_sum
	fdb->f_sum -= fdb->f_buff[0];

	//----- shift filter buff data, get rid of the oldest data
	for(i=0; i<np-1; i++)
	{
		fdb->f_buff[i]=fdb->f_buff[i+1];
	}

	//----- get new data to fdb->f_buff[np-1] --------
	//----- reassure limit first
	//fdb->f_limit = abs(fdb->f_limit);

	//----------------------- Deal with Limits ----------------
#if 0	//     Method 1:  as absolute limit 
	if( source[nmov] > fdb->f_limit )
	{
		fdb->f_buff[np-1]=fdb->f_limit;
	}
	else if (source[nmov] < -1*(fdb->f_limit) )
	{
		fdb->f_buff[np-1]=-1*(fdb->f_limit);
	}
#endif

#if 1	//     Method 2:  as relative limit, comparing with previous fdb->f_buff[]
	if( source[nmov] > (fdb->f_buff[np-2]+fdb->f_limit) ) //check up limit
	{
		fdb->f_buff[np-1]=fdb->f_buff[np-2]; // use previous data
	}
	else if (source[nmov] < ( fdb->f_buff[np-2]-fdb->f_limit) )//check low limit
	{
		fdb->f_buff[np-1]=fdb->f_buff[np-2]; // use previous data
	}
#endif

        // ------ else if the value is within normal scope
	else
		fdb->f_buff[np-1]=source[nmov];




	fdb->f_sum += fdb->f_buff[np-1]; //add new data to f_sum

	//----- update dest with average value
	dest[nmov]=(fdb->f_sum)>>(fdb->f_ng);

	return dest[nmov];
}

/*----------------------------------------------------------------------------
Init. int16 MA filter strut with given value

*fdb --- filter data buffer base
ng  --- filter grade number,  2^ng points average.
limit  --- limit value for input data, above which the value will be trimmed .
Return:
	0    OK
	<0   fails
--------------------------------------------------------------------------*/
inline static int Init_int16MAFilterDB(struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit)
{
//	int i;
	int np;

	//---- clear struct
	memset(fdb,0,sizeof(struct int16MAFilterDB)); 

	//---- set grade
	if(ng>INT16MAF_MAX_GRADE)
	{
		printf("Max. Filter grade is 10!\n");
		fdb->f_ng=INT16MAF_MAX_GRADE;
	}
	else
		fdb->f_ng=ng;

	//---- set np
	np=1<<(fdb->f_ng);
	fprintf(stdout," %d points moving average filter initilizing...\n",np);

	//---- set limit
	fdb->f_limit=abs(limit);
	//---- clear sum
	fdb->f_sum=0;
	//---- allocate mem for buff data
	fdb->f_buff = malloc( np*sizeof(int16_t) );
	if(fdb->f_buff == NULL)
	{
		fprintf(stderr,"Init_int16MAFilterDB(): malloc f_buff failed!\n");
		return -1;
	}

	//---- clear buff data if f_buff 
	memset(fdb->f_buff,0,np*sizeof(int16_t));

	return 0;
}


/*----------------------------------------------------------------------
Release int16 MA filter struct
----------------------------------------------------------------------*/
void Release_int16MAFilterDB(struct int16MAFilterDB *fdb)
{
	if(fdb->f_buff != NULL)
		free(fdb->f_buff);

}


/*--------------------------------------------------------
<<<<<<<<    multi group moving average filter   >>>>

m:        total group number of filtering data
**fdb:     int16 MAFilter data base in [m] groups
**source:  raw data input in [m] groups
**dest:    where you put your filtered data in [m] groups
	  (source and dest may be the same)
nmov:     nmov_th step of moving average calculation

!!!!! To filter data you must iterate nmov one by one  !!!!!

---------------------------------------------------------*/
inline static void int16_MAfilter_NG(uint8_t m, struct int16MAFilterDB *fdb, const int16_t *source, int16_t *dest, int nmov)
{
	int i;

	for(i=0; i<m; i++)
	{
		int16_MAfilter(fdb+i,source+i,dest+i,nmov);
	}

}

/*----------------------------------------------------------------------------
       <<<<<<<<    multi group moving average filter   >>>>

Init. int16 MA filter strut with given value
**fdb --- filter data buffer base in [m] groups
ng  --- filter grade number,  2^ng points average.
limit  --- limit value for input data, above which the value will be trimmed .
Return:
	0    OK
	<0   fails
--------------------------------------------------------------------------*/
inline static int Init_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb, uint16_t ng, int16_t limit)
{
	int i;
	for(i=0; i<m; i++)
	{
              if( Init_int16MAFilterDB(fdb+i, ng, limit)<0 )
			return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------
       <<<<<<<<    multi group moving average filter   >>>>

Release [m] groups of int16 MA filter struct
----------------------------------------------------------------------*/
void Release_int16MAFilterDB_NG(uint8_t m, struct int16MAFilterDB *fdb)
{
	int i;
	for(i=0; i<m; i++)
	{
	   Release_int16MAFilterDB(fdb+i);
	}
}



#endif
