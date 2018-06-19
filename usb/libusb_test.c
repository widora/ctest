/*--------------------------------------------------------------------------------------------------------------------
Reference: https://blog.csdn.net/kangear/article/details/32176659

1. The Max. packet size for the interrupt endpoints:
	64bytes for full-speed USB
	1024bytes for high-speed USB
2. For interrupt transfer, endpoint Min. polling interval is 1ms, better at 2ms.
   bInterval value to polling interval: 2^(bInterval-1)ms
3. If several USB devices attached have the same PID and VID,libusb_open_device_with_vid_pid() only gives you the first device
   You should use libusb_get_device_list()
4  ???? when to call libusb_free_device_list()? or not at all?  afer you open the device !!??
---------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <libusb.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h> //malloc
#include <string.h> //memset

/*----- VID and PID -----*/
//#define IdVendor 0xc251
//#define IdProduct 0x1c01
#define IdVendor 0x8888
#define IdProduct 0x8888



/*----------------------------------------------------------------------
Open usb device with PID&VID, and claim a interface

Parameters:
        ctx     	USB device context
	dev_handle	handle
	interface	interface number of the device
	idVendor,idProduct  VID&PID
Return:
	0   Success
	<0  Fail
----------------------------------------------------------------------*/
int open_dev_claim_interface( libusb_context * ctx, libusb_device_handle** pdev_handle, int interface,
			      uint16_t idVendor, uint16_t idProduct )
{
   int ret=0;

   //---- open specific USB device according to its VID &PID ---
   (*pdev_handle)=libusb_open_device_with_vid_pid(ctx,idVendor,idProduct);
   if( (*pdev_handle) == NULL){
	perror("open device Error");
	return -1;
   }
   else
	printf("Device with VID=0x%04x,PID=0x%04x opened successfully!\n",idVendor, idProduct);

   //---- detach kernel driver if necessar ----
   // this will remove kernel HID driver ...
   if(libusb_kernel_driver_active(*pdev_handle,0)==1)
   {
	if(libusb_detach_kernel_driver(*pdev_handle,0)==0)
		printf("Kernel driver detached!\n");
   }

   //---- claim interface 0 of device -----
   ret=libusb_claim_interface(*pdev_handle,0);
   if(ret<0)
   {
	perror("claim interface Error");
	return -2;
   }
   printf("Interface %d claimed.\n", interface);

   return ret;
}



/*----------------------------------------------------------------------
get a list of USB devices with specified PID and VID

parameters:
   ctx     context
   devs    all usb devices to be filtered.
   idVend,idProduct   VID,PID
   r_cnt   count of devices with the same VID&PID

Returns:
      list of USB devices
------------------------------------------------------------------------*/
libusb_device** filter_devs(libusb_context * ctx, uint16_t idVendor, uint16_t idProduct, int * r_cnt)
{
   libusb_device **devs;
   libusb_device *dev;
   int cnt;
   int i=0;

   libusb_device **r_devs=NULL;
   int j=0;


  //---- get USB device list ----
   cnt=libusb_get_device_list(ctx, &devs);
   if(cnt<0)
   {
	perror("libusb_get_devcie_list Error");
	return NULL;
   }

   if(cnt==0)
   {
	printf("No device with given PID&VID is found.\n");
	return NULL; 
   }

   //----- alloc mem for r_devs
   r_devs=malloc(sizeof(libusb_device *)*(cnt+1));
   memset(r_devs, 0, cnt+1); // r_devs[cnt]=NULL as end token;

   //---- filter with PID and VID ----
   while((dev=devs[i++]) != NULL)
   {
	struct libusb_device_descriptor desc;
	int r=libusb_get_device_descriptor(dev, &desc);
	if(r<0)
	{
		fprintf(stderr, "failed to get device descriptor");
		return;
	}

	//----- check  PID and VID ------
	if(desc.idVendor==idVendor && desc.idProduct==idProduct)
	{
		r_devs[j]=dev; // save dev to r_dev[]
		j++;
		//---- print the dev ----
		printf("%04x:%04x (bus %d, device %d)\n",
		desc.idVendor, desc.idProduct,
		libusb_get_bus_number(dev), libusb_get_device_address(dev));
	}
   }

  //---

  r_devs[j]=NULL; //end token
  *r_cnt=j;

  return r_devs;

}



/*---------------------------------------------------------
		          main()
----------------------------------------------------------*/
int main(void)
{
   libusb_device **devs;
   libusb_device **devs_filtered; //devs filtered with the same PID&VIP
   libusb_device_handle *dev_handle;
   libusb_context *ctx=NULL;

   struct libusb_transfer *trans;
   unsigned char data_out[64]={0}; // full speed interrupt packet: max. 64bytes.
   // wMaxPacketSize bytes defined in USB config. descriptor
   unsigned char data_in[4]={0};
   int r;
   ssize_t cnt;
   int i;
   int pv_vol;//volume percentage value
   char strCMD[50]={0};

   struct timeval tm_start,tm_end;
   int tm_used;

   //---- init. device context ---
   r=libusb_init(&ctx);
   if(r<0)
   {
	fprintf(stderr,"libusb_init() Error");
	return r;
   }

   //---- set libusb debug level ----
   //--- activate DEBUG when you compile libusb, otherwise it will not print any message.
   libusb_set_debug(ctx, 3);//LIBUSB_LOG_LEVEL_INFO);

   //---- get USB device list ----
   cnt=libusb_get_device_list(NULL, &devs);
   if(cnt<0)
   {
	fprintf(stderr,"libusb_get_device_list() Error");
	return (int)cnt; //error number
   }


   //---- print out all USB devices ----
   devs_filtered=filter_devs(ctx, IdVendor, IdProduct, &cnt);
   printf("total %d devices found with VID=0x%04x, PID=0x%04x\n",cnt,IdVendor, IdProduct);
   if(cnt==0)
	exit(0);
   printf("wait...\n");
   usleep(990000);

   //--- free dev pointer
   free(devs_filtered);

//return 0;

   r=open_dev_claim_interface(ctx, &dev_handle, 0, IdVendor, IdProduct);
   if(r != 0)
  	  return -1;


   //-------- data transfer --------

   //---- allocate trans ----
//   trans=libusb_alloc_transfer(0);//transfer intended for non-isochronous endpoint should specify an iso_packet count of zero

/*-------------------------------------------------------------------------------
   int libusb_interrupt_transfer(struct libusb_device_handle * dev_handle,
                                unsigned char           endpoint,
                                unsigned char *         data,
                                int                     length,
                                int *                   transferred, (NULL if you don't wish to receive this info.)
                                unsigned int            timeout (in millseconds, 0 for unlimited timeout)
                                )
Note:
    1. The direction of the transfer is inferred from the direction bits of the endpoint address
       For interrupt reads, the length field indicates the max. length of data you are expecting to receive.
       If less data arrives than expected, this function will return that data, so be sure to check the 
        transferred output parameter.
    2. The length field indicates the maxium length of data you are expectin to receive.
    3. Check transferred field for actual data read/writen.
    4. Also check transferred when dealing with a timeout error code. do not assume that timeout conditions indicate
       a complete lack of I/O. Because libusb may split your transfer into a number of chunks, and timeout error may results
       from not completing transferring all chunks.
    5. The default endpoint bInterval value is used as the polling interval.
    6. Return 0 as success.
--------------------------------------------------------------------------------*/
  data_out[64-1]=0x00;
  i=0;
 //------------------ loop transfering data to EP1 ----------
  printf("start looping ...\n");
 while(1)
 {
    //---- shift data ---
    data_out[64-1]+=1;

    gettimeofday(&tm_start,NULL);

    //---- write to USB device EP 1 OUT ---
    r=libusb_interrupt_transfer(dev_handle,0x01,data_out, 64, &cnt, 200); //0x00:host->dev, 0x80:dev->host
    if(r != 0)
    {
	perror("libusb_interrupt_transfer() EP1-Out error");
	printf(" try to reopen the USB device ...\n");
        //---- tryt to re-open specific USB device according to its VID &PID ---
	if(dev_handle != NULL)
	   	libusb_close(dev_handle);//close usb dev. 
	dev_handle=NULL;

	while(!dev_handle)
	{
	   //----- re-open and claim interface 0
	   r=open_dev_claim_interface(ctx, &dev_handle, 0, IdVendor, IdProduct);
  	   if(r != 0){
		usleep(800000);
  	  	continue;
	   }
	   else
		break;
	}//while end

    }//if end


    if(cnt != 64)
	printf("!!!Only %d bytes data have been transferred to EP01. return code r=%d .\n",cnt,r);


    //---- read from EP 1 IN  ---
    r=libusb_interrupt_transfer(dev_handle,0x81,data_in,4, &cnt, 200); //0x00:host->dev, 0x80:dev->host
    if(r != 0)
	perror("libusb_interrupt_transfer() EP1-IN error");
    //printf("%d bytes frome EP1-IN,data_in[]= 0x%02x%02x%02x%02x \n",cnt,data_in[3],data_in[2],data_in[1],data_in[0]);
    gettimeofday(&tm_end,NULL);
    tm_used=(tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000;
    printf("USB EP1 read by interrupt transfer, time used = %dms \n",tm_used);

/*
    //----- control volume -----
    pv_vol=60+40*data_in[0]/256;
    //--- set quiet threshold value
    if(pv_vol<62)pv_vol=0;

    sprintf(strCMD, "amixer set Speaker %d%% >/dev/null", pv_vol);
    printf("%s \n",strCMD);
    system(strCMD);
*/

    //---- espeak ----
/*
    sprintf(strCMD,"espeak --stdout -s 200 %d | aplay -N",data[0]);
    printf("%s \n",strCMD);
    system(strCMD);
*/

//    usleep(500000);
  }

   //---- free trans  ----
//   libusb_free_transfer(trans);

   //----- close dev and exit ctx ----
   libusb_close(dev_handle);//close usb dev. 
   libusb_exit(ctx);//exit usb context.
   printf("exit usb operation context.\n");

   return 0;


}
