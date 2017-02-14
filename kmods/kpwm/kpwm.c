#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
//#include <linux/poll.h> 
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME "pwm"

#define PWM_IOCTL_SET_FREQ 1
#define PWM_IOCTL_STOP 0

/*------------------    PWM hw registers      --------------------*/
volatile unsigned long *PWM_ENABLE; // bit[0]-PWM0  bit[1]-PWM1 bit[2]-PWM2 bit[3]-PWM3 
volatile unsigned long *PWM0_CON; //pwm0 control register
volatile unsigned long *PWM0_HDURATION; //pwm0 high duration register
volatile unsigned long *PWM0_LDURATION; //pwm0 low duration register
volatile unsigned long *PWM0_GDURATION; //pwm0 guard duration register
volatile unsigned long *PWM0_WAVE_NUM; //pwm0 wave number register


/*------------------    map GPIO register      --------------------*/
void remap_pwm_register(void)
{
	PWM_ENABLE=(volatile unsigned long *)ioremap(0x10005000,4);
	PWM0_CON=(volatile unsigned long *)ioremap(0x10005010,4);
	PWM0_HDURATION=(volatile unsigned long *)ioremap(0x10005014,4);
	PWM0_LDURATION=(volatile unsigned long *)ioremap(0x10005018,4);
	PWM0_GDURATION=(volatile unsigned long *)ioremap(0x1000501C,4); 
	PWM0_WAVE_NUM=(volatile unsigned long *)ioremap(0x10005038,4); 

}

static struct semaphore lock;

static void PWM_Set_Freq(unsigned long freq)
{
	printk("set PWM fre\n");
}

static void PWM_Stop(void)
{
	printk("PWM stop\n");
}

static int pwm_open(struct inode *inode,struct file *file)
{
	printk("PWM open\n");
	return 0;
}

static int pwm_close(struct inode *inode,struct file *file)
{
        printk("PWM close\n");
        return 0;
}

static long pwm_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
	printk("pwm ioctl\n");
	return 0;
}

static struct file_operations dev_fops={
	.owner = THIS_MODULE,
	.open = pwm_open,
	.release = pwm_close,
	.unlocked_ioctl = pwm_ioctl,
};

static struct miscdevice misc ={
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};

static int __init dev_init(void)
{
	int ret;
	//init_MUTEX(&lock);
	sema_init(&lock,1);
	ret = misc_register(&misc);
	printk(DEVICE_NAME"\t-----initialized!\n");
	return ret;
}

static void __exit dev_exit(void)
{
	misc_deregister(&misc);
	printk(DEVICE_NAME"\t-----exit!\n");
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("midas");
MODULE_DESCRIPTION("pwm driver");
