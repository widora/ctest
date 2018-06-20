/*-------------------------------------------------------------------------------------------------
Common functions and definitions for libusb applications

--------------------------------------------------------------------------------------------------*/
#ifndef __USB_COMMON_H
#define __USB_COMMON_H

#include <stdio.h>
#include <sys/types.h>
#include <libusb.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h> //malloc
#include <string.h> //memset


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


#endif
