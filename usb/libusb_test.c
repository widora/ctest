/*---------------------------------------------------------------------------
Reference: https://blog.csdn.net/kangear/article/details/32176659

1. The Max. packet size for the interrupt endpoints:
	64bytes for full-speed USB
	1024bytes for high-speed USB
2. For interrupt transfer, endpoint Min. polling interval is 1ms, better at 2ms.
   bInterval value to polling interval: 2^(bInterval-1)ms
-------------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <libusb.h>

/*----- VID and PID -----*/
#define IdVendor 0xc251
#define IdProduct 0x1c01


static void print_devs(libusb_device **devs)
{
   libusb_device *dev;
   int i=0;

   while((dev=devs[i++]) != NULL)
   {
	struct libusb_device_descriptor desc;
	int r=libusb_get_device_descriptor(dev, &desc);
	if(r<0)
	{
		fprintf(stderr, "failed to get device descriptor");
		return;
	}

	printf("%04x:%04x (bus %d, device %d)\n",
	desc.idVendor, desc.idProduct,
	libusb_get_bus_number(dev), libusb_get_device_address(dev));
   }
}



/*---------------------------------------------------------
		          main()
----------------------------------------------------------*/
int main(void)
{
   libusb_device **devs;
   libusb_device_handle *dev_handle;
   libusb_context *ctx=NULL;
   struct libusb_transfer *trans;
   unsigned char data_out[64]={0}; // full speed interrupt packet: max. 64bytes.
   // wMaxPacketSize bytes defined in USB config. descriptor
   unsigned char data_in[4]={0};
   int r;
   ssize_t cnt;
   int i;
   char strCMD[50]={0};

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
   print_devs(devs);

   //---- open specific USB device according to its VID &PID ---
   dev_handle=libusb_open_device_with_vid_pid(ctx,IdVendor,IdProduct);
   if(dev_handle == NULL)
	perror("Cannot open device\n");
   else
	printf("Device with VID=0x%04x,PID=0x%04x opened successfully!\n",IdVendor, IdProduct);
   //---- free the list, unref the devices only after you have opened  your device !!!! ---
   libusb_free_device_list(devs,1);

   //---- detach kernel driver if necessar ----
   // this will remove hid driver ...
   if(libusb_kernel_driver_active(dev_handle,0)==1)
   {
	if(libusb_detach_kernel_driver(dev_handle,0)==0)
		printf("Kernel driver detached!\n");
   }

   //---- claim interface 0 of device -----
   r=libusb_claim_interface(dev_handle,0);
   if(r<0)
   {
	perror("Fail to claim interface 0.\n");
	return 1;
   }
   printf("Interface 0 claimed.\n");


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
 while(1)
 {
    //---- shift data ---
    data_out[64-1]+=1;

/*
    //---- write to USB device EP 1 OUT ---
    r=libusb_interrupt_transfer(dev_handle,0x01,data_out, 64, &cnt, 200); //0x00:host->dev, 0x80:dev->host
    if(r != 0)
	perror("libusb_interrupt_transfer() EP1-Out error");
    if(cnt != 64)
	printf("!!!Only %d bytes data have been transferred to EP01 .\n",cnt);
*/

    //---- read from EP 1 IN  ---
    r=libusb_interrupt_transfer(dev_handle,0x81,data_in,4, &cnt, 200); //0x00:host->dev, 0x80:dev->host
    if(r != 0)
	perror("libusb_interrupt_transfer() EP1-IN error");
    printf("%d bytes frome EP1-IN,data_in[]= 0x%02x%02x%02x%02x \n",cnt,data_in[3],data_in[2],data_in[1],data_in[0]);

    //----- control volume -----
    sprintf(strCMD, "amixer set Speaker %d%%", 70+30*data_in[0]/256);
    printf("%s \n",strCMD);
    system(strCMD);


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
