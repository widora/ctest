#ifndef __EGI_IWINFO__
#define __EGI_IWINFO__


#define IW_TRAFFIC_SAMPLE_SEC 5 /* in second, time for measuring traffic rate, used to get an average value */

int iw_get_rssi(int *rssi);
int iw_get_speed(int *ws);

#endif
