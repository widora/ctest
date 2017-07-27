/*---------------------------------------------------------------------------

A DMA test program from m.blog.csdn.net/u014744063/article/details/53464948

----------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/fs.h> //struct file_operations
#include <linux/dma-mapping.h>

#define DEVICE_NAME "my_dma"

static dma_addr_t src_phy;
static dma_addr_t dst_phy;
static unsigned char* src_vir;
static unsigned char* dst_vir;

#define CMD_INIT_DMA 0x01
#define CMD_ENABLE_DMA 0x02

#define MY_DMA_BASE 0x10002800
#define Channel 0

static int dev_token=0;

int int_num=8; //--see dts,interrupt number

unsigned long *reg_src,*reg_dst;
unsigned long *reg_ct0,*reg_ct1,*reg_int;

static void ralink_regset(unsigned myreg, int data)
{
	myreg |=data;
}

static int ralink_dma_init(void)
{
   int i=0;
   *reg_src=src_phy;
   *reg_dst=dst_phy;
   *reg_ct0 |= (0x01|0x01<<2|0x4<<3|100<<16); //---------------- 100??? --!!!!!!!!!!!!!!
   *reg_ct1 |= (Channel <<3|32<<8|32<<16);
   *reg_int |= (0x01<<Channel);

   for(i=0;i<100;i++)
   {
	src_vir[i]=i;
   }

    return 0;
}

static void ralink_dma_enable(void)
{
   *reg_ct0 |= (0x01<<1);
}

static long my_dma_ioctl(struct file *file,unsigned int cmd, unsigned long arg)
{
   switch(cmd)
   {
	case CMD_INIT_DMA:
		ralink_dma_init();
		break;
	case CMD_ENABLE_DMA:
		ralink_dma_enable();
		break;
	default:
		break;
   }

   return 0;
}

struct file_operations my_dma=
{
   .owner    =THIS_MODULE,
   .unlocked_ioctl =my_dma_ioctl,
};

static irqreturn_t mydma_handler(int irq, void *dev_id)
{
   int i=0;
   //----to confirm the interrupt
   if( !((*reg_int) & (0x01<<Channel)) )
	return IRQ_NONE;
   else  //---clear INT flag
	   *reg_int &= ~(0x01 <<Channel);

   printk("in irq handler now!\n");
   //---- to print and confirm tranfermed data-----
   for(i=0;i<100;i++)
   {
	printk("%2x",dst_vir[i]);
   }
   printk("\n");

   return IRQ_HANDLED;
}

static __init int my_dma_init(void)
{

   int result=register_chrdev(888,DEVICE_NAME,&my_dma);
   if(result<0){
   printk("register chrdev failed!\n");
   return -1;
   }

   src_vir=dma_alloc_coherent(NULL,100,&src_phy,GFP_ATOMIC);
   if(src_vir == NULL)
   {
	unregister_chrdev(888,DEVICE_NAME);
	printk("src_vir is NULL!\n");
	return -1;
   }

   dst_vir = dma_alloc_coherent(NULL,100,&dst_phy,GFP_ATOMIC);
   if(dst_vir == NULL)
   {
	dma_free_coherent(NULL,100,src_vir,src_phy);
	unregister_chrdev(888,DEVICE_NAME);
	printk("dst_vir is NULL!\n");
	return -1;
   }

   if(request_irq(int_num,mydma_handler,IRQF_SHARED,"mydma",(void *)&dev_token)<0)
   {
	printk("request_irq failed!\n");
	dma_free_coherent(NULL,100,dst_vir,src_phy);
	dma_free_coherent(NULL,100,src_vir,src_phy);
	unregister_chrdev(888,DEVICE_NAME);
	return -1;
   }

   reg_src=(unsigned long*)ioremap(MY_DMA_BASE+Channel*0x10,4);
   if(reg_src==NULL)
   {
	printk("reg_src ioremap() failed!\n");
   }

   reg_dst=(unsigned long*)ioremap(MY_DMA_BASE+Channel*0x10+0x04,4);
   if(reg_dst == NULL)
   {
	printk("reg_dst ioremap() failed!\n");
   }

   reg_ct0=(unsigned long*)ioremap(MY_DMA_BASE+Channel*0x10+0x08,4);
   if(reg_ct0 == NULL)
   {
	printk("reg_ct0 ioremap() failed!\n");
   }

   reg_ct1=(unsigned long*)ioremap(MY_DMA_BASE+Channel*0x10+0x0c,4);
   if(reg_ct1 == NULL)
   {
	printk("reg_ct1 ioremap() failed!\n");
   }

   reg_int=(unsigned long*)ioremap(0x10002a04,4);
   if(reg_int == NULL)
   {
	printk("reg_int ioremap() failed!\n");
   }

   printk("----address map successfully!------\n");
   return 0;
}


static __exit  void my_dma_exit(void)
{

   #if 1
   dma_free_coherent(NULL,100,src_vir,src_phy);
   dma_free_coherent(NULL,100,dst_vir,dst_phy);
   #endif
 
   free_irq(int_num,(void *)&dev_token); //--free shared irq
   unregister_chrdev(888,DEVICE_NAME);
   iounmap(reg_ct0);
   iounmap(reg_ct1);
   iounmap(reg_dst);
   iounmap(reg_src);

   printk("----- my_dma eixt -----\n");
}

module_init(my_dma_init);
module_exit(my_dma_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Liu Long");

