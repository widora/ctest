/***************************** 
*
*   驱动程序模板
*   版本：V1
*   使用方法(末行模式下)：
*   :%s/midas_driver/"你的驱动名称"/g
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

#define MYLED_OFF 0
#define MYLED_ON 1

volatile unsigned long *GPIO_CTRL_0;// GPIO0-GPIO31 direction control register
volatile unsigned long *GPIO_DATA_0;// GPIO0-GPIO31 data register
volatile unsigned long *GPIO1_MODE; // GPIO1 purpose selection register
volatile unsigned long *AGPIO_CFG; // analog GPIO configuartion,GPIO14-17 purpose 
/****************  基本定义 **********************/
//内核空间缓冲区定义
#if 0
	#define KB_MAX_SIZE 20
	#define kbuf[KB_MAX_SIZE];
#endif


//加密函数参数内容： _IOW(IOW_CHAR , IOW_NUMn , IOW_TYPE)
//加密函数用于midas_driver_ioctl函数中
//使用举例：ioctl(fd , _IOW('L',0x80,long) , 0x1);
//#define NUMn midas_driver , if you need!
#define IOW_CHAR 'L'
#define IOW_TYPE  long
#define IOW_NUM1  0x80


//初始化函数必要资源定义
//用于初始化函数当中
//device number;
	dev_t dev_num;
//struct dev
	struct cdev midas_driver_cdev;
//auto "mknode /dev/midas_driver c dev_num minor_num"
struct class *midas_driver_class = NULL;
struct device *midas_driver_device = NULL;


/**************** 结构体 file_operations 成员函数 *****************/
e//open
static int midas_driver_open(struct inode *inode, struct file *file)
{
	printk("midas_driver drive open...,LED pin set to 0 \n");
        
        *GPIO_DATA_0 &=~(1<<17); //-------------------------------------------------------------set GPIO17 as 0
       
	return 0;
}

//close
static int midas_driver_close(struct inode *inode , struct file *file)
{
	printk("midas_driver drive close...\n");


	return 0;
}

//read
static ssize_t midas_driver_read(struct file *file, char __user *buffer,
			size_t len, loff_t *pos)
{
	int ret_v = 0;
	printk("midas_driver drive read...\n");


	return ret_v;
}

//write
static ssize_t midas_driver_write( struct file *file , const char __user *buffer,
			   size_t len , loff_t *offset )
{
	int ret_v = 0;
	printk("midas_driver drive write...\n");


	return ret_v;
}

//unlocked_ioctl
static int midas_driver_ioctl (struct file *filp , unsigned int cmd , unsigned long arg)
{
	int ret_v = 0;
	printk("midas_driver drive ioctl...\n");

	switch(cmd)
	{
		//常规：
		//cmd值自行进行修改
		case MYLED_ON:
		     *GPIO_DATA_0 |=(1<<17);  //-----------------------------------------set GPIO17 as 1
		break;

                case MYLED_OFF:
                     *GPIO_DATA_0 &=~(1<<17); //------------------------------------------set GPIO17 as 0
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
static const struct file_operations midas_driver_fops = {
	.owner   = THIS_MODULE,
	.open	 = midas_driver_open,
	.release = midas_driver_close,	
	.read	 = midas_driver_read,
	.write   = midas_driver_write,
	.unlocked_ioctl	= midas_driver_ioctl,
};


/*************  functions: init , exit*******************/
//条件值变量，用于指示资源是否正常使用
unsigned char init_flag = 0;
unsigned char add_code_flag = 0;

//init
static __init int midas_driver_init(void)
{
	int ret_v = 0;
	printk("------- midas_driver v2 drive init...\n");

	//函数alloc_chrdev_region主要参数说明：
	//参数2： 次设备号
	//参数3： 创建多少个设备
	if( ( ret_v = alloc_chrdev_region(&dev_num,0,1,"midas_driver") ) < 0 )
	{
		goto dev_reg_error;
	}
	init_flag = 1; //标示设备创建成功；

	printk("The drive info of midas_driver:\nmajor: %d\nminor: %d\n",
		MAJOR(dev_num),MINOR(dev_num));

	cdev_init(&midas_driver_cdev,&midas_driver_fops);
	if( (ret_v = cdev_add(&midas_driver_cdev,dev_num,1)) != 0 )
	{
		goto cdev_add_error;
	}

	midas_driver_class = class_create(THIS_MODULE,"midas_driver");
	if( IS_ERR(midas_driver_class) )
	{
		goto class_c_error;
	}

	midas_driver_device = device_create(midas_driver_class,NULL,dev_num,NULL,"midas_driver");
	if( IS_ERR(midas_driver_device) )
	{
		goto device_c_error;
	}
	printk("auto mknod success!\n");

	//------------   请在此添加您的初始化程序  --------------//
         AGPIO_CFG=(volatile unsigned long *)ioremap(0x1000003c,4);  
         GPIO1_MODE=(volatile unsigned long *)ioremap(0x10000060,4);  //---------------------------------init register address
         GPIO_CTRL_0=(volatile unsigned long *)ioremap(0x10000600,4);
         GPIO_DATA_0=(volatile unsigned long *)ioremap(0x10000620,4);        
         
         *AGPIO_CFG |=(0b1111<<17);//---set AGPIO_CFG[20:17] as 1111       
         *GPIO1_MODE &=~(1<<3);   //--------------------------------------------------------------- set SPIS purpose as GPIO17
         *GPIO1_MODE |=(1<<2);
         *GPIO_CTRL_0 |=(1<<17);  //----------------------------------------------------------- set GPIO17 as output pin
         

        //如果需要做错误处理，请：goto midas_driver_error;	

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
	cdev_del(&midas_driver_cdev);
 	unregister_chrdev_region(dev_num, 1);
	init_flag = 0;
	return PTR_ERR(midas_driver_class);

device_c_error:
	printk("device_create failed\n");
	cdev_del(&midas_driver_cdev);
 	unregister_chrdev_region(dev_num, 1);
	class_destroy(midas_driver_class);
	init_flag = 0;
	return PTR_ERR(midas_driver_device);

//------------------ 请在此添加您的错误处理内容 ----------------//
midas_driver_error:
         		



	add_code_flag = 0;
	return -1;
//--------------------          END         -------------------//
    
init_success:
	printk("midas_driver init success!\n");
	return 0;
}

//exit
static __exit void midas_driver_exit(void)
{
	printk("midas_driver drive exit...\n");	

	if(add_code_flag == 1)
 	{   
           //----------   请在这里释放您的程序占有的资源   ---------//
	    printk("free your resources...\n");	               
             
            iounmap(GPIO1_MODE); //----------------------------------------------------------release ioremap resource 
            iounmap(GPIO_CTRL_0);
            iounmap(GPIO_DATA_0);
            iounmap(AGPIO_CFG);

	    printk("free finish\n");		               
	    //----------------------     END      -------------------//
	}					            

	if(init_flag == 1)
	{
		//释放初始化使用到的资源;
		cdev_del(&midas_driver_cdev);
 		unregister_chrdev_region(dev_num, 1);
		device_unregister(midas_driver_device);
		class_destroy(midas_driver_class);
	}
}


/**************** module operations**********************/
//module loading
module_init(midas_driver_init);
module_exit(midas_driver_exit);

//some infomation
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("from Midas");
MODULE_DESCRIPTION("midas_driver drive");


/*********************  The End ***************************/
