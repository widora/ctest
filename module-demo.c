/***************************** 
*
*   驱动程序模板
*   版本：V1
*   使用方法(末行模式下)：
*   :%s/xxx/"你的驱动名称"/g
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


/****************  基本定义 **********************/
//内核空间缓冲区定义
#if 0
	#define KB_MAX_SIZE 20
	#define kbuf[KB_MAX_SIZE];
#endif


//加密函数参数内容： _IOW(IOW_CHAR , IOW_NUMn , IOW_TYPE)
//加密函数用于xxx_ioctl函数中
//使用举例：ioctl(fd , _IOW('L',0x80,long) , 0x1);
//#define NUMn xxx , if you need!
#define IOW_CHAR 'L'
#define IOW_TYPE  long
#define IOW_NUM1  0x80


//初始化函数必要资源定义
//用于初始化函数当中
//device number;
	dev_t dev_num;
//struct dev
	struct cdev xxx_cdev;
//auto "mknode /dev/xxx c dev_num minor_num"
struct class *xxx_class = NULL;
struct device *xxx_device = NULL;


/**************** 结构体 file_operations 成员函数 *****************/
//open
static int xxx_open(struct inode *inode, struct file *file)
{
	printk("xxx drive open...\n");


	return 0;
}

//close
static int xxx_close(struct inode *inode , struct file *file)
{
	printk("xxx drive close...\n");


	return 0;
}

//read
static ssize_t xxx_read(struct file *file, char __user *buffer,
			size_t len, loff_t *pos)
{
	int ret_v = 0;
	printk("xxx drive read...\n");


	return ret_v;
}

//write
static ssize_t xxx_write( struct file *file , const char __user *buffer,
			   size_t len , loff_t *offset )
{
	int ret_v = 0;
	printk("xxx drive write...\n");


	return ret_v;
}

//unlocked_ioctl
static int xxx_ioctl (struct file *filp , unsigned int cmd , unsigned long arg)
{
	int ret_v = 0;
	printk("xxx drive ioctl...\n");

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
static const struct file_operations xxx_fops = {
	.owner   = THIS_MODULE,
	.open	 = xxx_open,
	.release = xxx_close,	
	.read	 = xxx_read,
	.write   = xxx_write,
	.unlocked_ioctl	= xxx_ioctl,
};


/*************  functions: init , exit*******************/
//条件值变量，用于指示资源是否正常使用
unsigned char init_flag = 0;
unsigned char add_code_flag = 0;

//init
static __init int xxx_init(void)
{
	int ret_v = 0;
	printk("xxx drive init...\n");

	//函数alloc_chrdev_region主要参数说明：
	//参数2： 次设备号
	//参数3： 创建多少个设备
	if( ( ret_v = alloc_chrdev_region(&dev_num,0,1,"xxx") ) < 0 )
	{
		goto dev_reg_error;
	}
	init_flag = 1; //标示设备创建成功；

	printk("The drive info of xxx:\nmajor: %d\nminor: %d\n",
		MAJOR(dev_num),MINOR(dev_num));

	cdev_init(&xxx_cdev,&xxx_fops);
	if( (ret_v = cdev_add(&xxx_cdev,dev_num,1)) != 0 )
	{
		goto cdev_add_error;
	}

	xxx_class = class_create(THIS_MODULE,"xxx");
	if( IS_ERR(xxx_class) )
	{
		goto class_c_error;
	}

	xxx_device = device_create(xxx_class,NULL,dev_num,NULL,"xxx");
	if( IS_ERR(xxx_device) )
	{
		goto device_c_error;
	}
	printk("auto mknod success!\n");

	//------------   请在此添加您的初始化程序  --------------//
       



        //如果需要做错误处理，请：goto xxx_error;	

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
	cdev_del(&xxx_cdev);
 	unregister_chrdev_region(dev_num, 1);
	init_flag = 0;
	return PTR_ERR(xxx_class);

device_c_error:
	printk("device_create failed\n");
	cdev_del(&xxx_cdev);
 	unregister_chrdev_region(dev_num, 1);
	class_destroy(xxx_class);
	init_flag = 0;
	return PTR_ERR(xxx_device);

//------------------ 请在此添加您的错误处理内容 ----------------//
xxx_error:
		



	add_code_flag = 0;
	return -1;
//--------------------          END         -------------------//
    
init_success:
	printk("xxx init success!\n");
	return 0;
}

//exit
static __exit void xxx_exit(void)
{
	printk("xxx drive exit...\n");	

	if(add_code_flag == 1)
 	{   
           //----------   请在这里释放您的程序占有的资源   ---------//
	    printk("free your resources...\n");	               





	    printk("free finish\n");		               
	    //----------------------     END      -------------------//
	}					            

	if(init_flag == 1)
	{
		//释放初始化使用到的资源;
		cdev_del(&xxx_cdev);
 		unregister_chrdev_region(dev_num, 1);
		device_unregister(xxx_device);
		class_destroy(xxx_class);
	}
}


/**************** module operations**********************/
//module loading
module_init(xxx_init);
module_exit(xxx_exit);

//some infomation
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("from Jafy");
MODULE_DESCRIPTION("xxx drive");


/*********************  The End ***************************/
