#ifndef   ___MATHWORK__H__
#define   ___MATHWORK__H__


/*----------------------------------------------
 calculate and return time difference in us

Return
  if tm_start > tm_end then return 0 !!!!
  us
----------------------------------------------*/
inline uint32_t get_costtimeus(struct timeval tm_start, struct timeval tm_end) {
        uint32_t time_cost;
        if(tm_end.tv_sec>tm_start.tv_sec)
                time_cost=(tm_end.tv_sec-tm_start.tv_sec)*1000000+(tm_end.tv_usec-tm_start.tv_usec);
        //--- also consider tm_sart > tm_end !!!!!!!
        else if( tm_end.tv_sec==tm_start.tv_sec )
        {
                time_cost=tm_end.tv_usec-tm_start.tv_usec;
                time_cost=time_cost>0 ? time_cost:0;
        }
        else
                time_cost=0;

        return time_cost;
}


/*-------------------------------------------------------------------------------
               Integral of time difference and fx[num]

1. Only ONE instance is allowed, since timeval and sum_dt is static var.
2. In order to get a regular time difference,you shall call this function
in a loop.
dt_us will be 0 at first call
3. parameters:
	*num ---- number of integral groups.
	*fx ---- [num] functions of time,whose unit is us(10^-6 s) !!!
	*sum ----[num] results integration(summation) of fx * dt_us in sum[num]

return:
	summation of dt_us
-------------------------------------------------------------------------------*/
inline uint32_t math_tmIntegral_NG(uint8_t num, const double *fx, double *sum)
{
	   uint32_t dt_us;//time in us
	   static struct timeval tm_end,tm_start;
	   static struct timeval tm_start_1, tm_start_2;
	   static double sum_dt; //summation of dt_us 
	   int i;

	   gettimeofday(&tm_end,NULL);// !!!--end of read !!!
           //------  to minimize error between two timer functions
           gettimeofday(&tm_start,NULL);// !!! --start time !!!
	   tm_start_2 = tm_start_1;
	   tm_start_1 = tm_start;
           //------ get time difference for integration calculation
	   if(tm_start_2.tv_sec != 0) 
	           dt_us=get_costtimeus(tm_start_2,tm_end); //return  0 if tm_start > tm_end
	   else // discard fisrt value
		   dt_us=0;

  	   sum_dt +=dt_us;

	   //------ integration calculation -----
	   for(i=0;i<num;i++)
	   {
         	  sum[i] += fx[i]*(double)dt_us;
	   }

	   return sum_dt;
}


/*------------  single group time integral  -------------------*/
inline uint32_t math_tmIntegral(const double fx, double *sum)
{
	   uint32_t dt_us;//time in us
	   static struct timeval tm_end,tm_start;
	   static struct timeval tm_start_1, tm_start_2;
	   static double sum_dt; //summation of dt_us 

	   gettimeofday(&tm_end,NULL);// !!!--end of read !!!
           //------  to minimize error between two timer functions
           gettimeofday(&tm_start,NULL);// !!! --start time !!!
	   tm_start_2 = tm_start_1;
	   tm_start_1 = tm_start;
           //------ get time difference for integration calculation
	   if(tm_start_2.tv_sec != 0) 
	           dt_us=get_costtimeus(tm_start_2,tm_end); //return  0 if tm_start > tm_end
	   else // discard fisrt value
		   dt_us=0;

	   sum_dt +=dt_us;

           (*sum) += fx*dt_us;

	   return sum_dt;
}

#endif
