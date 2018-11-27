/*----------------------------------------------------------------------------
Codes derived from LINUX DEVICE DRIVERS 3rd Edition
Example:
	sudo insmod hello.ko whom="myhello" howmany=5
	sudo ./rw_hello

WARNING:
1. IS_ERR(), PTR_ERR() is for pointer operation !!!
2. alloc_chrdev_region(&devnum, ....),devnum must NOT be an NULL pointer !!!
3. cat /proc/devices  | grep hello  to see MAJOR nummber of the char devcie
4. Manually add dev node file example: mknod /dev/dev0 c 239 0
5. You have to change /dev file mode(permission) manually in this case,
   or you may use struct miscdevice which has a mode memeber: umode_t mode.
   or add a udev rule file: /etc/udev/rules.d/88-my-udev.rules
	KERNEL=="hello_dev",MODE="0666"


log: read + write + ioctl + capable_check + wait_event + mmap

------------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/init.h>  	/* __init() __exit() */
#include <linux/sched.h>
#include <linux/fs.h>  		/* register_chrdev_region(), alloc_chrdev_region()...*/
#include <linux/cdev.h> 	/* cdev_init(), cdev_add(), cdev_del() */
#include <linux/device.h>
#include <asm/errno.h>
#include <linux/slab.h> 	/* kmalloc() kfree() */
#include <linux/uaccess.h>  	/* copy_to_user(), copy_from_user() */
#include <linux/semaphore.h>
#include <linux/capability.h>
#include <linux/wait.h>
#include <linux/mm.h>		/* mmap */


#define HELLO_DATA_LEN	4096	/* (bytes)one page!!!  data length for hello_cdev.data */

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Free");
MODULE_DESCRIPTION("A LINUX Free Hello World Example");
MODULE_VERSION("0.1");

/*--- module parameters ---*/
static char * whom ="This is the Free Linux World! "; //Not necessary to allocate mem for 'whom', you can use long string input when load the module.
static int howmany = 1;
module_param(whom,charp, S_IRUGO);  /* S_IRUGO = S_IRUSR | S_IRGRP | S_IROTH */
module_param(howmany,int,S_IRUGO);
MODULE_PARM_DESC(whom, "The name to display");
MODULE_PARM_DESC(howmany, "How many times to print");

/*--- device relevant struct ---*/
static dev_t hello_devnum; /* 32bits device number: major(12) + minor(20) */
struct hello_cdev {
	char *data;		/* mem data for the device */
	struct semaphore sem;   /* mutual exclusion */
	struct cdev cdev; 	/* Char device */
};
static struct hello_cdev my_hello_cdev;
struct class * hello_class;
struct device *hello_device;

/*---  ioctl definition ---*/
#define HELLO_IOC_MAGIC 'h'
#define HELLO_IOCCMD_READ  _IOR(HELLO_IOC_MAGIC,1,int) //_IOR(type,nr,size)
#define HELLO_IOCCMD_WRITE _IOW(HELLO_IOC_MAGIC,2,int)  //_IOW(type,nr,size)
#define HELLO_IOC_MAXNR 2

/*--- wait event ---*/
static DECLARE_WAIT_QUEUE_HEAD(hello_wq);
static int wait_flag=0;



/* -----------------------  File Operation Functions:  open/close/read/write/mmap/lseek  ------------------- */
static int hello_open(struct inode *inode, struct file *filp)
{
	struct hello_cdev *pdev;

	pdev=container_of(inode->i_cdev,struct hello_cdev, cdev); // to locate and get helloc_cdev here.
	printk(KERN_INFO "%s: Hello, you just open the Free Linux World!\n",__func__);

	filp->private_data = pdev; /* put pdev to private data for other operation reference */

	return 0;
}

static int hello_close(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "%s: Hello, you just close the Free Linux World!\n",__func__);
	return 0;
}

ssize_t hello_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct hello_cdev *dev = filp->private_data;
	ssize_t retval = 0;


	/* wait for write first */
	printk(KERN_DEBUG "process %i (%s) going to sleep waiting for write to be finished.\n",current->pid, current->comm);
	wait_event_interruptible(hello_wq, wait_flag!=0);
	wait_flag=0;

	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if( *f_pos > HELLO_DATA_LEN-1 )
	{
		printk(KERN_ALERT "%s: f_pos exceeds the end of the file.\n",__func__);
		goto out;
	}

	if( count > HELLO_DATA_LEN-*f_pos )
		count = HELLO_DATA_LEN-*f_pos;

	if( copy_to_user(buf, (dev->data)+*f_pos, count))
	{
		retval=-EFAULT;
		goto out;
	}

	*f_pos += count;
	retval = count;

out:
	up(&dev->sem);
	return retval;
}

ssize_t hello_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct hello_cdev *dev = filp->private_data;
	ssize_t retval =0;

	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if ( count > HELLO_DATA_LEN - *f_pos )
		count = HELLO_DATA_LEN - *f_pos;

	if (copy_from_user(dev->data+*f_pos, buf, count))
	{
		retval = -EFAULT;
		goto out;
	}

	*f_pos += count;
	retval = count;

	/* wake up read wait */
	printk(KERN_DEBUG "process %i (%s) awakening the readers....\n",current->pid, current->comm);
	wait_flag=1;
	wake_up_interruptible(&hello_wq);

out:
	up(&dev->sem);
	return retval;
}


loff_t hello_llseek(struct file *filp, loff_t off, int whence)
{
//	struct hello_cdev *dev = filp->private_data;
	loff_t newpos;

	switch(whence) {
		case 0: /*SEEK_SET*/
			newpos = off;
			break;
		case 1: /*SEEK_CUR*/
			newpos = filp->f_pos + off;
			break;
		default:
			return -EINVAL;
	}

	if (newpos < 0 || newpos > HELLO_DATA_LEN-1 )
		return -EINVAL;

	filp->f_pos=newpos;
	return newpos;
}


long hello_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret=0;
	int tmp;

	/* check cmd */
	if (_IOC_TYPE(cmd) != HELLO_IOC_MAGIC)
	{
		printk(KERN_ALERT "ioctl cmd is not HELLO_IOC_MAGIC!\n");
		return -EINVAL;
	}
	if (_IOC_NR(cmd) > HELLO_IOC_MAXNR)
	{
		printk(KERN_ALERT "ioctl cmd nr > HELLO_IOC_MAXNR!\n");
		return -EINVAL;
	}

	switch(cmd)
	{
		/* read and write is from the application's point of view */
		case HELLO_IOCCMD_READ:
			tmp=12345;
			ret=put_user(tmp, (int __user *)arg);
				if(ret != 0){
					printk(KERN_ALERT "user pointer address error!\n");
					break;
				}
			printk("--- HELLO_IOCCMD_READ from %s:  put_user() tmp=%d\n",current->comm,tmp);
			break;
		case HELLO_IOCCMD_WRITE:
			/* capability check */
			if (! capable(CAP_SYS_ADMIN))
			{
				printk(KERN_ALERT "%s:Permission not allowed for the user to write!\n",current->comm);
				return -EPERM;
			}
			ret=get_user(tmp, (int __user *)arg);
				if(ret != 0){
					printk(KERN_ALERT "user pointer address error!\n");
					break;
				}
			printk("--- HELLO_IOCCMD_WRITE from %s: get_user() tmp=%d\n",current->comm,tmp);
			break;
		default:
			return -EINVAL;
	}

	return ret;
}


/*-------------  mmap operation -----------------*/
void simple_vma_open(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "simple VMA open, virt %lx, phys %lx\n",vma->vm_start, (long unsigned int)virt_to_phys(my_hello_cdev.data) );
}

void simple_vma_close(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "simple VMA close,\n");
}

static struct vm_operations_struct simple_vm_ops = {
	.open = simple_vma_open,
	.close = simple_vma_close,
};

static int simple_mmap(struct file *filp, struct vm_area_struct *vma)
{
	/* the physical address of data, to which the VMA virtual address should be mapped */
	unsigned long phy_addr=virt_to_phys(my_hello_cdev.data);

	/* map to continuous phy pages */
	if(remap_pfn_range(vma,vma->vm_start,
			phy_addr>>PAGE_SHIFT,
			vma->vm_end-vma->vm_start,
			vma->vm_page_prot))
	   return -EAGAIN;

	/* assign vm operations */
	vma->vm_ops = &simple_vm_ops;
	simple_vma_open(vma);
	return 0;
};


/*----------- struct file operations --------------*/
static const struct file_operations hello_fops={
	.owner = THIS_MODULE,
	.open  = hello_open,
	.read = hello_read,
	.write = hello_write,
	.llseek = hello_llseek,
	.unlocked_ioctl = hello_ioctl,
	.mmap = simple_mmap,
	.release = hello_close,
};




/* ------------------------------   Module Functions: init, exit  ------------------------- */
static int __init hello_init(void)
{
	int k;
	int ret=0;
	char buffer[30];

	for(k=0;k<howmany;k++)
	{
		printk(KERN_ALERT "from %s: Hello, %s!\n",current->comm,whom); /* No';' after KERN_ALERT !!!*/
	}
	printk(KERN_INFO "The process is \"%s\" (pid %i)\n",current->comm, current->pid);


   /* ------------------  init struct hello_cdev my_hello_cdev data ---------------------- */
	/* --- allocate mem for hello_cdev.data --- */
	my_hello_cdev.data = kmalloc(HELLO_DATA_LEN, GFP_KERNEL);
	if(IS_ERR(my_hello_cdev.data)) {
		printk(KERN_ALERT "kmalloc_fail, kmalloc for my_hello_cdev.data failed!\n");
		ret = -1;
		goto kmalloc_fail;
	}
	/* prevent the mem from swapping out */
	SetPageReserved(virt_to_page(my_hello_cdev.data));

	/* put some data for test */
	memset(my_hello_cdev.data,0,HELLO_DATA_LEN);
	strcpy(my_hello_cdev.data,"-/-/-/-/-/-/-/-/-/-/-/");
	/* --- init. semaphore for hello_cdev ---*/
	sema_init(&my_hello_cdev.sem,1);


   /* ------------------  Char Device Registration and Creation ---------------------- */

	/* --- 1. register and get dev number / name  ---  !!!!! ASSOCIATE name WITH major devnum */
	ret=alloc_chrdev_region(&hello_devnum, 0, 1, "dev_hello"); //warning: devnum must NOT be pointer type.
	if(ret<0){
		printk("%s: alloc_chrdev_region() error!\n",__func__);
		ret = -2;
		goto alloc_region_fail;
	}
	/* --- 2. char device init ---  !!!!! ASSOCIATE file operations */
	cdev_init(&my_hello_cdev.cdev, &hello_fops); // !!!! ASSOCIATE hello_cdev with inode,  to use container_of() to exact helloc_cdev later.
	/* --- 3. add char dev to kernel ---- */
	ret=cdev_add(&my_hello_cdev.cdev,hello_devnum,1);
	if(ret<0){
		printk("%s: cdev_add() error!\n",__func__);
		ret = -3;
		goto cdev_fail;
	}
	print_dev_t(buffer,hello_devnum);
	printk("hello dev number:%s \n", buffer); //printk dev number
	/* --- 4. create class in sysfs */
	hello_class = class_create(THIS_MODULE,"hello_class");
	if( IS_ERR(hello_class)) {
		printk(KERN_INFO "class_create() fail!\n");
		ret = -4;
		goto class_fail;
	}
	/* --- 5. create devices file in /dev */
	hello_device = device_create(hello_class, NULL, hello_devnum, NULL, "hello_dev"); 
	if ( IS_ERR(hello_device)) {
		printk(KERN_INFO "device_create() fail!\n");
		ret = -5;
		goto device_fail;
	}
	/* --- 6. module init. success --- */
	goto init_success;

/* ------- Fail entry: clean_up and roll_back  ------ */
device_fail:	   /* --- 1. destroy class --- */
	class_destroy(hello_class);
class_fail:	   /* --- 2. del char dev --- */
	cdev_del(&my_hello_cdev.cdev);
cdev_fail:	   /* --- 3. unregister char dev region --- */
	unregister_chrdev_region(hello_devnum,1);
	 	   /* --- 4. clear reserved page --- */
	ClearPageReserved(virt_to_page(my_hello_cdev.data));
	 	   /* --- 5. kfree mem --- */
	kfree(my_hello_cdev.data);

kmalloc_fail:
alloc_region_fail:
	return ret; /* if ret !=0; the module will NOT be inserted to the kernel. */

/* ------- Success entry  ------ */
init_success:
	return 0;
}


static void __exit hello_exit(void)  /* !!!! void */
{

	/*--- 1. unregister device ---*/
	printk(KERN_INFO "unregister device \n");
	device_unregister(hello_device);
        /*--- 2. del class ---*/
        printk(KERN_INFO "destroy class \n");
        class_destroy(hello_class);
	/*--- 3. del char dev ---*/
	printk(KERN_INFO "delete char dev.\n");
	cdev_del(&my_hello_cdev.cdev);
	/*--- 4. unregister char dev region ---*/
	printk(KERN_INFO "unregister chardev region.\n");
	unregister_chrdev_region(hello_devnum,1);
	/*--- 5. clear reserved pages ---*/
	printk(KERN_INFO "clear reserved pages.\n");
	ClearPageReserved(virt_to_page(my_hello_cdev.data));
	/*--- 6. kfree all mem ---*/
	printk(KERN_INFO "kfree mem.\n");
	kfree(my_hello_cdev.data);

	printk(KERN_INFO "The process is \"%s\" (pid %i)\n",current->comm, current->pid);
	printk(KERN_ALERT "Goodbye, Cruel World!\n");
}

module_init(hello_init);
module_exit(hello_exit);


