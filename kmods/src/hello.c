#include <linux/init.h> /* printk() */
#include <linux/module.h> /* _init_exit */

static int __init hello_init(void)  /* init function, will be called  in insmod */
{

 printk(KERN_INFO "Hello World enter\n");
 return 0;

}

static void __exit hello_exit(void)
{
  printk(KERN_INFO "Hello World exit\n");

}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("midas zhou"); //-- optional
MODULE_LICENSE("GPL"); //--mandatory
MODULE_DESCRIPTION("A smiple Hello World Module"); //--optional
MODULE_ALIAS("a smiple moudle"); //---optional
