
/*---------------------------------------------------------------------------

A DMA test program from: m.blog.csdn.net/u014744063/article/details/53464948

----------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/fs.h> //struct file_operations
#include <linux/dma-mapping.h>

#define DEVICE_NAME "my_dma"

static dma_addr_t src_phy;
static dma_addr_t dst_phy;
unsigned char* src_vir;
unsigned char* dst_vir;

#define CMD_INIT_DMA 0x01
#define CMD_ENABLE_DMA 0x02

#define MY_DMA_BASE 0x10002800
#define Channel 1

static int dev_token=0;

static int int_num=17; //interrupt number
static int buff_size=128; //buff_size for DMA source and dest.

static volatile uint32_t *reg_src,*reg_dst;
static volatile uint32_t *reg_ct0,*reg_ct1,*reg_int;
static volatile uint32_t *reg_gct; //global control

static int ralink_dma_init(void)
{
   int i=0;
   *reg_src=src_phy;
   *reg_dst=dst_phy;
   *reg_ct0 |= (0x01|0x01<<2|0x4<<3|buff_size<<16); //softwrare mode DMA, when reg_ct0[0]=1, 16Words each burst transaction,!!!!!!!!
//   (*reg_ct0) &= 0xfffffffe; //reg_ct0[0]=0 HW mode DMA, data transfer starts when the DMA request is asserted.
   *reg_ct1 |= (Channel<<3|32<<8|32<<16);// not continuous mode,MEM 
//   *reg_ct1 &= ~(0xf<<22);// set NUM_SEGMENT=0;
   *reg_int |= (0x01<<Channel);// segment done interrupt , !!!! IRQ conflict !!  NO EFFECT !!!???????
   printk("  register reg_int: 0x%08x \n",*reg_int);

   //---- init src data
   printk("----init src_vir[] as: ");
   for(i=0;i<buff_size;i++)
   {
	src_vir[i]=i;
	printk("%02x",src_vir[i]);
   }
   printk("\n");

   //---- init dst data
   //memset(dst_vir,0,buff_size);

   printk("----init dst_vir[] as: ");
   for(i=0;i<buff_size;i++)
   {
	printk("%02x",dst_vir[i]);
   }
   printk("\n");
    return 0;
}


static void ralink_dma_enable(void)
{
   *reg_ct0 |= (0x01<<1);  //--in Software mode, data transfer starts when retg_ct0[1] is set to 1
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

static struct file_operations my_dma=
{
   .owner    =THIS_MODULE,
   .unlocked_ioctl =my_dma_ioctl,
};

static irqreturn_t mydma_handler(int irq, void *dev_id)
{
   int i=0;
   //----to confirm the interrupt
   //----seems Ralink_DMA will clear int flag auotmaticcaly.....
/*
   if( !((*reg_int) & (0x01<<Channel)) )
	return IRQ_NONE;
   else  //---clear INT flag
	   *reg_int &= ~(0x01 <<Channel);
*/

   printk("in irq handler now!\n");
   //---- to print and confirm tranfermed data-----
   for(i=0;i<buff_size;i++)
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

   src_vir=dma_alloc_coherent(NULL,buff_size,&src_phy,GFP_ATOMIC);
   if(src_vir == NULL)
   {
	unregister_chrdev(888,DEVICE_NAME);
	printk("src_vir is NULL!\n");
	return -1;
   }

   dst_vir = dma_alloc_coherent(NULL,buff_size,&dst_phy,GFP_ATOMIC);
   if(dst_vir == NULL)
   {
	dma_free_coherent(NULL,buff_size,src_vir,src_phy);
	unregister_chrdev(888,DEVICE_NAME);
	printk("dst_vir is NULL!\n");
	return -1;
   }

   if(request_irq(int_num,mydma_handler,IRQF_DISABLED,"mydma",(void *)&dev_token)<0)
   {
	printk("request_irq failed!\n");
	dma_free_coherent(NULL,buff_size,dst_vir,src_phy);
	dma_free_coherent(NULL,buff_size,src_vir,src_phy);
	unregister_chrdev(888,DEVICE_NAME);
	return -1;
   }

   reg_src=(uint32_t *)ioremap(MY_DMA_BASE+Channel*0x10,4);
   if(reg_src==NULL)
   {
	printk("reg_src ioremap() failed!\n");
   }

   reg_dst=(uint32_t *)ioremap(MY_DMA_BASE+Channel*0x10+0x04,4);
   if(reg_dst == NULL)
   {
	printk("reg_dst ioremap() failed!\n");
   }

   reg_ct0=(uint32_t *)ioremap(MY_DMA_BASE+Channel*0x10+0x08,4);
   if(reg_ct0 == NULL)
   {
	printk("reg_ct0 ioremap() failed!\n");
   }

   reg_ct1=(uint32_t *)ioremap(MY_DMA_BASE+Channel*0x10+0x0c,4);
   if(reg_ct1 == NULL)
   {
	printk("reg_ct1 ioremap() failed!\n");
   }

   reg_int=(uint32_t *)ioremap(0x10002a04,4);
   if(reg_int == NULL)
   {
	printk("reg_int ioremap() failed!\n");
   }

   reg_gct=(uint32_t *)ioremap(0x10002a20,4);
   if(reg_gct == NULL)
   {
	printk("reg_gct ioremap() failed!\n");
   }

   printk("----address map successfully!------\n");

   printk("----- register value before init and enable DMA -----\n");
   printk("  register reg_src: 0x%08x \n",*reg_src);
   printk("  register reg_dst: 0x%08x \n",*reg_dst);
   printk("  register reg_ct0: 0x%08x \n",*reg_ct0);
   printk("  register reg_ct1: 0x%08x \n",*reg_ct1);
   printk("  register reg_int: 0x%08x \n",*reg_int);
   printk("  register reg_gct: 0x%08x \n",*reg_gct);


   //-----init and enable DMA -------
   ralink_dma_init();

   printk("----- register value after DMA init -----\n");
   printk("  register reg_src: 0x%08x \n",*reg_src);
   printk("  register reg_dst: 0x%08x \n",*reg_dst);
   printk("  register reg_ct0: 0x%08x \n",*reg_ct0);
   printk("  register reg_ct1: 0x%08x \n",*reg_ct1);
   printk("  register reg_int: 0x%08x \n",*reg_int);
   printk("  register reg_gct: 0x%08x \n",*reg_gct);

   //----- enable DMA -----
   ralink_dma_enable();  //-- enable DMA

   printk("----- ralink_dma_init() and ralink_dma_enable() finish! -----\n");

   return 0;
}


static __exit  void my_dma_exit(void)
{
   int i;
   //---- to print and confirm tranfermed data-----
   printk("------reg_int after DMA transfer(maybe): %08x -----\n",*reg_int);
   printk("----now src_vir[] is: ");
   for(i=0;i<buff_size;i++)
   {
	printk("%02x",dst_vir[i]);
   }
   printk("\n");
   //------------  print register value ------
   printk("----- register value just before my_dma exit -----\n");
   printk("  register reg_src: 0x%08x \n",*reg_src);
   printk("  register reg_dst: 0x%08x \n",*reg_dst);
   printk("  register reg_ct0: 0x%08x \n",*reg_ct0);
   printk("  register reg_ct1: 0x%08x \n",*reg_ct1);
   printk("  register reg_int: 0x%08x \n",*reg_int);
   printk("  register reg_gct: 0x%08x \n",*reg_gct);

   #if 1
   dma_free_coherent(NULL,buff_size,src_vir,src_phy);
   dma_free_coherent(NULL,buff_size,dst_vir,dst_phy);
   #endif
 
   free_irq(int_num,(void *)&dev_token); //--free shared irq
   unregister_chrdev(888,DEVICE_NAME);
   iounmap(reg_ct0);
   iounmap(reg_ct1);
   iounmap(reg_dst);
   iounmap(reg_src);
   iounmap(reg_int);
   iounmap(reg_gct);

   printk("----- my_dma exit -----\n");
}

module_init(my_dma_init);
module_exit(my_dma_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Liu Long");

