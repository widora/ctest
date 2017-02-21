/***************************** 
*
*   驱动程序模板
*   版本：V1
*   使用方法(末行模式下)：
*   :%s/kdraw/"你的驱动名称"/g
*
*******************************/


#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/crash_dump.h>
#include <linux/backing-dev.h>
#include <linux/bootmem.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/aio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/gpio.h> //-----gpio_to_irq()
#include <asm-generic/gpio.h>
#include <linux/interrupt.h> //------request_irq()
#include "kdraw.h"
#include <linux/workqueue.h> //--workqueue,struct work_struct  DECLARE_WORK(), INIT_WORK()

#define GPIO_PIN_NUM 20
#define LED_PIN_NUM 21 //----A beeper will activate an interrupt accidently again after finishing beeping!!! so use LED instead.
static unsigned int INT_STATUS=0; //1--interrupt triggered, 0 no interrupt
static unsigned int GPIO_INT_NUM=0; //IRQ Number
static unsigned int IRQ_DISABLED_TOKEN;

/****************  基本定义 **********************/
//内核空间缓冲区定义
#if 0
	#define KB_MAX_SIZE 20
	#define kbuf[KB_MAX_SIZE];
#endif

//加密函数参数内容： _IOW(IOW_CHAR , IOW_NUMn , IOW_TYPE)
//加密函数用于kdraw_ioctl函数中
//使用举例：ioctl(fd , _IOW('L',0x80,long) , 0x1);
//#define NUMn kdraw , if you need!
#define IOW_CHAR 'L'
#define IOW_TYPE  long
#define IOW_NUM1  0x80


//初始化函数必要资源定义
//用于初始化函数当中
//device number;
	dev_t dev_num;
//struct char dev
	struct cdev kdraw_cdev;
//auto "mknode /dev/kdraw c dev_num minor_num"
struct class *kdraw_class = NULL;
struct device *kdraw_device = NULL;

//--------------------------- function claim ------------------------
static int  read_user_space(void);


//---------------------------  GET GPIO IRQ NUMBER  --------------------------------
static void get_gpio_INT_num(void)
{
   GPIO_INT_NUM=gpio_to_irq(GPIO_PIN_NUM);
   if(GPIO_INT_NUM!=0)
      printk("GPIO%d GET IRQ=%d successfully! \n",GPIO_PIN_NUM,GPIO_INT_NUM);
   else
      printk("Get GPIO_INT_NUM failed! \n");
}

//----------------------  schedule work function   ---------------------
//static struct work_struct kdraw_wq;
static void  kdraw_wq_handler(void *data)
{
	int k=0;
	int lbit=0;
	printk("....Entering KDRAW work queue ...\n");
	//msleep_interruptible(100); //--delay to avoid key-jitter

	for(k=0;k<10;k++)
	{
		//--- see set_pin_gpio() in module init 
    		lbit=!lbit;
		printk("----k=%d  lbit=%d -----\n",k,lbit);
		set_gpio_value(LED_PIN_NUM,lbit); //--set_gpio_value() will call set_gpio_output() first
		msleep(50);
	}
	msleep(100);
	enable_irq(GPIO_INT_NUM); //--re-enable irq
        enable_gpio_rise_int(GPIO_PIN_NUM);//---must re-enable-rising-edge-INT!!

}
//-----static declare wore queue;
DECLARE_WORK(KDRAW_WQ,kdraw_wq_handler); //-- direct explicit use of KDRAW_WQ, no need for pre-definition. 

//----------------------- Interrupt Handler  ---------------------------
static irqreturn_t gpio_int_handler(int irq, void *dev_id,struct pt_regs *regs)
{
  printk(" --------------KDRAW_GPIO Interrupt Triggered! ------------\n");
  read_user_space();

  //----confirm the interrupt here if necessary -----
  INT_STATUS=1;

  disable_irq_nosync(GPIO_INT_NUM); //--disable IRQ
  
  schedule_work(&KDRAW_WQ); //--- schedule work queue.
 
  IRQ_DISABLED_TOKEN=1;  

  return IRQ_HANDLED; //  !!!! ignore this return will result in printing out crap information from kernel, and more running time cost.
}


//---------------------- Register GPIO Interrupt  ----------------
static int register_gpio_IRQ(void)
{
	int int_result;
	int_result=request_irq(GPIO_INT_NUM,gpio_int_handler,IRQF_DISABLED,"KDRAW_INT",NULL);
        //----- flag IRQF_DISABLED will disable all local INTs while excecuting upper part gpio_int_handler.
        //-----IRQF_RISING not effective! Relevant functions in gpio.h & interrupt.h have not been realized yet.
        //----- cat /proc/interrupts to see KDRAW_INT info.
	if(int_result!=0)
	{
		printk("GPIO Interrupt register failed! \n");
		return 1;
	}
	else
	{
		printk("GPIO Interrupt register succeed! \n");
	     	return  0;
	}
}



/**************** 结构体 file_operations 成员函数 *****************/
//open
static int kdraw_open(struct inode *inode, struct file *file)
{
	int ret_v=0;
	printk("kdraw drive open...\n");

	return ret_v;
}

//close
static int kdraw_close(struct inode *inode , struct file *file)
{
	printk("kdraw drive close...\n");


	return 0;
}

//read
static ssize_t kdraw_read(struct file *file, char __user *buffer,
			size_t len, loff_t *pos)
{
	int ret_v = 0;
	printk("kdraw drive read...\n");


	return ret_v;
}

//write
static ssize_t kdraw_write( struct file *file , const char __user *buffer,
			   size_t len , loff_t *offset )
{
	int ret_v = 0;
	printk("kdraw drive write...\n");


	return ret_v;
}

//unlocked_ioctl
static int kdraw_ioctl (struct file *filp , unsigned int cmd , unsigned long arg)
{
	int ret_v = 0;
	printk("kdraw drive ioctl...\n");

	switch(cmd)
	{
		//常规：
		//cmd值自行进行修改
		case 0x1:
		{
			if(arg == 0x1) //第二条件；
			{

			}
		}
		break;

		//带密码保护：
		//请在"基本定义"进行必要的定义
		case _IOW(IOW_CHAR,IOW_NUM1,IOW_TYPE):
		{
			if(arg == 0x1) //第二条件
			{
				
			}

		}
		break;

		default:
			break;
	}

	return ret_v;
}


/***************** 结构体： file_operations ************************/
//struct
static const struct file_operations kdraw_fops = {
	.owner   = THIS_MODULE,
	.open	 = kdraw_open,
	.release = kdraw_close,	
	.read	 = kdraw_read,
	.write   = kdraw_write,
	.unlocked_ioctl	= kdraw_ioctl,
};


/*************  functions: init , exit*******************/
//条件值变量，用于指示资源是否正常使用
unsigned char init_flag = 0;
unsigned char add_code_flag = 0;


//---- read and write  file in user's space
static int  read_user_space(void)
{
   char str_f[]="/tmp/user.data";
   char buf[]="Hello,Happy New Year!";
   char buf1[30];
   struct file *fp;
   mm_segment_t fs;
   loff_t pos;
   printk("---- start to write and read user space file: %s ----\n",str_f);
   fp=filp_open(str_f,O_RDWR|O_CREAT,0777);
   if(IS_ERR(fp))
    {
      printk("Create file error.\n");
      return -1;
    }
    
   fs=get_fs();
   set_fs(KERNEL_DS);
   pos=0;
   vfs_write(fp,buf,sizeof(buf),&pos);
   pos=0;
   vfs_read(fp,buf1,sizeof(buf),&pos);
   printk("read file: %s\n",buf1);
   filp_close(fp,NULL);
   set_fs(fs);
   return 0;
}




//init
static __init int kdraw_init(void)
{
	int ret_v = 0;

	printk("kdraw drive init...\n");

	//函数alloc_chrdev_region主要参数说明：
	//参数2： 次设备号
	//参数3： 创建多少个设备
	if( ( ret_v = alloc_chrdev_region(&dev_num,0,1,"kdraw_proc") ) < 0 ) //--create a group of dev numbers in /proc/devices
	{
		goto dev_reg_error;
	}
	init_flag = 1; //标示设备创建成功；

	printk("The drive info of kdraw:\nmajor: %d\nminor: %d\n",
		MAJOR(dev_num),MINOR(dev_num));

	cdev_init(&kdraw_cdev,&kdraw_fops); //--init a char dev
	if( (ret_v = cdev_add(&kdraw_cdev,dev_num,1)) != 0 ) //--here dev_num and cdev are being related
	{
		goto cdev_add_error;
	}

	kdraw_class = class_create(THIS_MODULE,"kdraw_class");  //-- to create /sys/class/kdraw-class
	if( IS_ERR(kdraw_class) )
	{
		goto class_c_error;
	}

	kdraw_device = device_create(kdraw_class,NULL,dev_num,NULL,"kdraw_dev"); //--create /dev/kdraw-dev
	if( IS_ERR(kdraw_device) )
	{
		goto device_c_error;
	}
	printk("auto mknod success!\n");

	//------------   请在此添加您的初始化程序  --------------//
	//------------- read user space file test  ---------
        read_user_space();   
        //------------ Init GPIO Interrupt ------------
	map_gpio_register();
        if(!verify_gpio_num(GPIO_PIN_NUM)) //--check pin number first
	{
		printk("GPIO pin not available! Use GPIO #14-#21 instead.");
		return ret_v;
	}
        set_pin_gpio(GPIO_PIN_NUM); //--set pin group for GPIO purpose
        set_gpio_value(GPIO_PIN_NUM,0);
	set_gpio_input(GPIO_PIN_NUM);
        enable_gpio_rise_int(GPIO_PIN_NUM);//---It seems kernel will clear rising-edge set after INT handler called,you shall re-set this then.
        get_gpio_INT_num();
        register_gpio_IRQ(); 



        //如果需要做错误处理，请：goto kdraw_error;	

	 add_code_flag = 1;
	//----------------------  END  ---------------------------// 

	goto init_success;

dev_reg_error:
	printk("alloc_chrdev_region failed\n");	
	return ret_v;

cdev_add_error:
	printk("cdev_add failed\n");
 	unregister_chrdev_region(dev_num, 1);
	init_flag = 0;
	return ret_v;

class_c_error:
	printk("class_create failed\n");
	cdev_del(&kdraw_cdev);
 	unregister_chrdev_region(dev_num, 1);
	init_flag = 0;
	return PTR_ERR(kdraw_class);

device_c_error:
	printk("device_create failed\n");
	cdev_del(&kdraw_cdev);
 	unregister_chrdev_region(dev_num, 1);
	class_destroy(kdraw_class);
	init_flag = 0;
	return PTR_ERR(kdraw_device);

//------------------ 请在此添加您的错误处理内容 ----------------//
kdraw_error:
		



	add_code_flag = 0;
	return -1;
//--------------------          END         -------------------//
    
init_success:
	printk("-----  kdraw init succeed!  ------\n");
	return 0;
}

//exit
static __exit void kdraw_exit(void)
{
	printk("-----  kdraw drive exit...  ----\n");	

	if(add_code_flag == 1)
 	{   
           //----------   请在这里释放您的程序占有的资源   ---------//
	   //printk("free your resources...\n");	               

//-----------------------FREE IRQ RESOURCE ----------------------------------          
           printk("free IRQ resource ...\n");
           free_irq(GPIO_INT_NUM,NULL);  
	   printk("free GPIO IO-MAP resource ...\n");
	   unmap_gpio_register();

            printk("free finish\n");
	    //----------------------     END      -------------------//
	}

	if(init_flag == 1)
	{
		//释放初始化使用到的资源;
		cdev_del(&kdraw_cdev);
 		unregister_chrdev_region(dev_num, 1);
		device_unregister(kdraw_device);
		class_destroy(kdraw_class);
	}
}


/**************** module operations**********************/
//module loading
module_init(kdraw_init);
module_exit(kdraw_exit);

//some infomation
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("from Jafy, Midas-Zhou");
MODULE_DESCRIPTION("kdraw drive");


/*********************  The End ***************************/
